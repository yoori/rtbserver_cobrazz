#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/ProcessControl.grpc.pb.h>

namespace
{
  namespace pc = adserver::grpc::process_control;

  int usage()
  {
    std::cerr
      << "usage: GrpcProbeObj <host:port> [timeout_ms] "
         "[-control DB {-1|0|1}]\n";
    return 2;
  }

  bool parse_int(const char* value, int& result)
  {
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (!end || *end != '\0')
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
    return result;
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

  auto channel = grpc::CreateChannel(
    endpoint,
    grpc::InsecureChannelCredentials());
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
    context.set_deadline(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(timeout_ms));

    if (db_state == -1)
    {
      pc::GetDbStateRequest request;
      pc::GetDbStateResponse response;
      const grpc::Status status =
        stub->get_db_state(&context, request, &response);
      if (!status.ok())
      {
        print_error(status);
        return 2;
      }

      std::cout << (response.enabled() ? "DB enabled" : "DB disabled") <<
        '\n';
      return response.enabled() ? 0 : 1;
    }

    pc::SetDbStateRequest request;
    request.set_enabled(db_state != 0);
    pc::SetDbStateResponse response;
    const grpc::Status status =
      stub->set_db_state(&context, request, &response);
    if (!status.ok())
    {
      print_error(status);
      return 2;
    }

    return 0;
  }

  grpc::ClientContext context;
  context.set_deadline(
    std::chrono::system_clock::now() +
    std::chrono::milliseconds(timeout_ms));

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
