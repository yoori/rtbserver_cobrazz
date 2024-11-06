// THIS
#include <Commons/GrpcAlgs.hpp>
#include <Frontends/Modules/AdFrontendGrpc/DebugSink.hpp>

namespace AdServer::Grpc
{
  namespace Debug
  {
    const char ERROR_HEAD[] = "=== ERROR ===";
    const char REQUEST_INFO_HEAD[] = "=== Request params ===";
    const char CHANNEL_MATCHING_HEAD[] = "=== Channel matching result ===";
    const char HISTORY_MATCHING_HEAD[] = "=== History matching result ===";
    const char CREATIVE_SELECTION_INFO_HEAD[] = "=== Creative selection ===";
    const char TRACE_CCG_INFO_HEAD[] = "=== Expected ===";
    const char TIME_METERING_HEAD[] = "=== Time Metering ===";
  } // namespace Debug

  namespace
  {
    struct GetChannelId
    {
      std::uint32_t operator() (
        const AdServer::ChannelSvcs::Proto::ChannelAtom& atom) noexcept
      {
        return atom.id();
      }
    };
  }

  DebugSink::DebugSink(const bool allow_show_history_profile) noexcept
    : allow_show_history_profile_(allow_show_history_profile)
  {
  }

  void DebugSink::write_response(
    HttpResponse& response,
    std::string& str_response,
    int& http_status) const
  {
    if (require_debug_info())
    {
      DebugStream ostr;

      if (require_debug_body())
      {
        ostr << "<html>\n<head>\n"
          "<meta http-equiv=\"Content-Type\" "
          "content=\"text/html; charset=utf-8\"/>\n</head>\n<body>\n<pre>\n";
      }

      std::string res;

      String::StringManip::replace(
        debug_info_str_.str(),
        res,
        String::SubString("<"),
        String::SubString("&lt;"));

      ostr << res;

      if(require_debug_body())
      {
        ostr << "</pre>\n</body>\n</html>\n";

        response.set_content_type(Response::Type::TEXT_HTML);

        str_response = ostr.str();
        http_status = 200;
      }
      else
      {
        response.add_header(
          Response::Header::DEBUG_INFO,
          ostr.str());
      }
    }
  }

  void DebugSink::fill_debug_body_(
    HttpResponse& response,
    const String::SubString& error_description) noexcept
  {
    try
    {
      std::string res_error;

      String::StringManip::replace(
        error_description,
        res_error,
        String::SubString("<"),
        String::SubString("&lt;"));

      std::ostringstream debug_content;
      debug_content <<
        "<html>\n<head>\n"
        "<meta http-equiv=\"Content-Type\" "
        "content=\"text/html; charset=utf-8\"/>\n</head>\n<body>\n<pre>\n\n" <<
        Debug::ERROR_HEAD << "\n" << res_error <<
        "</pre>\n</body>\n</html>\n";

      response.set_content_type(Response::Type::TEXT_HTML);

      const std::string& debug_content_str = debug_content.str();
      response.get_output_stream().write(
        debug_content_str.data(),
        debug_content_str.size());
    }
    catch(...)
    {
    }
  }

  void DebugSink::fill_debug_body(
    HttpResponse& response,
    int& http_status,
    Stream::Error& ostr) const noexcept
  {
    if (require_debug_body())
    {
      fill_debug_body_(response, ostr.str());
      http_status = 200;
    }
  }

