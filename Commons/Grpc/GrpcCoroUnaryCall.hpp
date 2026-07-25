#pragma once

namespace AdServer::Grpc
{
  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  class GrpcCoroUnaryCall final: public GrpcUnaryCallBase<Request, Response>
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
    using Handler = GrpcCoroutine (ServiceImplType::*)(
      Request&&,
      Response&,
      ::grpc::Status&) const;

    GrpcCoroUnaryCall(
      ServiceImplType* service_impl,
      AsyncServiceType* async_service,
      RequestMethod request_method,
      Handler handler,
      ::grpc::ServerCompletionQueue* completion_queue);

    ~GrpcCoroUnaryCall() noexcept override;

  private:
    bool request_method_() override;
    void spawn_next_() override;
    bool process_() override;
    void finish_();

  private:
    ServiceImplType* const service_impl_;
    AsyncServiceType* const async_service_;
    const RequestMethod request_rpc_;
    const Handler handler_rpc_;
    ::grpc::Status status_;
    std::optional<GrpcCoroutine> operation_;
    std::optional<AdServer::Commons::ActivityGate::Guard> grpc_operation_guard_;
  };

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void register_grpc_unary_call(
    ServiceImplType* service_impl,
    AsyncServiceType* async_service,
    const GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>& call,
    ::grpc::ServerCompletionQueue* completion_queue);
}

#include "GrpcCoroUnaryCall.tpp"
