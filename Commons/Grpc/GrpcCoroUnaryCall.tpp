#pragma once

namespace AdServer::Grpc
{
  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void register_grpc_unary_call(
    ServiceImplType* service_impl,
    AsyncServiceType* async_service,
    const GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>& call,
    ::grpc::ServerCompletionQueue* completion_queue)
  {
    auto* grpc_call = new GrpcCoroUnaryCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>(service_impl, async_service, call.request_method, call.handler, completion_queue);
    grpc_call->proceed(true);
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCoroUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    GrpcCoroUnaryCall(
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
  {
    service_impl_->add_coro_unary_call_created_();
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCoroUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    ~GrpcCoroUnaryCall() noexcept
  {
    service_impl_->add_coro_unary_call_deleted_();
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  bool
  GrpcCoroUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    request_method_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      return false;
    }

    (async_service_->*request_rpc_)(
      &this->context_,
      this->request_,
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
  GrpcCoroUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    spawn_next_()
  {
    auto* next_call = new GrpcCoroUnaryCall(
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
  GrpcCoroUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    process_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      return false;
    }
    grpc_operation_guard_.emplace(std::move(grpc_operation_guard));

    try
    {
      operation_.emplace(
        (service_impl_->*handler_rpc_)(std::move(*this->request_), this->response_, status_));
    }
    catch (const std::exception& ex)
    {
      status_ = ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
      finish_();
      return true;
    }
    catch (...)
    {
      status_ = ::grpc::Status(::grpc::StatusCode::INTERNAL, "Unknown coroutine handler exception");
      finish_();
      return true;
    }

    operation_->start([this](std::optional<std::exception_ptr> exception) {
      if (exception)
      {
        try
        {
          std::rethrow_exception(*exception);
        }
        catch (const std::exception& ex)
        {
          status_ = ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
        }
        catch (...)
        {
          status_ = ::grpc::Status(
            ::grpc::StatusCode::INTERNAL,
            "Unknown coroutine handler exception");
        }
      }

      finish_();
    });

    return true;
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void
  GrpcCoroUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::
    finish_()
  {
    if (status_.ok())
    {
      this->responder_.Finish(this->response_, status_, this);
    }
    else
    {
      this->responder_.FinishWithError(status_, this);
    }
  }
}
