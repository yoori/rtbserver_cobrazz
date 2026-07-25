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
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::GrpcUnaryCall(
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
    service_impl_->add_unary_call_created_();
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::~GrpcUnaryCall() noexcept
  {
    service_impl_->add_unary_call_deleted_();
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  bool
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::request_method_()
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
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::spawn_next_()
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
  GrpcUnaryCall<ServiceImplType, AsyncServiceType, Request, Response>::process_()
  {
    ::grpc::Status status;
    (service_impl_->*handler_rpc_)(*this->request_, this->response_, status);

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
}
