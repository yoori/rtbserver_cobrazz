#pragma once

namespace AdServer::Grpc
{
  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  class GrpcUnaryCall final: public GrpcUnaryCallBase<Request, Response>
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

    ~GrpcUnaryCall() noexcept override;

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
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void register_grpc_unary_call(
    ServiceImplType* service_impl,
    AsyncServiceType* async_service,
    const GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>& call,
    ::grpc::ServerCompletionQueue* completion_queue);
}

#include "GrpcUnaryCall.tpp"
