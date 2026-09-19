#pragma once

#include <atomic>
#include <boost/intrusive/set.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <Generics/ActiveObject.hpp>

#include "RocksDBBatchingProfileMap.hpp"
#include "RocksDBProfileMapCache.hpp"

namespace AdServer::ProfilingCommons
{
  class RocksDBProfileMapProcessor final: public Generics::SimpleActiveObject
  {
  public:
    struct Stats
    {
      std::uint64_t check_total = 0;
      std::uint64_t get_total = 0;
      std::uint64_t touch_total = 0;
      std::uint64_t save_total = 0;
      std::uint64_t remove_total = 0;
      std::uint64_t read_batch_total = 0;
      std::uint64_t read_batch_total_time = 0;
      std::uint64_t write_batch_total = 0;
      std::uint64_t write_batch_total_time = 0;
      std::uint64_t failed_batch_total = 0;
      std::uint64_t failed_operation_total = 0;
      std::uint64_t failed_callback_expected = 0;
      std::uint64_t failed_callback_completed = 0;
      std::uint64_t workers = 0;
      std::uint64_t queue_count = 0;
      std::uint64_t pending_operations = 0;
      std::uint64_t active_workers = 0;
      std::uint64_t cache_limit = 0;
      std::uint64_t cache_size = 0;
      std::uint64_t cache_entries = 0;
      std::uint64_t cache_hits = 0;
      std::uint64_t cache_misses = 0;
      std::uint64_t cache_evictions = 0;
    };

    explicit RocksDBProfileMapProcessor(
      unsigned long workers_count = 2,
      unsigned long enqueue_buckets_count = 32,
      std::size_t cache_size = 0,
      unsigned long cache_portions_count = 256);

    RocksDBProfileMapProcessor(
      unsigned long workers_count,
      unsigned long enqueue_buckets_count,
      std::size_t cache_size,
      unsigned long cache_portions_count,
      int compaction_threads,
      int per_compaction_threads,
      int flush_threads,
      int max_mem_tables);

    ~RocksDBProfileMapProcessor() noexcept override;

    Stats stats() const noexcept;

  private:
    friend class RocksDBBatchingProfileMapImpl;

    using ProfileMapImpl = RocksDBBatchingProfileMapImpl;
    using MapQueue = RocksDBBatchingProcessorQueue;
    using Operation = MapQueue::Operation;
    using Operations = MapQueue::Operations;
    using SelectedKeys = MapQueue::SelectedKeys;
    using ReadyState = MapQueue::ReadyState;

    using ReadyHook = boost::intrusive::set_member_hook<
      boost::intrusive::link_mode<boost::intrusive::safe_link>>;

    struct Registration final
    {
      Registration(ProfileMapImpl& map_impl_val, MapQueue& queue_val, std::uint64_t id_val)
        : map_impl(map_impl_val),
          queue(queue_val),
          id(id_val)
      {}

      ProfileMapImpl& map_impl;
      MapQueue& queue;
      const std::uint64_t id;
      Generics::Time ready_time;
      ReadyHook ready_hook;
    };

    struct ReadyCompare
    {
      bool operator()(const Registration& left, const Registration& right) const noexcept;
    };

    using ReadyIndex = boost::intrusive::multiset<
      Registration,
      boost::intrusive::member_hook<
        Registration,
        ReadyHook,
        &Registration::ready_hook>,
      boost::intrusive::compare<ReadyCompare>,
      boost::intrusive::constant_time_size<false>>;

    using Registrations = boost::unordered_flat_map<
      MapQueue*,
      std::unique_ptr<Registration>>;

    using CacheResultType = RocksDBProfileMapCache::ResultType;
    using CacheLookupResult = RocksDBProfileMapCache::LookupResult;

    struct CacheWriteTicket final
    {
      std::uint64_t map_id = 0;
      Generics::StringHashAdapter key;
      RocksDBProfileMapCache::OperationState state;
    };

