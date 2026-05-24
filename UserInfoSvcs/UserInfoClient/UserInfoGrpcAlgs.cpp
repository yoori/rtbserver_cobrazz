#include "UserInfoGrpcAlgs.hpp"

#include <string>
#include <utility>

#include <Commons/Grpc/GrpcSync.hpp>

namespace AdServer::UserInfoSvcs::GrpcAlgs
{
  namespace
  {
    namespace pb = adserver::user_info_svcs::user_info_manager;

    void throw_user_info_exception_(const grpc::Status& status)
    {
      const std::string message = status.error_message();
      switch(status.error_code())
      {
      case grpc::StatusCode::UNAVAILABLE:
        throw NotReady(message);
      case grpc::StatusCode::NOT_FOUND:
        throw ChunkNotFound(message);
      default:
        throw Exception(message);
      }
    }
  }

  bool history_match(
    UserInfoManagerGrpcAsyncClient& client,
    const pb::MatchRequest& request,
    pb::MatchResponse& response)
  {
    response = AdServer::Grpc::sync_call<pb::MatchResponse>(
      [&](auto callback)
      {
        client.match(request, std::move(callback));
      },
      throw_user_info_exception_);
    return response.matched();
  }

  bool fraud_user(
    UserInfoManagerGrpcAsyncClient& client,
    const std::string& user_id,
    const std::string& time)
  {
    pb::FraudUserRequest request;
    request.set_user_id(user_id);
    request.set_time(time);
    const auto response =
      AdServer::Grpc::sync_call<pb::FraudUserResponse>(
        [&](auto callback)
        {
          client.fraud_user(request, std::move(callback));
        },
        throw_user_info_exception_);
    return response.fraud();
  }
}
