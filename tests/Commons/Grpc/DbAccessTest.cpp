#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <Commons/Grpc/DbAccess.hpp>

namespace
{
  namespace pc = adserver::grpc::process_control;
  using namespace std::chrono_literals;

  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  struct Server
  {
    explicit Server(const std::string& mode);
    ~Server();
    grpc::Status set(bool value, std::chrono::milliseconds timeout = 2s);
    pc::GetDbStateResponse get();

    std::atomic<bool> enabled{true};
    std::atomic<unsigned int> switches{0};
    std::atomic<bool> switching{false};
    std::atomic<bool> overlap{false};
    AdServer::Grpc::DbAccessService service;
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<pc::ProcessControl::Stub> stub;
    std::string endpoint;
  };

  Server::Server(const std::string& mode)
    : service(
        [this, mode]() -> std::optional<bool>
        {
          return mode == "unsupported" ? std::nullopt : std::optional<bool>(enabled.load());
        },
        [this, mode](bool value)
        {
          if (mode == "failure")
          {
            throw std::runtime_error("Test DB failure");
          }

          if (switching.exchange(true))
          {
            overlap = true;
          }
          ++switches;
          std::this_thread::sleep_for(mode == "slow" ? 300ms : 5ms);
          enabled = value;
          switching = false;
        },
        [](auto& response) { response.set_ready(true); })
  {
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&service);
    server = builder.BuildAndStart();
    require(server != nullptr && port != 0, "cannot start test server");
    endpoint = "127.0.0.1:" + std::to_string(port);
    grpc::ChannelArguments arguments;
    arguments.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    stub = pc::ProcessControl::NewStub(
      grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), arguments));
  }

  Server::~Server()
  {
    service.stop();
    server->Shutdown();
    server->Wait();
  }

  grpc::Status Server::set(bool value, std::chrono::milliseconds timeout)
  {
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + timeout);
    pc::SetDbStateRequest request;
    request.set_enabled(value);
    pc::SetDbStateResponse response;
    return stub->set_db_state(&context, request, &response);
  }

  pc::GetDbStateResponse Server::get()
  {
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + 2s);
    pc::GetDbStateResponse response;
    require(stub->get_db_state(&context, {}, &response).ok(), "get_db_state failed");
    return response;
  }

  void test()
  {
    Server normal("normal");
    require(normal.get().supported() && normal.get().enabled(), "initial DB state");
    require(normal.set(false).ok() && !normal.get().enabled(), "disable DB");
    require(normal.set(false).ok() && normal.switches == 1, "disable is not idempotent");
    require(normal.set(true).ok() && normal.get().enabled(), "enable DB");
    std::vector<std::future<grpc::Status>> requests;
    for (int i = 0; i < 20; ++i)
    {
      requests.push_back(std::async(std::launch::async,
        [&normal, i] { return normal.set(i % 2); }));
    }
    for (auto& request : requests)
    {
      require(request.get().ok(), "concurrent switch failed");
    }
    require(!normal.overlap, "DB switches overlapped");

    Server unsupported("unsupported");
    require(unsupported.get().has_supported() && !unsupported.get().supported(), "unsupported DB");
    require(unsupported.set(false).error_code() == grpc::StatusCode::FAILED_PRECONDITION,
      "unsupported DB accepted a switch");

    Server failure("failure");
    require(failure.set(false).error_code() == grpc::StatusCode::FAILED_PRECONDITION,
      "DB exception was lost");

    Server slow("slow");
    auto pending = std::async(std::launch::async, [&slow] { return slow.set(false, 100ms); });
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!slow.switching && std::chrono::steady_clock::now() < deadline)
    {
      std::this_thread::sleep_for(1ms);
    }
    require(slow.switching, "slow request did not start");
    const auto cancelled = slow.set(true, 50ms);
    require(cancelled.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED, "queued timeout");
    slow.service.stop();
    require(!slow.switching && !slow.enabled && slow.switches == 1,
      "stop did not drain the switch, or executed a cancelled request");
    require(pending.get().error_code() == grpc::StatusCode::DEADLINE_EXCEEDED, "switch timeout");
    require(slow.set(true).error_code() == grpc::StatusCode::UNAVAILABLE, "switch after stop");
  }
}

int main(int argc, char** argv)
{
  try
  {
    if (argc == 3 && std::string(argv[1]) == "--serve")
    {
      Server server(argv[2]);
      std::cout << server.endpoint << std::endl;
      std::string line;
      std::getline(std::cin, line);
      return 0;
    }
    test();
    std::cout << "PASS: DBAccess state, idempotence, concurrency, errors, deadlines and shutdown\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
