#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

#include <rocksdb/env.h>

#include <Commons/ThreadName.hpp>

#include "RocksDBProfileMapCache.hpp"
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
    unsigned long enqueue_buckets_count,
    std::size_t cache_size,
    unsigned long cache_portions_count)
    : RocksDBProfileMapProcessor(
        workers_count,
        enqueue_buckets_count,
        cache_size,
        cache_portions_count,
        32,
        4,
        32,
        6)
  {
  }

  RocksDBProfileMapProcessor::RocksDBProfileMapProcessor(
    unsigned long workers_count,
    unsigned long enqueue_buckets_count,
    std::size_t cache_size,
    unsigned long cache_portions_count,
    int compaction_threads,
    int per_compaction_threads,
    int flush_threads,
    int max_mem_tables)
    : workers_count_(std::max(1UL, workers_count)),
      enqueue_buckets_count_(std::max(1UL, enqueue_buckets_count)),
      cache_size_(cache_size),
      compaction_threads_(compaction_threads),
      per_compaction_threads_(per_compaction_threads),
      flush_threads_(flush_threads),
      max_mem_tables_(max_mem_tables),
      cache_(cache_size == 0 ? nullptr :
        std::make_unique<RocksDBProfileMapCache>(cache_size, cache_portions_count))
  {
    if (compaction_threads_ <= 0 || per_compaction_threads_ <= 0 || flush_threads_ <= 0 ||
      max_mem_tables_ < 2)
    {
      throw std::invalid_argument(
        "RocksDB thread counts must be positive and max mem tables must be at least two");
    }

    auto* env = rocksdb::Env::Default();
    env->SetBackgroundThreads(compaction_threads_, rocksdb::Env::LOW);
    env->SetBackgroundThreads(flush_threads_, rocksdb::Env::HIGH);
  }

  RocksDBProfileMapProcessor::~RocksDBProfileMapProcessor() noexcept
  {
    while (!ready_.empty())
    {
      remove_from_ready_i_(*ready_.begin());
    }
  }

  RocksDBProfileMapProcessor::Stats
  RocksDBProfileMapProcessor::stats() const noexcept
  {
    Stats result;
    result.check_total = check_total_.load(std::memory_order_relaxed);
    result.get_total = get_total_.load(std::memory_order_relaxed);
    result.touch_total = touch_total_.load(std::memory_order_relaxed);
    result.save_total = save_total_.load(std::memory_order_relaxed);
    result.remove_total = remove_total_.load(std::memory_order_relaxed);
    result.read_batch_total = read_batch_total_.load(std::memory_order_relaxed);
    result.read_batch_total_time = read_batch_total_time_.load(std::memory_order_relaxed);
    result.write_batch_total = write_batch_total_.load(std::memory_order_relaxed);
    result.write_batch_total_time = write_batch_total_time_.load(std::memory_order_relaxed);
    result.failed_batch_total = failed_batch_total_.load(std::memory_order_relaxed);
    result.failed_operation_total = failed_operation_total_.load(std::memory_order_relaxed);
    result.failed_callback_expected = failed_callback_expected_.load(std::memory_order_relaxed);
    result.failed_callback_completed = failed_callback_completed_.load(std::memory_order_relaxed);
    result.workers = workers_count_;
    result.cache_limit = cache_size_;
    if (cache_)
    {
      const auto cache_stats = cache_->stats();
      result.cache_size = cache_stats.size;
      result.cache_entries = cache_stats.entries;
      result.cache_hits = cache_stats.hits;
      result.cache_misses = cache_stats.misses;
      result.cache_evictions = cache_stats.evictions;
    }

    try
    {
      std::lock_guard guard(ready_lock_);
      result.queue_count = registrations_.size();
      for (const auto& [_, registration] : registrations_)
      {
        const auto queue_stats = registration->queue.stats();
        result.pending_operations += queue_stats.pending_operations;
        result.active_workers += queue_stats.active_workers;
      }
    }
    catch (...)
    {
    }

    return result;
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
      for (unsigned long i = 0; i < workers_count_; ++i)
      {
        workers_.emplace_back(&RocksDBProfileMapProcessor::worker_loop_, this);
      }
    }
    catch (...)
    {
      {
        std::lock_guard guard(ready_lock_);
        accepting_.store(false, std::memory_order_release);
        stopping_.store(true, std::memory_order_release);
      }
      ready_cond_.notify_all();

      for (auto& worker : workers_)
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
    std::lock_guard guard(ready_lock_);
    accepting_.store(false, std::memory_order_release);
  }

  void
  RocksDBProfileMapProcessor::wait_object_()
  {
    {
      std::lock_guard guard(ready_lock_);
      stopping_.store(true, std::memory_order_release);
    }

    ready_cond_.notify_all();

    for (auto& worker : workers_)
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
      if (!accepting_.load(std::memory_order_acquire))
      {
        throw ProfileMap<std::string>::Exception(
          "RocksDBProfileMapProcessor::register_map_(): processor isn't active");
      }

      if (registrations_.find(&map_queue) != registrations_.end())
      {
        throw ProfileMap<std::string>::Exception(
          "RocksDBProfileMapProcessor::register_map_(): map is already registered");
      }

      const std::uint64_t registration_id = ++next_registration_id_;
      registrations_.emplace(
        &map_queue,
        std::make_unique<Registration>(map_impl, map_queue, registration_id));
      map_impl.processor_registration_id_.store(registration_id, std::memory_order_release);
    }
  }

  void
  RocksDBProfileMapProcessor::wait_unregister_map_(ProfileMapImpl& map_impl)
  {
    wait_pending_operations_(map_impl);

    MapQueue& map_queue = map_impl.processor_queue_;
    std::uint64_t registration_id = 0;
    {
      std::lock_guard guard(ready_lock_);
      const auto it = registrations_.find(&map_queue);
      if (it == registrations_.end())
      {
        return;
      }

      registration_id = it->second->id;
      remove_from_ready_i_(*it->second);
      registrations_.erase(it);
      map_impl.processor_registration_id_.store(0, std::memory_order_release);
    }

    if (registration_id != 0)
    {
      cache_remove_map_(registration_id);
    }
  }

  bool
  RocksDBProfileMapProcessor::enqueue_operation_(
    const ProfileMapImpl& map_impl,
    Operation& operation)
  {
    auto submission_guard = map_impl.submission_gate_.enter();
    if (!submission_guard)
    {
      return false;
    }

    Operations operations;
    operations.emplace_back(std::move(operation));
    enqueue_operations_i_(map_impl, std::move(operations));
    return true;
  }

  void
  RocksDBProfileMapProcessor::enqueue_operation_i_(
    const ProfileMapImpl& map_impl,
    Operation& operation)
  {
    Operations operations;
    operations.emplace_back(std::move(operation));
    enqueue_operations_i_(map_impl, std::move(operations));
  }

  bool
  RocksDBProfileMapProcessor::enqueue_operations_(
    const ProfileMapImpl& map_impl,
    Operations&& operations)
  {
    if (operations.empty())
    {
      return true;
    }

    auto submission_guard = map_impl.submission_gate_.enter();
    if (!submission_guard)
    {
      return false;
    }

    enqueue_operations_i_(map_impl, std::move(operations));
    return true;
  }

  void
  RocksDBProfileMapProcessor::enqueue_operations_i_(
    const ProfileMapImpl& map_impl,
    Operations&& operations)
  {
    MapQueue& map_queue = map_impl.processor_queue_;
    auto result = map_queue.enqueue(std::move(operations));
    add_operation_counts_(result.counts);
    if (result.ready_state && apply_ready_(map_queue, *result.ready_state))
    {
      ready_cond_.notify_one();
    }
  }

  void
  RocksDBProfileMapProcessor::wait_pending_operations_(const ProfileMapImpl& map_impl)
  {
    MapQueue& map_queue = map_impl.processor_queue_;
    const ReadyState state = map_queue.flush_pending();
    if (apply_ready_(map_queue, state))
    {
      ready_cond_.notify_all();
    }

    map_queue.wait_pending();
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

    while (pop_batch_(map_impl, map_queue, batch, selected_keys))
    {
      const bool write_batch = MapQueue::is_write_operation(batch.front().type);
      if (write_batch)
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
      catch (const eh::Exception& ex)
      {
        map_impl->set_background_error_(ex.what());
        failed_batch_total_.fetch_add(1, std::memory_order_relaxed);
        failed_operation_total_.fetch_add(batch.size(), std::memory_order_relaxed);
        for (const auto& operation : batch)
        {
          cache_fail_(*map_impl, operation);
        }
        map_impl->notify_failed_operations_(batch, ex.what());
      }
      catch (...)
      {
        map_impl->set_background_error_("unknown background error");
        failed_batch_total_.fetch_add(1, std::memory_order_relaxed);
        failed_operation_total_.fetch_add(batch.size(), std::memory_order_relaxed);
        for (const auto& operation : batch)
        {
          cache_fail_(*map_impl, operation);
        }
        map_impl->notify_failed_operations_(batch, "unknown background error");
      }
      batch_timer.stop();

      const std::uint64_t elapsed_us = batch_timer.elapsed_time().microseconds();
      if (write_batch)
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
    while (true)
    {
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

          Registration& registration = *ready_.begin();
          if (!stopping_.load(std::memory_order_acquire))
          {
            const Generics::Time now = Generics::Time::get_time_of_day();
            if (now < registration.ready_time)
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
      if (apply_ready_(*map_queue, state))
      {
        ready_cond_.notify_one();
      }

      if (!batch.empty())
      {
        return true;
      }

      map_queue->finish_batch();

      map_impl = nullptr;
      map_queue = nullptr;
    }
  }

  void
  RocksDBProfileMapProcessor::complete_batch_(MapQueue& map_queue, const Operations& batch)
    noexcept
  {
    const ReadyState state = map_queue.complete_batch(batch);
    if (apply_ready_(map_queue, state))
    {
      ready_cond_.notify_one();
    }
    map_queue.finish_batch();
  }

  bool
  RocksDBProfileMapProcessor::apply_ready_(MapQueue& map_queue, const ReadyState& state) noexcept
  {
    if (!state.has_operation)
    {
      return false;
    }

    const Generics::Time now = Generics::Time::get_time_of_day();
    std::lock_guard guard(ready_lock_);

    const auto it = registrations_.find(&map_queue);
    if (it == registrations_.end())
    {
      return false;
    }

    Registration& registration = *it->second;
    const bool had_ready = !ready_.empty();
    const bool was_indexed = registration.ready_hook.is_linked();
    const bool was_immediately_ready = was_indexed && registration.ready_time <= now;
    const Generics::Time previous_first_time = had_ready ?
      ready_.begin()->ready_time : Generics::Time::ZERO;

    if (!was_indexed)
    {
      registration.ready_time = state.ready_time;
      ready_.insert(registration);
    }
    else if (state.ready_time < registration.ready_time)
    {
      remove_from_ready_i_(registration);
      registration.ready_time = state.ready_time;
      ready_.insert(registration);
    }

    const bool immediately_ready = registration.ready_time <= now;
    return (immediately_ready && !was_immediately_ready) ||
      (!ready_.empty() && (!had_ready || ready_.begin()->ready_time < previous_first_time));
  }

  void
  RocksDBProfileMapProcessor::remove_from_ready_i_(Registration& registration) noexcept
  {
    if (registration.ready_hook.is_linked())
    {
      ready_.erase(ready_.iterator_to(registration));
    }
  }

  void
  RocksDBProfileMapProcessor::add_operation_counts_(
    const MapQueue::OperationCounts& counts) noexcept
  {
    if (counts.check)
    {
      check_total_.fetch_add(counts.check, std::memory_order_relaxed);
    }

    if (counts.get)
    {
      get_total_.fetch_add(counts.get, std::memory_order_relaxed);
    }

    if (counts.touch)
    {
      touch_total_.fetch_add(counts.touch, std::memory_order_relaxed);
    }

    if (counts.save)
    {
      save_total_.fetch_add(counts.save, std::memory_order_relaxed);
    }

    if (counts.remove)
    {
      remove_total_.fetch_add(counts.remove, std::memory_order_relaxed);
    }
  }

  RocksDBProfileMapProcessor::CacheLookupResult
  RocksDBProfileMapProcessor::cache_lookup_(
    const ProfileMapImpl& map_impl,
    Operation& operation,
    bool allow_touch)
  {
    if (!cache_)
    {
      return {};
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    if (map_id == 0)
    {
      return {};
    }

    return cache_->lookup(
      map_id,
      operation.key,
      map_impl.expire_time_,
      operation.cache,
      allow_touch);
  }

  RocksDBProfileMapProcessor::CacheWriteTicket
  RocksDBProfileMapProcessor::cache_publish_(
    const ProfileMapImpl& map_impl,
    Operation& operation)
  {
    CacheWriteTicket ticket;
    if (!cache_)
    {
      return ticket;
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    if (map_id != 0)
    {
      ticket.map_id = map_id;
      ticket.key = operation.key;
      cache_->publish(
        map_id,
        operation.key,
        operation.profile,
        operation.type == MapQueue::OT_REMOVE,
        operation.cache);
      ticket.state = operation.cache;
    }

    return ticket;
  }

  void
  RocksDBProfileMapProcessor::cache_cancel_(const CacheWriteTicket& ticket) noexcept
  {
    if (!cache_ || ticket.state.write_revision == 0)
    {
      return;
    }

    cache_->cancel(ticket.map_id, ticket.key, ticket.state);
  }

  bool
  RocksDBProfileMapProcessor::cache_requires_write_(
    const ProfileMapImpl& map_impl,
    const Operation& operation) noexcept
  {
    if (!cache_ ||
      (operation.cache.write_revision == 0 && operation.cache.touch_revision == 0))
    {
      return true;
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    return map_id == 0 || cache_->requires_write(
      map_id,
      operation.key,
      operation.cache,
      operation.type == MapQueue::OT_TOUCH);
  }

  void
  RocksDBProfileMapProcessor::cache_complete_(
    const ProfileMapImpl& map_impl,
    const Operation& operation) noexcept
  {
    if (!cache_ ||
      (operation.cache.write_revision == 0 && operation.cache.touch_revision == 0))
    {
      return;
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    if (map_id != 0)
    {
      cache_->complete(
        map_id,
        operation.key,
        operation.cache,
        operation.type == MapQueue::OT_TOUCH);
    }
  }

  void
  RocksDBProfileMapProcessor::cache_fail_(
    const ProfileMapImpl& map_impl,
    const Operation& operation) noexcept
  {
    if (!cache_ ||
      (operation.cache.write_revision == 0 && operation.cache.touch_revision == 0))
    {
      return;
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    if (map_id != 0)
    {
      cache_->fail(
        map_id,
        operation.key,
        operation.cache,
        operation.type == MapQueue::OT_TOUCH);
    }
  }

  std::uint64_t
  RocksDBProfileMapProcessor::cache_fill_(
    const ProfileMapImpl& map_impl,
    const Operation& operation,
    Generics::ConstSmartMemBuf_var profile,
    const Generics::Time& write_time,
    bool schedule_touch) noexcept
  {
    if (!cache_)
    {
      return 0;
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    return map_id == 0 ? 0 :
      cache_->fill(
        map_id,
        operation.key,
        operation.cache,
        std::move(profile),
        write_time,
        schedule_touch);
  }

  void
  RocksDBProfileMapProcessor::cache_cancel_touch_(
    const ProfileMapImpl& map_impl,
    const Generics::StringHashAdapter& key,
    std::uint64_t touch_revision) noexcept
  {
    if (!cache_ || touch_revision == 0)
    {
      return;
    }

    const std::uint64_t map_id =
      map_impl.processor_registration_id_.load(std::memory_order_acquire);
    if (map_id != 0)
    {
      cache_->cancel_touch(map_id, key, touch_revision);
    }
  }

  void RocksDBProfileMapProcessor::cache_remove_map_(std::uint64_t map_id) noexcept
  {
    if (cache_)
    {
      cache_->remove_map(map_id);
    }
  }

  void
  RocksDBProfileMapProcessor::account_cache_hit_(MapQueue::OperationType type) noexcept
  {
    MapQueue::OperationCounts counts;
    switch (type)
    {
      case MapQueue::OT_CHECK:
        counts.check = 1;
        break;
      case MapQueue::OT_GET:
        counts.get = 1;
        break;
      default:
        return;
    }
    add_operation_counts_(counts);
  }
}
