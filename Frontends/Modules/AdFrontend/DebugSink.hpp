#ifndef FRONTENDS_ADFRONTEND_DEBUGSINK_HPP_
#define FRONTENDS_ADFRONTEND_DEBUGSINK_HPP_

#include <sstream>
#include <string>

#include <String/SubString.hpp>

#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

#include "AdFrontendStat.hpp"
#include "RequestInfo.hpp"

namespace AdServer
{
  namespace Response
  {
    namespace Header
    {
      const String::SubString MERGE_FAILED("x-Merge-Failed");
      const String::SubString DEBUG_INFO("Debug-Info");
    }

    namespace Type
    {
      const String::SubString TEXT_HTML("text/html");
    }
  }

  enum DebugInfo
  {
    DI_NONE,
    DI_HEADER,
    DI_HEADER_SHORT,
    DI_BODY
  };

  struct PassbackInfo
  {
    std::string url;
  };

  typedef std::ostringstream DebugStream;

  class DebugInfoStatus
  {
  public:
    DebugInfoStatus();

    void
    set(DebugInfo new_value) noexcept;

    bool
    require_debug_info() const;

    bool
    require_long_debug_info() const;

    bool
    require_short_header() const;

    bool
    require_debug_body() const;
  protected:
    const char* field_begin_;
    const char* field_end_;
    const char* sep_;
    String::SubString creative_start_sep_;
    const char* creative_end_sep_;
  private:
    DebugInfo require_debug_info_;
  };

  /**
   * If requested fill debug information and send response
   */
  class DebugSink : public DebugInfoStatus
  {
  public:
    typedef FCGI::HttpResponse HttpResponse;

    DebugSink(bool allow_show_history_profile) noexcept;

    /// debug info printing
    void
    write_response(
      HttpResponse& response,
      std::string& str_response,
      int& http_status) const;

    void
    print_acquire_ad(
      const RequestInfo& request_info,
      const ChannelSvcs::ChannelServerBase::MatchResult_var& trigger_matched_channels,
      const ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var ccg_keywords,
      const UserInfoSvcs::UserInfoMatcher::MatchResult& history_match_result)
      noexcept;

    void
    print_creative_selection_debug_info(
      const RequestInfo& request_info,
      const PassbackInfo& passback_info,
      const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
        campaign_matching_result,
      const RequestTimeMetering& request_time_metering)
      noexcept;

    void
    print_request_debug_info(
      const RequestInfo& request_info)
      noexcept;

    void
    print_trigger_matching_debug_info(
      const ChannelSvcs::ChannelServerBase::MatchResult& match_result,
      const ChannelSvcs::ChannelServerBase::CCGKeywordSeq* ccg_keywords,
      const char* prefix = "")
      noexcept;

    void
    fill_debug_body(
      HttpResponse& response,
      int& http_status,
      Stream::Error& ostr) const noexcept;

    void
    print_time_metering_debug_info(
      const RequestTimeMetering& time_metering)
      noexcept;

  private:
    typedef std::set<unsigned int> IdSetType;

    void
    print_channel_matching_seq_(
      const ChannelSvcs::ChannelServerBase::ChannelAtomSeq& channels,
      char type)
      noexcept;

    void
    print_history_matching_debug_info_(
      const UserInfoSvcs::UserInfoMatcher::MatchResult&
        history_match_result)
      noexcept;

    void
    print_creative_selection_debug_info_(
      const PassbackInfo& passback_info,
      const CampaignSvcs::CampaignManager::AdSlotResult&
        request_creative_result)
      noexcept;

    void
    print_empty_creative_selection_debug_info_()
      noexcept;

    static void
    fill_debug_body_(
      HttpResponse& response,
      const String::SubString& error_description) noexcept;

  private:
    const bool allow_show_history_profile_;

    DebugStream debug_info_str_;
  };
}

/** Inlines */
namespace AdServer
{
  inline
  DebugInfoStatus::DebugInfoStatus() : field_begin_(0),
    field_end_(0), sep_(0), creative_end_sep_(0),
    require_debug_info_(DI_NONE)
  {}

  inline
  bool
  DebugInfoStatus::require_debug_info() const
  {
    return require_debug_info_ != DI_NONE;
  }

  inline
  bool
  DebugInfoStatus::require_debug_body() const
  {
    return require_debug_info_ == DI_BODY;
  }

  inline
  bool
  DebugInfoStatus::require_short_header() const
  {
    return require_debug_info_ == DI_HEADER_SHORT;
  }

  inline
  void
  DebugInfoStatus::set(DebugInfo new_value) noexcept
  {
    require_debug_info_ = new_value;
    const bool NEED_BODY = require_debug_body();
    field_begin_ = NEED_BODY ? "  " : "( ";
    field_end_ = NEED_BODY ? "\n" : ") ";
    sep_ = NEED_BODY ? ";\n" : "; ";
    creative_start_sep_ = NEED_BODY ?
      String::SubString("\n------\n") : String::SubString("( ");
    creative_end_sep_ = NEED_BODY ? "" : ") ";
  }

  inline
  bool
  DebugInfoStatus::require_long_debug_info() const
  {
    return require_debug_info() && require_debug_info_ != DI_HEADER_SHORT;
  }
}

#endif /* FRONTENDS_ADFRONTEND_DEBUGSINK_HPP_ */
