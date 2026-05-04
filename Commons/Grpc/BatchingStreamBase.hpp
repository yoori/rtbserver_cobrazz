#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <grpcpp/client_context.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/status.h>
#include <grpc/impl/channel_arg_names.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <google/protobuf/arena.h>

#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/BatchingQueue.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>

namespace AdServer::Grpc
{
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
    enum class StreamState
    {
      Starting,
      Open,
      Closing,
      Broken,
      Finished
    };

    bool start_write_(
      std::vector<std::shared_ptr<PendingRequest>>&& pending_batch,
      bool measure_consumer_stream_write,
      std::vector<std::shared_ptr<PendingRequest>>* failed_batch);
    bool start_stream_();
    void maybe_start_read_i_();
    void maybe_start_shutdown_i_();
    void process_read_completion_(bool ok, std::unique_ptr<BatchResponse> response);
    void process_write_completion_(
      bool ok,
      std::vector<std::shared_ptr<PendingRequest>>&& write_requests);
    void process_start_completion_(bool ok);
    void process_writes_done_completion_(bool ok);
    void process_finish_completion_(bool ok);
    void finish_with_error_(
      grpc::StatusCode status_code,
      const char* status_message);
    void finish_requests_with_error_(
      std::vector<std::shared_ptr<PendingRequest>>& requests,
      grpc::StatusCode status_code,
      const char* status_message);
    void fail_inflight_with_error_(
      grpc::StatusCode status_code,
      const char* status_message);
    void handle_executor_shutdown_i_() noexcept;
    void complete_shutdown_i_() noexcept;
    bool accepts_requests_i_() const noexcept;
    void add_pending_completion_tag_() noexcept;
    void remove_pending_completion_tag_() noexcept;

    struct CompletionTag;
    struct StartTag;
    struct ReadTag;
    struct WriteTag;
    struct WritesDoneTag;
    struct FinishTag;

  private:
    const AdServer::Grpc::BatchingOptions options_;
    const std::string batch_stream_full_method_;
    ReferenceCounting::SmartPtr<AdServer::Grpc::BatchingQueue> batching_queue_;
    ReadyCallback ready_callback_;
    ClosedCallback closed_callback_;

    std::atomic<std::size_t> pending_completion_tags_{0};

    std::mutex inflight_lock_;
    std::unordered_map<std::uint64_t, std::shared_ptr<PendingRequest>> inflight_;

    std::mutex state_lock_;
    std::mutex callback_lock_;
    std::condition_variable shutdown_cv_;
    std::condition_variable callback_cv_;

    bool read_in_flight_ = false;
    bool writes_done_started_ = false;
    bool writes_done_in_flight_ = false;
    bool finish_in_flight_ = false;
    std::atomic_bool write_in_flight_{false};
    std::atomic<StreamState> stream_state_{StreamState::Starting};
    std::unique_ptr<grpc::ClientContext> stream_context_;
    std::unique_ptr<BatchTransport> batch_stub_;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<BatchRequest, BatchResponse>> stream_;
    grpc::Status finish_status_;
    google::protobuf::Arena write_arena_;

    AdServer::Grpc::GrpcExecutor_var grpc_executor_;
    const unsigned int queue_index_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor::CQ> grpc_queue_;
  };
}
