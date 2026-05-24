#pragma once

#include <string>

#include <eh/Exception.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>

namespace AdServer::UserInfoSvcs::GrpcAlgs
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(NotReady, Exception);
  DECLARE_EXCEPTION(ChunkNotFound, Exception);

  bool history_match(
    UserInfoManagerGrpcAsyncClient& client,
    const adserver::user_info_svcs::user_info_manager::MatchRequest& request,
    adserver::user_info_svcs::user_info_manager::MatchResponse& response);

  bool fraud_user(
    UserInfoManagerGrpcAsyncClient& client,
    const std::string& user_id,
    const std::string& time);
}
