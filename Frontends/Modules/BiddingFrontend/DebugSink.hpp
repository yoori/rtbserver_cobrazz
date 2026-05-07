#ifndef BIDDINGFRONTEND_DEBUGSINK_HPP_
#define BIDDINGFRONTEND_DEBUGSINK_HPP_

#include <string>
#include <sstream>

#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
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

  class DebugSink
  {
  public:
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
      const AdServer::Commons::UserId& user_id) noexcept;

    void
    print_creative_selection_debug_info(
      const AdServer::CampaignSvcs_v360::CampaignManager::RequestCreativeResult&
        campaign_match_result) noexcept;

    void
    print_channel_matching_debug_info(
      const AdServer::ChannelSvcs::ChannelServerBase::MatchResult&
        match_result) noexcept;

    void
    print_history_matching_debug_info(
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult&
        match_result) noexcept;

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

#endif
