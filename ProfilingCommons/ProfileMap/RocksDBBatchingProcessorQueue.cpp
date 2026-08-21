#include <algorithm>
#include <functional>

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
    for(unsigned long i = 0; i < enqueue_buckets_count; ++i)
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
      const std::string_view key(current_operation->key);
      OperationGroup* group = target.acquire_group();
      group->key = key;
      group->key_hash = current_operation->key_hash;
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

      const bool new_earliest_operation =
        current_operation->enqueue_time < group->enqueue_time;
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

  inline RocksDBBatchingProcessorQueue::ReadyQueueIndex
  RocksDBBatchingProcessorQueue::make_ready_queue_index_(
    std::size_t bucket_index,
    bool write_operations) noexcept
  {
    return (bucket_index << 1 | write_operations) + 1;
  }

  inline std::size_t
  RocksDBBatchingProcessorQueue::queue_bucket_index_(ReadyQueueIndex queue_index) noexcept
  {
    return (queue_index - 1) >> 1;
  }

  inline bool
  RocksDBBatchingProcessorQueue::queue_write_operations_(ReadyQueueIndex queue_index) noexcept
  {
    return (queue_index - 1) & 1;
  }

  inline Generics::Time
  RocksDBBatchingProcessorQueue::time_from_microseconds_(std::int64_t microseconds) noexcept
  {
    return Generics::Time(
      microseconds / Generics::Time::USEC_MAX,
      microseconds % Generics::Time::USEC_MAX);
  }

  void
  RocksDBBatchingProcessorQueue::activate() noexcept
  {
    accepting_.store(true, std::memory_order_release);
  }

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::deactivate() noexcept
  {
    std::lock_guard guard(state_lock_);
    accepting_.store(false, std::memory_order_release);
    ReadyState state = ready_state_i_();
    drain_condition_.notify_all();
    return state;
  }

  RocksDBBatchingProcessorQueue::EnqueueResult
  RocksDBBatchingProcessorQueue::enqueue(Operations&& operations)
  {
    EnqueueResult result;
    if (operations.empty())
    {
      result.accepted = true;
      return result;
    }

    if (!accepting())
    {
      return result;
    }

    unsigned long read_count = 0;
    unsigned long write_count = 0;
    unsigned long previous_read_count = 0;
    unsigned long previous_write_count = 0;
    unsigned long previous_pending_count = 0;
    ReadyQueueIndex enqueued_queue_index = 0;
    Generics::Time enqueued_time;
    const Generics::Time now = Generics::Time::get_time_of_day();

    if (operations.size() == 1)
    {
      auto operation_it = operations.begin();
      operation_it->enqueue_time = now;
      operation_it->key_hash = std::hash<std::string_view>{}(operation_it->key);
      account_operation_(result.counts, operation_it->type);
      const bool write_operation = is_write_operation(operation_it->type);
      const std::size_t bucket_index =
        operation_it->key_hash % enqueue_buckets_.size();
      auto& bucket = *enqueue_buckets_[bucket_index];
      enqueued_time = now;
      enqueued_queue_index = make_ready_queue_index_(bucket_index, write_operation);

      if (write_operation)
      {
        write_count = 1;
        previous_write_count = pending_write_operations_.fetch_add(
          1,
          std::memory_order_acq_rel);
      }
      else
      {
        read_count = 1;
        previous_read_count = pending_read_operations_.fetch_add(
          1,
          std::memory_order_acq_rel);
      }

      previous_pending_count = pending_operations_.fetch_add(1, std::memory_order_acq_rel);

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
        current_it->key_hash = std::hash<std::string_view>{}(current_it->key);
        account_operation_(result.counts, current_it->type);
        const bool write_operation = is_write_operation(current_it->type);
        const std::size_t bucket_index =
          current_it->key_hash % staged_buckets.size();
        Operations& target = write_operation ?
          staged_buckets[bucket_index].write_operations :
          staged_buckets[bucket_index].read_operations;
        target.splice(target.end(), operations, current_it);
        write_count += write_operation;
        read_count += !write_operation;
      }

      previous_pending_count = pending_operations_.fetch_add(
        read_count + write_count,
        std::memory_order_acq_rel);

      if (read_count)
      {
        previous_read_count = pending_read_operations_.fetch_add(
          read_count,
          std::memory_order_acq_rel);
      }

      if (write_count)
      {
        previous_write_count = pending_write_operations_.fetch_add(
          write_count,
          std::memory_order_acq_rel);
      }

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
            if (!enqueued_queue_index)
            {
              enqueued_queue_index = make_ready_queue_index_(i, true);
              enqueued_time = staged_bucket.write_operations.front().enqueue_time;
            }

            enqueue_operations_i_(bucket.write_operations, staged_bucket.write_operations);
          }

          if (!staged_bucket.read_operations.empty())
          {
            if (!enqueued_queue_index)
            {
              enqueued_queue_index = make_ready_queue_index_(i, false);
              enqueued_time = staged_bucket.read_operations.front().enqueue_time;
            }

            enqueue_operations_i_(bucket.read_operations, staged_bucket.read_operations);
          }

          bucket.has_pending_operations.store(true, std::memory_order_release);
        }
      }
    }

    result.accepted = true;
    const bool fills_read_batch = read_count &&
      previous_read_count < batch_size_ &&
      previous_read_count + read_count >= batch_size_;
    const bool fills_write_batch = write_count &&
      previous_write_count < batch_size_ &&
      previous_write_count + write_count >= batch_size_;

    result.ready_state = publish_enqueued_queue_(enqueued_queue_index, enqueued_time);

    if (!result.ready_state &&
      (previous_pending_count == 0 || fills_read_batch || fills_write_batch))
    {
      std::lock_guard guard(state_lock_);
      result.ready_state = ready_state_i_();
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
    ReadyQueueIndex queue_index = reset_ready_queue_i_();
    if (!batch.empty())
    {
      const bool write_operations = is_write_operation(batch.front().type);
      const std::size_t bucket_index =
        batch.front().key_hash % enqueue_buckets_.size();
      queue_index = make_ready_queue_index_(bucket_index, write_operations);
    }
    else
    {
      if (!queue_index)
      {
        return ready_state_i_();
      }
    }

    const bool collect_reads = !queue_write_operations_(queue_index);
    const std::size_t start_bucket_index = queue_bucket_index_(queue_index);
    next_bucket_index_ = (start_bucket_index + 1) % enqueue_buckets_.size();
    collect_batch_i_(batch, selected_keys, collect_reads, start_bucket_index);

    if (!batch.empty())
    {
      const bool write_batch = is_write_operation(batch.front().type);
      auto& in_flight_keys = write_batch ? in_flight_write_keys_ : in_flight_read_keys_;
      for (const auto& operation : batch)
      {
        ++in_flight_keys[operation.key];
      }
    }

    recalculate_ready_queue_i_();
    return ready_state_i_();
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

    if (recalculate_on_complete_)
    {
      reset_ready_queue_i_();
      recalculate_ready_queue_i_();
    }

    return ready_state_i_();
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
    std::unique_lock guard(state_lock_);
    while (accepting() && !drained())
    {
      drain_condition_.wait(guard);
    }
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

  RocksDBBatchingProcessorQueue::ReadyState
  RocksDBBatchingProcessorQueue::ready_state_i_() noexcept
  {
    ReadyState result;
    result.generation = ready_generation_.fetch_add(1, std::memory_order_relaxed) + 1;

    ReadyQueueIndex queue_index = 0;
    std::int64_t enqueue_time_microseconds = NO_READY_ENQUEUE_TIME;
    ReadyQueueIndex verified_queue_index = 0;
    do
    {
      queue_index = ready_queue_index_.load(std::memory_order_acquire);
      if (!queue_index)
      {
        return result;
      }

      if (queue_index == UPDATING_READY_QUEUE_INDEX)
      {
        continue;
      }

      enqueue_time_microseconds = min_ready_enqueue_time_.load(std::memory_order_acquire);
      verified_queue_index = ready_queue_index_.load(std::memory_order_acquire);
    }
    while (queue_index != verified_queue_index ||
      verified_queue_index == UPDATING_READY_QUEUE_INDEX ||
      enqueue_time_microseconds == NO_READY_ENQUEUE_TIME);

    result.has_operation = true;
    result.write_operations = queue_write_operations_(queue_index);
    result.enqueue_time = time_from_microseconds_(enqueue_time_microseconds);
    result.ready_time = operation_ready_time_i_(result.write_operations, result.enqueue_time);
    return result;
  }

  std::optional<RocksDBBatchingProcessorQueue::ReadyState>
  RocksDBBatchingProcessorQueue::publish_enqueued_queue_(
    ReadyQueueIndex queue_index,
    const Generics::Time& enqueue_time) noexcept
  {
    if (!queue_index || !try_publish_ready_queue_i_(queue_index, enqueue_time))
    {
      return std::nullopt;
    }

    return ready_state_i_();
  }

  bool
  RocksDBBatchingProcessorQueue::try_publish_ready_queue_i_(
    ReadyQueueIndex queue_index,
    const Generics::Time& enqueue_time) noexcept
  {
    ReadyQueueIndex expected = 0;
    if (!ready_queue_index_.compare_exchange_strong(
      expected,
      UPDATING_READY_QUEUE_INDEX,
      std::memory_order_acq_rel,
      std::memory_order_acquire))
    {
      return false;
    }

    min_ready_enqueue_time_.store(enqueue_time.microseconds(), std::memory_order_release);
    ready_queue_index_.store(queue_index, std::memory_order_release);
    return true;
  }

  RocksDBBatchingProcessorQueue::ReadyQueueIndex
  RocksDBBatchingProcessorQueue::reset_ready_queue_i_() noexcept
  {
    ReadyQueueIndex queue_index = ready_queue_index_.load(std::memory_order_acquire);
    while (queue_index == UPDATING_READY_QUEUE_INDEX ||
      !ready_queue_index_.compare_exchange_weak(
        queue_index,
        UPDATING_READY_QUEUE_INDEX,
        std::memory_order_acq_rel,
        std::memory_order_acquire))
    {
      if (queue_index == UPDATING_READY_QUEUE_INDEX)
      {
        queue_index = ready_queue_index_.load(std::memory_order_acquire);
      }
    }

    min_ready_enqueue_time_.store(NO_READY_ENQUEUE_TIME, std::memory_order_release);
    ready_queue_index_.store(0, std::memory_order_release);
    return queue_index;
  }

  void
  RocksDBBatchingProcessorQueue::recalculate_ready_queue_i_() noexcept
  {
    ReadyQueueIndex candidate = 0;
    Generics::Time candidate_enqueue_time;
    bool blocked_queue_seen = false;
    for (std::size_t offset = 0; offset < enqueue_buckets_.size(); ++offset)
    {
      const std::size_t bucket_index = (next_bucket_index_ + offset) % enqueue_buckets_.size();
      auto& bucket = *enqueue_buckets_[bucket_index];
      if (!bucket.has_pending_operations.load(std::memory_order_acquire))
      {
        continue;
      }

      std::lock_guard bucket_guard(bucket.lock);
      bool blocked_operation_seen = false;
      const OperationGroup* write_group = find_ready_group_i_(
        bucket.write_operations,
        true,
        blocked_operation_seen);
      blocked_queue_seen |= blocked_operation_seen;

      const OperationGroup* read_group = find_ready_group_i_(
        bucket.read_operations,
        false,
        blocked_operation_seen);
      blocked_queue_seen |= blocked_operation_seen;

      if (!write_group && !read_group)
      {
        continue;
      }

      const bool write_operations = !read_group ||
        (write_group && write_group->enqueue_time <= read_group->enqueue_time);
      const OperationGroup* group = write_operations ? write_group : read_group;
      candidate = make_ready_queue_index_(bucket_index, write_operations);
      candidate_enqueue_time = group->enqueue_time;
      break;
    }

    if (candidate)
    {
      try_publish_ready_queue_i_(candidate, candidate_enqueue_time);
    }

    recalculate_on_complete_ = blocked_queue_seen;
  }

  const RocksDBBatchingProcessorQueue::OperationGroup*
  RocksDBBatchingProcessorQueue::find_ready_group_i_(
    const OperationQueue& operation_queue,
    bool write_operations,
    bool& blocked_operation_seen) const noexcept
  {
    blocked_operation_seen = false;
    for (const auto& group : operation_queue.operation_groups)
    {
      bool ready;
      if (write_operations)
      {
        ready = in_flight_read_keys_.find(group.key) == in_flight_read_keys_.end() &&
          in_flight_write_keys_.find(group.key) == in_flight_write_keys_.end();
      }
      else
      {
        ready = in_flight_write_keys_.find(group.key) == in_flight_write_keys_.end();
      }

      if (ready)
      {
        return &group;
      }

      blocked_operation_seen = true;
    }

    return nullptr;
  }

  Generics::Time
  RocksDBBatchingProcessorQueue::operation_ready_time_i_(
    bool write_operations,
    const Generics::Time& enqueue_time) const noexcept
  {
    const unsigned long pending_operations = write_operations ?
      pending_write_operations_.load(std::memory_order_acquire) :
      pending_read_operations_.load(std::memory_order_acquire);
    return !accepting() || max_delay_ == Generics::Time::ZERO ||
      pending_operations >= batch_size_ ?
      enqueue_time : enqueue_time + max_delay_;
  }

  void
  RocksDBBatchingProcessorQueue::collect_batch_i_(
    Operations& batch,
    SelectedKeys& selected_keys,
    bool collect_reads,
    std::size_t start_bucket_index) noexcept
  {
    if (empty_i_())
    {
      return;
    }

    unsigned long collected = 0;
    for (std::size_t offset = 0; offset < enqueue_buckets_.size(); ++offset)
    {
      const std::size_t bucket_index =
        (start_bucket_index + offset) % enqueue_buckets_.size();
      auto& bucket = *enqueue_buckets_[bucket_index];
      if (!bucket.has_pending_operations.load(std::memory_order_acquire))
      {
        continue;
      }

      std::lock_guard bucket_guard(bucket.lock);
      OperationQueue& source = collect_reads ?
        bucket.read_operations : bucket.write_operations;
      collected += collect_from_queue_i_(
        source,
        collect_reads,
        batch,
        selected_keys);

      if (bucket.read_operations.empty() && bucket.write_operations.empty())
      {
        bucket.has_pending_operations.store(false, std::memory_order_release);
      }

      if (selected_keys.size() >= batch_size_)
      {
        break;
      }
    }

    if (collected)
    {
      auto& pending_operations = collect_reads ?
        pending_read_operations_ : pending_write_operations_;
      pending_operations.fetch_sub(collected, std::memory_order_acq_rel);
      pending_operations_.fetch_sub(collected, std::memory_order_acq_rel);
    }
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
      if (collect_reads &&
        in_flight_write_keys_.find(group.key) != in_flight_write_keys_.end())
      {
        continue;
      }

      if (!collect_reads &&
        (in_flight_read_keys_.find(group.key) != in_flight_read_keys_.end() ||
        in_flight_write_keys_.find(group.key) != in_flight_write_keys_.end()))
      {
        continue;
      }

      selected_keys.emplace_back(group.key);
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
