#include <algorithm>

#include "RocksDBBatchingProcessorQueue.hpp"

namespace AdServer::ProfilingCommons
{
  RocksDBBatchingProcessorQueue::OperationQueue::OperationQueue(std::size_t reserve_size)
    : group_storage(Generics::MonoAllocator<OperationGroup>(arena)),
      free_groups(Generics::MonoAllocator<OperationGroup*>(arena))
  {
    groups.reserve(reserve_size);
  }

  inline bool
  RocksDBBatchingProcessorQueue::OperationQueue::empty() const noexcept
  {
    return operation_groups.empty();
  }

  inline RocksDBBatchingProcessorQueue::OperationGroup*
  RocksDBBatchingProcessorQueue::OperationQueue::acquire_group()
  {
    if (free_groups.empty())
    {
      group_storage.emplace_back();
      return &group_storage.back();
    }

    OperationGroup* group = free_groups.back();
    free_groups.pop_back();
    return group;
  }

  inline void
  RocksDBBatchingProcessorQueue::OperationQueue::release_group(OperationGroup* group) noexcept
  {
    free_groups.emplace_back(group);
  }

  RocksDBBatchingProcessorQueue::RocksDBBatchingProcessorQueue(
    unsigned long batch_size,
    const Generics::Time& max_delay,
    unsigned long enqueue_buckets_count)
    : batch_size_(std::max(1UL, batch_size)),
      max_delay_(max_delay)
  {
    enqueue_buckets_count = std::max(1UL, enqueue_buckets_count);
    enqueue_buckets_.reserve(enqueue_buckets_count);
    for (unsigned long i = 0; i < enqueue_buckets_count; ++i)
    {
      enqueue_buckets_.emplace_back(std::make_unique<EnqueueBucket>(batch_size_));
    }
  }

  RocksDBBatchingProcessorQueue::~RocksDBBatchingProcessorQueue() = default;

  void
  RocksDBBatchingProcessorQueue::enqueue_operations_i_(OperationQueue& target, Operations& source)
  {
    for (auto operation_it = source.begin(); operation_it != source.end();)
    {
      auto current_operation = operation_it++;
      OperationGroup* group = target.acquire_group();
      group->key = Generics::StringViewHashAdapter(current_operation->key);
      const auto insert_group = [&]()
      {
        try
        {
          return target.groups.insert(group);
        }
        catch (...)
        {
          target.release_group(group);
          throw;
        }
      };
      auto [group_it, inserted] = insert_group();
      if (inserted)
      {
        group->enqueue_time = current_operation->enqueue_time;
        group->operations.splice(group->operations.end(), source, current_operation);
        insert_operation_group_i_(target, *group);
        continue;
      }

      target.release_group(group);
      group = *group_it;
      auto operation_position = group->operations.end();
      if (is_write_operation(current_operation->type))
      {
        auto last_operation = group->operations.end();
        --last_operation;
        const bool current_is_touch = current_operation->type == OT_TOUCH;
        const bool last_is_touch = last_operation->type == OT_TOUCH;
        const bool current_is_latest =
          (!current_is_touch && last_is_touch) ||
          (current_is_touch == last_is_touch &&
            last_operation->enqueue_time <= current_operation->enqueue_time);
        if (!current_is_latest)
        {
          operation_position = last_operation;
        }
      }

      const bool new_earliest_operation = current_operation->enqueue_time < group->enqueue_time;
      group->operations.splice(operation_position, source, current_operation);
      if (new_earliest_operation)
      {
        group->enqueue_time = current_operation->enqueue_time;
        target.operation_groups.erase(target.operation_groups.iterator_to(*group));
        insert_operation_group_i_(target, *group);
      }
    }
  }

  inline void
  RocksDBBatchingProcessorQueue::insert_operation_group_i_(
    OperationQueue& target,
    OperationGroup& group) noexcept
  {
    auto group_position = target.operation_groups.end();
    while (group_position != target.operation_groups.begin())
    {
      auto previous = group_position;
      --previous;
      if (previous->enqueue_time <= group.enqueue_time)
      {
        break;
      }

      group_position = previous;
    }

    target.operation_groups.insert(group_position, group);
  }

  inline Generics::Time
  RocksDBBatchingProcessorQueue::time_from_microseconds_(std::int64_t microseconds) noexcept
  {
    return Generics::Time(
      microseconds / Generics::Time::USEC_MAX,
      microseconds % Generics::Time::USEC_MAX);
  }

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::flush_pending() noexcept
  {
    std::lock_guard guard(state_lock_);
    ReadyState state = request_ready_i_(!empty_i_(), true);
    if (state.has_operation)
    {
      state.ready_time = state.enqueue_time;
    }
    return state;
  }