  private:
    void activate_object_() override;

    void deactivate_object_() override;

    void wait_object_() override;

    void register_map_(ProfileMapImpl& map_impl);

    void wait_unregister_map_(ProfileMapImpl& map_impl);

    bool enqueue_operation_(const ProfileMapImpl& map_impl, Operation& operation);

    void enqueue_operation_i_(const ProfileMapImpl& map_impl, Operation& operation);

    bool enqueue_operations_(const ProfileMapImpl& map_impl, Operations&& operations);

    void enqueue_operations_i_(const ProfileMapImpl& map_impl, Operations&& operations);

    void wait_pending_operations_(const ProfileMapImpl& map_impl);

    void worker_loop_() noexcept;

    bool pop_batch_(
      ProfileMapImpl*& map_impl,
      MapQueue*& map_queue,
      Operations& batch,
      SelectedKeys& selected_keys) noexcept;

    void complete_batch_(MapQueue& map_queue, const Operations& batch) noexcept;

    bool apply_ready_(MapQueue& map_queue, const ReadyState& state) noexcept;

    void remove_from_ready_i_(Registration& registration) noexcept;

    void add_operation_counts_(const MapQueue::OperationCounts& counts) noexcept;

    CacheLookupResult cache_lookup_(
      const ProfileMapImpl& map_impl,
      Operation& operation,
      bool allow_touch);

    CacheWriteTicket cache_publish_(const ProfileMapImpl& map_impl, Operation& operation);

    void cache_cancel_(const CacheWriteTicket& ticket) noexcept;

    bool cache_requires_write_(const ProfileMapImpl& map_impl, const Operation& operation)
      noexcept;

    void cache_complete_(const ProfileMapImpl& map_impl, const Operation& operation) noexcept;

    void cache_fail_(const ProfileMapImpl& map_impl, const Operation& operation) noexcept;

    std::uint64_t cache_fill_(
      const ProfileMapImpl& map_impl,
      const Operation& operation,
      Generics::ConstSmartMemBuf_var profile,
      const Generics::Time& write_time,
      bool schedule_touch) noexcept;

    void cache_cancel_touch_(
      const ProfileMapImpl& map_impl,
      const Generics::StringHashAdapter& key,
      std::uint64_t touch_revision) noexcept;

    void cache_remove_map_(std::uint64_t map_id) noexcept;

    void account_cache_hit_(MapQueue::OperationType type) noexcept;

  private:
    const unsigned long workers_count_;
    const unsigned long enqueue_buckets_count_;
    const std::size_t cache_size_;
    const int compaction_threads_;
    const int per_compaction_threads_;
    const int flush_threads_;
    const int max_mem_tables_;

    mutable std::mutex ready_lock_;
    mutable std::condition_variable ready_cond_;
    ReadyIndex ready_;
    Registrations registrations_;
    std::uint64_t next_registration_id_ = 0;
    std::atomic<bool> accepting_{false};
    std::atomic<bool> stopping_{true};

    std::atomic<std::uint64_t> check_total_{0};
    std::atomic<std::uint64_t> get_total_{0};
    std::atomic<std::uint64_t> touch_total_{0};
    std::atomic<std::uint64_t> save_total_{0};
    std::atomic<std::uint64_t> remove_total_{0};
    std::atomic<std::uint64_t> read_batch_total_{0};
    std::atomic<std::uint64_t> read_batch_total_time_{0};
    std::atomic<std::uint64_t> write_batch_total_{0};
    std::atomic<std::uint64_t> write_batch_total_time_{0};
    std::atomic<std::uint64_t> failed_batch_total_{0};
    std::atomic<std::uint64_t> failed_operation_total_{0};
    std::atomic<std::uint64_t> failed_callback_expected_{0};
    std::atomic<std::uint64_t> failed_callback_completed_{0};

    std::unique_ptr<RocksDBProfileMapCache> cache_;

    std::vector<std::thread> workers_;
  };
}
