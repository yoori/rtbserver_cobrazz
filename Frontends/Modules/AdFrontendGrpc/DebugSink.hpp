#ifndef FRONTENDS_ADFRONTEND_DEBUGSINKGRPC_HPP
#define FRONTENDS_ADFRONTEND_DEBUGSINKGRPC_HPP

// STD
#include <sstream>
#include <string>

// UNIXCOMMONS
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <String/SubString.hpp>

// THIS
#include <ChannelSvcs/ChannelCommons/proto/ChannelServer.pb.h>
#include <Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/Modules/AdFrontend/AdFrontendStat.hpp>
#include <Frontends/Modules/AdFrontend/RequestInfo.hpp>
#include <UserInfoSvcs/UserInfoManager/proto/UserInfoManager.pb.h>

namespace AdServer::Grpc
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
  } // namespace Response

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

  using DebugStream = std::ostringstream;

  class DebugInfoStatus
  {
  public:
    DebugInfoStatus();

    void set(DebugInfo new_value) noexcept;

    bool require_debug_info() const;

    bool require_long_debug_info() const;

    bool require_short_header() const;

    bool require_debug_body() const;

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
   **/
  class DebugSink final : public DebugInfoStatus
  {
  public:
    using HttpResponse = FrontendCommons::HttpResponse;

  private:
    using IdSetType = std::set<std::uint32_t>;

  public:
    DebugSink(const bool allow_show_history_profile) noexcept;

    // debug info printing
    void write_response(
      HttpResponse& response,
      std::string& str_response,
      int& http_status) const;

    void print_acquire_ad(
      const RequestInfo& request_info,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_matched_channels,
      const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::CCGKeyword>* ccg_keywords,
      const AdServer::UserInfoSvcs::Proto::MatchResult& history_match_result) noexcept;

    void print_creative_selection_debug_info(
      const RequestInfo& request_info,
      const PassbackInfo& passback_info,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_matching_result,
      const RequestTimeMetering& request_time_metering) noexcept;

    void print_request_debug_info(
      const RequestInfo& request_info) noexcept;

    void print_trigger_matching_debug_info(
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo& match_result,
      const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::CCGKeyword>* ccg_keywords,
      const char* prefix = "") noexcept;

    void fill_debug_body(
      HttpResponse& response,
      int& http_status,
      Stream::Error& ostr) const noexcept;

    void print_time_metering_debug_info(
      const RequestTimeMetering& time_metering) noexcept;

  private:
    void print_channel_matching_seq_(
      const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::ChannelAtom>& channels,
      const char type) noexcept;

    void print_history_matching_debug_info_(
      const AdServer::UserInfoSvcs::Proto::MatchResult& history_match_result) noexcept;

    void print_creative_selection_debug_info_(
      const PassbackInfo& passback_info,
      const AdServer::CampaignSvcs::Proto::AdSlotResult&
        request_creative_result) noexcept;

    void print_empty_creative_selection_debug_info_() noexcept;

    static void fill_debug_body_(
      HttpResponse& response,
      const String::SubString& error_description) noexcept;

  private:
    const bool allow_show_history_profile_;

    DebugStream debug_info_str_;
  };
} // namespace AdServer::Grpc

/** Inlines */
namespace AdServer::Grpc
{
  inline
  DebugInfoStatus::DebugInfoStatus()
  : field_begin_(0),
    field_end_(0),
    sep_(0),
    creative_end_sep_(0),
    require_debug_info_(DI_NONE)
  {
  }

  inline
  bool DebugInfoStatus::require_debug_info() const
  {
    return require_debug_info_ != DI_NONE;
  }

  inline
  bool DebugInfoStatus::require_debug_body() const
  {
    return require_debug_info_ == DI_BODY;
  }

  inline
  bool DebugInfoStatus::require_short_header() const
  {
    return require_debug_info_ == DI_HEADER_SHORT;
  }

  inline
  void DebugInfoStatus::set(DebugInfo new_value) noexcept
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
  bool DebugInfoStatus::require_long_debug_info() const
  {
    return require_debug_info() && require_debug_info_ != DI_HEADER_SHORT;
  }
} // namespace AdServer::Grpc

#endif /* FRONTENDS_ADFRONTEND_DEBUGSINKGRPC_HPP */