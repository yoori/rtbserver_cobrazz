#include <csignal>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <Generics/AppUtils.hpp>

#include "UserBindServerGrpc.hpp"

namespace
{
  struct Endpoint
  {
    std::string host = "0.0.0.0";
    unsigned int port = 26528;
  };

  Endpoint
  parse_endpoint(const std::string& value)
  {
    Endpoint endpoint;
    if (value.empty())
    {
      return endpoint;
    }

    const auto pos = value.rfind(':');
    if (pos == std::string::npos)
    {
      endpoint.port = static_cast<unsigned int>(std::stoul(value));
      return endpoint;
    }

    endpoint.host = pos == 0 ? "0.0.0.0" : value.substr(0, pos);
    endpoint.port = static_cast<unsigned int>(std::stoul(value.substr(pos + 1)));
    return endpoint;
  }

  void
  print_usage(std::ostream& out)
  {
    out
	      << "Usage: MockUserBindServer [OPTIONS]\n"
	      << "  --grpc-endpoint <host:port|port> gRPC endpoint (default: 0.0.0.0:26528)\n"
	      << "  --response-sleep-ms <N> response sleep in ms (default: 0)\n"
	      << "  SIGUSR1 toggles response sleep between 0 and configured value\n";
  }
}

int
main(int argc, char** argv)
{
  try
  {
    using namespace Generics::AppUtils;

    StringOption opt_grpc_endpoint("0.0.0.0:26528");
    Option<unsigned int> opt_response_sleep_ms(0);

    Args args(-1);
    args.add(equal_name("grpc-endpoint") || short_name("g"), opt_grpc_endpoint);
    args.add(equal_name("response-sleep-ms"), opt_response_sleep_ms);
    args.parse(argc - 1, argv + 1);

    if (args.commands().begin() != args.commands().end())
    {
      print_usage(std::cerr);
      return 1;
    }

    const Endpoint endpoint = parse_endpoint(*opt_grpc_endpoint);

    sigset_t signals;
	    sigemptyset(&signals);
	    sigaddset(&signals, SIGINT);
	    sigaddset(&signals, SIGTERM);
	    sigaddset(&signals, SIGUSR1);
	    pthread_sigmask(SIG_BLOCK, &signals, nullptr);

	    auto response_sleep_ms =
        std::make_shared<std::atomic_uint>(*opt_response_sleep_ms);
	    AdServer::UserInfoSvcs::UserBindServerGrpc_var server =
	      new AdServer::UserInfoSvcs::UserBindServerGrpc(
	        nullptr,
	        nullptr,
	        endpoint.host,
	        endpoint.port,
	        128,
	        response_sleep_ms);
    server->activate_object();

    std::cout << "MockUserBindServer listening at " << endpoint.host << ":" <<
      endpoint.port << ", response_sleep_ms=" <<
      response_sleep_ms->load(std::memory_order_acquire) << std::endl;

	    for (;;)
	    {
	      int signal = 0;
	      sigwait(&signals, &signal);
	      if (signal == SIGUSR1)
	      {
          const auto current =
            response_sleep_ms->load(std::memory_order_acquire);
          const auto next = current == 0 ? *opt_response_sleep_ms : 0;
	        response_sleep_ms->store(next, std::memory_order_release);
	        std::cout << "MockUserBindServer response_sleep_ms=" <<
            next << std::endl;
	        continue;
	      }
	      break;
	    }

    server->deactivate_object();
    server->wait_object();
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "MockUserBindServer failed: " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "MockUserBindServer failed: unknown exception" << std::endl;
  }

  return 1;
}
