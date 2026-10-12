#include <chrono>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>
#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.grpc.pb.h>

namespace
{
  namespace ChannelProto = adserver::channel_svcs::channel_server;

  template<class Request, class Response, class Method>
  void call(ChannelProto::ChannelServerGrpc::Stub& stub, Method method)
  {
    const std::string input{std::istreambuf_iterator<char>(std::cin), {}};
    Request request;
    const auto parsed = google::protobuf::util::JsonStringToMessage(input, &request);
    if (!parsed.ok())
    {
      throw std::runtime_error(parsed.ToString());
    }

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));
    context.set_wait_for_ready(true);
    Response response;
    const auto status = (stub.*method)(&context, request, &response);
    if (!status.ok())
    {
      throw std::runtime_error(
        std::to_string(status.error_code()) + ": " + status.error_message());
    }

    google::protobuf::util::JsonPrintOptions options;
    options.preserve_proto_field_names = true;
    std::string output;
    const auto printed = google::protobuf::util::MessageToJsonString(response, &output, options);
    if (!printed.ok())
    {
      throw std::runtime_error(printed.ToString());
    }

    std::cout << output << '\n';
  }
}

int main(int argc, char** argv)
{
  try
  {
    if (argc != 3)
    {
      throw std::runtime_error("Usage: ChannelServerGrpcTestClient HOST:PORT METHOD < request.json");
    }

    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    auto channel = grpc::CreateCustomChannel(argv[1], grpc::InsecureChannelCredentials(), args);
    auto stub = ChannelProto::ChannelServerGrpc::NewStub(channel);
    const std::string method = argv[2];
    if (method == "match")
    {
      call<ChannelProto::MatchRequest, ChannelProto::MatchResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::match);
    }
    else if (method == "get_ccg_traits")
    {
      call<ChannelProto::GetCcgTraitsRequest, ChannelProto::GetCcgTraitsResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::get_ccg_traits);
    }
    else if (method == "check_configuration")
    {
      call<ChannelProto::CheckConfigurationRequest, ChannelProto::CheckConfigurationResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::check_configuration);
    }
    else if (method == "set_sources")
    {
      call<ChannelProto::SetSourcesRequest, ChannelProto::SetSourcesResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::set_sources);
    }
    else if (method == "set_proxy_sources")
    {
      call<ChannelProto::SetProxySourcesRequest, ChannelProto::SetProxySourcesResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::set_proxy_sources);
    }
    else if (method == "check")
    {
      call<ChannelProto::CheckRequest, ChannelProto::CheckResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::check);
    }
    else if (method == "update_triggers")
    {
      call<ChannelProto::UpdateTriggersRequest, ChannelProto::UpdateTriggersResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::update_triggers);
    }
    else if (method == "update_all_ccg")
    {
      call<ChannelProto::UpdateAllCcgRequest, ChannelProto::UpdateAllCcgResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::update_all_ccg);
    }
    else if (method == "get_count_chunks")
    {
      call<ChannelProto::GetCountChunksRequest, ChannelProto::GetCountChunksResponse>(
        *stub, &ChannelProto::ChannelServerGrpc::Stub::get_count_chunks);
    }
    else
    {
      throw std::runtime_error("Unknown method: " + method);
    }
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
