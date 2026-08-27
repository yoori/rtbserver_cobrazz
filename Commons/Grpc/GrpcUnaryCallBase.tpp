#pragma once

namespace AdServer::Grpc
{
  template<typename Request, typename Response>
  GrpcUnaryCallBase<Request, Response>::GrpcUnaryCallBase(
    ::grpc::ServerCompletionQueue* completion_queue)
    : completion_queue_(completion_queue),
      request_(google::protobuf::Arena::CreateMessage<Request>(&request_arena_)),
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
}