  void DebugSink::print_request_debug_info(
    const RequestInfo& request_info) noexcept
  {
    if (require_long_debug_info())
    {
      if (require_debug_body())
      {
        debug_info_str_ << "\n" << Debug::REQUEST_INFO_HEAD << "\n";
      }

      {
        char hostname[HOST_NAME_MAX + 1];
        if (gethostname(hostname, sizeof(hostname)) == -1)
        {
          *hostname = 0;
        }
        debug_info_str_ << "server-id = " << hostname << sep_;
      }

      const char* user_status_str = 0;

      switch (request_info.user_status)
      {
        case CampaignSvcs::US_UNDEFINED: user_status_str = "undefined"; break;
        case CampaignSvcs::US_OPTIN: user_status_str = "optin"; break;
        case CampaignSvcs::US_OPTOUT: user_status_str = "optout"; break;
        case CampaignSvcs::US_PROBE: user_status_str = "probe"; break;
        case CampaignSvcs::US_TEMPORARY: user_status_str = "temporary"; break;
        default: break;
      };

      debug_info_str_ << "time = ";
      try
      {
        debug_info_str_ << request_info.current_time.gm_ft();
      }
      catch (...)
      {
        debug_info_str_ << "invalid";
      }
      debug_info_str_ <<  sep_ <<
        "user_status = " << (user_status_str ? user_status_str : "unknown") << sep_ <<
        "tid = " << request_info.tag_id << sep_ <<
        "ext_tag_id = " << request_info.ext_tag_id << sep_ <<
        "request_id = " << request_info.request_id << sep_ <<
        "page_load_id = " << request_info.page_load_id << sep_ <<
        "ccid = " << request_info.ccid << sep_ <<
        "test_request = " << request_info.test_request << sep_ <<
        "log_as_test = " << request_info.log_as_test << sep_ <<
        "location = ";

      if (request_info.location.in())
      {
        debug_info_str_ << request_info.location->country << "/" <<
          request_info.location->region << "/" <<
          request_info.location->city;
      }

      debug_info_str_ << sep_ <<
        "referer = " << request_info.referer << sep_ <<
        "uid = " << (!request_info.client_id.is_null() ?
           request_info.client_id.to_string().c_str() : "") << sep_ <<
        "signed_uid = " << request_info.signed_client_id << sep_ <<
        "temporary_uid = " << (!request_info.temp_client_id.is_null() ?
           request_info.temp_client_id.to_string().c_str() : "") << sep_ <<
        "merge_persistent_uid = " << (!request_info.merge_persistent_client_id.is_null() ?
           request_info.merge_persistent_client_id.to_string().c_str() : "") << sep_ <<
        "household_uid = " << (!request_info.household_client_id.is_null() ?
           request_info.household_client_id.to_string().c_str() : "") << sep_ <<
        "ip = " << request_info.peer_ip << sep_ <<
        "search-phrase = " << request_info.search_words << sep_ <<
        "search_engine_id = " << request_info.search_engine_id << sep_ <<
        "do_passback = " << (request_info.do_passback ? "true" : "false") << sep_ <<
        "colocation_passback = " << (request_info.passback_by_colocation ? "true" : "false") << sep_ <<
        "browser = " << request_info.web_browser << sep_ <<
        "platform = " << request_info.platform << sep_ <<
        "full_platform = " << request_info.full_platform << sep_ <<
        "platform_ids = ";
      Algs::print(debug_info_str_, request_info.platform_ids.begin(), request_info.platform_ids.end());
      debug_info_str_ << sep_;
    }
  }

  void DebugSink::print_channel_matching_seq_(
    const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::ChannelAtom>& channels,
    const char type) noexcept
  {
    for (const auto& channel : channels)
    {
      debug_info_str_ << field_begin_
                      << channel.id()
                      << type
                      << " :: "
                      << channel.trigger_channel_id()
                      << field_end_;
    }
  }

  template<typename It>
  std::ostream& print_with_type(std::ostream& out,
    It it_begin,
    It it_end,
    const char type,
    const bool first_delimiter,
    const char* delim = ", ")
  {
    for (It it = it_begin; it != it_end; ++it)
    {
      if (it != it_begin || first_delimiter)
      {
        out << delim;
      }

      out << *it << type;
    }

    return out;
  }

