#include <algorithm>
#include <chrono>

#include <Commons/ThreadName.hpp>

#include "RocksDBProfileMapProcessor.hpp"

namespace AdServer::ProfilingCommons
{
  bool
  RocksDBProfileMapProcessor::ReadyCompare::operator()(
    const Registration& left,
    const Registration& right) const noexcept
  {
    return left.ready_time < right.ready_time;
  }

  RocksDBProfileMapProcessor::RocksDBProfileMapProcessor(
    unsigned long workers_count,
    unsigned long enqueue_buckets_count)
    : workers_count_(std::max(1UL, workers_count)),
      enqueue_buckets_count_(std::max(1UL, enqueue_buckets_count))
  {}

  RocksDBProfileMapProcessor::~RocksDBProfileMapProcessor() noexcept
  {
    while(!ready_.empty())
    {
      remove_from_ready_i_(*ready_.begin());
    }
  }

  RocksDBProfileMapProcessor::Stats
  RocksDBProfileMapProcessor::stats() const noexcept
  {
    return {
      check_total_.load(std::memory_order_relaxed),
      get_total_.load(std::memory_order_relaxed),
      touch_total_.load(std::memory_order_relaxed),
      save_total_.load(std::memory_order_relaxed),
      remove_total_.load(std::memory_order_relaxed),
      read_batch_total_.load(std::memory_order_relaxed),
      read_batch_total_time_.load(std::memory_order_relaxed),
      write_batch_total_.load(std::memory_order_relaxed),
      write_batch_total_time_.load(std::memory_order_relaxed)
    };
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
    {
      std::lock_guard guard(ready_lock_);
      if(!accepting_.load(std::memory_order_acquire))
      {
        throw ProfileMap<std::string>::Exception(
          "RocksDBProfileMapProcessor::register_map_(): processor isn't active");
      }

      if(registrations_.find(&map_queue) != registrations_.end())
      {
        throw ProfileMap<std::string>::Exception(
          "RocksDBProfileMapProcessor::register_map_(): map is already registered");
      }

      registrations_.emplace(
        &map_queue,
        std::make_unique<Registration>(map_impl, map_queue));
    }

    map_queue.activate();
  }

  void
  RocksDBProfileMapProcessor::unregister_map_(ProfileMapImpl& map_impl) noexcept
  {
    MapQueue& map_queue = map_impl.processor_queue_;
    const ReadyState state = map_queue.deactivate();
    if(apply_ready_(map_queue, state))
    {
      ready_cond_.notify_all();
    }
  }

  void
  RocksDBProfileMapProcessor::wait_unregister_map_(ProfileMapImpl& map_impl)
  {
    MapQueue& map_queue = map_impl.processor_queue_;
    const ReadyState state = map_queue.deactivate();
    if(apply_ready_(map_queue, state))
    {
      ready_cond_.notify_all();
    }

    map_queue.wait_drained();

    std::lock_guard guard(ready_lock_);
    const auto it = registrations_.find(&map_queue);
    if(it == registrations_.end())
    {
      return;
    }

    remove_from_ready_i_(*it->second);
    registrations_.erase(it);
  }

  bool
  RocksDBProfileMapProcessor::enqueue_operation_(
    const ProfileMapImpl& map_impl,
    Operation&& operation)
  {
    Operations operations;
    operations.emplace_back(std::move(operation));
    return enqueue_operations_(map_impl, std::move(operations));
  }

  bool
  RocksDBProfileMapProcessor::enqueue_operations_(
    const ProfileMapImpl& map_impl,
    Operations&& operations)
  {
    if(operations.empty())
    {
      return true;
    }

    if(!accepting_.load(std::memory_order_acquire))
    {
      return false;
    }

    MapQueue& map_queue = map_impl.processor_queue_;
    auto result = map_queue.enqueue(std::move(operations));
    if(!result.accepted)
    {
      return false;
    }

    add_operation_counts_(result.counts);
    if(result.ready_state && apply_ready_(map_queue, *result.ready_state))
    {
      ready_cond_.notify_one();
    }

    return true;
  }

  void
  RocksDBProfileMapProcessor::wait_pending_operations_(ProfileMapImpl& map_impl)
  {
    map_impl.processor_queue_.wait_pending();
  }