  RocksDBBatchingProcessorQueue::EnqueueResult
  RocksDBBatchingProcessorQueue::enqueue(Operations&& operations)
  {
    EnqueueResult result;
    if (operations.empty())
    {
      return result;
    }

    const unsigned long operation_count = operations.size();
    unsigned long previous_pending_count = 0;
    const Generics::Time now = Generics::Time::get_time_of_day();
    const auto add_pending_operations = [&]()
    {
      previous_pending_count = pending_operations_.fetch_add(
        operation_count,
        std::memory_order_acq_rel);
      if (max_delay_ != Generics::Time::ZERO && previous_pending_count == 0)
      {
        min_enqueue_time_.store(now.microseconds(), std::memory_order_release);
      }
    };

    if (operation_count == 1)
    {
      auto operation_it = operations.begin();
      operation_it->enqueue_time = now;
      account_operation_(result.counts, operation_it->type);
      const bool write_operation = is_write_operation(operation_it->type);
      const std::size_t bucket_index = operation_it->key.hash() % enqueue_buckets_.size();
      auto& bucket = *enqueue_buckets_[bucket_index];

      add_pending_operations();

      std::lock_guard bucket_guard(bucket.lock);
      OperationQueue& target = write_operation ? bucket.write_operations : bucket.read_operations;
      enqueue_operations_i_(target, operations);
      bucket.has_pending_operations.store(true, std::memory_order_release);
    }
    else
    {
      std::vector<StagedBucket> staged_buckets(enqueue_buckets_.size());
      auto operation_it = operations.begin();
      while (operation_it != operations.end())
      {
        auto current_it = operation_it++;
        current_it->enqueue_time = now;
        account_operation_(result.counts, current_it->type);
        const bool write_operation = is_write_operation(current_it->type);
        const std::size_t bucket_index = current_it->key.hash() % staged_buckets.size();
        Operations& target = write_operation ?
          staged_buckets[bucket_index].write_operations :
          staged_buckets[bucket_index].read_operations;
        target.splice(target.end(), operations, current_it);
      }

      add_pending_operations();

      for (std::size_t i = 0; i < staged_buckets.size(); ++i)
      {
        auto& staged_bucket = staged_buckets[i];
        if (staged_bucket.read_operations.empty() && staged_bucket.write_operations.empty())
        {
          continue;
        }

        auto& bucket = *enqueue_buckets_[i];

        {
          std::lock_guard bucket_guard(bucket.lock);

          if (!staged_bucket.write_operations.empty())
          {
            enqueue_operations_i_(bucket.write_operations, staged_bucket.write_operations);
          }

          if (!staged_bucket.read_operations.empty())
          {
            enqueue_operations_i_(bucket.read_operations, staged_bucket.read_operations);
          }

          bucket.has_pending_operations.store(true, std::memory_order_release);
        }
      }
    }

    const bool fills_batch = max_delay_ != Generics::Time::ZERO &&
      previous_pending_count < batch_size_ &&
      previous_pending_count + operation_count >= batch_size_;

    ReadyState ready_state = request_ready_i_(true, previous_pending_count == 0 || fills_batch);
    if (ready_state.has_operation)
    {
      result.ready_state = ready_state;
    }

    return result;
  }

  void
  RocksDBBatchingProcessorQueue::start_batch() noexcept
  {
    active_workers_.fetch_add(1, std::memory_order_relaxed);
  }

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::collect_batch(Operations& batch, SelectedKeys& selected_keys)
    noexcept
  {
    std::lock_guard guard(state_lock_);
    ready_published_.store(false, std::memory_order_release);
    const CollectResult collect_result = collect_batch_i_(batch, selected_keys);

    if (!batch.empty())
    {
      const bool write_batch = is_write_operation(batch.front().type);
      auto& in_flight_keys = write_batch ? in_flight_write_keys_ : in_flight_read_keys_;
      for (const auto& operation : batch)
      {
        add_in_flight_key_(in_flight_keys, operation);
      }
    }

    if (collect_result.min_enqueue_time)
    {
      min_enqueue_time_.store(
        collect_result.min_enqueue_time->microseconds(),
        std::memory_order_release);
    }

    return collect_result.collected ? request_ready_i_(!empty_i_()) : ReadyState{};
  }

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::complete_batch(const Operations& batch) noexcept
  {
    std::lock_guard guard(state_lock_);

    auto& in_flight_keys =
      !batch.empty() && is_write_operation(batch.front().type) ?
      in_flight_write_keys_ : in_flight_read_keys_;
    for (const auto& operation : batch)
    {
      const auto it = in_flight_keys.find(operation.key);
      if (it != in_flight_keys.end() && --it->second == 0)
      {
        in_flight_keys.erase(it);
      }
    }

    return request_ready_i_(!empty_i_());
  }

