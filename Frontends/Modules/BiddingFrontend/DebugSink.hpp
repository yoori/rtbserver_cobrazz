#pragma once

#include <string>
#include <sstream>

#include "CampaignManagerTypes.hpp"
#include <ChannelServerGrpc.grpc.pb.h>
#include <UserInfoManagerGrpc.grpc.pb.h>
#include <Generics/Time.hpp>
#include <String/SubString.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>

namespace AdServer::Bidding
{
  struct RequestInfo;

  enum DebugInfo
  {
    DI_NONE,
    DI_HEADER,
    DI_BODY
  };

  struct RequestTimeMetering
  {
    Generics::Time total_time;
    Generics::Time user_resolving_started_at;
    Generics::Time user_resolving_time;
    Generics::Time trigger_match_started_at;
    Generics::Time trigger_match_time;
    Generics::Time history_match_started_at;
    Generics::Time history_match_time;
    Generics::Time history_match_local_time;
    bool history_match_error = false;
    Generics::Time creative_selection_started_at;
    Generics::Time creative_selection_time;
    Generics::Time creative_selection_local_time;
    bool creative_selection_error = false;
  };

  class DebugSink
  {
  public:
    struct UserResolvingDebugInfo
    {
      bool response_present = false;
      std::string user_id;
      bool min_age_reached = false;
      bool created = false;
      bool invalid_operation = false;
      bool user_found = false;
    };

    explicit DebugSink(std::string server_id);

    void
    set(const String::SubString& require_debug_info) noexcept;

    bool
    require_debug_info() const noexcept;

    static bool
    require_debug_info(
      const String::SubString& require_debug_info) noexcept;

    void
    print_request_debug_info(
      const RequestInfo& request_info,
      const AdServer::Bidding::CampaignManager::RequestParams&
        request_params,
      const AdServer::Commons::UserId& user_id,
      const std::string& channel_keywords) noexcept;

    void
    print_user_resolving_debug_info(
      const UserResolvingDebugInfo& user_resolving_debug_info) noexcept;

    void
    print_creative_selection_debug_info(
      const AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result) noexcept;

    void
    print_channel_matching_debug_info(
      const adserver::channel_svcs::channel_server::MatchResponse&
        response) noexcept;

    void
    print_history_matching_debug_info(
      const adserver::user_info_svcs::user_info_manager::MatchResult&
        match_result) noexcept;

    void
    print_time_metering_debug_info(
      const RequestTimeMetering& time_metering) noexcept;

    void
    print_interrupt_debug_info(
      const String::SubString& interrupted_step,
      const std::string& user_info_client_endpoint,
      unsigned long request_in_progress,
      unsigned long user_resolving_in_progress,
      unsigned long trigger_match_in_progress,
      unsigned long history_match_in_progress,
      unsigned long campaign_selection_in_progress,
      const AdServer::Grpc::Stats& user_bind_client_stats,
      const AdServer::Grpc::Stats& user_info_client_stats) noexcept;

    void
    write_response(
      FCGI::HttpResponse_var& response,
      int& http_status,
      const AdServer::Commons::UserId& user_id) const noexcept;

  private:
    std::string
    make_debug_info_(
      const AdServer::Commons::UserId& user_id) const;

    void
    print_empty_creative_selection_debug_info_() noexcept;

    void
    print_expected_debug_info_(
      const AdServer::Bidding::CampaignManager::AdSlotDebugInfo&
        debug_info) noexcept;

    void
    print_creative_selection_debug_info_(
      const AdServer::Bidding::CampaignManager::AdSlotResult&
        ad_slot_result) noexcept;

    static DebugInfo
    parse_require_debug_info_(
      const String::SubString& require_debug_info) noexcept;

  private:
    std::string server_id_;
    const char* sep_ = nullptr;
    DebugInfo require_debug_info_ = DI_NONE;
    std::ostringstream debug_info_str_;
  };
}
