#pragma once

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <eh/Exception.hpp>

namespace AdServer::ChannelSvcs::GrpcAlgs
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  adserver::channel_svcs::channel_server::MatchResponse channel_match(
    ChannelServerGrpcAsyncClient& client,
    const adserver::channel_svcs::channel_server::MatchRequest& request);

  void make_get_ccg_traits_request(
    const ChannelIdSeq& source,
    adserver::channel_svcs::channel_server::GetCcgTraitsRequest& target);

  ChannelServerBase::CCGKeywordSeq* make_ccg_traits_result(
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse&
      source);
}
