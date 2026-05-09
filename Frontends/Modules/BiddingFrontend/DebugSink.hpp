#pragma once

#include <string>
#include <sstream>

#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <Generics/Time.hpp>
#include <String/SubString.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

namespace AdServer
{
namespace Bidding
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
    Generics::Time user_resolving_time;
    Generics::Time trigger_match_time;
    Generics::Time history_match_time;
    Generics::Time history_match_local_time;
    Generics::Time creative_selection_time;
    Generics::Time creative_selection_local_time;
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
      const AdServer::CampaignSvcs_v360::CampaignManager::RequestParams&
        request_params,
      const AdServer::Commons::UserId& user_id,
      const std::string& channel_keywords) noexcept;

    void
    print_user_resolving_debug_info(
      const UserResolvingDebugInfo& user_resolving_debug_info) noexcept;

    void
    print_creative_selection_debug_info(
      const AdServer::CampaignSvcs_v360::CampaignManager::RequestCreativeResult&
        campaign_match_result) noexcept;

    void
    print_channel_matching_debug_info(
      const adserver::channel_svcs::channel_server::MatchResponse&
        response) noexcept;

    void
    print_history_matching_debug_info(
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult&
        match_result) noexcept;

    void
    print_time_metering_debug_info(
      const RequestTimeMetering& time_metering) noexcept;

    void
    print_interrupt_debug_info(
      const String::SubString& interrupted_step) noexcept;

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
      const AdServer::CampaignSvcs_v360::CampaignManager::AdSlotDebugInfo&
        debug_info) noexcept;

    void
    print_creative_selection_debug_info_(
      const AdServer::CampaignSvcs_v360::CampaignManager::AdSlotResult&
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
}
