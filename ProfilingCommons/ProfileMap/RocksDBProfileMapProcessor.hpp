#pragma once

#include <atomic>
#include <boost/intrusive/set.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <Generics/ActiveObject.hpp>

#include "RocksDBBatchingProfileMap.hpp"

namespace AdServer::ProfilingCommons
{
  class RocksDBProfileMapProcessor final: public Generics::SimpleActiveObject
  {
  public:
    explicit RocksDBProfileMapProcessor(unsigned long workers_count = 2);

    ~RocksDBProfileMapProcessor() noexcept override;

  private:
    friend class RocksDBBatchingProfileMapImpl;

    using ProfileMapImpl = RocksDBBatchingProfileMapImpl;
    using Operation = ProfileMapImpl::Operation;
    using Operations = ProfileMapImpl::Operations;
    using SelectedKeys = boost::unordered_flat_set<std::string_view>;
    using MapQueue = ProfileMapImpl::ProcessorQueue;

    struct ReadyCompare
    {
      bool operator()(const MapQueue& left, const MapQueue& right) const noexcept;
    };

    using ReadyIndex = boost::intrusive::multiset<
      MapQueue,
      boost::intrusive::member_hook<
        MapQueue,
        MapQueue::ReadyHook,
        &MapQueue::ready_hook>,
      boost::intrusive::compare<ReadyCompare>,
      boost::intrusive::constant_time_size<false>>;

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

    bool pop_batch_(MapQueue*& map_queue, Operations& batch, SelectedKeys& selected_keys)
      noexcept;

    void collect_batch_i_(MapQueue& map_queue, Operations& batch, SelectedKeys& selected_keys)
      noexcept;

    void collect_from_queue_(
      MapQueue& map_queue,
      Operations& source,
      Operations& batch,
      SelectedKeys& selected_keys) noexcept;

    void complete_batch_(MapQueue& map_queue, Operations& batch) noexcept;

    bool update_ready_(MapQueue& map_queue, const Operation* operation) noexcept;

    bool promote_ready_(MapQueue& map_queue, bool write_operations) noexcept;

    void remove_from_ready_i_(MapQueue& map_queue) noexcept;

    static const Operation* oldest_ready_operation_i_(const MapQueue& map_queue) noexcept;

    static Generics::Time operation_ready_time_i_(
      const MapQueue& map_queue, const Operation& operation) noexcept;

    static bool empty_i_(const MapQueue& map_queue) noexcept;

  private:
    const unsigned long workers_count_;

    // When both locks are needed, MapQueue::lock is acquired first.
    mutable std::mutex ready_lock_;
    mutable std::condition_variable ready_cond_;
    ReadyIndex ready_;
    std::atomic<bool> accepting_{false};
    std::atomic<bool> stopping_{true};

    std::vector<std::thread> workers_;
  };
}
