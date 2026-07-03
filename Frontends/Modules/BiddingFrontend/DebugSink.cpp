#include "DebugSink.hpp"

#include <algorithm>
#include <iterator>
#include <set>
#include <utility>

#include <String/AsciiStringManip.hpp>
#include <Commons/Algs.hpp>
#include <Commons/GrpcAlgs.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer::Bidding
{
  namespace
  {
    namespace Response::Header
    {
      const std::string DEBUG_INFO("Debug-Info");
    }

    namespace Response::Type
    {
      const std::string TEXT_PLAIN("text/plain");
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

    std::uint64_t
    outstanding_items_(const AdServer::Grpc::Stats& stats) noexcept
    {
      return stats.input_items >= stats.completed_items ?
        stats.input_items - stats.completed_items : 0;
    }

    template<typename Writer>
    void
    print_client_stats_(
      Writer& out,
      const char* prefix,
      const AdServer::Grpc::Stats& stats,
      const char* sep)
    {
      out <<
        prefix << "_input_items = " <<
          stats.input_items << sep <<
        prefix << "_completed_items = " <<
          stats.completed_items << sep <<
        prefix << "_completed_error_items = " <<
          stats.completed_error_items << sep <<
        prefix << "_outstanding_items = " <<
          outstanding_items_(stats) << sep <<
        prefix << "_write_batches = " <<
          stats.write_batches << sep <<
        prefix << "_write_items = " <<
          stats.write_items << sep <<
        prefix << "_read_batches = " <<
          stats.read_batches << sep <<
        prefix << "_read_items = " <<
          stats.read_items << sep <<
        prefix << "_queue_items = " <<
          stats.queue_items << sep <<
        prefix << "_pending_batches = " <<
          stats.pending_batches << sep <<
        prefix << "_pending_batch_items = " <<
          stats.pending_batch_items << sep <<
        prefix << "_inflight_items = " <<
          stats.inflight_items << sep <<
        prefix << "_stream_inflight_items = " <<
          stats.stream_inflight_items << sep <<
        prefix << "_active_streams = " <<
          stats.active_streams << sep <<
        prefix << "_available_streams = " <<
          stats.available_streams << sep <<
        prefix << "_connecting_streams = " <<
          stats.connecting_streams << sep <<
        prefix << "_draining_streams = " <<
          stats.draining_streams << sep <<
        prefix << "_deferred_streams = " <<
          stats.deferred_streams << sep;
      if (stats.last_error.has_value())
      {
        out <<
          prefix << "_last_error_time = " <<
            stats.last_error->time.get_gm_time().format("%F %T") << sep <<
          prefix << "_last_error_endpoint = " <<
            stats.last_error->endpoint << sep <<
          prefix << "_last_error_code = " <<
            stats.last_error->code << sep <<
          prefix << "_last_error_message = " <<
            stats.last_error->message << sep <<
          prefix << "_last_error_source = " <<
            stats.last_error->source << sep;
      }
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

    template<typename Writer>
    void
    print_channel_ids_(
      Writer& out,
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

    template<typename Writer>
    void
    print_channel_atom_seq_(
      Writer& out,
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

    template<typename Writer, typename Iterator>
    void
    print_sequence_(
      Writer& out,
      Iterator begin,
      Iterator end)
    {
      if(begin == end)
      {
        return;
      }

      out << *begin;
      for(++begin; begin != end; ++begin)
      {
        out << "," << *begin;
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
    normalize_debug_field_(std::string_view value)
    {
      std::string result(value);
      std::replace(result.begin(), result.end(), '\r', ' ');
      std::replace(result.begin(), result.end(), '\n', ' ');
      return result;
    }

    std::string
    normalize_debug_field_(std::string value)
    {
      std::replace(value.begin(), value.end(), '\r', ' ');
      std::replace(value.begin(), value.end(), '\n', ' ');
      return value;
    }

    const std::string&
    stage_error_source_to_string_(const StageResult::Error::Source source)
      noexcept
    {
      static const std::string grpc = "grpc";
      static const std::string exception = "exception";
      static const std::string channel_server = "channel_server";
      static const std::string unknown = "unknown";

      switch(source)
      {
      case StageResult::Error::Source::Grpc:
        return grpc;
      case StageResult::Error::Source::Exception:
        return exception;
      case StageResult::Error::Source::ChannelServer:
        return channel_server;
      }

      return unknown;
    }

    std::string
    extract_grpc_endpoint_(const std::string& message)
    {
      static const std::string marker = "[grpc_endpoint=";

      const auto begin = message.rfind(marker);
      if(begin == std::string::npos)
      {
        return {};
      }

      const auto endpoint_begin = begin + marker.size();
      const auto endpoint_end = message.find(']', endpoint_begin);
      if(endpoint_end == std::string::npos)
      {
        return {};
      }

      return message.substr(endpoint_begin, endpoint_end - endpoint_begin);
    }

    template<typename Writer>
    void
    print_stage_server_id_(
      Writer& out,
      const char* name,
      const StageResult* stage,
      const char* sep)
    {
      if(stage && !stage->server_id.empty())
      {
        out << name << "_server_id = " << stage->server_id << sep;
      }
    }

    template<typename Writer>
    void
    print_stage_time_(
      Writer& out,
      const char* name,
      const std::optional<StageResult>& stage,
      const char* sep)
    {
      out << name << "_started_at = ";
      if(!stage)
      {
        out << "not started" << sep <<
          name << "_time = not started" << sep;
        return;
      }

      out << stage->started_at.float_str() << sep <<
        name << "_time = ";
      if(stage->finished_at)
      {
        out << (*stage->finished_at - stage->started_at).float_str();
      }
      else
      {
        out << "incomplete";
      }

      if(stage->local_time)
      {
        out << " : " << stage->local_time->float_str();
      }

      if(stage->error)
      {
        out << " => error";
      }
      out << sep;

      if(stage->error)
      {
        out << name << "_error_source = " <<
          stage_error_source_to_string_(stage->error->source) << sep <<
          name << "_error_code = " << stage->error->code << sep;
        if(!stage->error->endpoint.empty())
        {
          out << name << "_error_endpoint = " <<
            stage->error->endpoint << sep;
        }
        out <<
          name << "_error_message = " <<
            normalize_debug_field_(stage->error->message) << sep;
      }
    }
  }

  DebugSink::DebugSink(std::string server_id)
    : server_id_(std::move(server_id))
  {}

  void
  StageResult::finish(const Generics::Time& request_started_at)
  {
    finished_at = Generics::Time::get_time_of_day() - request_started_at;
  }

  void
  StageResult::set_grpc_error(
    const grpc::Status& status,
    std::string endpoint)
  {
    if(endpoint.empty())
    {
      endpoint = extract_grpc_endpoint_(status.error_message());
    }

    error = Error{
      Error::Source::Grpc,
      static_cast<int>(status.error_code()),
      status.error_message(),
      std::move(endpoint)};
  }

  void
  StageResult::set_exception_error(const eh::Exception& ex)
  {
    error = Error{
      Error::Source::Exception,
      0,
      ex.what(),
      std::string()};
  }

  void
  StageResult::set_channel_server_error(std::string message)
  {
    error = Error{
      Error::Source::ChannelServer,
      0,
      std::move(message),
      std::string()};
  }

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
    const AdServer::Commons::UserId& user_id,
    std::string_view channel_keywords) noexcept
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
        user_status_to_string(request_info.user_status) << sep_ <<
      "source_id = " << request_info.source_id << sep_ <<
      "request_id = " << request_info.request_id << sep_ <<
      "bid_request_id = " << request_info.bid_request_id << sep_ <<
      "bid_site_id = " << request_info.bid_site_id << sep_ <<
      "bid_publisher_id = " << request_info.bid_publisher_id << sep_ <<
      "publisher_site_id = " << request_info.publisher_site_id << sep_ <<
      "publisher_account_ids = ";
    print_sequence_(
      debug_info_str_,
      request_info.publisher_account_ids.begin(),
      request_info.publisher_account_ids.end());

    debug_info_str_ << sep_ <<
      "test_request = " << request_info.test_request << sep_ <<
      "log_as_test = " << request_info.log_as_test << sep_ <<
      "location = ";

    if(request_info.location)
    {
      debug_info_str_ << request_info.location->country << "/" <<
        request_info.location->region << "/" <<
        request_info.location->city;
    }

    debug_info_str_ << sep_ <<
      "referer = " << request_info.referer << sep_ <<
      "full_referer = " << request_info.full_referer << sep_ <<
      "uid = " << (user_id.is_null() ? "" : user_id.to_string()) << sep_ <<
      "external_user_id = " << request_info.external_user_id << sep_ <<
      "ip = " << request_info.peer_ip << sep_ <<
      "user_agent = " << request_info.user_agent << sep_ <<
      "search-phrase = " << request_info.search_words << sep_ <<
      "channel_keywords = " << normalized_channel_keywords << sep_ <<
      "search_engine_id = " << request_info.search_engine_id << sep_ <<
      "filter_request = " << (request_info.filter_request ? "true" : "false") << sep_ <<
      "passback_url = " << request_info.passback_url << sep_ <<
      "format = " << request_info.format << sep_ <<
      "seat = " << request_info.seat << sep_ <<
      "app = " << (request_info.is_app ? "true" : "false") << sep_ <<
      "application_id = " << request_info.application_id << sep_ <<
      "advertising_id = " << request_info.advertising_id << sep_ <<
      "idfa = " << request_info.idfa << sep_ <<
      "ssp_devicetype = " << request_info.ssp_devicetype_str << sep_ <<
      "ssp_video_placementtype = " <<
        request_info.ssp_video_placementtype_str << sep_ <<
      "browser = " << request_info.web_browser << sep_ <<
      "platform = " << request_info.platform << sep_ <<
      "full_platform = " << request_info.full_platform << sep_ <<
      "platform_ids = ";
    print_sequence_(
      debug_info_str_,
      request_info.platform_ids.begin(),
      request_info.platform_ids.end());
    debug_info_str_ << sep_ << "ad_slots = " <<
      request_info.ad_slots.size() << sep_;
  }

  void
  DebugSink::print_user_resolving_debug_info(
    const UserResolvingDebugInfo& user_resolving_debug_info,
    const StageResult* stage) noexcept
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

    print_stage_server_id_(
      debug_info_str_,
      "user_resolving",
      stage,
      sep_);

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
      campaign_match_result,
    const StageResult* stage) noexcept
  {
    if(!require_debug_info())
    {
      return;
    }

    bool ad_selected = false;
    const AdServer::Bidding::CampaignManager::AdSlotDebugInfo*
      expected_debug_info = nullptr;
    for(std::size_t i = 0; i < campaign_match_result.ad_slots.size(); ++i)
    {
      const auto& ad_slot_result = campaign_match_result.ad_slots[i];
      if(!expected_debug_info && ad_slot_result.debug_info.trace_ccg[0] != 0)
      {
        expected_debug_info = &ad_slot_result.debug_info;
      }

      if(ad_slot_result.selected_creatives.size() > 0)
      {
        ad_selected = true;
        print_creative_selection_debug_info_(ad_slot_result, stage);
      }
    }

    if(!ad_selected)
    {
      print_empty_creative_selection_debug_info_(stage);
    }

    if(expected_debug_info)
    {
      print_expected_debug_info_(*expected_debug_info);
    }
  }

  void
  DebugSink::print_channel_matching_debug_info(
    const adserver::channel_svcs::channel_server::MatchResponse&
      response,
    const StageResult* stage) noexcept
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

    print_stage_server_id_(
      debug_info_str_,
      "trigger_match",
      stage,
      sep_);

    ChannelIdSet ids;
    const auto& match_result = response.matched_channels();
    add_channel_ids_(ids, match_result.url_channels());
    add_channel_ids_(ids, match_result.url_keyword_channels());
    std::copy(
      match_result.uid_channels().begin(),
      match_result.uid_channels().end(),
      std::inserter(ids, ids.end()));

    debug_info_str_ <<
      "full_loaded = " << (response.full_loaded() ? "true" : "false") <<
        sep_ <<
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
      match_result,
    const StageResult* stage) noexcept
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

    print_stage_server_id_(
      debug_info_str_,
      "history_match",
      stage,
      sep_);

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
        time_metering.total_time.float_str() << sep_;
    print_stage_time_(
      debug_info_str_,
      "user_resolving",
      time_metering.user_resolving,
      sep_);
    print_stage_time_(
      debug_info_str_,
      "trigger_match",
      time_metering.trigger_match,
      sep_);
    print_stage_time_(
      debug_info_str_,
      "history_match",
      time_metering.history_match,
      sep_);
    print_stage_time_(
      debug_info_str_,
      "creative_selection",
      time_metering.creative_selection,
      sep_);
  }

  void
  DebugSink::print_interrupt_debug_info(
    const String::SubString& interrupted_step,
    const std::string& user_info_client_endpoint,
    unsigned long request_in_progress,
    unsigned long user_resolving_in_progress,
    unsigned long trigger_match_in_progress,
    unsigned long history_match_in_progress,
    unsigned long campaign_selection_in_progress,
    unsigned long history_post_match_in_progress,
    const AdServer::Grpc::Stats& user_bind_client_stats,
    const AdServer::Grpc::Stats& user_info_client_stats,
    const AdServer::Grpc::Stats& channel_client_stats,
    const AdServer::Grpc::Stats& campaign_client_stats) noexcept
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

    debug_info_str_ <<
      "server-id = " << server_id_ << sep_ <<
      "user_info_client_endpoint = " <<
        user_info_client_endpoint << sep_ <<
      "rtb_request_in_progress = " << request_in_progress << sep_ <<
      "rtb_request_user_resolving_in_progress = " <<
        user_resolving_in_progress << sep_ <<
      "rtb_request_trigger_match_in_progress = " <<
        trigger_match_in_progress << sep_ <<
      "rtb_request_history_match_in_progress = " <<
        history_match_in_progress << sep_ <<
      "rtb_request_campaign_selection_in_progress = " <<
        campaign_selection_in_progress << sep_ <<
      "rtb_request_history_post_match_in_progress = " <<
        history_post_match_in_progress << sep_;

    print_client_stats_(
      debug_info_str_,
      "user_bind_client",
      user_bind_client_stats,
      sep_);
    print_client_stats_(
      debug_info_str_,
      "user_info_client",
      user_info_client_stats,
      sep_);
    print_client_stats_(
      debug_info_str_,
      "channel_client",
      channel_client_stats,
      sep_);
    print_client_stats_(
      debug_info_str_,
      "campaign_client",
      campaign_client_stats,
      sep_);

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
    if(!debug_info_str_.empty())
    {
      return debug_info_str_.str();
    }

    std::ostringstream out;
    out << "server-id = " << server_id_ << sep_ <<
      "uid = " << (user_id.is_null() ? "" : user_id.to_string());
    return out.str();
  }

  void
  DebugSink::print_empty_creative_selection_debug_info_(
    const StageResult* stage) noexcept
  {

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::CREATIVE_SELECTION_INFO_HEAD << "\n";
    }

    print_stage_server_id_(
      debug_info_str_,
      "creative_selection",
      stage,
      sep_);

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
      ad_slot_result,
    const StageResult* stage) noexcept
  {

    const auto& selected_creatives = ad_slot_result.selected_creatives;
    const auto& debug_info = ad_slot_result.debug_info;
    const auto& debug_selected_creatives = debug_info.selected_creatives;

    if(require_debug_info_ == DI_BODY)
    {
      debug_info_str_ << "\n" << Debug::CREATIVE_SELECTION_INFO_HEAD << "\n";
    }

    print_stage_server_id_(
      debug_info_str_,
      "creative_selection",
      stage,
      sep_);

    unsigned long first_ccid = 0;
    unsigned long first_cmp_id = 0;
    if(selected_creatives.size() != 0)
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

    for(std::size_t i = 0; i < debug_selected_creatives.size(); ++i)
    {
      imp_revenue += GrpcAlgs::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          debug_selected_creatives[i].imp_revenue);
      click_revenue += GrpcAlgs::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          debug_selected_creatives[i].click_revenue);
      action_revenue += GrpcAlgs::unpack_decimal<
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
        GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
          debug_info.cpm_threshold) << sep_ <<
      "walled_garden = " << debug_info.walled_garden << sep_ <<
      "auction_type = " << auction_type_to_string(debug_info.auction_type) <<
      sep_ << "selected_creatives = ";

    const std::size_t debug_count = debug_selected_creatives.size();
    for(std::size_t i = 0; i < selected_creatives.size(); ++i)
    {
      const auto& creative = selected_creatives[i];
      const auto* debug_creative = i < debug_count ?
        &debug_selected_creatives[i] : nullptr;
      const std::string offset(require_debug_info_ == DI_BODY ? "  " : "");
      const std::string creative_start_sep(
        require_debug_info_ == DI_BODY ? "\n------\n" : "( ");
      const char* creative_end_sep = require_debug_info_ == DI_BODY ? "" : ") ";

      debug_info_str_ << creative_start_sep <<
        offset << "request_id = " << GrpcAlgs::unpack_request_id(
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
        offset << "ecpm = " << GrpcAlgs::unpack_decimal<
          CampaignSvcs::RevenueDecimal>(creative.ecpm) << sep_ <<
        offset << "pub_ecpm = " << GrpcAlgs::unpack_decimal<
          CampaignSvcs::RevenueDecimal>(creative.pub_ecpm) << sep_ <<
        offset << "ecpm_bid = " <<
          (debug_creative ? GrpcAlgs::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->ecpm_bid).str() : "") << sep_ <<
        offset << "click_url = " << creative.click_url << sep_ <<
        offset << "destination_url = " << creative.destination_url << sep_ <<
        offset << "html_url = " <<
          (debug_creative ? debug_creative->html_url : "") << sep_ <<
        offset << "revenue = " <<
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            creative.revenue) << sep_ <<
        offset << "imp_revenue = " <<
          (debug_creative ? GrpcAlgs::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->imp_revenue).str() : "") << sep_ <<
        offset << "click_revenue = " <<
          (debug_creative ? GrpcAlgs::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->click_revenue).str() : "") << sep_ <<
        offset << "action_revenue = " <<
          (debug_creative ? GrpcAlgs::unpack_decimal<
            CampaignSvcs::RevenueDecimal>(debug_creative->action_revenue).str() : "");
      debug_info_str_ << creative_end_sep;
    }

    debug_info_str_ << sep_;
  }
}
