#pragma once

namespace AdServer::Grpc
{
  template<typename Request, typename Response>
  class GrpcUnaryCallBase: public GrpcAsyncCall
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
    google::protobuf::Arena request_arena_;
    Request* const request_;
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
}

#include "GrpcUnaryCallBase.tpp"
