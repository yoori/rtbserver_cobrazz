#pragma once

namespace AdServer::Grpc
{
  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  class GrpcBatchStreamCall final:
    public GrpcServiceBase::BatchStreamReadLimiter::Waiter,
    public std::enable_shared_from_this<
      GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>>
  {
  public:
    using Request = adserver::grpc::BatchRequest;
    using Response = adserver::grpc::BatchResponse;
    using RequestMethod = void (AsyncServiceType::*)(
      ::grpc::ServerContext*,
      ::grpc::ServerAsyncReaderWriter<Response, Request>*,
      ::grpc::CompletionQueue*,
      ::grpc::ServerCompletionQueue*,
      void*);

    GrpcBatchStreamCall(
      ServiceImplType* service_impl,
      AsyncServiceType* async_service,
      RequestMethod request_method,
      ::grpc::ServerCompletionQueue* completion_queue);

    ~GrpcBatchStreamCall() noexcept override;

    static void create(
      ServiceImplType* service_impl,
      AsyncServiceType* async_service,
      RequestMethod request_method,
      ::grpc::ServerCompletionQueue* completion_queue);

    void start_read_from_limiter() noexcept override;

  private:
    struct BatchContext;
    using BatchContextPtr = std::shared_ptr<BatchContext>;

    struct StartTag;
    struct ReadTag;
    struct WriteTag;
    struct FinishTag;

    bool start_request_();
    void handle_start_completion_(bool ok) noexcept;
    void try_start_read_() noexcept;
    bool start_read_reserved_() noexcept;
    void handle_read_completion_(bool ok, BatchContextPtr context) noexcept;
    void start_batch_processing_(BatchContextPtr context) noexcept;
    void process_batch_(BatchContextPtr context) noexcept;
    void handle_batch_processed_(
      BatchContextPtr context,
      std::exception_ptr exception) noexcept;
    void try_start_write_() noexcept;
    bool start_write_i_(BatchContextPtr context) noexcept;
    void handle_write_completion_(
      bool ok,
      BatchContextPtr context) noexcept;
    void handle_finish_completion_(bool ok) noexcept;
    void finish_or_delete_() noexcept;
    void start_inprogress_stats_(BatchContext& context);
    void finish_inprogress_stats_(BatchContext& context) noexcept;
    void drop_ready_responses_() noexcept;
    void drop_context_(BatchContextPtr context) noexcept;
    void maybe_finish_or_delete_() noexcept;

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    using DebugWatchdogState = GrpcBatchStreamDebugWatchdogState;

    void start_debug_response_watchdog_(BatchContext& context);
    void finish_debug_response_watchdog_(BatchContext& context) noexcept;
#endif

    ServiceImplType* const service_impl_;
    AsyncServiceType* const async_service_;
    const RequestMethod request_stream_;
    ::grpc::ServerCompletionQueue* const completion_queue_;
    ::grpc::ServerContext context_;
    ::grpc::ServerAsyncReaderWriter<Response, Request> responder_;
    std::mutex state_lock_;
    bool read_in_flight_ = false;
    bool write_in_flight_ = false;
    bool finish_in_flight_ = false;
    bool client_closed_ = false;
    bool closing_ = false;
    bool waiting_for_read_grant_ = false;
    std::size_t processing_count_ = 0;
    std::deque<BatchContextPtr> ready_responses_;
  };
}

#include "GrpcBatchStreamCall.tpp"
