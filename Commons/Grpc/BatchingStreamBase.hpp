#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/generic/generic_stub.h>

#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/BatchingQueue.hpp>
#include <Commons/Grpc/GrpcClient.hpp>

namespace AdServer::Grpc
{
  class BatchingQueue;
  class GrpcExecutor;

  class BatchingStreamBase
    : public Generics::SimpleActiveObject,
      public virtual AdServer::Grpc::Client,
      public virtual ReferenceCounting::AtomicImpl
  {
  public:
    using BatchResponseItem = adserver::grpc::BatchResponseItem;
    using BatchRequest = adserver::grpc::BatchRequest;
    using BatchResponse = adserver::grpc::BatchResponse;
    using BatchTransport = grpc::TemplatedGenericStub<BatchRequest, BatchResponse>;
    using PendingRequest = AdServer::Grpc::BatchingPendingRequest;
    using PendingBatch = std::vector<std::shared_ptr<PendingRequest>>;
    using ReadyCallback = std::function<void(BatchingStreamBase*)>;
    using ClosedCallback = std::function<void(BatchingStreamBase*)>;

    explicit BatchingStreamBase(
      const std::string& endpoint,
      AdServer::Grpc::GrpcExecutor* grpc_executor,
      AdServer::Grpc::BatchingQueue* batching_queue,
      unsigned int queue_index,
      AdServer::Grpc::Client* stats_owner,
      ReadyCallback ready_callback,
      ClosedCallback closed_callback,
      AdServer::Grpc::BatchingOptions options = {});

    ~BatchingStreamBase() override;

    bool available() noexcept;
    bool try_start_write(
      PendingBatch&& pending_batch,
      bool measure_consumer_stream_write,
      PendingBatch* failed_batch);

  private:
    void activate_object_() override;
    void deactivate_object_() override;
    bool wait_more_() override;
    void wait_object_() override;

  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
  };
}