  void
  DebugSink::print_trigger_matching_debug_info(
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo& match_result,
    const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::CCGKeyword>* ccg_keywords,
    const char* prefix) noexcept
  {
    IdSetType uniq_ids;
    if (require_short_header())
    {
      size_t count = 0;
      const auto& matched_channels = match_result.matched_channels();
      std::transform(
        std::begin(matched_channels.page_channels()),
        std::end(matched_channels.page_channels()),
        std::inserter(uniq_ids, uniq_ids.end()),
        GetChannelId());
      count = uniq_ids.size();
      uniq_ids.clear();
      std::transform(
        std::begin(matched_channels.search_channels()),
        std::end(matched_channels.search_channels()),
        std::inserter(uniq_ids, uniq_ids.end()),
        GetChannelId());
      count += uniq_ids.size();
      uniq_ids.clear();
      std::transform(
        std::begin(matched_channels.url_channels()),
        std::end(matched_channels.url_channels()),
        std::inserter(uniq_ids, uniq_ids.end()),
        GetChannelId());
      std::copy(
        std::begin(matched_channels.uid_channels()),
        std::end(matched_channels.uid_channels()),
        std::inserter(uniq_ids, uniq_ids.end()));
      count += uniq_ids.size();

      debug_info_str_ << "trigger_channels_count = "
        << count << sep_;
    }
    else if (require_long_debug_info())
    {
      if (require_debug_body())
      {
        debug_info_str_ << "\n" << Debug::CHANNEL_MATCHING_HEAD << "\n";
      }

      debug_info_str_ <<
        prefix << "special_channels_effects = " <<
          (match_result.no_track() ? "NO TRACK": "TRACK") << ", " <<
          (match_result.no_adv() ? "NO ADV": "ADV") << sep_ <<
        prefix <<
          "triggers = " << (require_debug_body() ? "\n" : "");

      const auto& matched_channels = match_result.matched_channels();
      print_channel_matching_seq_(
        matched_channels.page_channels(), 'P');
      print_channel_matching_seq_(
        matched_channels.search_channels(), 'S');
      print_channel_matching_seq_(
        matched_channels.url_channels(), 'U');
      print_channel_matching_seq_(
        matched_channels.url_keyword_channels(), 'R');

      debug_info_str_ << sep_ <<
        prefix << "trigger_channels = ";

      IdSetType uniq_ids;
      bool first_delimiter = false;
      if (!matched_channels.page_channels().empty())
      {
        std::transform(
          std::begin(matched_channels.page_channels()),
          std::end(matched_channels.page_channels()),
          std::inserter(uniq_ids, uniq_ids.end()),
          GetChannelId());
        print_with_type(
          debug_info_str_,
          uniq_ids.begin(), uniq_ids.end(), 'P', first_delimiter);
        first_delimiter = true;
      }
      if (!matched_channels.search_channels().empty())
      {
        uniq_ids.clear();
        std::transform(
          std::begin(matched_channels.search_channels()),
          std::end(matched_channels.search_channels()),
          std::inserter(uniq_ids, uniq_ids.end()),
          GetChannelId());
        print_with_type(
          debug_info_str_,
          uniq_ids.begin(), uniq_ids.end(), 'S', first_delimiter);
        first_delimiter = true;
      }
      if (!matched_channels.url_channels().empty())
      {
        uniq_ids.clear();
        std::transform(
          std::begin(matched_channels.url_channels()),
          std::end(matched_channels.url_channels()),
          std::inserter(uniq_ids, uniq_ids.end()),
          GetChannelId());
        print_with_type(
          debug_info_str_,
          uniq_ids.begin(), uniq_ids.end(), 'U', first_delimiter);
        first_delimiter = true;
      }
      if (!matched_channels.url_keyword_channels().empty())
      {
        uniq_ids.clear();
        std::transform(
          std::begin(matched_channels.url_keyword_channels()),
          std::end(matched_channels.url_keyword_channels()),
          std::inserter(uniq_ids, uniq_ids.end()),
          GetChannelId());
        print_with_type(
          debug_info_str_,
          uniq_ids.begin(), uniq_ids.end(), 'R', first_delimiter);
        first_delimiter = true;
      }
      if (!matched_channels.uid_channels().empty())
      {
        uniq_ids.clear();
        std::copy(
          std::begin(matched_channels.uid_channels()),
          std::end(matched_channels.uid_channels()),
          std::inserter(uniq_ids, uniq_ids.end()));
        print_with_type(
          debug_info_str_,
          uniq_ids.begin(), uniq_ids.end(), 'A', first_delimiter);
      }

      debug_info_str_ << sep_ << prefix << "content_channels = ";

      const auto& content_channels = match_result.content_channels();
      for (int i = 0; i < content_channels.size(); ++i)
      {
        if (i != 0)
        {
          debug_info_str_ << ", ";
        }
        const auto& atom = content_channels[i];
        debug_info_str_ << atom.id() << "(" << atom.weight() << ")";
      }

      debug_info_str_ << sep_;

      if (ccg_keywords && allow_show_history_profile_)
      {
        debug_info_str_ << prefix << "ccg_keywords =";
        for(const auto& ccg_keyword : *ccg_keywords)
        {
          debug_info_str_ << field_begin_ <<
            "'" << ccg_keyword.original_keyword() << "'" <<
            " :: ccgk_id = " << ccg_keyword.ccg_keyword_id() <<
            " :: ch_id = " << ccg_keyword.channel_id() <<
            " :: ccg_id = " << ccg_keyword.ccg_id() <<
            " :: max_cpc = " << GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
              ccg_keyword.max_cpc()) <<
            " :: ctr = " << GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
              ccg_keyword.ctr()) << field_end_;
        }

        debug_info_str_ << sep_;
      }
    }
  }

  void DebugSink::print_history_matching_debug_info_(
    const AdServer::UserInfoSvcs::Proto::MatchResult& history_match_result) noexcept
  {
    if (require_debug_body())
    {
      debug_info_str_ << "\n" << Debug::HISTORY_MATCHING_HEAD << "\n";
    }
    else if (require_short_header())
    {
      debug_info_str_ << "history_channels_count = "
        << history_match_result.channels().size() << sep_;
      return;
    }

    debug_info_str_ << "last_request_time = ";
    try
    {
      debug_info_str_ << GrpcAlgs::unpack_time(
          history_match_result.last_request_time()).gm_ft() <<
        sep_ << "create_time = " << GrpcAlgs::unpack_time(
          history_match_result.create_time()).gm_ft() <<
        sep_ << "session_start = " << GrpcAlgs::unpack_time(
          history_match_result.session_start()).gm_ft();
    }
    catch (...)
    {
      debug_info_str_ << "invalid time";
    }

    if (allow_show_history_profile_)
    {
      debug_info_str_ << sep_ << "history_channels = ";

      GrpcAlgs::print_repeated_fields(
        debug_info_str_,
        ",",
        ":",
        history_match_result.channels(),
        &AdServer::UserInfoSvcs::Proto::ChannelWeight::channel_id);

      debug_info_str_ << sep_ << "household_history_channels = ";

      GrpcAlgs::print_repeated_fields(
        debug_info_str_,
        ",",
        ":",
        history_match_result.hid_channels(),
        &AdServer::UserInfoSvcs::Proto::ChannelWeight::channel_id);
    }

    debug_info_str_ << sep_ << "full_freq_caps = ";
    GrpcAlgs::print_repeated(
      debug_info_str_,
      ",",
      history_match_result.full_freq_caps());

    debug_info_str_ << sep_ << "fraud_request = ";

    if (history_match_result.fraud_request())
    {
      debug_info_str_ << "true";
    }
    else
    {
      debug_info_str_ << "false";
    }

    debug_info_str_ << sep_;
  }

  void DebugSink::print_time_metering_debug_info(
    const RequestTimeMetering& time_metering) noexcept
  {
    if (require_debug_body())
    {
      debug_info_str_ << "\n" << Debug::TIME_METERING_HEAD << "\n";
    }

    debug_info_str_ <<
      "trigger_match_time = " <<
      time_metering.trigger_match_time;
    if (require_long_debug_info())
    {
      debug_info_str_ << " : ";
      Algs::print(debug_info_str_,
        time_metering.detail_trigger_match_time.begin(),
        time_metering.detail_trigger_match_time.end());
    }

    debug_info_str_ << sep_ <<
      "request_fill_time = " <<
      time_metering.request_fill_time << sep_ <<
      "history_match_time = " <<
      time_metering.history_match_time << " : " <<
      time_metering.history_match_local_time << sep_ <<
      "creative_selection_time = " <<
      time_metering.creative_selection_time << ": " <<
      time_metering.creative_selection_local_time << sep_;
    if (require_long_debug_info())
    {
      debug_info_str_ << "history_post_match_time = " <<
        time_metering.history_post_match_time << sep_;
    }
  }

  void DebugSink::print_empty_creative_selection_debug_info_() noexcept
  {
    if (require_debug_body())
    {
      debug_info_str_ << "\n" <<
        Debug::CREATIVE_SELECTION_INFO_HEAD << "\n";
    }
    else if (require_short_header())
    {
      debug_info_str_ << "fc_click_url = " << sep_ <<
        "fc_action_url = " << sep_ <<
        "passback_url = " << sep_ <<
        "ccids = 0" << sep_;
      return;
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

  void DebugSink::print_creative_selection_debug_info_(
    const PassbackInfo& passback_info,
    const AdServer::CampaignSvcs::Proto::AdSlotResult&
      request_creative_result) noexcept
  {
    const auto& selected_creatives = request_creative_result.selected_creatives();
    const auto& debug_info = request_creative_result.debug_info();
    const auto& debug_selected_creatives = debug_info.selected_creatives();
    const int creative_count = selected_creatives.size();

    assert(selected_creatives.size() == debug_selected_creatives.size());

    if (require_debug_body())
    {
      debug_info_str_ << "\n" <<
        Debug::CREATIVE_SELECTION_INFO_HEAD << "\n";
    }
    else if (require_short_header())
    {
      const auto dc_count = debug_selected_creatives.size();
      debug_info_str_ << "fc_click_url = " <<
        (dc_count ? selected_creatives[0].click_url() : "") << sep_ <<
        "fc_action_url = " <<
          (dc_count ? debug_selected_creatives[0].action_adv_url() : "") << sep_ <<
        "passback_url = " << passback_info.url << sep_ <<
        "ccids = " << creative_count << sep_;
      return;
    }

    std::uint32_t first_ccid = 0;
    std::uint32_t first_cmp_id = 0;

    if (creative_count != 0)
    {
      first_ccid = selected_creatives[0].ccid();
      first_cmp_id = selected_creatives[0].cmp_id();
    }

    CampaignSvcs::RevenueDecimal imp_revenue(
      CampaignSvcs::RevenueDecimal::ZERO);
    CampaignSvcs::RevenueDecimal click_revenue(
      CampaignSvcs::RevenueDecimal::ZERO);
    CampaignSvcs::RevenueDecimal action_revenue(
      CampaignSvcs::RevenueDecimal::ZERO);

    for(const auto& creative : debug_selected_creatives)
    {
      imp_revenue += GrpcAlgs::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          creative.imp_revenue());
      click_revenue += GrpcAlgs::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          creative.click_revenue());
      action_revenue += GrpcAlgs::unpack_decimal<
        CampaignSvcs::RevenueDecimal>(
          creative.action_revenue());
    }

    debug_info_str_ <<
      "ccid = " << first_ccid << sep_ <<
      "cmpid = " << first_cmp_id << sep_ <<
      "creative_size_id = " << debug_info.tag_size_id() << sep_ <<
      "mime_format = " << request_creative_result.mime_format() << sep_ <<
      "tag_id = " << debug_info.tag_id() << sep_ <<
      "site_id = " << debug_info.site_id() << sep_ <<
      "site_rate_id = " << debug_info.site_rate_id() << sep_ <<
      "imp_revenue = " << imp_revenue << sep_ <<
      "click_revenue = " << click_revenue << sep_ <<
      "action_revenue = " << action_revenue << sep_ <<
      "min_no_adv_ecpm = " << debug_info.min_no_adv_ecpm() << sep_ <<
      "min_text_ecpm = " << debug_info.min_text_ecpm() << sep_ <<
      "test_request = " << request_creative_result.test_request() << sep_ <<
      "passback_url = " << passback_info.url << sep_ <<
      "track_pixel_url = " << debug_info.track_pixel_url() << sep_ <<
      "cpm_threshold = " <<
        GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
          debug_info.cpm_threshold()) << sep_ <<
      "walled_garden = " << debug_info.walled_garden() << sep_ <<
      "auction_type = ";

    if (debug_info.auction_type() == CampaignSvcs::AT_RANDOM)
    {
      debug_info_str_ << "random";
    }
    else if (debug_info.auction_type() == CampaignSvcs::AT_MAX_ECPM)
    {
      debug_info_str_ << "max ecpm";
    }
    else if (debug_info.auction_type() == CampaignSvcs::AT_PROPORTIONAL_PROBABILITY)
    {
      debug_info_str_ << "proportional probability";
    }
    else
    {
      debug_info_str_ << "unknown";
    }

    debug_info_str_ << sep_ <<
      "selected_creatives = ";

    for(int i = 0; i < creative_count; ++i)
    {
      const String::SubString OFFSET(require_debug_body() ? "  " : "");

      debug_info_str_ << creative_start_sep_ <<
        OFFSET << "request_id = " << GrpcAlgs::unpack_request_id(
          selected_creatives[i].request_id()) << sep_ <<
        OFFSET << "ccid = " << selected_creatives[i].ccid() << sep_ <<
        OFFSET << "cmp_id = " << selected_creatives[i].cmp_id() << sep_ <<
        OFFSET << "order_set_id = " << selected_creatives[i].order_set_id() << sep_ <<
        OFFSET << "triggered_expression = " <<
          debug_selected_creatives[i].triggered_expression() << sep_ <<
        OFFSET << "ecpm = " << GrpcAlgs::unpack_decimal<
          AdServer::CampaignSvcs::RevenueDecimal>(selected_creatives[i].ecpm()) << sep_ <<
        OFFSET << "ecpm_bid = " << GrpcAlgs::unpack_decimal<
          AdServer::CampaignSvcs::RevenueDecimal>(debug_selected_creatives[i].ecpm_bid()) << sep_ <<
        OFFSET << "click_url = " << selected_creatives[i].click_url() << sep_ <<
        OFFSET << "html_url = " << debug_selected_creatives[i].html_url() << sep_ <<
        OFFSET << "action_adv_url = " <<
          debug_selected_creatives[i].action_adv_url() << sep_ <<
        OFFSET << "revenue = " <<
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            selected_creatives[i].revenue()) << sep_ <<
        OFFSET << "imp_revenue = " <<
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            debug_selected_creatives[i].imp_revenue()) << sep_ <<
        OFFSET << "click_revenue = " <<
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            debug_selected_creatives[i].click_revenue()) << sep_ <<
        OFFSET << "action_revenue = " <<
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            debug_selected_creatives[i].action_revenue());
      debug_info_str_ << creative_end_sep_;
    }

    debug_info_str_ << sep_;

    if (require_debug_body() && !debug_info.trace_ccg().empty())
    {
      debug_info_str_ << "\n" << Debug::TRACE_CCG_INFO_HEAD << "\n";
      debug_info_str_ << debug_info.trace_ccg();
    }
  }

  void
  DebugSink::print_acquire_ad(
    const RequestInfo& request_info,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_matched_channels,
    const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::CCGKeyword>* ccg_keywords,
    const AdServer::UserInfoSvcs::Proto::MatchResult& history_match_result) noexcept
  {
    if (require_debug_info())
    {
      print_request_debug_info(request_info);

      if (trigger_matched_channels)
      {
        print_trigger_matching_debug_info(
          *trigger_matched_channels,
          ccg_keywords);
      }

      print_history_matching_debug_info_(
        history_match_result);
    }
  }

  void DebugSink::print_creative_selection_debug_info(
    const RequestInfo& request_info,
    const PassbackInfo& passback_info,
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
      campaign_matching_result,
    const RequestTimeMetering& request_time_metering) noexcept
  {
    if (require_debug_info())
    {
      const auto& debug_info = campaign_matching_result.debug_info();

      debug_info_str_ << "colo_id = " << debug_info.colo_id() << sep_ << "freq-caps = ";
      for (const auto& ad_slot : campaign_matching_result.ad_slots())
      {
        GrpcAlgs::print_repeated(
          debug_info_str_,
          ",",
          ad_slot.freq_caps());
      }

      debug_info_str_ << sep_ << "uc-freq-caps = ";
      for (const auto& ad_slot : campaign_matching_result.ad_slots())
      {
        GrpcAlgs::print_repeated(
          debug_info_str_,
          ",",
          ad_slot.uc_freq_caps());
      }

      debug_info_str_ << sep_ <<
        "user_group_id = " << debug_info.user_group_id() << sep_ <<
        "geo_channels = ";
      GrpcAlgs::print_repeated(debug_info_str_, ",", debug_info.geo_channels());
      debug_info_str_ << sep_ << "device_channels = ";
      GrpcAlgs::print_repeated(debug_info_str_, ",", debug_info.platform_channels());
      debug_info_str_ << sep_ <<
        "last_device_channel_id = " << debug_info.last_platform_channel_id() << sep_ <<
        "random = " << request_info.random << sep_;

      if (!campaign_matching_result.ad_slots().empty())
      {
        print_creative_selection_debug_info_(
          passback_info,
          campaign_matching_result.ad_slots()[0]);
      }
      else
      {
        print_empty_creative_selection_debug_info_();
      }

      print_time_metering_debug_info(
        request_time_metering);
    }
  }
} // namespace AdServer::Grpc