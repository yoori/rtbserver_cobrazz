
#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include <Generics/Time.hpp>

#include <Commons/Grpc/BatchingStreamBase.hpp>
#include <Commons/Grpc/GrpcClient.hpp>

namespace AdServer::Grpc
{
  class BatchingQueue final
  {
  public:
    using PendingRequest = BatchingPendingRequest;
    using PendingRequestPtr = std::shared_ptr<PendingRequest>;
    using PendingOperation = BatchingPendingOperation;
    using Batch = std::vector<PendingOperation>;
    struct EnqueueResult
    {
      Batch ready_batch;
      bool was_empty_before_push = false;
      bool queue_empty_after_enqueue = true;
    };

  public:
    explicit BatchingQueue(BatchingOptions options);

    ~BatchingQueue();

    EnqueueResult enqueue(
      PendingRequestPtr request,
      const Generics::Time& enqueue_time);

    bool try_pop_due_batch(
      Batch& batch,
      const Generics::Time& now,
      const Generics::Time& max_batch_delay);

    std::vector<Batch> drain_all();

    std::optional<Generics::Time> oldest_enqueue_time();

    std::size_t size() const noexcept;

  private:
    struct HotBucket
    {
      std::mutex lock;
      std::deque<PendingOperation> queue;
    };

  private:
    std::size_t hot_bucket_index_(std::uint64_t bucket_id) const noexcept;
    bool flush_hot_batch_if_full_(Batch& batch);
    std::size_t pop_reserved_items_(
      Batch& batch,
      std::size_t count,
      std::size_t start_bucket_index);
    std::optional<Generics::Time> oldest_enqueue_time_();

  private:
    const BatchingOptions options_;

    std::mutex hot_pop_lock_;
    std::vector<std::unique_ptr<HotBucket>> hot_buckets_;
    std::atomic<std::size_t> hot_size_{0};
    std::atomic<std::uint64_t> next_bucket_id_{0};
  };
}
