#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/generic/generic_stub.h>

#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>

namespace AdServer::Grpc
{
  class BatchingQueue;
  class GrpcExecutor;

  struct BatchingPendingRequest
  {
    const char* full_method = nullptr;
    std::string payload;
    std::function<void(const adserver::grpc::BatchResponseItem&)> callback;
    AdServer::Grpc::InflightLimiter* inflight_limiter = nullptr;
    AdServer::Grpc::Client* stats_owner = nullptr;

    void complete(const adserver::grpc::BatchResponseItem& item)
    {
      if (stats_owner)
      {
        stats_owner->add_completed_stats(
          item.status_code() != grpc::StatusCode::OK);
      }

      if (callback)
      {
        callback(item);
      }

      if (inflight_limiter)
      {
        inflight_limiter->release();
      }
    }
  };

  struct BatchingPendingOperation
  {
    Generics::Time enqueue_time;
    std::shared_ptr<BatchingPendingRequest> request;
  };

  class BatchingStreamBase
    : public Generics::SimpleActiveObject,
      public virtual AdServer::Grpc::Client
  {
  public:
    using BatchResponseItem = adserver::grpc::BatchResponseItem;
    using BatchRequest = adserver::grpc::BatchRequest;
    using BatchResponse = adserver::grpc::BatchResponse;
    using BatchTransport = grpc::TemplatedGenericStub<BatchRequest, BatchResponse>;
    using PendingRequest = AdServer::Grpc::BatchingPendingOperation;
    using PendingBatch = std::vector<PendingRequest>;
    using ReadyCallback = std::function<void(BatchingStreamBase*)>;
    using ClosedCallback = std::function<void(BatchingStreamBase*)>;
    using DrainedCallback = std::function<void(BatchingStreamBase*)>;

    explicit BatchingStreamBase(
      std::shared_ptr<grpc::Channel> channel,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Grpc::BatchingQueue> batching_queue,
      unsigned int queue_index,
      AdServer::Grpc::Client* stats_owner,
      ReadyCallback ready_callback,
      ClosedCallback closed_callback,
      DrainedCallback drained_callback,
      AdServer::Grpc::BatchingOptions options = {});

    ~BatchingStreamBase() override;

    bool available() noexcept;
    std::uint64_t inflight_items() noexcept;
    bool try_start_write(
      PendingBatch&& pending_batch,
      bool measure_consumer_stream_write,
      PendingBatch* failed_batch);

  private:
    void activate_object_() override;
    void deactivate_object_() override;
    bool wait_more_() override;
    void wait_object_() override;
    void mark_finished_() noexcept;

  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
  };
}
