#pragma once

#include <atomic>
#include <boost/intrusive/set.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <Generics/ActiveObject.hpp>

#include "RocksDBBatchingProfileMap.hpp"

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
    };

    explicit RocksDBProfileMapProcessor(
      unsigned long workers_count = 2,
      unsigned long enqueue_buckets_count = 32);

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
      Registration(ProfileMapImpl& map_impl_val, MapQueue& queue_val)
        : map_impl(map_impl_val),
          queue(queue_val)
      {}

      ProfileMapImpl& map_impl;
      MapQueue& queue;
      std::uint64_t applied_generation = 0;
      Generics::Time ready_time;
      bool ready_write_operations = false;
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

  private:
    void activate_object_() override;

    void deactivate_object_() override;

    void wait_object_() override;

    void register_map_(ProfileMapImpl& map_impl);

    void unregister_map_(ProfileMapImpl& map_impl) noexcept;

    void wait_unregister_map_(ProfileMapImpl& map_impl);

    bool enqueue_operation_(const ProfileMapImpl& map_impl, Operation&& operation);

    bool enqueue_operations_(const ProfileMapImpl& map_impl, Operations&& operations);

    void wait_pending_operations_(ProfileMapImpl& map_impl);

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

  private:
    const unsigned long workers_count_;
    const unsigned long enqueue_buckets_count_;

    mutable std::mutex ready_lock_;
    mutable std::condition_variable ready_cond_;
    ReadyIndex ready_;
    Registrations registrations_;
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

    std::vector<std::thread> workers_;
  };
}
