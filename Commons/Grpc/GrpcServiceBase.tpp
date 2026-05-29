#pragma once

namespace AdServer::Grpc
{
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
    std::string batch_full_method)
  {
    return {request_method, handler, std::move(batch_full_method)};
  }

  template<typename ServiceImplType, typename Calls>
  void
  GrpcServiceBase::register_batch_methods(
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
  void
  GrpcServiceBase::register_batch_method(
    std::string full_method,
    Handler&& handler)
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
  void
  GrpcServiceBase::register_batch_method(
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
  GrpcAsyncServiceBase<
    ServiceImplType,
    ServiceType,
    AsyncServiceType>::GrpcAsyncServiceBase()
  {
    add_grpc_service(&async_service_);
    add_grpc_service(&batch_transport_service_);
    register_batch_methods(ServiceImplType::grpc_calls());
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Request, typename Response>
  GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
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
      const char* batch_method_name)
  {
    return {
      request_method,
      handler,
      batch_method_name ?
        std::string("/") + ServiceType::service_full_name() + "/" + batch_method_name :
        std::string()
    };
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Calls>
  void
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    register_batch_methods(const Calls& calls)
  {
    GrpcServiceBase::register_batch_methods(
      static_cast<ServiceImplType*>(this),
      calls);
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  void
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    register_in_queue(::grpc::ServerCompletionQueue* completion_queue)
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

  template<typename Request, typename Response>
  GrpcUnaryCallBase<Request, Response>::GrpcUnaryCallBase(
    ::grpc::ServerCompletionQueue* completion_queue)
    : completion_queue_(completion_queue),
      responder_(&context_),
      state_(State::Create)
  {}

  template<typename Request, typename Response>
  void
  GrpcUnaryCallBase<Request, Response>::proceed(bool ok)
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

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
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

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  bool
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    request_method_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
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

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    spawn_next_()
  {
    auto* next_call = new GrpcUnaryCall(
      service_impl_,
      async_service_,
      request_rpc_,
      handler_rpc_,
      this->completion_queue_);
    next_call->proceed(true);
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  bool
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    process_()
  {
    ::grpc::Status status;
    (service_impl_->*handler_rpc_)(this->request_, this->response_, status);

    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      return false;
    }

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

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::
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

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  GrpcBatchStreamCall<
    ServiceImplType,
    AsyncServiceType>::~GrpcBatchStreamCall() noexcept
  {
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    finish_debug_response_watchdog_();
#endif
    finish_inprogress_stats_();
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::proceed(bool ok)
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
        if (!start_request_())
        {
          delete this;
          return;
        }
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
        if (!read_or_delete_())
        {
          return;
        }
        return;
      }

      case State::Read:
      {
        if (!ok)
        {
          finish_or_delete_();
          return;
        }

        response_.Clear();
        start_inprogress_stats_();
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
        start_debug_response_watchdog_();
#endif
        service_impl_->handle_batch_request(request_, response_);
        if (!write_or_delete_())
        {
          return;
        }
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

        if (!read_or_delete_())
        {
          return;
        }
        return;
      }

      case State::Finish:
      {
        delete this;
        return;
      }
    }
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_request_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      return false;
    }

    (async_service_->*request_stream_)(
      &context_,
      &responder_,
      completion_queue_,
      completion_queue_,
      this);
    return true;
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::read_or_delete_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      delete this;
      return false;
    }

    state_ = State::Read;
    responder_.Read(&request_, this);
    return true;
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::write_or_delete_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      finish_inprogress_stats_();
      delete this;
      return false;
    }

    state_ = State::Write;
    responder_.Write(response_, this);
    return true;
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::finish_or_delete_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      delete this;
      return;
    }

    state_ = State::Finish;
    responder_.Finish(::grpc::Status::OK, this);
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::
    start_inprogress_stats_()
  {
    finish_inprogress_stats_();

    inprogress_stats_receiver_id_ = service_impl_->inprogress_stats_->add(
      request_.items_size(),
      Generics::Time::get_time_of_day());
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::
    finish_inprogress_stats_() noexcept
  {
    if (inprogress_stats_receiver_id_)
    {
      service_impl_->inprogress_stats_->remove(*inprogress_stats_receiver_id_);
      inprogress_stats_receiver_id_.reset();
    }
  }

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::
    start_debug_response_watchdog_()
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

  template<
    typename ServiceImplType,
    typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::
    finish_debug_response_watchdog_() noexcept
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
}
