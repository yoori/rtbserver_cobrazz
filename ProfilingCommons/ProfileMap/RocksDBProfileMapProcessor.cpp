#include <algorithm>
#include <chrono>
#include <Commons/ThreadName.hpp>

#include "RocksDBProfileMapProcessor.hpp"

namespace AdServer::ProfilingCommons
{
  bool
  RocksDBProfileMapProcessor::ReadyCompare::operator()(
    const MapQueue& left,
    const MapQueue& right) const noexcept
  {
    return left.ready_time < right.ready_time;
  }

  RocksDBProfileMapProcessor::RocksDBProfileMapProcessor(
    unsigned long workers_count)
    : workers_count_(std::max(1UL, workers_count))
  {}

  RocksDBProfileMapProcessor::~RocksDBProfileMapProcessor() noexcept
  {
    while(!ready_.empty())
    {
      remove_from_ready_i_(*ready_.begin());
    }
  }

  void
  RocksDBProfileMapProcessor::activate_object_()
  {
    {
      std::lock_guard guard(ready_lock_);
      stopping_.store(false, std::memory_order_release);
      accepting_.store(true, std::memory_order_release);
    }

    workers_.reserve(workers_count_);
    try
    {
      for(unsigned long i = 0; i < workers_count_; ++i)
      {
        workers_.emplace_back(&RocksDBProfileMapProcessor::worker_loop_, this);
      }
    }
    catch(...)
    {
      {
        std::lock_guard guard(ready_lock_);
        accepting_.store(false, std::memory_order_release);
        stopping_.store(true, std::memory_order_release);
      }
      ready_cond_.notify_all();
      for(auto& worker : workers_)
      {
        worker.join();
      }
      workers_.clear();
      throw;
    }
  }

  void
  RocksDBProfileMapProcessor::deactivate_object_()
  {
    {
      std::lock_guard guard(ready_lock_);
      accepting_.store(false, std::memory_order_release);
      stopping_.store(true, std::memory_order_release);
    }
    ready_cond_.notify_all();
  }

  void
  RocksDBProfileMapProcessor::wait_object_()
  {
    for(auto& worker : workers_)
    {
      worker.join();
    }
    workers_.clear();
  }

  void
  RocksDBProfileMapProcessor::register_map_(ProfileMapImpl& map_impl)
  {
    MapQueue& map_queue = map_impl.processor_queue_;
    std::lock_guard map_guard(map_queue.lock);
    std::lock_guard ready_guard(ready_lock_);

    if (!accepting_.load(std::memory_order_acquire))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBProfileMapProcessor::register_map_(): processor isn't active");
    }