  void
  RocksDBProfileMapProcessor::worker_loop_() noexcept
  {
    AdServer::Commons::set_current_thread_name("rdb-batch");

    ProfileMapImpl* map_impl = nullptr;
    MapQueue* map_queue = nullptr;
    Operations batch;
    SelectedKeys selected_keys;
    const auto scratch = ProfileMapImpl::create_batch_scratch_();

    while(pop_batch_(map_impl, map_queue, batch, selected_keys))
    {
      const bool write_batch = MapQueue::is_write_operation(batch.front().type);
      if(write_batch)
      {
        write_batch_total_.fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
        read_batch_total_.fetch_add(1, std::memory_order_relaxed);
      }

      Generics::Timer batch_timer;
      batch_timer.start();
      try
      {
        map_impl->process_batch_(batch, *scratch);
      }
      catch(const eh::Exception& ex)
      {
        map_impl->notify_failed_operations_(batch, ex.what());
        map_impl->set_background_error_(ex.what());
      }
      catch(...)
      {
        map_impl->notify_failed_operations_(batch, "unknown background error");
        map_impl->set_background_error_("unknown background error");
      }
      batch_timer.stop();

      const std::uint64_t elapsed_us = batch_timer.elapsed_time().microseconds();
      if(write_batch)
      {
        write_batch_total_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
      }
      else
      {
        read_batch_total_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
      }

      complete_batch_(*map_queue, batch);

      map_impl = nullptr;
      map_queue = nullptr;
      selected_keys.clear();
      selected_keys.reserve(batch.size());
      batch.clear();
    }
  }

  bool
  RocksDBProfileMapProcessor::pop_batch_(
    ProfileMapImpl*& map_impl,
    MapQueue*& map_queue,
    Operations& batch,
    SelectedKeys& selected_keys) noexcept
  {
    while(true)
    {
      {
        std::unique_lock guard(ready_lock_);
        while(true)
        {
          while(ready_.empty())
          {
            if(stopping_.load(std::memory_order_acquire))
            {
              return false;
            }
            ready_cond_.wait(guard);
          }

          Registration& registration = *ready_.begin();
          if(!stopping_.load(std::memory_order_acquire))
          {
            const Generics::Time now = Generics::Time::get_time_of_day();
            if(now < registration.ready_time)
            {
              const auto deadline = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                  std::chrono::microseconds(registration.ready_time.microseconds())));
              ready_cond_.wait_until(guard, deadline);
              continue;
            }
          }

          map_impl = &registration.map_impl;
          map_queue = &registration.queue;
          map_queue->start_batch();
          remove_from_ready_i_(registration);
          break;
        }
      }

      const ReadyState state = map_queue->collect_batch(batch, selected_keys);
      if(apply_ready_(*map_queue, state))
      {
        ready_cond_.notify_one();
      }

      if(!batch.empty())
      {
        return true;
      }

      map_queue->finish_batch();

      map_impl = nullptr;
      map_queue = nullptr;
    }
  }

  void
  RocksDBProfileMapProcessor::complete_batch_(
    MapQueue& map_queue,
    const Operations& batch) noexcept
  {
    const ReadyState state = map_queue.complete_batch(batch);
    if(apply_ready_(map_queue, state))
    {
      ready_cond_.notify_one();
    }
    map_queue.finish_batch();
  }

  bool
  RocksDBProfileMapProcessor::apply_ready_(
    MapQueue& map_queue,
    const ReadyState& state) noexcept
  {
    const Generics::Time now = Generics::Time::get_time_of_day();
    std::lock_guard guard(ready_lock_);

    const auto it = registrations_.find(&map_queue);
    if(it == registrations_.end())
    {
      return false;
    }

    Registration& registration = *it->second;
    if(state.generation < registration.applied_generation)
    {
      return false;
    }

    const bool had_ready = !ready_.empty();
    const bool was_indexed = registration.ready_hook.is_linked();
    const bool was_immediately_ready = was_indexed && registration.ready_time <= now;
    const Generics::Time previous_first_time = had_ready ?
      ready_.begin()->ready_time : Generics::Time::ZERO;

    registration.applied_generation = state.generation;
    if(!state.has_operation)
    {
      remove_from_ready_i_(registration);
    }
    else if(!was_indexed || registration.ready_time != state.ready_time ||
      registration.ready_write_operations != state.write_operations)
    {
      remove_from_ready_i_(registration);
      registration.ready_time = state.ready_time;
      registration.ready_write_operations = state.write_operations;
      ready_.insert(registration);
    }

    const bool immediately_ready = state.has_operation && state.ready_time <= now;
    return (immediately_ready && !was_immediately_ready) ||
      (!ready_.empty() && (!had_ready || ready_.begin()->ready_time < previous_first_time));
  }

  void
  RocksDBProfileMapProcessor::remove_from_ready_i_(Registration& registration) noexcept
  {
    if(registration.ready_hook.is_linked())
    {
      ready_.erase(ready_.iterator_to(registration));
    }
  }

  void
  RocksDBProfileMapProcessor::add_operation_counts_(
    const MapQueue::OperationCounts& counts) noexcept
  {
    if(counts.check)
    {
      check_total_.fetch_add(counts.check, std::memory_order_relaxed);
    }
    if(counts.get)
    {
      get_total_.fetch_add(counts.get, std::memory_order_relaxed);
    }
    if(counts.touch)
    {
      touch_total_.fetch_add(counts.touch, std::memory_order_relaxed);
    }
    if(counts.save)
    {
      save_total_.fetch_add(counts.save, std::memory_order_relaxed);
    }
    if(counts.remove)
    {
      remove_total_.fetch_add(counts.remove, std::memory_order_relaxed);
    }
  }
}
