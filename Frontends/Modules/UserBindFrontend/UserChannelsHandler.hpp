#ifndef ADSERVER_USERCHANNELSHANDLER_HPP
#define ADSERVER_USERCHANNELSHANDLER_HPP

// THIS
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/Modules/UserBindFrontend/RequestInfoFiller.hpp>

namespace AdServer
{

class UserChannelsHandler final
{
public:
  using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
  using UserInfoClient = FrontendCommons::UserInfoClient;
  using UserInfoClient_var = FrontendCommons::UserInfoClient_var;
  using RequestInfo = UserBind::RequestInfo;
  using HttpResponse = FrontendCommons::HttpResponse;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit UserChannelsHandler(
    const GrpcContainerPtr& grpc_container,
    UserInfoClient* user_info_client,
    Logger* logger);

  int handle(
    const RequestInfo& request_info,
    HttpResponse& response);

private:
  const GrpcContainerPtr grpc_container_;

  const UserInfoClient_var user_info_client_;

  const Logger_var logger_;
};

} // namespace AdServer

#endif //ADSERVER_USERCHANNELSHANDLER_HPP