#include <iostream>
#include <stdexcept>
#include <string>

#include <Logger/StreamLogger.hpp>
#include <Frontends/Modules/ImprTrackFrontend/RequestInfoFiller.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManagerGrpc.pb.h>

namespace
{
  void check(const std::string& query, const std::string& expected)
  {
    Logging::Logger_var logger = new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    AdServer::CommonModule_var common_module = new AdServer::CommonModule(logger);
    AdServer::ImprTrack::RequestInfoFiller filler(
      logger, nullptr, common_module, 1, nullptr, {}, {});
    HTTP::ParamList params;
    FCGI::HttpRequest::parse_params(query, params);
    FCGI::HttpRequest request;
    request.set_params(std::move(params));
    AdServer::ImprTrack::RequestInfo info;
    filler.fill(info, request);
    if (info.verify_type != AdServer::CampaignSvcs::RVT_CUSTOM ||
      info.action_name != "visibility" || info.action_value != expected)
    {
      throw std::runtime_error("ImprTrack action payload parsing failed");
    }

    adserver::campaign_svcs::campaign_manager::ImpressionInfo sent;
    sent.set_action_name(info.action_name);
    sent.set_action_value(info.action_value);
    adserver::campaign_svcs::campaign_manager::ImpressionInfo received;
    if (!received.ParseFromString(sent.SerializeAsString()) ||
      received.action_name() != "visibility" || received.action_value() != expected)
    {
      throw std::runtime_error("ImpressionInfo gRPC payload round trip failed");
    }
  }
}

int main()
{
  try
  {
    const std::string prefix = "t=c&colo=1&nm=visibility";
    check(prefix, "");
    check(prefix + "&value=", "");
    check(prefix + "&value=62.5&rnd=123", "62.5");
    check(prefix + "&value=0", "0");
    check(prefix + "&value=%7B%22label%22%3A%22a+b%22%7D%09%0A%5C",
      "{\"label\":\"a b\"}\t\n\\");
    std::cout << "ImprTrack action payload tests passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
