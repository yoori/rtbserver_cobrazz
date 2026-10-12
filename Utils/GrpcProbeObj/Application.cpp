#include <chrono>
#include <cstdlib>
#include <cerrno>
#include <limits>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/ProcessControl.grpc.pb.h>

namespace
{
  namespace pc = adserver::grpc::process_control;

  int usage()
  {
    std::cerr << "usage: GrpcProbeObj <host:port> [timeout_ms] " "[-control DB {-1|0|1}]\n";
    return 2;
  }

  bool parse_int(const char* value, int& result)
  {
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno || end == value || *end != '\0' ||
        parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max())
    {
      return false;
    }

    result = static_cast<int>(parsed);
    return true;
  }

  std::string status_description(const grpc::Status& status)
  {
    std::string result = status.error_message();
    if (result.empty())
    {
      result = std::to_string(static_cast<int>(status.error_code()));
    }

    if (!status.error_details().empty())
    {
      if (!result.empty())
      {
        result += ": ";
      }
      result += status.error_details();
    }
    return std::to_string(static_cast<int>(status.error_code())) + ": " + result;
  }

  void print_error(const grpc::Status& status)
  {
    std::cout << "GrpcProbeObj: " << status_description(status) << '\n';
  }
}

int
main(int argc, char** argv)
{
  if (argc < 2)
  {
    return usage();
  }

  const std::string endpoint = argv[1];
  int timeout_ms = 5000;
  int arg_pos = 2;
  if (argc > arg_pos)
  {
    int parsed_timeout = 0;
    if (parse_int(argv[arg_pos], parsed_timeout))
    {
      timeout_ms = parsed_timeout;
      ++arg_pos;
    }
  }

  if (timeout_ms <= 0 || timeout_ms > 3600000)
  {
    return usage();
  }

  grpc::ChannelArguments channel_args;
  channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
  auto channel = AdServer::Grpc::create_custom_channel(
    endpoint,
    grpc::InsecureChannelCredentials(),
    channel_args);
  auto stub = pc::ProcessControl::NewStub(channel);

  if (argc > arg_pos)
  {
    if (argc != arg_pos + 3 ||
      std::string(argv[arg_pos]) != "-control" ||
      std::string(argv[arg_pos + 1]) != "DB")
    {
      return usage();
    }

    int db_state = 0;
    if (!parse_int(argv[arg_pos + 2], db_state) ||
      (db_state != -1 && db_state != 0 && db_state != 1))
    {
      return usage();
    }

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms));

    pc::GetDbStateRequest get_request;
    pc::GetDbStateResponse state;
    const grpc::Status get_status = stub->get_db_state(&context, get_request, &state);
    if (!get_status.ok())
    {
      print_error(get_status);
      return 2;
    }

    if (state.has_supported() && !state.supported())
    {
      std::cout << "DB not applicable (no direct DB connection)\n";
      return 0;
    }

    if (db_state == -1)
    {
      std::cout << (state.enabled() ? "DB enabled" : "DB disabled") << '\n';
      return state.enabled() ? 0 : 1;
    }

    pc::SetDbStateRequest request;
    request.set_enabled(db_state != 0);
    pc::SetDbStateResponse response;
    grpc::ClientContext set_context;
    set_context.set_deadline(context.deadline());
    const grpc::Status status = stub->set_db_state(&set_context, request, &response);
    if (!status.ok())
    {
      print_error(status);
      std::cout << "DB state is unconfirmed; query status before retrying\n";
      return 2;
    }

    grpc::ClientContext verify_context;
    verify_context.set_deadline(context.deadline());
    const grpc::Status verified = stub->get_db_state(&verify_context, get_request, &state);
    if (!verified.ok())
    {
      print_error(verified);
      std::cout << "DB state is unconfirmed; query status before retrying\n";
      return 2;
    }

    if ((state.has_supported() && !state.supported()) || state.enabled() != request.enabled())
    {
      std::cout << "DB state differs from requested state\n";
      return 2;
    }
    std::cout << (state.enabled() ? "DB enabled" : "DB disabled") << '\n';
    return 0;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms));

  pc::GetStatusRequest request;
  pc::GetStatusResponse response;
  const grpc::Status status = stub->get_status(&context, request, &response);
  if (!status.ok())
  {
    print_error(status);
    return 2;
  }

  if (!response.description().empty())
  {
    std::cout << response.description() << '\n';
  }
  else
  {
    std::cout << (response.ready() ? "ready" : "not ready") << '\n';
  }

  return response.ready() ? 0 : 1;
}
