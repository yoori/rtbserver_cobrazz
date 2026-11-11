#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>

#include <grpcpp/server_builder.h>
#include <Logger/StreamLogger.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>
#include <ChannelSvcs/ChannelController/ChannelControllerGrpc.grpc.pb.h>

namespace AdServer::ChannelSvcs
{
  struct ChannelDistributedGrpcClientTestAccess
  {
    using Client = ChannelDistributedGrpcClient;
    static std::optional<Client::Pool::Ref> get_ref(Client& client);
    static bool has_pool(Client& client);
    static bool create_client(Client& client);
  };

  std::optional<ChannelDistributedGrpcClient::Pool::Ref>
  ChannelDistributedGrpcClientTestAccess::get_ref(Client& client)
  {
    return client.get_ref_();
  }

  bool ChannelDistributedGrpcClientTestAccess::has_pool(Client& client)
  {
    std::shared_lock<std::shared_mutex> lock(client.pool_lock_);
    return static_cast<bool>(client.pool_);
  }

  bool ChannelDistributedGrpcClientTestAccess::create_client(Client& client)
  {
    return static_cast<bool>(client.get_or_create_client_holder_("127.0.0.1:3"));
  }
}

namespace
{
  using namespace std::chrono_literals;
  namespace Proto = adserver::channel_svcs::channel_controller;
  using Client = AdServer::ChannelSvcs::ChannelDistributedGrpcClient;
  using Access = AdServer::ChannelSvcs::ChannelDistributedGrpcClientTestAccess;

  void require(bool value, const char* message)
  {
    if (!value)
    {
      std::cerr << message << std::endl;
      std::abort();
    }
  }

  void await(std::future<void>& future)
  {
    require(future.wait_for(3s) == std::future_status::ready, "completion timed out");
    future.get();
  }

  class Controller final: public Proto::ChannelControllerGrpc::Service
  {
  public:
    grpc::Status get_session_description(grpc::ServerContext*,
      const Proto::GetSessionDescriptionRequest*, Proto::GetSessionDescriptionResponse*) override;

    std::promise<void> resolving;
    std::promise<void> release;

  private:
    std::atomic<unsigned int> calls_{0};
    std::shared_future<void> resume_ = release.get_future().share();
  };

  grpc::Status Controller::get_session_description(grpc::ServerContext*,
    const Proto::GetSessionDescriptionRequest*, Proto::GetSessionDescriptionResponse* response)
  {
    const auto call = calls_.fetch_add(1);
    if (call == 1)
    {
      resolving.set_value();
      resume_.wait();
    }

    auto* server = response->add_channel_server_groups()->add_channel_servers();
    server->set_channel_server_endpoint(call == 0 ? "127.0.0.1:1" : "127.0.0.1:2");
    return grpc::Status::OK;
  }

  void test_stop_during_resolve()
  {
    Controller controller;
    auto resolving = controller.resolving.get_future();
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&controller);
    auto server = builder.BuildAndStart();
    require(server && port, "failed to start controller");

    Logging::Logger_var logger = new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr));
    auto executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    executor->activate_object();
    Client client({{"127.0.0.1:" + std::to_string(port)}}, {}, executor, {}, logger);
    client.activate_object();
    auto retained = Access::get_ref(client);
    require(retained.has_value(), "initial pool has no ref");
    await(resolving);

    auto deactivating = std::async(std::launch::async, [&client] { client.deactivate_object(); });
    await(deactivating);
    require(!Access::has_pool(client), "deactivate left the pool attached until wait");
    require(!Access::get_ref(client), "deactivated client issued a ref");
    require(!Access::create_client(client), "deactivated client created an endpoint client");

    executor->deactivate_object();
    executor->wait_object();
    require(executor->execute([] {}), "executor stopped before client wait");
    auto waiting = std::async(std::launch::async, [&client] { client.wait_object(); });
    require(waiting.wait_for(20ms) == std::future_status::timeout,
      "wait ignored the running resolver");
    controller.release.set_value();
    await(waiting);
    require(!Access::has_pool(client), "late resolver response installed a new pool");
    require(!Access::get_ref(client), "late resolver response restored ref issuance");
    require(client.endpoint_stats().empty(), "late resolver response retained an endpoint");
    require(!executor->execute([] {}), "client wait did not release executor activity");

    retained.reset();
    executor->deletion_queue()->wait();
    server->Shutdown();
    server->Wait();
  }
}

int main()
{
  test_stop_during_resolve();
  std::cout << "ChannelDistributedShutdownTest passed" << std::endl;
}
