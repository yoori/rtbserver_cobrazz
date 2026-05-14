#include "DebugSink.hpp"

#include <algorithm>
#include <iterator>
#include <set>
#include <utility>

#include <String/AsciiStringManip.hpp>
#include <Commons/Algs.hpp>
#include <Commons/GrpcAlgs.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Bidding
{
  namespace
  {
    namespace Response
    {
      namespace Header
      {
        const String::SubString DEBUG_INFO("Debug-Info");
      }

      namespace Type
      {
        const String::SubString TEXT_PLAIN("text/plain");
      }
    }

    namespace Debug
    {
      const char REQUEST_INFO_HEAD[] = "=== Request params ===";
      const char USER_RESOLVING_HEAD[] = "=== User resolving result ===";
      const char CHANNEL_MATCHING_HEAD[] = "=== Channel matching result ===";
      const char HISTORY_MATCHING_HEAD[] = "=== History matching result ===";
      const char CREATIVE_SELECTION_INFO_HEAD[] = "=== Creative selection ===";
      const char TRACE_CCG_INFO_HEAD[] = "=== Expected ===";
      const char TIME_METERING_HEAD[] = "=== Time Metering ===";
      const char INTERRUPT_HEAD[] = "=== Interrupt ===";
    }

    struct GetChannelId
    {
      unsigned long
      operator()(
        const adserver::channel_svcs::channel_server::ChannelAtom& atom)
        const noexcept
      {
        return atom.id();
      }
    };

    using ChannelIdSet = std::set<unsigned long>;

    void
    add_channel_ids_(
      ChannelIdSet& ids,
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>& channels)
    {
      std::transform(
        channels.begin(),
        channels.end(),
        std::inserter(ids, ids.end()),
        GetChannelId());
    }

    void
    print_channel_ids_(
      std::ostream& out,
      const ChannelIdSet& ids,
      char type,
      bool& print_delimiter)
    {
      for(const auto id : ids)
      {
        if(print_delimiter)
        {
          out << ", ";
        }
        out << id << type;
        print_delimiter = true;
      }
    }

    void
    print_channel_atom_seq_(
      std::ostream& out,
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>& channels,
      char type,
      bool& print_delimiter)
    {
      for(const auto& channel : channels)
      {
        if(print_delimiter)
        {
          out << ", ";
        }
        out << channel.id() << type << " :: " <<
          channel.trigger_channel_id();
        print_delimiter = true;
      }
    }

    const char*
    user_status_to_string(std::size_t user_status) noexcept
    {
      switch(user_status)
      {
      case CampaignSvcs::US_UNDEFINED:
        return "undefined";
      case CampaignSvcs::US_OPTIN:
        return "optin";
      case CampaignSvcs::US_OPTOUT:
        return "optout";
      case CampaignSvcs::US_PROBE:
        return "probe";
      case CampaignSvcs::US_TEMPORARY:
        return "temporary";
      default:
        return "unknown";
      }
    }

    const char*
    auction_type_to_string(std::size_t auction_type) noexcept
    {
      if(auction_type == CampaignSvcs::AT_RANDOM)
      {
        return "random";
      }
      else if(auction_type == CampaignSvcs::AT_MAX_ECPM)
      {
        return "max ecpm";
      }
      else if(auction_type == CampaignSvcs::AT_PROPORTIONAL_PROBABILITY)
      {
        return "proportional probability";
      }

      return "unknown";
    }

    std::string
    normalize_debug_field_(std::string value)
    {
      std::replace(value.begin(), value.end(), '\r', ' ');
      std::replace(value.begin(), value.end(), '\n', ' ');
      return value;
    }
  }

  DebugSink::DebugSink(std::string server_id)
    : server_id_(std::move(server_id))
  {}

  void
  DebugSink::set(const String::SubString& require_debug_info) noexcept
  {
    require_debug_info_ = parse_require_debug_info_(require_debug_info);
    sep_ = require_debug_info_ == DI_BODY ? "\n" : "; ";
  }

  bool
  DebugSink::require_debug_info() const noexcept
  {
    return require_debug_info_ != DI_NONE;
  }

  bool
  DebugSink::require_debug_info(
    const String::SubString& require_debug_info) noexcept
  {
    return parse_require_debug_info_(require_debug_info) != DI_NONE;
  }

  void
  DebugSink::print_request_debug_info(
    const RequestInfo& request_info,
    const AdServer::Bidding::CampaignManager::RequestParams&
      request_params,
    const AdServer::Commons::UserId& user_id,
    const std::string& channel_keywords) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::REQUEST_INFO_HEAD << "\n";
    }

    debug_info_str_ << "server-id = " << server_id_ << sep_ << "time = ";
    try
    {
      debug_info_str_ << request_info.current_time.gm_ft();
    }
    catch(...)
    {
      debug_info_str_ << "invalid";
    }

    const std::string normalized_channel_keywords =
      normalize_debug_field_(channel_keywords);

    debug_info_str_ << sep_ <<
      "user_status = " <<
        user_status_to_string(request_params.common_info.user_status) << sep_ <<
      "source_id = " << request_info.source_id << sep_ <<
      "request_id = " << CampaignManager::unpack_request_id(
        request_params.common_info.request_id) << sep_ <<
      "bid_request_id = " << request_info.bid_request_id << sep_ <<
      "bid_site_id = " << request_info.bid_site_id << sep_ <<
      "bid_publisher_id = " << request_info.bid_publisher_id << sep_ <<
      "publisher_site_id = " << request_params.publisher_site_id << sep_ <<
      "publisher_account_ids = ";
    Algs::print(
      debug_info_str_,
      request_info.publisher_account_ids.begin(),
      request_info.publisher_account_ids.end());

    debug_info_str_ << sep_ <<
      "test_request = " << request_params.common_info.test_request << sep_ <<
      "log_as_test = " << request_params.common_info.log_as_test << sep_ <<
      "location = ";

    if(request_info.location.in())
    {
      debug_info_str_ << request_info.location->country << "/" <<
        request_info.location->region << "/" <<
        request_info.location->city;
    }

    debug_info_str_ << sep_ <<
      "referer = " << request_params.common_info.referer << sep_ <<
      "full_referer = " << request_params.common_info.full_referer << sep_ <<
      "uid = " << (user_id.is_null() ? "" : user_id.to_string()) << sep_ <<
      "external_user_id = " << request_params.common_info.external_user_id << sep_ <<
      "ip = " << request_params.common_info.peer_ip << sep_ <<
      "user_agent = " << request_params.common_info.user_agent << sep_ <<
      "search-phrase = " << request_params.search_words << sep_ <<
      "channel_keywords = " << normalized_channel_keywords << sep_ <<
      "search_engine_id = " << request_params.search_engine_id << sep_ <<
      "filter_request = " << (request_info.filter_request ? "true" : "false") << sep_ <<
      "passback_url = " << request_params.common_info.passback_url << sep_ <<
      "format = " << request_info.format << sep_ <<
      "seat = " << request_info.seat << sep_ <<
      "app = " << (request_info.is_app ? "true" : "false") << sep_ <<
      "application_id = " << request_info.application_id << sep_ <<
      "advertising_id = " << request_info.advertising_id << sep_ <<
      "idfa = " << request_info.idfa << sep_ <<
      "ssp_devicetype = " << request_info.ssp_devicetype_str << sep_ <<
      "ssp_video_placementtype = " <<
        request_info.ssp_video_placementtype_str << sep_ <<
      "browser = " << request_params.context_info.web_browser << sep_ <<
      "platform = " << request_params.context_info.platform << sep_ <<
      "full_platform = " << request_params.context_info.full_platform << sep_ <<
      "platform_ids = ";
    Algs::print(
      debug_info_str_,
      request_params.context_info.platform_ids.get_buffer(),
      request_params.context_info.platform_ids.get_buffer() +
        request_params.context_info.platform_ids.length());
    debug_info_str_ << sep_ << "ad_slots = " <<
      request_params.ad_slots.length() << sep_;
  }

  void
  DebugSink::print_user_resolving_debug_info(
    const UserResolvingDebugInfo& user_resolving_debug_info) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::USER_RESOLVING_HEAD << "\n";
    }
    else
    {
      debug_info_str_ << Debug::USER_RESOLVING_HEAD << sep_;
    }

    debug_info_str_ << "user_id = ";
    if(user_resolving_debug_info.response_present)
    {
      debug_info_str_ << user_resolving_debug_info.user_id;
    }

    debug_info_str_ << sep_ << "min_age_reached = ";
    if(user_resolving_debug_info.response_present)
    {
      debug_info_str_ <<
        (user_resolving_debug_info.min_age_reached ? "true" : "false");
    }

    debug_info_str_ << sep_ << "created = ";
    if(user_resolving_debug_info.response_present)
    {
      debug_info_str_ <<
        (user_resolving_debug_info.created ? "true" : "false");
    }

    debug_info_str_ << sep_ << "invalid_operation = ";
    if(user_resolving_debug_info.response_present)
    {
      debug_info_str_ <<
        (user_resolving_debug_info.invalid_operation ? "true" : "false");
    }

    debug_info_str_ << sep_ << "user_found = ";
    if(user_resolving_debug_info.response_present)
    {
      debug_info_str_ <<
        (user_resolving_debug_info.user_found ? "true" : "false");
    }

    debug_info_str_ << sep_;
  }

  void
  DebugSink::print_creative_selection_debug_info(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    bool ad_selected = false;
    const AdServer::Bidding::CampaignManager::AdSlotDebugInfo*
      expected_debug_info = nullptr;
    for(std::size_t i = 0; i < campaign_match_result.ad_slots.length(); ++i)
    {
      const auto& ad_slot_result = campaign_match_result.ad_slots[i];
      if(!expected_debug_info && ad_slot_result.debug_info.trace_ccg[0] != 0)
      {
        expected_debug_info = &ad_slot_result.debug_info;
      }

      if(ad_slot_result.selected_creatives.length() > 0)
      {
        ad_selected = true;
        print_creative_selection_debug_info_(ad_slot_result);
      }
    }

    if(!ad_selected)
    {
      print_empty_creative_selection_debug_info_();
    }

    if(expected_debug_info)
    {
      print_expected_debug_info_(*expected_debug_info);
    }
  }

  void
  DebugSink::print_channel_matching_debug_info(
    const adserver::channel_svcs::channel_server::MatchResponse&
      response) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::CHANNEL_MATCHING_HEAD << "\n";
    }
    else
    {
      debug_info_str_ << Debug::CHANNEL_MATCHING_HEAD << sep_;
    }

    ChannelIdSet ids;
    const auto& match_result = response.matched_channels();
    add_channel_ids_(ids, match_result.url_channels());
    add_channel_ids_(ids, match_result.url_keyword_channels());
    std::copy(
      match_result.uid_channels().begin(),
      match_result.uid_channels().end(),
      std::inserter(ids, ids.end()));

    debug_info_str_ <<
      "special_channels_effects = " <<
        (response.no_track() ? "NO TRACK" : "TRACK") << ", " <<
        (response.no_adv() ? "NO ADV" : "ADV") << sep_ <<
      "triggers = ";

    bool print_delimiter = false;
    print_channel_atom_seq_(
      debug_info_str_,
      match_result.page_channels(),
      'P',
      print_delimiter);
    print_channel_atom_seq_(
      debug_info_str_,
      match_result.search_channels(),
      'S',
      print_delimiter);
    print_channel_atom_seq_(
      debug_info_str_,
      match_result.url_channels(),
      'U',
      print_delimiter);
    print_channel_atom_seq_(
      debug_info_str_,
      match_result.url_keyword_channels(),
      'R',
      print_delimiter);

    debug_info_str_ << sep_ << "trigger_channels = ";

    print_delimiter = false;
    ids.clear();
    add_channel_ids_(ids, match_result.page_channels());
    print_channel_ids_(debug_info_str_, ids, 'P', print_delimiter);
    ids.clear();
    add_channel_ids_(ids, match_result.search_channels());
    print_channel_ids_(debug_info_str_, ids, 'S', print_delimiter);
    ids.clear();
    add_channel_ids_(ids, match_result.url_channels());
    print_channel_ids_(debug_info_str_, ids, 'U', print_delimiter);
    ids.clear();
    add_channel_ids_(ids, match_result.url_keyword_channels());
    print_channel_ids_(debug_info_str_, ids, 'R', print_delimiter);
    ids.clear();
    std::copy(
      match_result.uid_channels().begin(),
      match_result.uid_channels().end(),
      std::inserter(ids, ids.end()));
    print_channel_ids_(debug_info_str_, ids, 'A', print_delimiter);

    debug_info_str_ << sep_ << "content_channels = ";
    for(int i = 0; i < response.content_channels_size(); ++i)
    {
      if(i != 0)
      {
        debug_info_str_ << ", ";
      }
      debug_info_str_ << response.content_channels(i).id() << "(" <<
        response.content_channels(i).weight() << ")";
    }

    debug_info_str_ << sep_;
  }

  void
  DebugSink::print_history_matching_debug_info(
    const adserver::user_info_svcs::user_info_manager::MatchResult&
      match_result) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::HISTORY_MATCHING_HEAD << "\n";
    }
    else
    {
      debug_info_str_ << Debug::HISTORY_MATCHING_HEAD << sep_;
    }

    debug_info_str_ << "last_request_time = ";
    try
    {
      debug_info_str_ << GrpcAlgs::unpack_time(
        match_result.last_request_time()).gm_ft() << sep_ <<
        "create_time = " << GrpcAlgs::unpack_time(
          match_result.create_time()).gm_ft() << sep_ <<
        "session_start = " << GrpcAlgs::unpack_time(
          match_result.session_start()).gm_ft();
    }
    catch(...)
    {
      debug_info_str_ << "invalid time";
    }

    debug_info_str_ << sep_ <<
      "history_channels = ";

    for(int i = 0; i < match_result.channels_size(); ++i)
    {
      if(i != 0)
      {
        debug_info_str_ << ",";
      }
      debug_info_str_ << match_result.channels(i).channel_id();
    }

    debug_info_str_ << sep_;
  }

  void
  DebugSink::print_time_metering_debug_info(
    const RequestTimeMetering& time_metering) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::TIME_METERING_HEAD << "\n";
    }
    else
    {
      debug_info_str_ << Debug::TIME_METERING_HEAD << sep_;
    }

    debug_info_str_ <<
      "total_time = " <<
        time_metering.total_time.float_str() << sep_ <<
      "user_resolving_started_at = " <<
        time_metering.user_resolving_started_at.float_str() << sep_ <<
      "user_resolving_time = " <<
        time_metering.user_resolving_time.float_str() << sep_ <<
      "trigger_match_started_at = " <<
        time_metering.trigger_match_started_at.float_str() << sep_ <<
      "trigger_match_time = " <<
        time_metering.trigger_match_time.float_str() << sep_ <<
      "history_match_started_at = " <<
        time_metering.history_match_started_at.float_str() << sep_ <<
      "history_match_time = " <<
        time_metering.history_match_time.float_str() << " : " <<
        time_metering.history_match_local_time.float_str();
    if(time_metering.history_match_error)
    {
      debug_info_str_ << " => error";
    }
    debug_info_str_ << sep_ <<
      "creative_selection_started_at = " <<
        time_metering.creative_selection_started_at.float_str() << sep_ <<
      "creative_selection_time = " <<
        time_metering.creative_selection_time.float_str() << " : " <<
        time_metering.creative_selection_local_time.float_str();
    if(time_metering.creative_selection_error)
    {
      debug_info_str_ << " => error";
    }
    debug_info_str_ << sep_;
  }

  void
  DebugSink::print_interrupt_debug_info(
    const String::SubString& interrupted_step) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::INTERRUPT_HEAD << "\n";
    }
    else
    {
      debug_info_str_ << Debug::INTERRUPT_HEAD << sep_;
    }

    debug_info_str_ << "interrupted_step = " << interrupted_step << sep_;
  }

  DebugInfo
  DebugSink::parse_require_debug_info_(
    const String::SubString& require_debug_info) noexcept
  {
    String::AsciiStringManip::Caseless value("");
    value.str.assign(require_debug_info.data(), require_debug_info.size());

    if(value == String::SubString("header"))
    {
      return DI_HEADER;
    }
    else if(value == String::SubString("body"))
    {
      return DI_BODY;
    }
    else
    {
      return DI_NONE;
    }
  }

  void
  DebugSink::write_response(
    FCGI::HttpResponse_var& response,
    int& http_status,
    const AdServer::Commons::UserId& user_id) const noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    const std::string debug_info = make_debug_info_(user_id);

    try
    {
      if(require_debug_info_ == DI_HEADER)
      {
        response->add_header_nocopy_name(
          Response::Header::DEBUG_INFO,
          debug_info);
      }
      else if(require_debug_info_ == DI_BODY)
      {
        FCGI::HttpResponse_var debug_response(new FCGI::HttpResponse());
        debug_response->set_content_type_nocopy(Response::Type::TEXT_PLAIN);
        debug_response->write(String::SubString(debug_info));
        debug_response->write(String::SubString("\n"));

        response = debug_response;
        http_status = 200;
      }
    }
    catch(...)
    {}
  }

  std::string
  DebugSink::make_debug_info_(
    const AdServer::Commons::UserId& user_id) const
  {
    std::ostringstream out;
    if(!debug_info_str_.str().empty())
    {
      out << debug_info_str_.str();
    }
    else
    {
      out << "server-id = " << server_id_ << sep_ <<
        "uid = " << (user_id.is_null() ? "" : user_id.to_string());
    }
    return out.str();
  }

  void
  DebugSink::print_empty_creative_selection_debug_info_() noexcept
  {
    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::CREATIVE_SELECTION_INFO_HEAD << "\n";
    }

    debug_info_str_ <<
      "ccid = 0" << sep_ <<
      "cmpid = 0" << sep_ <<
      "creative_size_id = 0" << sep_ <<
      "mime_format = " << sep_ <<
      "tag_id = 0" << sep_ <<
      "site_id = 0" << sep_ <<
      "site_rate_id = 0" << sep_;
  }

  void
  DebugSink::print_expected_debug_info_(
    const AdServer::Bidding::CampaignManager::AdSlotDebugInfo&
      debug_info) noexcept
  {
    if(require_debug_info_ == DI_BODY && debug_info.trace_ccg[0] != 0)
    {
      debug_info_str_ << "\n" << Debug::TRACE_CCG_INFO_HEAD << "\n" <<
        debug_info.trace_ccg << "\n";
    }
  }

  void
  DebugSink::print_creative_selection_debug_info_(
    const AdServer::Bidding::CampaignManager::AdSlotResult&
      ad_slot_result) noexcept
  {
    const auto& selected_creatives = ad_slot_result.selected_creatives;
    const auto& debug_info = ad_slot_result.debug_info;
    const auto& debug_selected_creatives = debug_info.selected_creatives;

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::CREATIVE_SELECTION_INFO_HEAD << "\n";
    }

    unsigned long first_ccid = 0;
    unsigned long first_cmp_id = 0;
    if(selected_creatives.length() != 0)
    {
      first_ccid = selected_creatives[0].ccid;
      first_cmp_id = selected_creatives[0].cmp_id;
    }

    CampaignSvcs::RevenueDecimal imp_revenue(
      CampaignSvcs::RevenueDecimal::ZERO);
    CampaignSvcs::RevenueDecimal click_revenue(
      CampaignSvcs::RevenueDecimal::ZERO);
    CampaignSvcs::RevenueDecimal action_revenue(
      CampaignSvcs::RevenueDecimal::ZERO);

    for(std::size_t i = 0; i < debug_selected_creatives.length(); ++i)
    {
      imp_revenue += CampaignManager::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          debug_selected_creatives[i].imp_revenue);
      click_revenue += CampaignManager::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          debug_selected_creatives[i].click_revenue);
      action_revenue += CampaignManager::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          debug_selected_creatives[i].action_revenue);
    }

    debug_info_str_ <<
      "ad_slot_id = " << ad_slot_result.ad_slot_id << sep_ <<
      "ccid = " << first_ccid << sep_ <<
      "cmpid = " << first_cmp_id << sep_ <<
      "creative_size_id = " << debug_info.tag_size_id << sep_ <<
      "mime_format = " << ad_slot_result.mime_format << sep_ <<
      "tag_id = " << debug_info.tag_id << sep_ <<
      "site_id = " << debug_info.site_id << sep_ <<
      "site_rate_id = " << debug_info.site_rate_id << sep_ <<
      "imp_revenue = " << imp_revenue << sep_ <<
      "click_revenue = " << click_revenue << sep_ <<
      "action_revenue = " << action_revenue << sep_ <<
      "min_no_adv_ecpm = " << debug_info.min_no_adv_ecpm << sep_ <<
      "min_text_ecpm = " << debug_info.min_text_ecpm << sep_ <<
      "test_request = " << ad_slot_result.test_request << sep_ <<
      "passback_url = " << ad_slot_result.passback_url << sep_ <<
      "creative_url = " << ad_slot_result.creative_url << sep_ <<
      "notice_url = " << ad_slot_result.notice_url << sep_ <<
      "track_pixel_url = " << debug_info.track_pixel_url << sep_ <<
      "cpm_threshold = " <<
        CampaignManager::unpack_decimal<CampaignSvcs::RevenueDecimal>(
          debug_info.cpm_threshold) << sep_ <<
      "walled_garden = " << debug_info.walled_garden << sep_ <<
      "auction_type = " << auction_type_to_string(debug_info.auction_type) <<
      sep_ << "selected_creatives = ";

    const std::size_t debug_count = debug_selected_creatives.length();
    for(std::size_t i = 0; i < selected_creatives.length(); ++i)
    {
      const auto& creative = selected_creatives[i];
      const auto* debug_creative = i < debug_count ?
        &debug_selected_creatives[i] : nullptr;
      const String::SubString offset(require_debug_info_ == DI_BODY ? "  " : "");
      const String::SubString creative_start_sep(
        require_debug_info_ == DI_BODY ? "\n------\n" : "( ");
      const char* creative_end_sep = require_debug_info_ == DI_BODY ? "" : ") ";

      debug_info_str_ << creative_start_sep <<
        offset << "request_id = " << CampaignManager::unpack_request_id(
          creative.request_id) << sep_ <<
        offset << "ccid = " << creative.ccid << sep_ <<
        offset << "cmp_id = " << creative.cmp_id << sep_ <<
        offset << "campaign_group_id = " << creative.campaign_group_id << sep_ <<
        offset << "order_set_id = " << creative.order_set_id << sep_ <<
        offset << "advertiser_id = " << creative.advertiser_id << sep_ <<
        offset << "advertiser_name = " << creative.advertiser_name << sep_ <<
        offset << "creative_id = " << creative.creative_id << sep_ <<
        offset << "creative_version_id = " << creative.creative_version_id << sep_ <<
        offset << "creative_size = " << creative.creative_size << sep_ <<
        offset << "triggered_expression = " <<
          (debug_creative ? debug_creative->triggered_expression.in() : "") << sep_ <<
        offset << "ecpm = " << CampaignManager::unpack_decimal<
          CampaignSvcs::RevenueDecimal>(creative.ecpm) << sep_ <<
        offset << "pub_ecpm = " << CampaignManager::unpack_decimal<
          CampaignSvcs::RevenueDecimal>(creative.pub_ecpm) << sep_ <<
        offset << "ecpm_bid = " <<
          (debug_creative ? CampaignManager::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->ecpm_bid).str() : "") << sep_ <<
        offset << "click_url = " << creative.click_url << sep_ <<
        offset << "destination_url = " << creative.destination_url << sep_ <<
        offset << "html_url = " <<
          (debug_creative ? debug_creative->html_url.in() : "") << sep_ <<
        offset << "action_adv_url = " <<
          (debug_creative ? debug_creative->action_adv_url.in() : "") << sep_ <<
        offset << "revenue = " <<
          CampaignManager::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            creative.revenue) << sep_ <<
        offset << "imp_revenue = " <<
          (debug_creative ? CampaignManager::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->imp_revenue).str() : "") << sep_ <<
        offset << "click_revenue = " <<
          (debug_creative ? CampaignManager::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->click_revenue).str() : "") << sep_ <<
        offset << "action_revenue = " <<
          (debug_creative ? CampaignManager::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->action_revenue).str() : "");
      debug_info_str_ << creative_end_sep;
    }

    debug_info_str_ << sep_;
  }
}
}
