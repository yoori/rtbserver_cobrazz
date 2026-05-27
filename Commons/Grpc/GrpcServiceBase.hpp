#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
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

    static GrpcBatchStreamDebugTimerService& instance()
    {
      static auto* service = new GrpcBatchStreamDebugTimerService();
      return *service;
    }

    void schedule(std::shared_ptr<WatchdogState> state)
    {
      auto timer = std::make_shared<boost::asio::steady_timer>(io_service_);
      timer->expires_after(std::chrono::seconds(20));
      timer->async_wait(
        [
          timer = std::move(timer),
          state = std::move(state)
        ](const boost::system::error_code& error)
        {
          if (!error && state &&
            !state->done.load(std::memory_order_acquire))
          {
            std::cout
              << "ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT: "
              << "response timeout after 20 sec"
              << ", peer=" << state->peer
              << ", batch_items=" << state->items_size;
            if (!state->first_method.empty())
            {
              std::cout << ", first_method=" << state->first_method;
            }
            std::cout << std::endl;
          }
        });
    }

  private:
    GrpcBatchStreamDebugTimerService()
      : work_(io_service_),
        thread_([this] { io_service_.run(); })
    {}

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
    std::string batch_full_method = {})
  {
    return {request_method, handler, std::move(batch_full_method)};
  }

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
        const Generics::Time& read_time)
      {
        const auto receiver_id = next_receiver_id_.fetch_add(
          1,
          std::memory_order_relaxed);

        std::lock_guard<std::mutex> lock(lock_);
        requests_.emplace(receiver_id, Request{read_time, call_inflight});
        call_inflight_ += call_inflight;
        if (!min_time_of_request_in_progress_ ||
          read_time < *min_time_of_request_in_progress_)
        {
          min_time_of_request_in_progress_ = read_time;
        }

        return receiver_id;
      }

      void remove(const std::uint64_t receiver_id) noexcept
      {
        std::lock_guard<std::mutex> lock(lock_);
        const auto it = requests_.find(receiver_id);
        if (it == requests_.end())
        {
          return;
        }

        call_inflight_ -= it->second.call_inflight;
        requests_.erase(it);
        recalculate_min_time_();
      }

      InprogressStatsSnapshot snapshot() const
      {
        std::lock_guard<std::mutex> lock(lock_);
        return InprogressStatsSnapshot{
          call_inflight_,
          min_time_of_request_in_progress_};
      }

    private:
      struct Request
      {
        Generics::Time read_time;
        std::uint64_t call_inflight = 0;
      };

      void recalculate_min_time_() noexcept
      {
        min_time_of_request_in_progress_.reset();
        for (const auto& [_, request] : requests_)
        {
          if (!min_time_of_request_in_progress_ ||
            request.read_time < *min_time_of_request_in_progress_)
          {
            min_time_of_request_in_progress_ = request.read_time;
          }
        }
      }

      std::atomic<std::uint64_t> next_receiver_id_{1};
      mutable std::mutex lock_;
      std::unordered_map<std::uint64_t, Request> requests_;
      std::uint64_t call_inflight_ = 0;
      std::optional<Generics::Time> min_time_of_request_in_progress_;
    };

    virtual ~GrpcServiceBase() noexcept = default;

    void register_services(::grpc::ServerBuilder& builder)
    {
      for (auto* service : grpc_services_)
      {
        builder.RegisterService(service);
      }
    }

    void start(const CompletionQueues& completion_queues)
    {
      finish_gate_.activate_object();
      accepting_requests_.store(true, std::memory_order_release);

      const auto registrations = registrations_per_queue();
      for (auto* completion_queue : completion_queues)
      {
        for (std::size_t i = 0; i < registrations; ++i)
        {
          register_in_queue(completion_queue);
        }
      }
    }

    void stop_accepting_requests() noexcept
    {
      accepting_requests_.store(false, std::memory_order_release);

      std::unique_lock<std::mutex> lock(registration_lock_);
      registration_cv_.wait(lock, [this]() noexcept {
        return active_registrations_.load(std::memory_order_acquire) == 0;
      });
    }

    void stop_finishing_requests() noexcept
    {
      finish_gate_.deactivate_object();
      finish_gate_.wait_object();
    }

    InprogressStatsSnapshot inprogress_stats() const
    {
      return inprogress_stats_->snapshot();
    }

  protected:
    virtual std::size_t registrations_per_queue() const noexcept
    {
      return 64;
    }

    virtual void register_in_queue(
      ::grpc::ServerCompletionQueue* completion_queue) = 0;

    void add_grpc_service(::grpc::Service* service)
    {
      grpc_services_.push_back(service);
    }

    template<typename ServiceImplType, typename Calls>
    void register_batch_methods(
      ServiceImplType* service_impl,
      const Calls& calls)
    {
      std::apply(
        [this, service_impl](const auto&... call)
        {
          (register_batch_method(service_impl, call), ...);
        },
        calls);
    }

    template<typename Request, typename Response, typename Handler>
    void register_batch_method(std::string full_method, Handler&& handler)
    {
      batch_methods_.emplace(
        std::move(full_method),
        [handler = std::forward<Handler>(handler)](
          const adserver::grpc::BatchRequestItem& batch_request,
          adserver::grpc::BatchResponseItem& batch_response)
        {
          Request request;
          if (!request.ParseFromString(batch_request.payload()))
          {
            batch_response.set_status_code(::grpc::StatusCode::INVALID_ARGUMENT);
            batch_response.set_status_message("Unable to parse payload");
            return;
          }

          Response response;
          ::grpc::Status status;
          handler(request, response, status);

          batch_response.set_status_code(status.error_code());
          batch_response.set_status_message(status.error_message());
          if (status.ok())
          {
            batch_response.set_payload(response.SerializeAsString());
          }
        });
    }

    template<
      typename ServiceImplType,
      typename AsyncServiceType,
      typename Request,
      typename Response>
    void register_batch_method(
      ServiceImplType* service_impl,
      const GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>& call)
    {
      if (call.batch_full_method.empty())
      {
        return;
      }

      register_batch_method<Request, Response>(
        call.batch_full_method,
        [
          service_impl,
          handler = call.handler
        ](
          const Request& request,
          Response& response,
          ::grpc::Status& status)
        {
          (service_impl->*handler)(request, response, status);
        });
    }

    void handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const
    {
      for (int i = 0; i < batch_request.items_size(); ++i)
      {
        const auto& request_item = batch_request.items(i);
        auto* response_item = batch_response.add_items();
        response_item->set_request_id(request_item.request_id());

        const auto it = batch_methods_.find(request_item.full_method());
        if (it == batch_methods_.end())
        {
          response_item->set_status_code(::grpc::StatusCode::UNIMPLEMENTED);
          response_item->set_status_message("Unknown method");
          continue;
        }

        try
        {
          it->second(request_item, *response_item);
        }
        catch (const std::exception& ex)
        {
          response_item->set_status_code(::grpc::StatusCode::INTERNAL);
          response_item->set_status_message(ex.what());
        }
        catch (...)
        {
          response_item->set_status_code(::grpc::StatusCode::INTERNAL);
          response_item->set_status_message("Unknown batch handler exception");
        }
      }
    }

    class RequestRegistrationGuard
    {
    public:
      explicit RequestRegistrationGuard(GrpcServiceBase* service) noexcept
        : service_(service),
          active_(service_ && service_->begin_request_registration())
      {}

      ~RequestRegistrationGuard() noexcept
      {
        if (active_)
        {
          service_->end_request_registration();
        }
      }

      bool active() const noexcept
      {
        return active_;
      }

    private:
      GrpcServiceBase* service_;
      bool active_;
    };

    bool begin_request_registration() noexcept
    {
      if (!accepting_requests_.load(std::memory_order_acquire))
      {
        return false;
      }

      active_registrations_.fetch_add(1, std::memory_order_acq_rel);
      if (accepting_requests_.load(std::memory_order_acquire))
      {
        return true;
      }

      end_request_registration();
      return false;
    }

    void end_request_registration() noexcept
    {
      if (active_registrations_.fetch_sub(1, std::memory_order_acq_rel) == 1 &&
        !accepting_requests_.load(std::memory_order_acquire))
      {
        std::lock_guard<std::mutex> lock(registration_lock_);
        registration_cv_.notify_all();
      }
    }

    bool accepting_requests() const noexcept
    {
      return accepting_requests_.load(std::memory_order_acquire);
    }

    AdServer::Commons::ActivityGate::Guard enter_finish_operation() noexcept
    {
      return finish_gate_.enter();
    }

  private:
    using BatchDispatchFn = std::function<void(
      const adserver::grpc::BatchRequestItem&,
      adserver::grpc::BatchResponseItem&)>;

    std::unordered_map<std::string, BatchDispatchFn> batch_methods_;
    std::vector<::grpc::Service*> grpc_services_;
    std::atomic_bool accepting_requests_{false};
    std::atomic_size_t active_registrations_{0};
    std::mutex registration_lock_;
    std::condition_variable registration_cv_;
    AdServer::Commons::ActivityGate finish_gate_;
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
    ::grpc::ServerCompletionQueue* completion_queue)
  {
    auto* grpc_call = new GrpcUnaryCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>(
      service_impl,
      async_service,
      call.request_method,
      call.handler,
      completion_queue);
    grpc_call->proceed(true);
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  class GrpcAsyncServiceBase:
    public GrpcServiceBase
  {
  protected:
    GrpcAsyncServiceBase()
    {
      add_grpc_service(&async_service_);
      add_grpc_service(&batch_transport_service_);
      register_batch_methods(ServiceImplType::grpc_calls());
    }

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
      const char* batch_method_name = nullptr)
    {
      return {
        request_method,
        handler,
        batch_method_name ?
          std::string("/") + ServiceType::service_full_name() + "/" + batch_method_name :
          std::string()
      };
    }

    template<typename Calls>
    void register_batch_methods(const Calls& calls)
    {
      GrpcServiceBase::register_batch_methods(
        static_cast<ServiceImplType*>(this),
        calls);
    }

  private:
    void register_in_queue(
      ::grpc::ServerCompletionQueue* completion_queue) override
    {
      auto* service_impl = static_cast<ServiceImplType*>(this);
      std::apply(
        [this, service_impl, completion_queue](const auto&... call)
        {
          (register_grpc_unary_call(
            service_impl,
            &async_service_,
            call,
            completion_queue), ...);
        },
        ServiceImplType::grpc_calls());

      auto* batch_stream_call = new GrpcBatchStreamCall<
        ServiceImplType,
        adserver::grpc::BatchTransport::AsyncService>(
        service_impl,
        &batch_transport_service_,
        &adserver::grpc::BatchTransport::AsyncService::Requeststream_batches,
        completion_queue);
      batch_stream_call->proceed(true);
    }

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
    GrpcUnaryCallBase(::grpc::ServerCompletionQueue* completion_queue)
      : completion_queue_(completion_queue),
        responder_(&context_),
        state_(State::Create)
    {}

    void proceed(bool ok) override
    {
      if (state_ == State::Create)
      {
        if (!ok)
        {
          delete this;
          return;
        }

        state_ = State::Process;
        if (!request_method_())
        {
          delete this;
        }
        return;
      }

      if (state_ == State::Process)
      {
        if (!ok)
        {
          delete this;
          return;
        }

        state_ = State::Finish;
        spawn_next_();
        if (!process_())
        {
          delete this;
        }
        return;
      }

      delete this;
    }

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
      ::grpc::ServerCompletionQueue* completion_queue)
      : Base(completion_queue),
        service_impl_(service_impl),
        async_service_(async_service),
        request_rpc_(request_method),
        handler_rpc_(handler)
    {}

  private:
    bool request_method_() override
    {
      GrpcServiceBase::RequestRegistrationGuard registration_guard(service_impl_);
      if (!registration_guard.active())
      {
        return false;
      }

      (async_service_->*request_rpc_)(
        &this->context_,
        &this->request_,
        &this->responder_,
        this->completion_queue_,
        this->completion_queue_,
        this);
      return true;
    }

    void spawn_next_() override
    {
      auto* next_call = new GrpcUnaryCall(
        service_impl_,
        async_service_,
        request_rpc_,
        handler_rpc_,
        this->completion_queue_);
      next_call->proceed(true);
    }

    bool process_() override
    {
      ::grpc::Status status;
      (service_impl_->*handler_rpc_)(this->request_, this->response_, status);

      auto finish_guard = service_impl_->enter_finish_operation();
      if (!finish_guard)
      {
        return false;
      }
      finish_guard_.emplace(std::move(finish_guard));

      if (status.ok())
      {
        this->responder_.Finish(this->response_, status, this);
      }
      else
      {
        this->responder_.FinishWithError(status, this);
      }
      return true;
    }

  private:
    ServiceImplType* const service_impl_;
    AsyncServiceType* const async_service_;
    const RequestMethod request_rpc_;
    const Handler handler_rpc_;
    std::optional<AdServer::Commons::ActivityGate::Guard> finish_guard_;
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
      ::grpc::ServerCompletionQueue* completion_queue)
      : service_impl_(service_impl),
        async_service_(async_service),
        request_stream_(request_method),
        completion_queue_(completion_queue),
        responder_(&context_),
        state_(State::Create)
    {}

    ~GrpcBatchStreamCall() noexcept override
    {
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
      finish_debug_response_watchdog_();
#endif
      finish_inprogress_stats_();
    }

    void proceed(bool ok) override
    {
      switch (state_)
      {
        case State::Create:
        {
          if (!ok)
          {
            delete this;
            return;
          }

          state_ = State::Start;
          GrpcServiceBase::RequestRegistrationGuard registration_guard(service_impl_);
          if (!registration_guard.active())
          {
            delete this;
            return;
          }

          (async_service_->*request_stream_)(
            &context_,
            &responder_,
            completion_queue_,
            completion_queue_,
            this);
          return;
        }

        case State::Start:
        {
          if (!ok)
          {
            delete this;
            return;
          }

          auto* next_call = new GrpcBatchStreamCall(
            service_impl_,
            async_service_,
            request_stream_,
            completion_queue_);
          next_call->proceed(true);
          state_ = State::Read;
          responder_.Read(&request_, this);
          return;
        }

        case State::Read:
        {
          if (!ok)
          {
            if (service_impl_->accepting_requests())
            {
              auto finish_guard = service_impl_->enter_finish_operation();
              if (!finish_guard)
              {
                delete this;
                return;
              }
              finish_guard_.emplace(std::move(finish_guard));

              state_ = State::Finish;
              responder_.Finish(::grpc::Status::OK, this);
            }
            else
            {
              delete this;
            }
            return;
          }

          response_.Clear();
          start_inprogress_stats_();
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
          start_debug_response_watchdog_();
#endif
          service_impl_->handle_batch_request(request_, response_);
          state_ = State::Write;
          responder_.Write(response_, this);
          return;
        }

        case State::Write:
        {
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
          finish_debug_response_watchdog_();
#endif
          finish_inprogress_stats_();
          if (!ok)
          {
            delete this;
            return;
          }

          state_ = State::Read;
          responder_.Read(&request_, this);
          return;
        }

        case State::Finish:
        {
          delete this;
          return;
        }
      }
    }

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
    void start_inprogress_stats_()
    {
      finish_inprogress_stats_();

      inprogress_stats_receiver_id_ = service_impl_->inprogress_stats_->add(
        request_.items_size(),
        Generics::Time::get_time_of_day());
    }

    void finish_inprogress_stats_() noexcept
    {
      if (inprogress_stats_receiver_id_)
      {
        service_impl_->inprogress_stats_->remove(*inprogress_stats_receiver_id_);
        inprogress_stats_receiver_id_.reset();
      }
    }

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    using DebugWatchdogState = GrpcBatchStreamDebugWatchdogState;

    void start_debug_response_watchdog_()
    {
      finish_debug_response_watchdog_();

      auto state = std::make_shared<DebugWatchdogState>();
      state->peer = context_.peer();
      state->items_size = request_.items_size();
      if (state->items_size > 0)
      {
        state->first_method = request_.items(0).full_method();
      }

      debug_response_watchdog_state_ = state;

      GrpcBatchStreamDebugTimerService::instance().schedule(std::move(state));
    }

    void finish_debug_response_watchdog_() noexcept
    {
      if (debug_response_watchdog_state_)
      {
        debug_response_watchdog_state_->done.store(
          true,
          std::memory_order_release);
        debug_response_watchdog_state_.reset();
      }
    }
#endif

    ServiceImplType* const service_impl_;
    AsyncServiceType* const async_service_;
    const RequestMethod request_stream_;
    ::grpc::ServerCompletionQueue* const completion_queue_;
    ::grpc::ServerContext context_;
    ::grpc::ServerAsyncReaderWriter<Response, Request> responder_;
    Request request_;
    Response response_;
    std::optional<AdServer::Commons::ActivityGate::Guard> finish_guard_;
    State state_;
    std::optional<std::uint64_t> inprogress_stats_receiver_id_;
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    std::shared_ptr<DebugWatchdogState> debug_response_watchdog_state_;
#endif
  };
}
