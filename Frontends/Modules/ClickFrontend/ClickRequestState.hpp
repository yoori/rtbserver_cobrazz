#pragma once

#include <list>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <Generics/Time.hpp>
#include <Commons/UserInfoManip.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <UserBindServerGrpc.pb.h>
#include <UserInfoManagerGrpc.pb.h>

namespace AdServer
{
  class ClickFrontend;

  class ClickRequestState:
    public std::enable_shared_from_this<ClickRequestState>
  {
  public:
    ClickRequestState(
      ClickFrontend* frontend,
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::UserId& cookie_user_id,
      const Generics::Time& now,
      unsigned long campaign_id,
      unsigned long advertiser_id,
      const String::SubString& peer_ip,
      const std::list<std::string>& markers);

    void
    start();

  private:
    void
    channel_match_stage_();

    void
    channel_match_done_stage_(
      const grpc::Status& status,
      const adserver::channel_svcs::channel_server::MatchResponse& response);

    void
    resolve_cookie_stage_();

    void
    resolve_cookie_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::GetUserIdResponse& response);

    void
    match_history_user_stage_();

    void
    match_history_user_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_info_manager::MatchResponse& response);

    void
    match_history_cookie_stage_();

    void
    match_history_cookie_done_stage_(const grpc::Status& status);

    void
    process_match_stage_();

    void
    process_match_done_stage_(const grpc::Status& status);

    adserver::user_info_svcs::user_info_manager::MatchRequest
    make_history_match_request_(
      const AdServer::Commons::UserId& match_user_id) const;

    void
    log_user_bind_error_(const eh::Exception& ex) const noexcept;

  private:
    ClickFrontend* frontend_;
    AdServer::Commons::UserId user_id_;
    AdServer::Commons::UserId cookie_user_id_;
    AdServer::Commons::UserId resolved_cookie_user_id_;
    Generics::Time now_;
    unsigned long campaign_id_;
    unsigned long advertiser_id_;
    std::string peer_ip_;
    std::list<std::string> markers_;
    adserver::channel_svcs::channel_server::MatchResponse
      trigger_match_result_;
    bool trigger_match_result_present_;
    std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchResponse>
      history_match_result_;
  };
}
