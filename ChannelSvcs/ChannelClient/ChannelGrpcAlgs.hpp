#pragma once

#include <ChannelServerGrpc.grpc-client.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <eh/Exception.hpp>

namespace AdServer::ChannelSvcs::GrpcAlgs
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  adserver::channel_svcs::channel_server::MatchResponse channel_match(
    ChannelServerGrpcAsyncClient& client,
    const adserver::channel_svcs::channel_server::MatchRequest& request);
}
