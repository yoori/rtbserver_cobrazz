#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/support/status.h>
#include <google/protobuf/arena.h>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Commons/ActivityGate.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/BatchingQueue.hpp>
#include <Commons/Grpc/BatchingStreamBase.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/ResponseHolder.hpp>

namespace AdServer::Grpc
{
  namespace Detail
  {
    inline constexpr const char PARSE_RESPONSE_SOURCE[] = "parse_response";

    inline
    std::string hex_prefix_(const std::string& value, const std::size_t limit)
    {
      static constexpr char HEX[] = "0123456789abcdef";

      const auto size = std::min(value.size(), limit);
      std::string result;
      result.reserve(size * 2);
      for (std::size_t i = 0; i < size; ++i)
      {
        const auto ch = static_cast<unsigned char>(value[i]);
        result.push_back(HEX[ch >> 4]);
        result.push_back(HEX[ch & 0x0F]);
      }

      return result;
    }

    inline
    std::string make_parse_error_message_(
      const char* parse_error_message,
      const char* full_method,
      const std::string& payload)
    {
      std::string message = parse_error_message ? parse_error_message : "";
      message += " [method=";
      message += full_method ? full_method : "";
      message += ", payload_size=";
      message += std::to_string(payload.size());
      message += ", payload_hex_prefix=";
      message += hex_prefix_(payload, 64);
      message += "]";
      return message;
    }
  }

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
    void schedule_timing_coalesce_(
      const Generics::Time& oldest_enqueue_time) noexcept;
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
    std::shared_mutex coalesce_timer_lock_;
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
        this,
        full_method,
        callback = std::move(callback),
        parse_error_message
      ](const auto& batch_response) mutable
      {
        const auto status = grpc::Status(
          static_cast<grpc::StatusCode>(batch_response.status_code()),
          batch_response.status_message());

        auto arena = std::make_shared<google::protobuf::Arena>();
        auto* response =
          google::protobuf::Arena::CreateMessage<Response>(arena.get());
        if (status.ok() && !response->ParseFromString(batch_response.payload()))
        {
          assert(false && "Unable to parse grpc response");
          const auto message = AdServer::Grpc::Detail::make_parse_error_message_(
            parse_error_message,
            full_method,
            batch_response.payload());
          set_last_error(
            endpoint_,
            grpc::StatusCode::INTERNAL,
            message,
            AdServer::Grpc::Detail::PARSE_RESPONSE_SOURCE);
          if (callback)
          {
            callback(
              grpc::Status(grpc::StatusCode::INTERNAL, message),
              AdServer::Grpc::ResponseHolder<Response>::make_arena(
                *response,
                std::move(arena)));
          }
          return;
        }

        if (callback)
        {
          callback(
            status,
            AdServer::Grpc::ResponseHolder<Response>::make_arena(
              *response,
              std::move(arena)));
        }
      });
  }
}
