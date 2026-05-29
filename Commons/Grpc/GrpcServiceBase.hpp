#pragma once

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#define ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
#include <thread>

#include <boost/asio.hpp>
#endif

#include <grpcpp/grpcpp.h>

#include <Commons/ActivityGate.hpp>
#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Generics/Time.hpp>

#define MAKE_GRPC_CALL(RequestType, ResponseType, MethodName) \
  make_grpc_call<RequestType, ResponseType>( \
    &AsyncService::Request##MethodName, \
    &ServiceImpl::MethodName, \
    #MethodName)

namespace AdServer::Grpc
{
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
  struct GrpcBatchStreamDebugWatchdogState
  {
    std::atomic_bool done{false};
    std::string peer;
    std::string first_method;
    int items_size = 0;
  };

  class GrpcBatchStreamDebugTimerService final
  {
  public:
    using WatchdogState = GrpcBatchStreamDebugWatchdogState;

    static GrpcBatchStreamDebugTimerService& instance();

    void schedule(std::shared_ptr<WatchdogState> state);

  private:
    GrpcBatchStreamDebugTimerService();

    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
    std::thread thread_;
  };
#endif

  template<typename ServiceImplType, typename AsyncServiceType>
  class GrpcBatchStreamCall;

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  class GrpcUnaryCall;

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  struct GrpcCall
  {
    using RequestMethod = void (AsyncServiceType::*)(
      ::grpc::ServerContext*,
      Request*,
      ::grpc::ServerAsyncResponseWriter<Response>*,
      ::grpc::CompletionQueue*,
      ::grpc::ServerCompletionQueue*,
      void*);
    using Handler = void (ServiceImplType::*)(
      const Request&,
      Response&,
      ::grpc::Status&) const;

    RequestMethod request_method;
    Handler handler;
    std::string batch_full_method;
  };

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_grpc_call(
    typename GrpcCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method = {});

  class GrpcServiceBase
  {
    template<typename ServiceImplType, typename AsyncServiceType>
    friend class GrpcBatchStreamCall;
    template<
      typename ServiceImplType,
      typename AsyncServiceType,
      typename Request,
      typename Response>
    friend class GrpcUnaryCall;

  public:
    using CompletionQueues = std::vector<::grpc::ServerCompletionQueue*>;

    struct InprogressStatsSnapshot
    {
      std::uint64_t call_inflight = 0;
      std::optional<Generics::Time> min_time_of_request_in_progress;
    };

    class InprogressStats final
    {
    public:
      std::uint64_t add(
        const std::uint64_t call_inflight,
        const Generics::Time& read_time);

      void remove(const std::uint64_t receiver_id) noexcept;

      InprogressStatsSnapshot snapshot() const;

    private:
      struct Request
      {
        Generics::Time read_time;
        std::uint64_t call_inflight = 0;
      };

      void recalculate_min_time_() noexcept;

      std::atomic<std::uint64_t> next_receiver_id_{1};
      mutable std::mutex lock_;
      std::unordered_map<std::uint64_t, Request> requests_;
      std::uint64_t call_inflight_ = 0;
      std::optional<Generics::Time> min_time_of_request_in_progress_;
    };

    virtual ~GrpcServiceBase() noexcept;

    void register_services(::grpc::ServerBuilder& builder);

    void start(const CompletionQueues& completion_queues);

    void stop_accepting_requests() noexcept;

    void stop_finishing_requests() noexcept;

    InprogressStatsSnapshot inprogress_stats() const;

  protected:
    virtual std::size_t registrations_per_queue() const noexcept;

    virtual void register_in_queue(
      ::grpc::ServerCompletionQueue* completion_queue) = 0;

    void add_grpc_service(::grpc::Service* service);

    template<typename ServiceImplType, typename Calls>
    void register_batch_methods(
      ServiceImplType* service_impl,
      const Calls& calls);

    template<typename Request, typename Response, typename Handler>
    void register_batch_method(std::string full_method, Handler&& handler);

    template<
      typename ServiceImplType,
      typename AsyncServiceType,
      typename Request,
      typename Response>
    void register_batch_method(
      ServiceImplType* service_impl,
      const GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>& call);

    void handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const;

    AdServer::Commons::ActivityGate::Guard enter_grpc_operation() noexcept;

  private:
    using BatchDispatchFn = std::function<void(
      const adserver::grpc::BatchRequestItem&,
      adserver::grpc::BatchResponseItem&)>;

    std::unordered_map<std::string, BatchDispatchFn> batch_methods_;
    std::vector<::grpc::Service*> grpc_services_;
    AdServer::Commons::ActivityGate grpc_operation_gate_;
    std::shared_ptr<InprogressStats> inprogress_stats_ =
      std::make_shared<InprogressStats>();
  };

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void register_grpc_unary_call(
    ServiceImplType* service_impl,
    AsyncServiceType* async_service,
    const GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>& call,
    ::grpc::ServerCompletionQueue* completion_queue);

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  class GrpcAsyncServiceBase:
    public GrpcServiceBase
  {
  protected:
    GrpcAsyncServiceBase();

    template<typename Request, typename Response>
    static GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>
    make_grpc_call(
      typename GrpcCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name = nullptr);

    template<typename Calls>
    void register_batch_methods(const Calls& calls);

  private:
    void register_in_queue(
      ::grpc::ServerCompletionQueue* completion_queue) override;

  private:
    AsyncServiceType async_service_;
    adserver::grpc::BatchTransport::AsyncService batch_transport_service_;
  };

  struct GrpcAsyncCall
  {
    virtual ~GrpcAsyncCall() noexcept = default;
    virtual void proceed(bool ok) = 0;
  };

  template<typename Request, typename Response>
  class GrpcUnaryCallBase:
    public GrpcAsyncCall
  {
  public:
    GrpcUnaryCallBase(::grpc::ServerCompletionQueue* completion_queue);

    void proceed(bool ok) override;

  protected:
    virtual bool request_method_() = 0;

    virtual void spawn_next_() = 0;

    virtual bool process_() = 0;

  protected:
    ::grpc::ServerCompletionQueue* const completion_queue_;
    ::grpc::ServerContext context_;
    Request request_;
    Response response_;
    ::grpc::ServerAsyncResponseWriter<Response> responder_;

  private:
    enum class State
    {
      Create,
      Process,
      Finish
    };

    State state_;
  };

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  class GrpcUnaryCall final:
    public GrpcUnaryCallBase<Request, Response>
  {
  public:
    using Base = GrpcUnaryCallBase<Request, Response>;
    using RequestMethod = void (AsyncServiceType::*)(
      ::grpc::ServerContext*,
      Request*,
      ::grpc::ServerAsyncResponseWriter<Response>*,
      ::grpc::CompletionQueue*,
      ::grpc::ServerCompletionQueue*,
      void*);
    using Handler = void (ServiceImplType::*)(
      const Request&,
      Response&,
      ::grpc::Status&) const;

    GrpcUnaryCall(
      ServiceImplType* service_impl,
      AsyncServiceType* async_service,
      RequestMethod request_method,
      Handler handler,
      ::grpc::ServerCompletionQueue* completion_queue);

  private:
    bool request_method_() override;
    void spawn_next_() override;
    bool process_() override;

  private:
    ServiceImplType* const service_impl_;
    AsyncServiceType* const async_service_;
    const RequestMethod request_rpc_;
    const Handler handler_rpc_;
  };

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  class GrpcBatchStreamCall final:
    public GrpcAsyncCall
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

    void proceed(bool ok) override;

  private:
    enum class State
    {
      Create,
      Start,
      Read,
      Write,
      Finish
    };

  private:
    bool start_request_();
    bool read_or_delete_();
    bool write_or_delete_();
    void finish_or_delete_();
    void start_inprogress_stats_();
    void finish_inprogress_stats_() noexcept;

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    using DebugWatchdogState = GrpcBatchStreamDebugWatchdogState;

    void start_debug_response_watchdog_();
    void finish_debug_response_watchdog_() noexcept;
#endif

    ServiceImplType* const service_impl_;
    AsyncServiceType* const async_service_;
    const RequestMethod request_stream_;
    ::grpc::ServerCompletionQueue* const completion_queue_;
    ::grpc::ServerContext context_;
    ::grpc::ServerAsyncReaderWriter<Response, Request> responder_;
    Request request_;
    Response response_;
    State state_;
    std::optional<std::uint64_t> inprogress_stats_receiver_id_;
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    std::shared_ptr<DebugWatchdogState> debug_response_watchdog_state_;
#endif
  };
}

#include "GrpcServiceBase.tpp"
