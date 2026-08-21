#pragma once

#include <atomic>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include <ProfilingCommons/ProfileMap/RocksDBBatchingProcessorQueue.hpp>

namespace AdServer::ProfilingCommons::Test
{
  class SimpleRocksDBBatchingProcessorQueue final
  {
  public:
    using BaseQueue = RocksDBBatchingProcessorQueue;
    using OperationType = BaseQueue::OperationType;
    using Operation = BaseQueue::Operation;
    using Operations = BaseQueue::Operations;
    using SelectedKeys = boost::unordered_flat_set<std::string_view>;
    using OperationCounts = BaseQueue::OperationCounts;
    using ReadyState = BaseQueue::ReadyState;
    using EnqueueResult = BaseQueue::EnqueueResult;

    SimpleRocksDBBatchingProcessorQueue(
      unsigned long batch_size,
      const Generics::Time& max_delay,
      unsigned long enqueue_buckets_count);

    ReadyState flush_pending() noexcept;

    EnqueueResult enqueue(Operations&& operations);

    void start_batch() noexcept;

    ReadyState collect_batch(Operations& batch, SelectedKeys& selected_keys) noexcept;

    ReadyState complete_batch(const Operations& batch) noexcept;

    void finish_batch() noexcept;

    bool drained() const noexcept;

    static bool is_write_operation(OperationType type) noexcept;

  private:
    using InFlightKeys = boost::unordered_flat_map<std::string, unsigned long>;

    void account_operation_(OperationCounts& counts, OperationType type) const noexcept;

    ReadyState request_ready_i_(
      bool has_ready_operations,
      bool force_update = false) noexcept;

    ReadyState make_ready_state_i_() const noexcept;

    const Operation* oldest_ready_operation_i_() const noexcept;

    Generics::Time operation_ready_time_i_(const Operation& operation) const noexcept;

    unsigned long collect_from_queue_i_(
      Operations& source,
      bool collect_reads,
      Operations& batch,
      SelectedKeys& selected_keys) noexcept;

    bool empty_i_() const noexcept;

  private:
    const unsigned long batch_size_;
    const Generics::Time max_delay_;

    mutable std::mutex lock_;
    Operations read_operations_;
    Operations write_operations_;
    InFlightKeys in_flight_read_keys_;
    InFlightKeys in_flight_write_keys_;
    std::atomic<unsigned long> pending_operations_{0};
    std::atomic<unsigned long> active_workers_{0};
    std::atomic<bool> ready_published_{false};
  };

  inline bool
  SimpleRocksDBBatchingProcessorQueue::is_write_operation(OperationType type) noexcept
  {
    return BaseQueue::is_write_operation(type);
  }
}
