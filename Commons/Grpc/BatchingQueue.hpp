
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
    using PendingOperationPtr = std::shared_ptr<PendingOperation>;
    using Batch = std::vector<PendingOperationPtr>;
    struct EnqueueResult
    {
      Batch ready_batch;
      bool was_empty_before_push = false;
    };

  public:
    explicit BatchingQueue(BatchingOptions options);

    ~BatchingQueue();

    EnqueueResult enqueue(Batch&& batch);

    bool try_pop_ready_batch(Batch& batch);

    bool try_pop_due_batch(Batch& batch);

    std::vector<Batch> drain_all();

    std::optional<Generics::Time> next_deadline();

  private:
    struct HotBucket
    {
      std::mutex lock;
      std::deque<PendingOperationPtr> queue;
    };

  private:
    std::size_t hot_bucket_index_(std::uint64_t bucket_id) const noexcept;
    bool flush_hot_batch_if_full_(Batch& batch);
    bool flush_hot_batch_if_due_(Batch& batch);
    std::optional<Generics::Time> hot_deadline_();

  private:
    const BatchingOptions options_;

    std::mutex hot_pop_lock_;
    std::vector<std::unique_ptr<HotBucket>> hot_buckets_;
    std::atomic<std::size_t> hot_size_{0};
    std::atomic<std::uint64_t> next_bucket_id_{0};

  };
}
