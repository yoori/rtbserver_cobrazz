
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <grpcpp/support/status.h>

#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/GrpcClient.hpp>

namespace AdServer::Grpc
{
  struct BatchingPendingRequest
  {
    std::uint64_t request_id = 0;
    Generics::Time enqueue_time;
    Generics::Time write_time;
    const char* full_method = nullptr;
    std::string payload;
    std::function<void(const adserver::grpc::BatchResponseItem&)> callback;
  };

  class BatchingQueue final
    : public Generics::SimpleActiveObject
  {
  public:
    using PendingRequest = BatchingPendingRequest;
    using PendingRequestPtr = std::shared_ptr<PendingRequest>;
    using Batch = std::vector<PendingRequestPtr>;

  public:
    explicit BatchingQueue(BatchingOptions options);

    ~BatchingQueue() override;

    bool enqueue_request(
      const char* full_method,
      std::string payload,
      std::function<void(const adserver::grpc::BatchResponseItem&)> callback);

    bool pop_batch(Batch& batch);

    bool try_pop_batch(Batch& batch);

    void return_batch_to_front(Batch&& batch);

    void fail_all_with_error(
      grpc::StatusCode status_code,
      const char* status_message);

    void notify_all();

    std::optional<Generics::Time> next_deadline();

  private:
    struct HotBucket
    {
      std::mutex lock;
      std::deque<PendingRequestPtr> queue;
    };

  private:
    void activate_object_() override;
    void deactivate_object_() override;
    bool wait_more_() override;
    void wait_object_() override;

    std::size_t hot_bucket_index_(std::uint64_t request_id) const noexcept;
    bool flush_hot_batch_if_full_(Batch& batch);
    bool flush_hot_batch_if_due_(Batch& batch);
    bool has_due_hot_batch_();
    std::optional<Generics::Time> hot_deadline_();
    bool has_ready_batch_();

    void finish_batch_with_error_(
      Batch& batch,
      grpc::StatusCode status_code,
      const char* status_message);

    void finish_all_with_error_(
      grpc::StatusCode status_code,
      const char* status_message);

  private:
    const BatchingOptions options_;

    std::mutex hot_cv_lock_;
    std::mutex hot_pop_lock_;
    std::condition_variable hot_cv_;
    std::vector<std::unique_ptr<HotBucket>> hot_buckets_;
    std::atomic<std::size_t> hot_size_{0};
    std::atomic<std::uint64_t> next_request_id_{1};

    std::mutex ready_lock_;
    std::deque<Batch> ready_batches_;
  };
}
