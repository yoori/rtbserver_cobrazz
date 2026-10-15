#pragma once

#include <ChannelServerGrpc.grpc-client.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <eh/Exception.hpp>

namespace AdServer::ChannelSvcs::GrpcAlgs
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  // Inputs must satisfy the sorted, unique TriggerMatchResult contract.
  void merge_match_response(
    adserver::channel_svcs::channel_server::MatchResponse& target,
    const adserver::channel_svcs::channel_server::MatchResponse& source);

  adserver::channel_svcs::channel_server::MatchResponse channel_match(
    ChannelServerGrpcAsyncClient& client,
    const adserver::channel_svcs::channel_server::MatchRequest& request);
}