    if (map_queue.registered)
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBProfileMapProcessor::register_map_(): map is already registered");
    }

    map_queue.registered = true;
    map_queue.accepting = true;
  }

  void
  RocksDBProfileMapProcessor::unregister_map_(ProfileMapImpl& map_impl) noexcept
  {
    MapQueue& map_queue = map_impl.processor_queue_;

    bool signal_worker = false;

    {
      std::lock_guard guard(map_queue.lock);
      if (!map_queue.registered)
      {
        return;
      }
      map_queue.accepting = false;
      signal_worker = update_ready_(map_queue, oldest_ready_operation_i_(map_queue));
    }

    map_queue.drain_condition.notify_all();

    if (signal_worker)
    {
      ready_cond_.notify_all();
    }
  }

  void
  RocksDBProfileMapProcessor::wait_unregister_map_(ProfileMapImpl& map_impl)
  {
    MapQueue& map_queue = map_impl.processor_queue_;

    bool signal_worker = false;

    {
      std::lock_guard map_guard(map_queue.lock);
      if (!map_queue.registered)
      {
        return;
      }
      map_queue.accepting = false;
      signal_worker = update_ready_(map_queue, oldest_ready_operation_i_(map_queue));
    }

    map_queue.drain_condition.notify_all();

    if (signal_worker)
    {
      ready_cond_.notify_all();
    }

    std::unique_lock map_guard(map_queue.lock);
    while (true)
    {
      while (!empty_i_(map_queue) ||
        map_queue.active_workers.load(std::memory_order_relaxed) != 0)
      {
        map_queue.drain_condition.wait(map_guard);
      }

      {
        std::lock_guard guard(ready_lock_);
        if (!empty_i_(map_queue) ||
          map_queue.active_workers.load(std::memory_order_relaxed) != 0)
        {
          continue;
        }

        remove_from_ready_i_(map_queue);
        map_queue.registered = false;
        return;
      }
    }
  }

  bool
  RocksDBProfileMapProcessor::enqueue_operation_(
    const ProfileMapImpl& map_impl, Operation&& operation)
  {
    Operations operations;
    operations.emplace_back(std::move(operation));
    return enqueue_operations_(map_impl, std::move(operations));
  }

  bool
  RocksDBProfileMapProcessor::enqueue_operations_(
    const ProfileMapImpl& map_impl, Operations&& operations)
  {
    if (operations.empty())
    {
      return true;
    }

    MapQueue& map_queue = map_impl.processor_queue_;
    if (!accepting_.load(std::memory_order_acquire))
    {
      return false;
    }

    bool signal_worker = false;
    const Generics::Time now = Generics::Time::get_time_of_day();
    Operations read_operations;
    Operations write_operations;

    auto operation_it = operations.begin();
    while (operation_it != operations.end())
    {
      auto current_it = operation_it++;
      current_it->enqueue_time = now;
      Operations& target = ProfileMapImpl::is_write_operation_(current_it->type) ?
        write_operations : read_operations;
      target.splice(target.end(), operations, current_it);
    }

    {
      std::lock_guard map_guard(map_queue.lock);
      if (!map_queue.registered || !map_queue.accepting ||
        !accepting_.load(std::memory_order_acquire))
      {
        return false;
      }

      const bool was_empty = empty_i_(map_queue);
      const bool fills_read_batch =
        map_queue.read_operations.size() < map_queue.batch_size &&
        map_queue.read_operations.size() + read_operations.size() >= map_queue.batch_size;
      const bool fills_write_batch =
        map_queue.write_operations.size() < map_queue.batch_size &&
        map_queue.write_operations.size() + write_operations.size() >= map_queue.batch_size;

      map_queue.read_operations.splice(map_queue.read_operations.end(), read_operations);
      map_queue.write_operations.splice(map_queue.write_operations.end(), write_operations);

      if (was_empty)
      {
        signal_worker = update_ready_(map_queue, oldest_ready_operation_i_(map_queue));
      }
      else
      {
        if (fills_read_batch)
        {
          signal_worker = promote_ready_(map_queue, false);
        }
        if (fills_write_batch)
        {
          signal_worker = promote_ready_(map_queue, true) || signal_worker;
        }
      }
    }

    if (signal_worker)
    {
      ready_cond_.notify_all();
    }

    return true;
  }

  void
  RocksDBProfileMapProcessor::worker_loop_() noexcept
  {
    AdServer::Commons::set_current_thread_name("rdb-batch");

    MapQueue* map_queue = nullptr;
    Operations batch;
    SelectedKeys selected_keys;
    const auto scratch = ProfileMapImpl::create_batch_scratch_();

    while (pop_batch_(map_queue, batch, selected_keys))
    {
      ProfileMapImpl& map_impl = map_queue->map_impl;
      try
      {
        map_impl.process_batch_(batch, *scratch);
      }
      catch(const eh::Exception& ex)
      {
        map_impl.notify_failed_operations_(batch, ex.what());

        Sync::PosixGuard guard(map_impl.error_lock_);
        if (map_impl.background_error_.empty())
        {
          map_impl.background_error_ = ex.what();
        }
      }
      catch(...)
      {
        map_impl.notify_failed_operations_(batch, "unknown background error");

        Sync::PosixGuard guard(map_impl.error_lock_);
        if (map_impl.background_error_.empty())
        {
          map_impl.background_error_ = "unknown background error";
        }
      }

      complete_batch_(*map_queue, batch);

      map_queue = nullptr;

      selected_keys.clear();
      selected_keys.reserve(batch.size());

      batch.clear();
    }
  }

  bool
  RocksDBProfileMapProcessor::pop_batch_(
    MapQueue*& map_queue, Operations& batch, SelectedKeys& selected_keys) noexcept
  {
    while (true)
    {
      bool signal_worker = false;

      {
        std::unique_lock guard(ready_lock_);
        while (true)
        {
          while (ready_.empty())
          {
            if (stopping_.load(std::memory_order_acquire))
            {
              return false;
            }
            ready_cond_.wait(guard);
          }

          MapQueue& selected_queue = *ready_.begin();
          if (!stopping_.load(std::memory_order_acquire))
          {
            const Generics::Time now = Generics::Time::get_time_of_day();
            if (now < selected_queue.ready_time)
            {
              const auto deadline = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                  std::chrono::microseconds(
                    selected_queue.ready_time.microseconds())));
              ready_cond_.wait_until(guard, deadline);
              continue;
            }
          }

          map_queue = &selected_queue;
          map_queue->active_workers.fetch_add(1, std::memory_order_relaxed);
          remove_from_ready_i_(*map_queue);
          break;
        }
      }

      {
        std::lock_guard map_guard(map_queue->lock);
        collect_batch_i_(*map_queue, batch, selected_keys);

        if (!batch.empty())
        {
          const bool write_batch = ProfileMapImpl::is_write_operation_(batch.front().type);
          auto& in_flight_keys = write_batch ? map_queue->in_flight_write_keys :
            map_queue->in_flight_read_keys;
          for(const auto& operation : batch)
          {
            ++in_flight_keys[operation.key];
          }
        }

        signal_worker = update_ready_(*map_queue, oldest_ready_operation_i_(*map_queue));

        if (batch.empty())
        {
          const bool last_active_worker =
            map_queue->active_workers.fetch_sub(1, std::memory_order_relaxed) == 1;
          if (last_active_worker && empty_i_(*map_queue))
          {
            map_queue->drain_condition.notify_all();
          }
        }
      }

      if (signal_worker)
      {
        ready_cond_.notify_one();
      }

      if (!batch.empty())
      {
        return true;
      }

      map_queue = nullptr;
    }
  }

  void
  RocksDBProfileMapProcessor::collect_batch_i_(
    MapQueue& map_queue, Operations& batch, SelectedKeys& selected_keys) noexcept
  {
    if (empty_i_(map_queue))
    {
      return;
    }

    if (!batch.empty())
    {
      collect_from_queue_(
        map_queue,
        ProfileMapImpl::is_write_operation_(batch.front().type) ?
          map_queue.write_operations :
          map_queue.read_operations,
        batch,
        selected_keys);
    }
    else
    {
      const Operation* operation = oldest_ready_operation_i_(map_queue);
      if (operation)
      {
        collect_from_queue_(
          map_queue,
          ProfileMapImpl::is_write_operation_(operation->type) ?
            map_queue.write_operations :
            map_queue.read_operations,
          batch,
          selected_keys);
      }
    }
  }

  void
  RocksDBProfileMapProcessor::collect_from_queue_(
    MapQueue& map_queue,
    Operations& source,
    Operations& batch,
    SelectedKeys& selected_keys) noexcept
  {
    auto it = source.begin();
    const bool collect_reads = &source == &map_queue.read_operations;

    while (it != source.end())
    {
      const std::string_view key(it->key);
      const bool key_selected = selected_keys.find(key) != selected_keys.end();
      if (collect_reads && map_queue.in_flight_write_keys.find(it->key) !=
        map_queue.in_flight_write_keys.end())
      {
        ++it;
        continue;
      }

      if (!collect_reads &&
        (map_queue.in_flight_read_keys.find(it->key) !=
          map_queue.in_flight_read_keys.end() ||
        (!key_selected &&
          map_queue.in_flight_write_keys.find(it->key) !=
            map_queue.in_flight_write_keys.end())))
      {
        ++it;
        continue;
      }

      if (!key_selected)
      {
        if (selected_keys.size() >= map_queue.batch_size)
        {
          ++it;
          continue;
        }
        selected_keys.emplace(key);
      }

      auto current = it++;
      batch.splice(batch.end(), source, current);
    }
  }

  void
  RocksDBProfileMapProcessor::complete_batch_(MapQueue& map_queue, Operations& batch) noexcept
  {
    bool signal_worker = false;

    {
      std::lock_guard map_guard(map_queue.lock);

      const Operation* const oldest_before = oldest_ready_operation_i_(map_queue);
      const Generics::Time ready_time_before = oldest_before ?
        operation_ready_time_i_(map_queue, *oldest_before) : Generics::Time::ZERO;
      const bool write_operations_before = oldest_before &&
        ProfileMapImpl::is_write_operation_(oldest_before->type);
      const bool ready_indexed = map_queue.ready_indexed.load(std::memory_order_acquire);

      auto& in_flight_keys =
        !batch.empty() && ProfileMapImpl::is_write_operation_(batch.front().type) ?
        map_queue.in_flight_write_keys :
        map_queue.in_flight_read_keys;
      for(const auto& operation : batch)
      {
        const auto it = in_flight_keys.find(operation.key);
        if (it != in_flight_keys.end() && --it->second == 0)
        {
          in_flight_keys.erase(it);
        }
      }

      const Operation* const oldest_after = oldest_ready_operation_i_(map_queue);
      const Generics::Time ready_time_after = oldest_after ?
        operation_ready_time_i_(map_queue, *oldest_after) : Generics::Time::ZERO;
      const bool write_operations_after = oldest_after &&
        ProfileMapImpl::is_write_operation_(oldest_after->type);
      if ((oldest_after && !ready_indexed) ||
        static_cast<bool>(oldest_after) != static_cast<bool>(oldest_before) ||
        ready_time_after != ready_time_before ||
        write_operations_after != write_operations_before)
      {
        signal_worker = update_ready_(map_queue, oldest_after);
      }

      const bool last_active_worker =
        map_queue.active_workers.fetch_sub(1, std::memory_order_relaxed) == 1;
      if (last_active_worker && empty_i_(map_queue))
      {
        map_queue.drain_condition.notify_all();
      }
    }

    if (signal_worker)
    {
      ready_cond_.notify_all();
    }
  }

  bool
  RocksDBProfileMapProcessor::update_ready_(
    MapQueue& map_queue,
    const Operation* operation) noexcept
  {
    Generics::Time ready_time;
    bool write_operations = false;
    if (operation)
    {
      write_operations = ProfileMapImpl::is_write_operation_(operation->type);
      ready_time = operation_ready_time_i_(map_queue, *operation);
    }

    const Generics::Time now = Generics::Time::get_time_of_day();

    std::lock_guard guard(ready_lock_);
    const bool had_ready = !ready_.empty();
    const bool was_indexed = map_queue.ready_hook.is_linked();
    const bool was_immediately_ready = was_indexed && map_queue.ready_time <= now;
    const Generics::Time previous_first_time = had_ready ?
      ready_.begin()->ready_time : Generics::Time::ZERO;
    remove_from_ready_i_(map_queue);
    if (operation)
    {
      map_queue.oldest_operation_time = operation->enqueue_time;
      map_queue.ready_time = ready_time;
      map_queue.ready_write_operations = write_operations;
      ready_.insert(map_queue);
      map_queue.ready_indexed.store(true, std::memory_order_release);
    }

    const bool immediately_ready = operation && ready_time <= now;
    return (immediately_ready && !was_immediately_ready) ||
      (!ready_.empty() &&
        (!had_ready || ready_.begin()->ready_time < previous_first_time));
  }

  bool
  RocksDBProfileMapProcessor::promote_ready_(MapQueue& map_queue, bool write_operations) noexcept
  {
    std::lock_guard guard(ready_lock_);
    if (!map_queue.ready_hook.is_linked() ||
      map_queue.ready_write_operations != write_operations ||
      map_queue.ready_time == map_queue.oldest_operation_time)
    {
      return false;
    }

    remove_from_ready_i_(map_queue);
    map_queue.ready_time = map_queue.oldest_operation_time;
    ready_.insert(map_queue);
    map_queue.ready_indexed.store(true, std::memory_order_release);

    return true;
  }

  void
  RocksDBProfileMapProcessor::remove_from_ready_i_(MapQueue& map_queue) noexcept
  {
    if (map_queue.ready_hook.is_linked())
    {
      ready_.erase(ready_.iterator_to(map_queue));
    }

    map_queue.ready_indexed.store(false, std::memory_order_release);
  }

  const RocksDBProfileMapProcessor::Operation*
  RocksDBProfileMapProcessor::oldest_ready_operation_i_(const MapQueue& map_queue) noexcept
  {
    const Operation* read_operation = nullptr;
    for(const auto& operation : map_queue.read_operations)
    {
      if (map_queue.in_flight_write_keys.find(operation.key) == map_queue.in_flight_write_keys.end())
      {
        read_operation = &operation;
        break;
      }
    }

    const Operation* write_operation = nullptr;
    for(const auto& operation : map_queue.write_operations)
    {
      if (map_queue.in_flight_read_keys.find(operation.key) == map_queue.in_flight_read_keys.end() &&
        map_queue.in_flight_write_keys.find(operation.key) == map_queue.in_flight_write_keys.end())
      {
        write_operation = &operation;
        break;
      }
    }

    if (!read_operation)
    {
      return write_operation;
    }

    if (!write_operation)
    {
      return read_operation;
    }

    return read_operation->enqueue_time < write_operation->enqueue_time ? read_operation : write_operation;
  }

  Generics::Time
  RocksDBProfileMapProcessor::operation_ready_time_i_(
    const MapQueue& map_queue,
    const Operation& operation) noexcept
  {
    const Operations& operations =
      ProfileMapImpl::is_write_operation_(operation.type) ?
      map_queue.write_operations : map_queue.read_operations;
    return !map_queue.accepting ||
      map_queue.max_delay == Generics::Time::ZERO ||
      operations.size() >= map_queue.batch_size ?
      operation.enqueue_time : operation.enqueue_time + map_queue.max_delay;
  }

  bool
  RocksDBProfileMapProcessor::empty_i_(const MapQueue& map_queue) noexcept
  {
    return map_queue.read_operations.empty() && map_queue.write_operations.empty();
  }

  void
  RocksDBProfileMapProcessor::wait_pending_operations_(ProfileMapImpl& map_impl)
  {
    MapQueue& map_queue = map_impl.processor_queue_;

    std::unique_lock guard(map_queue.lock);
    while (map_queue.accepting &&
      (!empty_i_(map_queue) ||
        map_queue.active_workers.load(std::memory_order_relaxed) != 0))
    {
      map_queue.drain_condition.wait(guard);
    }
  }
}
