#pragma once

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <deque>
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

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
#include <thread>

#include <boost/asio.hpp>
#endif

#include <grpcpp/grpcpp.h>
#include <google/protobuf/arena.h>

#include <Commons/ActivityGate.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Generics/MonoAllocator.hpp>
#include <Generics/Time.hpp>

#define MAKE_GRPC_CALL(RequestType, ResponseType, MethodName) \
  make_grpc_call<RequestType, ResponseType>( \
    &AsyncService::Request##MethodName, \
    &ServiceImpl::MethodName, \
    #MethodName)

#define MAKE_GRPC_CORO_CALL(RequestType, ResponseType, MethodName, HandlerName) \
  make_grpc_coro_call<RequestType, ResponseType>( \
    &AsyncService::Request##MethodName, \
    &ServiceImpl::HandlerName, \
    #MethodName)

#define MAKE_DISTRIBUTED_GRPC_CORO_CALL( \
  RequestType, ResponseType, MethodName, HandlerName, UseArenaForResponse, ...) \
  make_distributed_grpc_coro_call<RequestType, ResponseType>( \
    &AsyncService::Request##MethodName, \
    &ServiceImpl::HandlerName, \
    #MethodName, \
    UseArenaForResponse __VA_OPT__(,) __VA_ARGS__)

namespace AdServer::Commons
{
  class ExecutorPool;
}

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
  class GrpcCoroUnaryCall;

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

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  struct GrpcCoroCall
  {
    using RequestMethod = void (AsyncServiceType::*)(
      ::grpc::ServerContext*,
      Request*,
      ::grpc::ServerAsyncResponseWriter<Response>*,
      ::grpc::CompletionQueue*,
      ::grpc::ServerCompletionQueue*,
      void*);
    using Handler =
      AdServer::Commons::StartableAwaitable<void> (ServiceImplType::*)(
      Request&&,
      Response&,
      ::grpc::Status&) const;
    using BatchHandler = void (ServiceImplType::*)(
      const Request&,
      Response&,
      ::grpc::Status&) const;
    using BatchHashFn = std::function<std::size_t(
      const Request&)>;

    RequestMethod request_method;
    Handler handler;
    BatchHandler batch_handler = nullptr;
    std::string batch_full_method;
    BatchHashFn batch_hash;
    bool distributed_batch = false;
    bool use_arena_for_response = true;
  };

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_grpc_coro_call(
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method = {});

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response,
    typename HashFn>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_distributed_grpc_coro_call(
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method,
    bool use_arena_for_response,
    HashFn hash_fn);

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_distributed_grpc_coro_call(
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method,
    bool use_arena_for_response);

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
    template<
      typename ServiceImplType,
      typename AsyncServiceType,
      typename Request,
      typename Response>
    friend class GrpcCoroUnaryCall;

  public:
    using CompletionQueues = std::vector<::grpc::ServerCompletionQueue*>;
    using BatchCompletion =
      AdServer::Commons::StartableAwaitable<void>::Completion;
    using BatchProcessingHandle = std::shared_ptr<void>;

    struct InprogressStatsSnapshot
    {
      std::uint64_t call_inflight = 0;
      std::optional<Generics::Time> min_time_of_request_in_progress;
    };

    struct LifecycleStatsSnapshot
    {
      std::uint64_t unary_call_created_total = 0;
      std::uint64_t unary_call_deleted_total = 0;
      std::uint64_t unary_call_live = 0;
      std::uint64_t coro_unary_call_created_total = 0;
      std::uint64_t coro_unary_call_deleted_total = 0;
      std::uint64_t coro_unary_call_live = 0;
      std::uint64_t batch_stream_call_created_total = 0;
      std::uint64_t batch_stream_call_deleted_total = 0;
      std::uint64_t batch_stream_call_live = 0;
      std::uint64_t debug_watchdog_scheduled_total = 0;
      std::uint64_t debug_watchdog_finished_total = 0;
      std::uint64_t debug_watchdog_live = 0;
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

    class BatchStreamReadLimiter final
    {
    public:
      static constexpr std::size_t DEFAULT_MAX_REQUESTS_IN_PROGRESS =
        16 * 1024;

      struct Options
      {
        Options(
          bool read_ahead_enabled = true,
          std::size_t max_requests_in_progress =
            DEFAULT_MAX_REQUESTS_IN_PROGRESS) noexcept;

        bool read_ahead_enabled = true;
        std::size_t max_requests_in_progress =
          DEFAULT_MAX_REQUESTS_IN_PROGRESS;
      };

      explicit BatchStreamReadLimiter(
        Options options = {}) noexcept;

      class Waiter
      {
      public:
        virtual ~Waiter() = default;
        virtual void start_read_from_limiter() noexcept = 0;
      };
      using WaiterPtr = std::shared_ptr<Waiter>;

      bool read_ahead_enabled() const noexcept;

      bool reserve_read_or_enqueue(WaiterPtr waiter);
      void complete_read_reservation(std::size_t requests) noexcept;
      void cancel_read_reservation() noexcept;
      void complete_requests(std::size_t requests) noexcept;
      void clear_waiters() noexcept;

    private:
      bool has_capacity_i_() const noexcept;
      void take_waiters_i_(Generics::MonoVector<WaiterPtr>& result);
      void grant_waiters_(Generics::MonoVector<WaiterPtr>& waiters) noexcept;

      mutable std::mutex lock_;
      bool read_ahead_enabled_ = true;
      std::size_t max_requests_in_progress_ =
        DEFAULT_MAX_REQUESTS_IN_PROGRESS;
      std::size_t requests_in_progress_ = 0;
      std::size_t read_reservations_ = 0;
      std::deque<WaiterPtr> waiters_;
    };

    virtual ~GrpcServiceBase() noexcept;

    void register_services(::grpc::ServerBuilder& builder);

    void start(const CompletionQueues& completion_queues);

    void stop_accepting_requests() noexcept;

    void stop_finishing_requests() noexcept;

    InprogressStatsSnapshot inprogress_stats() const;

    LifecycleStatsSnapshot lifecycle_stats() const noexcept;

  protected:
    using BatchStreamReadOptions = BatchStreamReadLimiter::Options;

    explicit GrpcServiceBase(
      BatchStreamReadOptions batch_stream_read_options = {}) noexcept;

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

    template<typename Request, typename Response, typename Handler>
    void register_batch_coro_method(
      std::string full_method,
      Handler&& handler,
      typename GrpcCoroCall<
        GrpcServiceBase,
        GrpcServiceBase,
        Request,
        Response>::BatchHashFn hash,
      bool distributed,
      bool use_arena_for_response);

    template<
      typename ServiceImplType,
      typename AsyncServiceType,
      typename Request,
      typename Response>
    void register_batch_method(
      ServiceImplType* service_impl,
      const GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>& call);

    template<
      typename ServiceImplType,
      typename AsyncServiceType,
      typename Request,
      typename Response>
    void register_batch_method(
      ServiceImplType* service_impl,
      const GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>& call);

    void handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const;

    AdServer::Commons::ActivityGate::Guard enter_grpc_operation() noexcept;

    virtual AdServer::Commons::StartableAwaitable<void> co_handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const;

    virtual void start_handle_batch_request(
      BatchProcessingHandle& handle,
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response,
      BatchCompletion completion) const;

    virtual std::size_t distributed_batch_max_sequential_ops() const noexcept;

    virtual std::shared_ptr<AdServer::Commons::ExecutorPool>
    batch_processing_executor_pool() const noexcept;

  private:
    using BatchDispatchFn = std::function<void(
      const adserver::grpc::BatchRequestItem&,
      adserver::grpc::BatchResponseItem&,
      google::protobuf::Arena&)>;

    struct BatchCoroMethod;
    class BatchRequestAwaiter;

    struct PreparedBatchCoroItem
    {
      const BatchCoroMethod* method = nullptr;
      void* request = nullptr;
      std::size_t hash = 0;
      bool hash_present = false;
    };

    struct BatchItemContext
    {
      const adserver::grpc::BatchRequestItem* request_item = nullptr;
      adserver::grpc::BatchResponseItem* response_item = nullptr;
      PreparedBatchCoroItem coro_item;
    };

    using BatchCoroPrepareFn = std::function<bool(
      const adserver::grpc::BatchRequestItem&,
      adserver::grpc::BatchResponseItem&,
      google::protobuf::Arena&,
      PreparedBatchCoroItem&)>;
    using BatchCoroDispatchFn = std::function<
      AdServer::Commons::StartableAwaitable<void>(
        void*,
        adserver::grpc::BatchResponseItem&,
        google::protobuf::Arena&)>;

    struct BatchCoroMethod
    {
      BatchCoroPrepareFn prepare;
      BatchCoroDispatchFn dispatch;
      bool distributed = false;
    };

    AdServer::Commons::StartableAwaitable<void>
    co_handle_batch_request_sequential_(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const;

    void start_handle_batch_request_i_(
      BatchProcessingHandle& handle,
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response,
      BatchCompletion completion) const;

    void start_handle_batch_request_sequential_(
      BatchProcessingHandle& handle,
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response,
      BatchCompletion completion) const;

    void start_handle_batch_request_distributed_(
      BatchProcessingHandle& handle,
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response,
      BatchCompletion completion,
      std::size_t max_sequential_ops,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool) const;

    AdServer::Commons::StartableAwaitable<void>
    co_handle_batch_lane_(
      Generics::MonoVector<BatchItemContext> batch_items,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      bool reschedule,
      google::protobuf::Arena& response_arena) const;

    AdServer::Commons::StartableAwaitable<void>
    co_handle_batch_item_(
      const adserver::grpc::BatchRequestItem& request_item,
      adserver::grpc::BatchResponseItem& response_item,
      google::protobuf::Arena& response_arena) const;

    bool prepare_batch_coro_item_(
      const BatchCoroMethod& method,
      const adserver::grpc::BatchRequestItem& request_item,
      adserver::grpc::BatchResponseItem& response_item,
      google::protobuf::Arena& request_arena,
      PreparedBatchCoroItem& coro_item) const;

    AdServer::Commons::StartableAwaitable<void>
    co_handle_prepared_batch_coro_item_(
      PreparedBatchCoroItem& coro_item,
      adserver::grpc::BatchResponseItem& response_item,
      google::protobuf::Arena& response_arena) const;

    BatchStreamReadLimiter& batch_stream_read_limiter() noexcept;

    void add_unary_call_created_() noexcept;
    void add_unary_call_deleted_() noexcept;
    void add_coro_unary_call_created_() noexcept;
    void add_coro_unary_call_deleted_() noexcept;
    void add_batch_stream_call_created_() noexcept;
    void add_batch_stream_call_deleted_() noexcept;
    void add_debug_watchdog_scheduled_() noexcept;
    void add_debug_watchdog_finished_() noexcept;

    std::unordered_map<std::string, BatchDispatchFn> batch_methods_;
    std::unordered_map<std::string, BatchCoroMethod> batch_coro_methods_;
    std::vector<::grpc::Service*> grpc_services_;
    AdServer::Commons::ActivityGate grpc_operation_gate_;
    BatchStreamReadLimiter batch_stream_read_limiter_;
    std::shared_ptr<InprogressStats> inprogress_stats_ =
      std::make_shared<InprogressStats>();
    std::atomic<std::uint64_t> unary_call_created_total_{0};
    std::atomic<std::uint64_t> unary_call_deleted_total_{0};
    std::atomic<std::uint64_t> unary_call_live_{0};
    std::atomic<std::uint64_t> coro_unary_call_created_total_{0};
    std::atomic<std::uint64_t> coro_unary_call_deleted_total_{0};
    std::atomic<std::uint64_t> coro_unary_call_live_{0};
    std::atomic<std::uint64_t> batch_stream_call_created_total_{0};
    std::atomic<std::uint64_t> batch_stream_call_deleted_total_{0};
    std::atomic<std::uint64_t> batch_stream_call_live_{0};
    std::atomic<std::uint64_t> debug_watchdog_scheduled_total_{0};
    std::atomic<std::uint64_t> debug_watchdog_finished_total_{0};
    std::atomic<std::uint64_t> debug_watchdog_live_{0};
  };

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  class GrpcAsyncServiceBase:
    public GrpcServiceBase
  {
  protected:
    explicit GrpcAsyncServiceBase(
      BatchStreamReadOptions batch_stream_read_options = {});

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

    template<typename Request, typename Response>
    static GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
    make_grpc_coro_call(
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name = nullptr);

    template<typename Request, typename Response, typename HashFn>
    static GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
    make_distributed_grpc_coro_call(
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name,
      bool use_arena_for_response,
      HashFn hash_fn);

    template<typename Request, typename Response>
    static GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
    make_distributed_grpc_coro_call(
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name = nullptr,
      bool use_arena_for_response = true);

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
}

#include "GrpcUnaryCallBase.hpp"
#include "GrpcUnaryCall.hpp"
#include "GrpcCoroUnaryCall.hpp"
#include "GrpcBatchStreamCall.hpp"
#include "GrpcServiceBase.tpp"
