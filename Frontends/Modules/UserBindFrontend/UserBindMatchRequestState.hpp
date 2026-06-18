#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/ResponseHolder.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/Location.hpp>
#include <Frontends/FrontendCommons/RequestTask.hpp>
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
      FrontendCommons::Location_var location,
      const String::SubString& source);

    void
    start();

  private:
    FrontendCommons::RequestTask
    co_process_(std::shared_ptr<UserBindMatchRequestState> self) noexcept;

    FrontendCommons::RequestTask
    co_channel_match_() noexcept;

    FrontendCommons::RequestTask
    co_history_() noexcept;

    FrontendCommons::RequestTask
    co_campaign_() noexcept;

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
    AdServer::Grpc::ResponseHolder<
      adserver::channel_svcs::channel_server::MatchResponse>
      trigger_match_result_;
    bool trigger_match_result_present_;
    AdServer::Grpc::ResponseHolder<
      adserver::user_info_svcs::user_info_manager::MatchResponse>
      history_match_result_;
  };
}