  void
  RocksDBBatchingProcessorQueue::finish_batch() noexcept
  {
    const bool last_active_worker = active_workers_.fetch_sub(1, std::memory_order_acq_rel) == 1;
    if (last_active_worker && empty_i_())
    {
      std::lock_guard guard(state_lock_);
      if (drained())
      {
        drain_condition_.notify_all();
      }
    }
  }

  void
  RocksDBBatchingProcessorQueue::wait_pending()
  {
    wait_drained();
  }

  void
  RocksDBBatchingProcessorQueue::wait_drained()
  {
    std::unique_lock guard(state_lock_);
    while (!drained())
    {
      drain_condition_.wait(guard);
    }
  }

  bool
  RocksDBBatchingProcessorQueue::drained() const noexcept
  {
    return empty_i_() && active_workers_.load(std::memory_order_acquire) == 0;
  }

  void
  RocksDBBatchingProcessorQueue::account_operation_(OperationCounts& counts, OperationType type)
    const noexcept
  {
    switch(type)
    {
      case OT_CHECK:
        ++counts.check;
        break;
      case OT_GET:
        ++counts.get;
        break;
      case OT_TOUCH:
        ++counts.touch;
        break;
      case OT_SAVE:
        ++counts.save;
        break;
      case OT_REMOVE:
        ++counts.remove;
        break;
    }
  }

  void
  RocksDBBatchingProcessorQueue::add_in_flight_key_(InFlightKeys& keys, const Operation& operation)
  {
    const auto it = keys.find(operation.key);
    if (it == keys.end())
    {
      keys.emplace(operation.key, 1);
    }
    else
    {
      ++it->second;
    }
  }

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::request_ready_i_(bool has_pending_operations, bool force_update)
    noexcept
  {
    if (!has_pending_operations)
    {
      return {};
    }

    const bool was_published = ready_published_.exchange(true, std::memory_order_acq_rel);
    if (was_published && !force_update)
    {
      return {};
    }

    return make_ready_state_i_();
  }

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::make_ready_state_i_() const noexcept
  {
    ReadyState result;
    result.has_operation = true;
    const Generics::Time now = Generics::Time::get_time_of_day();
    const std::int64_t enqueue_time = min_enqueue_time_.load(std::memory_order_acquire);
    result.enqueue_time = max_delay_ != Generics::Time::ZERO && enqueue_time != NO_ENQUEUE_TIME ?
      time_from_microseconds_(enqueue_time) : now;
    result.ready_time = operation_ready_time_i_(result.enqueue_time);
    return result;
  }

  const RocksDBBatchingProcessorQueue::OperationGroup*
  RocksDBBatchingProcessorQueue::find_ready_group_i_(
    const OperationQueue& operation_queue,
    bool write_operations) const noexcept
  {
    for (const auto& group : operation_queue.operation_groups)
    {
      bool ready;
      const auto& key = group.key;
      if (write_operations)
      {
        ready = in_flight_read_keys_.find(key) == in_flight_read_keys_.end() &&
          in_flight_write_keys_.find(key) == in_flight_write_keys_.end();
      }
      else
      {
        ready = in_flight_write_keys_.find(key) == in_flight_write_keys_.end();
      }

      if (ready)
      {
        return &group;
      }
    }

    return nullptr;
  }

  Generics::Time
  RocksDBBatchingProcessorQueue::operation_ready_time_i_(const Generics::Time& enqueue_time)
    const noexcept
  {
    return max_delay_ == Generics::Time::ZERO ||
      pending_operations_.load(std::memory_order_acquire) >= batch_size_ ?
      enqueue_time : enqueue_time + max_delay_;
  }

