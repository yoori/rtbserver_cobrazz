#pragma once

#include <memory>
#include <vector>

#include <Commons/Grpc/ResponseHolder.hpp>
#include <Commons/UserInfoManip.hpp>
#include <ChannelServerGrpc.pb.h>
#include <UserInfoManagerGrpc.pb.h>

#include "RequestInfoFiller.hpp"

namespace AdServer::ImprTrack
{
  class Frontend;

  class ImprTrackMatchRequestState final:
    public std::enable_shared_from_this<ImprTrackMatchRequestState>
  {
  public:
    explicit ImprTrackMatchRequestState(Frontend* frontend) noexcept;

  public:
    RequestInfo request_info;
    AdServer::Commons::UserId user_id;
    AdServer::Commons::UserId cookie_user_id;
    AdServer::Commons::UserId resolved_cookie_user_id;
    std::vector<unsigned long> campaign_ids;
    std::vector<unsigned long> advertiser_ids;
    AdServer::Grpc::ResponseHolder<
      adserver::channel_svcs::channel_server::MatchResponse>
        trigger_match_result;
    AdServer::Grpc::ResponseHolder<
      adserver::user_info_svcs::user_info_manager::MatchResponse>
      history_match_response;
    bool history_match_present = false;

  private:
    Frontend* frontend_;
  };
}
