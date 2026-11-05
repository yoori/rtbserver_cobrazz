#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>

#include <Commons/Grpc/GrpcServiceBase.hpp>

namespace AdServer::Grpc
{
  struct GrpcBatchStreamCallTestAccess
  {
    using AsyncService = adserver::grpc::BatchTransport::AsyncService;
    using Call = GrpcBatchStreamCall<GrpcServiceBase, AsyncService>;

    static std::shared_ptr<Call> create(GrpcServiceBase* service, AsyncService* async_service,
      grpc::ServerCompletionQueue* queue, void* tag);
    static void start_read(Call& call);
    static void retry_finish(Call& call);
  };

  std::shared_ptr<GrpcBatchStreamCallTestAccess::Call> GrpcBatchStreamCallTestAccess::create(
    GrpcServiceBase* service, AsyncService* async_service, grpc::ServerCompletionQueue* queue,
    void* tag)
  {
    auto call = std::make_shared<Call>(service, async_service,
      &AsyncService::Requeststream_batches, queue);
    async_service->Requeststream_batches(&call->context_, &call->responder_, queue, queue, tag);
    return call;
  }

  void GrpcBatchStreamCallTestAccess::start_read(Call& call)
  {
    call.try_start_read_();
  }

  void GrpcBatchStreamCallTestAccess::retry_finish(Call& call)
  {
    call.maybe_finish_or_delete_();
  }
}

namespace
{
  using Access = AdServer::Grpc::GrpcBatchStreamCallTestAccess;
  using Clock = std::chrono::system_clock;
  using namespace std::chrono_literals;

  class Service final: public AdServer::Grpc::GrpcServiceBase
  {
  private:
    void register_in_queue(grpc::ServerCompletionQueue*) override;
  };

  void Service::register_in_queue(grpc::ServerCompletionQueue*)
  {}

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << "BatchStreamFinishTest: " << message << std::endl;
      std::abort();
    }
  }

  void* next_event(grpc::ServerCompletionQueue& queue, bool expected_ok)
  {
    void* tag = nullptr;
    bool ok = false;
    require(queue.AsyncNext(&tag, &ok, Clock::now() + 5s) == grpc::CompletionQueue::GOT_EVENT,
      "completion timed out");
    require(ok == expected_ok, "unexpected completion status");
    return tag;
  }

  void test_finish_after_completion()
  {
    Service service;
    Access::AsyncService async_service;
    grpc::ServerBuilder builder;
    builder.RegisterService(&async_service);
    auto queue = builder.AddCompletionQueue();
    auto server = builder.BuildAndStart();
    require(server != nullptr, "server did not start");
    service.start({queue.get()});

    int accepted = 0;
    auto call = Access::create(&service, &async_service, queue.get(), &accepted);
    auto stub = adserver::grpc::BatchTransport::NewStub(
      server->InProcessChannel(grpc::ChannelArguments{}));
    grpc::ClientContext context;
    context.set_deadline(Clock::now() + 5s);
    auto stream = stub->stream_batches(&context);
    require(next_event(*queue, true) == &accepted, "unexpected accept tag");
    Access::start_read(*call);
    require(stream->WritesDone(), "client could not close its request stream");

    auto* read_tag = static_cast<AdServer::Grpc::GrpcAsyncCall*>(next_event(*queue, false));
    read_tag->proceed(false);
    Access::retry_finish(*call);
    auto* finish_tag = static_cast<AdServer::Grpc::GrpcAsyncCall*>(next_event(*queue, true));
    finish_tag->proceed(true);

    adserver::grpc::BatchResponse response;
    require(!stream->Read(&response), "empty stream returned a response");
    require(stream->Finish().ok(), "client did not receive successful completion");

    // A callback already running when Finish completed must not finish the RPC again.
    Access::retry_finish(*call);
    Access::retry_finish(*call);
    void* tag = nullptr;
    bool ok = false;
    require(queue->AsyncNext(&tag, &ok, Clock::now()) == grpc::CompletionQueue::TIMEOUT,
      "late callback submitted another gRPC operation");

    call.reset();
    service.stop_accepting_requests();
    server->Shutdown(Clock::now() + 5s);
    queue->Shutdown();
    require(!queue->Next(&tag, &ok), "unexpected completion during shutdown");
  }
}

int main()
{
  test_finish_after_completion();
  std::cout << "BatchStreamFinishTest passed" << std::endl;
  return 0;
}
