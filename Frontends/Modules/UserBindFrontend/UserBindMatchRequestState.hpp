#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/Location.hpp>
#include <ChannelServerGrpc.pb.h>
#include <UserInfoManagerGrpc.pb.h>

namespace AdServer
{
  class UserBindFrontend;

  class UserBindMatchRequestState final:
    public std::enable_shared_from_this<UserBindMatchRequestState>
  {
  public:
    UserBindMatchRequestState(
      UserBindFrontend* frontend,
      const Commons::UserId& result_user_id,
      const Commons::UserId& merge_user_id,
      bool create_user_profile,
      const String::SubString& keywords,
      const String::SubString& cohort,
      const String::SubString& referer,
      unsigned long colo_id,
      const FrontendCommons::Location* location,
      const String::SubString& source);

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
    history_stage_();

    void
    get_merge_profile_done_stage_(
      std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchRequest>
        history_match_request,
      const grpc::Status& status,
      const adserver::user_info_svcs::user_info_manager::
        GetUserProfileResponse& response);

    void
    merge_stage_(
      std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchRequest>
        history_match_request,
      const adserver::user_info_svcs::user_info_manager::
        GetUserProfileResponse& get_profile_response);

    void
    merge_done_stage_(const grpc::Status& status);

    void
    remove_merged_profile_stage_();

    void
    remove_merged_profile_done_stage_(const grpc::Status& status);

    void
    match_stage_(
      std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchRequest>
        history_match_request);

    void
    match_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_info_manager::MatchResponse& response);

    void
    campaign_stage_();

    void
    campaign_done_stage_(const grpc::Status& status);

    void
    fill_history_match_request_(
      adserver::user_info_svcs::user_info_manager::MatchRequest&
        history_match_request) const;

    bool
    need_history_() const;

    void
    log_channel_error_(const grpc::Status& status) const;

    void
    log_user_info_error_(
      const char* operation,
      const grpc::Status& status) const;

  private:
    UserBindFrontend* frontend_;
    Commons::UserId result_user_id_;
    Commons::UserId merge_user_id_;
    bool create_user_profile_;
    std::string keywords_;
    std::string cohort_;
    std::string referer_;
    unsigned long colo_id_;
    FrontendCommons::Location_var location_;
    std::string source_;
    Generics::Time now_;
    adserver::channel_svcs::channel_server::MatchResponse
      trigger_match_result_;
    bool trigger_match_result_present_;
    std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchResponse>
      history_match_result_;
  };
}
