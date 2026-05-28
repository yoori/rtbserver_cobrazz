#pragma once

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/support/status.h>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Commons/ActivityGate.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/BatchingQueue.hpp>
#include <Commons/Grpc/BatchingStreamBase.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>

namespace AdServer::Grpc
{
  inline const grpc::Status NO_ACTIVE_BATCHING_STREAMS_STATUS(
    grpc::StatusCode::UNAVAILABLE,
    NO_ACTIVE_BATCHING_STREAMS_MESSAGE);

  class AsyncBatchingClientBase
    : public Generics::CompositeActiveObject,
      public virtual AdServer::Grpc::Client
  {
  protected:
    DECLARE_EXCEPTION(InvalidParam, eh::DescriptiveException);

    AsyncBatchingClientBase(
      const std::string& endpoint,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      AdServer::Grpc::BatchingOptions options = {});

    ~AsyncBatchingClientBase() override;

    AdServer::Grpc::Stats stats() const noexcept override;

    template<typename Request, typename Response, typename Callback>
    void enqueue_request_(
      const char* full_method,
      const Request& request,
      Callback callback,
      const char* parse_error_message);

  private:
    using BatchingStreamPtr =
      std::shared_ptr<AdServer::Grpc::BatchingStreamBase>;
    using BatchingQueuePtr =
      std::shared_ptr<AdServer::Grpc::BatchingQueue>;
    struct StreamHolder;
    using StreamHolderPtr = std::shared_ptr<StreamHolder>;
    using ActivityGatePtr = std::shared_ptr<AdServer::Commons::ActivityGate>;
    using BatchResponseCallback =
      std::function<void(const adserver::grpc::BatchResponseItem&)>;

    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

    void enqueue_serialized_request_(
      const char* full_method,
      std::string payload,
      BatchResponseCallback callback);
    StreamHolderPtr make_stream_();
    void process_batch_(
      BatchingStreamBase::PendingBatch&& batch,
      const Generics::Time& now,
      const char* no_active_streams_context) noexcept;
    bool acquire_batch_inflight_(
      BatchingStreamBase::PendingBatch& batch,
      bool allow_limit_error) noexcept;
    bool maybe_start_connect_for_pending_(
      std::vector<BatchingStreamBase::PendingBatch>& failed_batches,
      const Generics::Time& now) noexcept;
    void finish_batch_with_error_(
      BatchingStreamBase::PendingBatch& batch,
      grpc::StatusCode status_code,
      const char* status_message,
      const char* source) noexcept;
    void finish_batches_with_error_(
      std::vector<BatchingStreamBase::PendingBatch>& batches,
      grpc::StatusCode status_code,
      const char* status_message,
      const char* source) noexcept;
    void start_connect_() noexcept;
    void release_or_dispatch_(const StreamHolderPtr& stream_holder) noexcept;
    bool dispatch_batch_(
      BatchingStreamBase::PendingBatch&& batch,
      const StreamHolderPtr& stream_holder) noexcept;
    void schedule_timing_coalesce_() noexcept;
    void run_timing_coalesce_(Generics::Time deadline) noexcept;
    void schedule_stream_shrink_() noexcept;
    void shrink_idle_streams_() noexcept;
    bool can_process_timed_batch_() const noexcept;
    bool coalesce_timed_batch_(const Generics::Time& now) noexcept;
    void handle_stream_ready_(BatchingStreamBase* stream);
    void handle_stream_closed_(BatchingStreamBase* stream) noexcept;
    void handle_stream_drained_(
      BatchingStreamBase* stream,
      const StreamHolderPtr& stream_holder) noexcept;
    void update_max_streams_(std::size_t streams_count) noexcept;
    void deactivate_streams_() noexcept;
    void wait_streams_() noexcept;
    void clear_streams_() noexcept;
    void clear_deferred_streams_() noexcept;

  private:
    const std::string endpoint_;
    const AdServer::Grpc::BatchingOptions options_;
    const std::size_t max_streams_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<grpc::Channel> channel_;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner_;
    BatchingQueuePtr batching_queue_;
    std::unordered_map<BatchingStreamBase*, StreamHolderPtr> streams_;
    std::vector<StreamHolderPtr> draining_streams_;
    std::vector<BatchingStreamPtr> deferred_streams_;
    mutable std::mutex streams_registry_lock_;
    std::deque<StreamHolderPtr> available_streams_;
    std::deque<BatchingStreamBase::PendingBatch> pending_batches_;
    StreamHolderPtr connecting_stream_;
    std::optional<Generics::Time> last_connect_failure_time_;
    bool connecting_ = false;
    mutable std::mutex streams_lock_;
    AdServer::Grpc::InflightLimiter inflight_limiter_;
    std::mutex coalesce_timer_lock_;
    std::optional<Generics::Time> coalesce_timer_deadline_;
    ActivityGatePtr submission_gate_;
    ActivityGatePtr timing_coalesce_gate_;
    ActivityGatePtr stream_shrink_gate_;
    std::atomic<unsigned int> next_queue_index_{0};
    std::atomic<std::size_t> up_streams_{0};
    std::atomic<std::uint64_t> max_streams_seen_{0};
  };

  template<typename Request, typename Response, typename Callback>
  void
  AsyncBatchingClientBase::enqueue_request_(
    const char* full_method,
    const Request& request,
    Callback callback,
    const char* parse_error_message)
  {
    auto payload = request.SerializeAsString();
    enqueue_serialized_request_(
      full_method,
      std::move(payload),
      [
        callback = std::move(callback),
        parse_error_message
      ](const auto& batch_response) mutable
      {
        const auto status = grpc::Status(
          static_cast<grpc::StatusCode>(batch_response.status_code()),
          batch_response.status_message());

        Response response;
        if (status.ok() && !response.ParseFromString(batch_response.payload()))
        {
          if (callback)
          {
            callback(
              grpc::Status(grpc::StatusCode::INTERNAL, parse_error_message),
              {});
          }
          return;
        }

        if (callback)
        {
          callback(status, response);
        }
      });
  }
}