  RocksDBBatchingProcessorQueue::CollectResult
  RocksDBBatchingProcessorQueue::collect_batch_i_(
    Operations& batch,
    SelectedKeys& selected_keys) noexcept
  {
    CollectResult result;
    if (empty_i_())
    {
      return result;
    }

    const bool track_min_enqueue_time = max_delay_ != Generics::Time::ZERO;
    const auto update_min_enqueue_time = [&result](const OperationQueue& operation_queue)
    {
      if (!operation_queue.empty() &&
        (!result.min_enqueue_time ||
          operation_queue.operation_groups.front().enqueue_time < *result.min_enqueue_time))
      {
        result.min_enqueue_time = operation_queue.operation_groups.front().enqueue_time;
      }
    };

    const bool fixed_operation_type = !batch.empty();
    const bool fixed_write_operations = fixed_operation_type &&
      is_write_operation(batch.front().type);
    const std::size_t start_bucket_index = next_bucket_index_;
    std::size_t selected_offset = enqueue_buckets_.size();
    bool selected_write_operations = false;
    for (std::size_t offset = 0; offset < enqueue_buckets_.size(); ++offset)
    {
      const std::size_t bucket_index = (start_bucket_index + offset) % enqueue_buckets_.size();
      auto& bucket = *enqueue_buckets_[bucket_index];
      if (!bucket.has_pending_operations.load(std::memory_order_acquire))
      {
        continue;
      }

      std::lock_guard bucket_guard(bucket.lock);
      if (fixed_operation_type)
      {
        const OperationQueue& operation_queue = fixed_write_operations ?
          bucket.write_operations : bucket.read_operations;
        if (find_ready_group_i_(operation_queue, fixed_write_operations))
        {
          selected_write_operations = fixed_write_operations;
          selected_offset = offset;
          break;
        }
      }
      else
      {
        const OperationGroup* write_group = find_ready_group_i_(bucket.write_operations, true);
        const OperationGroup* read_group = find_ready_group_i_(bucket.read_operations, false);
        if (write_group || read_group)
        {
          selected_write_operations = !read_group ||
            (write_group && write_group->enqueue_time <= read_group->enqueue_time);
          selected_offset = offset;
          break;
        }
      }

      if (track_min_enqueue_time)
      {
        update_min_enqueue_time(bucket.write_operations);
        update_min_enqueue_time(bucket.read_operations);
      }
    }

    if (selected_offset == enqueue_buckets_.size())
    {
      return result;
    }

    const bool collect_reads = !selected_write_operations;
    unsigned long collected = 0;
    for (std::size_t offset = selected_offset; offset < enqueue_buckets_.size(); ++offset)
    {
      const std::size_t bucket_index = (start_bucket_index + offset) % enqueue_buckets_.size();
      auto& bucket = *enqueue_buckets_[bucket_index];
      if (!bucket.has_pending_operations.load(std::memory_order_acquire))
      {
        continue;
      }

      std::lock_guard bucket_guard(bucket.lock);
      if (selected_keys.size() < batch_size_)
      {
        OperationQueue& source = collect_reads ? bucket.read_operations : bucket.write_operations;
        const std::size_t initial_batch_size = batch.size();
        collected += collect_from_queue_i_(source, collect_reads, batch, selected_keys);

        if (batch.size() != initial_batch_size)
        {
          next_bucket_index_ = (bucket_index + 1) % enqueue_buckets_.size();
        }
      }

      if (bucket.read_operations.empty() && bucket.write_operations.empty())
      {
        bucket.has_pending_operations.store(false, std::memory_order_release);
      }

      if (track_min_enqueue_time)
      {
        update_min_enqueue_time(bucket.write_operations);
        update_min_enqueue_time(bucket.read_operations);
      }

      if (!track_min_enqueue_time && selected_keys.size() >= batch_size_)
      {
        break;
      }
    }

    if (collected)
    {
      pending_operations_.fetch_sub(collected, std::memory_order_acq_rel);
    }

    result.collected = collected != 0;
    return result;
  }

  unsigned long
  RocksDBBatchingProcessorQueue::collect_from_queue_i_(
    OperationQueue& source,
    bool collect_reads,
    Operations& batch,
    SelectedKeys& selected_keys) noexcept
  {
    const std::size_t initial_batch_size = batch.size();
    for (auto group_it = source.operation_groups.begin();
      group_it != source.operation_groups.end() && selected_keys.size() < batch_size_;)
    {
      auto current_group = group_it++;
      OperationGroup& group = *current_group;
      const auto& key = group.key;
      if (collect_reads && in_flight_write_keys_.find(key) != in_flight_write_keys_.end())
      {
        continue;
      }

      if (!collect_reads &&
        (in_flight_read_keys_.find(key) != in_flight_read_keys_.end() ||
        in_flight_write_keys_.find(key) != in_flight_write_keys_.end()))
      {
        continue;
      }

      selected_keys.emplace_back(group.key.text());
      source.operation_groups.erase(current_group);
      batch.splice(batch.end(), group.operations);
      source.groups.erase(&group);
      source.release_group(&group);
    }

    return static_cast<unsigned long>(batch.size() - initial_batch_size);
  }

  bool
  RocksDBBatchingProcessorQueue::empty_i_() const noexcept
  {
    return pending_operations_.load(std::memory_order_acquire) == 0;
  }
}
