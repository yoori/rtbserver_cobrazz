#include "CampaignManagerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <string>
#include <utility>
#include <unistd.h>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/Grpc/ProcessControl.grpc.pb.h>
#include <Generics/Time.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <CampaignSvcs/CampaignManager/CampaignManagerGrpc.grpc.pb.h>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    constexpr const char campaign_manager_grpc_aspect[] =
      "CampaignManagerGrpc";

    namespace pb = adserver::campaign_svcs::campaign_manager;
    namespace pc = adserver::grpc::process_control;

    const std::string&
    service_hostname_()
    {
      static const std::string hostname = []()
      {
        char buffer[256];
        if (::gethostname(buffer, sizeof(buffer)) != 0)
        {
          return std::string();
        }
        buffer[sizeof(buffer) - 1] = 0;
        return std::string(buffer);
      }();
      return hostname;
    }

    class CallStatsGuard final
    {
    public:
      CallStatsGuard(
        std::atomic<std::uint64_t>& call_in_progress,
        std::atomic<std::uint64_t>& call_total,
        std::atomic<std::uint64_t>& call_time,
        std::atomic<std::uint64_t>& method_in_progress,
        std::atomic<std::uint64_t>& method_total,
        std::atomic<std::uint64_t>& method_time) noexcept
        : call_in_progress_(call_in_progress),
          call_time_(call_time),
          method_in_progress_(method_in_progress),
          method_time_(method_time)
      {
        call_in_progress_.fetch_add(1, std::memory_order_relaxed);
        call_total.fetch_add(1, std::memory_order_relaxed);
        method_in_progress_.fetch_add(1, std::memory_order_relaxed);
        method_total.fetch_add(1, std::memory_order_relaxed);
      }

      ~CallStatsGuard()
      {
        const auto elapsed_us = timer_.elapsed_time().microseconds();
        method_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
        call_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
        method_in_progress_.fetch_sub(1, std::memory_order_relaxed);
        call_in_progress_.fetch_sub(1, std::memory_order_relaxed);
      }

      CallStatsGuard(const CallStatsGuard&) = delete;
      CallStatsGuard& operator=(const CallStatsGuard&) = delete;

    private:
      Generics::Timer timer_;
      std::atomic<std::uint64_t>& call_in_progress_;
      std::atomic<std::uint64_t>& call_time_;
      std::atomic<std::uint64_t>& method_in_progress_;
      std::atomic<std::uint64_t>& method_time_;
    };

    CORBACommons::OctSeq
    unpack_oct_seq(const std::string& source)
    {
      CORBACommons::OctSeq result;
      result.length(source.size());
      for(CORBA::ULong i = 0; i < source.size(); ++i)
      {
        result[i] = static_cast<CORBA::Octet>(source[i]);
      }
      return result;
    }

    template<typename OctSeq>
    std::string
    pack_oct_seq(const OctSeq& source)
    {
      std::string result;
      result.resize(source.length());
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        result[i] = static_cast<char>(source[i]);
      }
      return result;
    }

    Generics::Time
    unpack_time(const std::string& source)
    {
      Generics::Time result;
      if(source.size() != Generics::Time::TIME_PACK_LEN)
      {
        Stream::Error ostr;
        ostr << "Invalid packed time size: " << source.size();
        throw CampaignManagerCore::Exception(ostr);
      }

      result.unpack(reinterpret_cast<const unsigned char*>(source.data()));
      return result;
    }

    std::string
    pack_time(const Generics::Time& source)
    {
      return pack_oct_seq(CorbaAlgs::pack_time(source));
    }

    AdServer::Commons::UserId
    unpack_user_id(const std::string& source)
    {
      return CorbaAlgs::unpack_user_id(unpack_oct_seq(source));
    }

    AdServer::Commons::RequestId
    unpack_request_id(const std::string& source)
    {
      return CorbaAlgs::unpack_request_id(unpack_oct_seq(source));
    }

    std::string
    pack_request_id(const AdServer::Commons::RequestId& source)
    {
      return pack_oct_seq(CorbaAlgs::pack_request_id(source));
    }

    RevenueDecimal
    unpack_revenue_decimal(const pb::DecimalInfo& source)
    {
      return CorbaAlgs::unpack_decimal<RevenueDecimal>(
        unpack_oct_seq(source.value()));
    }

    void
    pack_revenue_decimal(
      const RevenueDecimal& source,
      pb::DecimalInfo& target)
    {
      target.set_value(pack_oct_seq(
        CorbaAlgs::pack_decimal<RevenueDecimal>(source)));
    }

    void
    pack_ids(
      const CampaignManagerCore::IdVector& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      target->Add(source.begin(), source.end());
    }

    CampaignManagerCore::IdVector
    unpack_ids(
      const google::protobuf::RepeatedField<google::protobuf::uint64>& source)
    {
      CampaignManagerCore::IdVector result;
      result.reserve(source.size());
      for(const auto id : source)
      {
        result.push_back(id);
      }
      return result;
    }

    void
    unpack_strings(
      const google::protobuf::RepeatedPtrField<std::string>& source,
      CampaignManagerCore::StringVector& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(item);
      }
    }

    void
    pack_strings(
      const CampaignManagerCore::StringVector& source,
      google::protobuf::RepeatedPtrField<std::string>* target)
    {
      for(const auto& item : source)
      {
        *target->Add() = item;
      }
    }

    void
    unpack_tokens(
      const google::protobuf::RepeatedPtrField<pb::TokenInfo>& source,
      CampaignManagerCore::TokenVector& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back({item.name(), item.value()});
      }
    }

    void
    pack_tokens(
      const CampaignManagerCore::TokenVector& source,
      google::protobuf::RepeatedPtrField<pb::TokenInfo>* target)
    {
      for(const auto& item : source)
      {
        auto* token = target->Add();
        token->set_name(item.name);
        token->set_value(item.value);
      }
    }

    void
    pack_token_images(
      const CampaignManagerCore::TokenImageVector& source,
      google::protobuf::RepeatedPtrField<pb::TokenImageInfo>* target)
    {
      for(const auto& item : source)
      {
        auto* token = target->Add();
        token->set_name(item.name);
        token->set_value(item.value);
        token->set_width(item.width);
        token->set_height(item.height);
      }
    }

    void
    set_optional_uint64(
      const pb::OptionalUInt64& source,
      AdServer::Commons::Optional<unsigned long>& target)
    {
      if(source.defined())
      {
        target = static_cast<unsigned long>(source.value());
      }
    }

    CampaignManagerCore::GeoInfo
    unpack_geo_info(const pb::GeoInfo& source)
    {
      return {source.country(), source.region(), source.city()};
    }

    CampaignManagerCore::GeoCoordInfo
    unpack_geo_coord_info(const pb::GeoCoordInfo& source)
    {
      return {
        CorbaAlgs::unpack_decimal<CoordDecimal>(
          unpack_oct_seq(source.longitude())),
        CorbaAlgs::unpack_decimal<CoordDecimal>(
          unpack_oct_seq(source.latitude())),
        CorbaAlgs::unpack_decimal<CoordDecimal>(
          unpack_oct_seq(source.accuracy()))};
    }

    void
    unpack_geo_info_seq(
      const google::protobuf::RepeatedPtrField<pb::GeoInfo>& source,
      std::vector<CampaignManagerCore::GeoInfo>& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_geo_info(item));
      }
    }

    void
    unpack_geo_coord_info_seq(
      const google::protobuf::RepeatedPtrField<pb::GeoCoordInfo>& source,
      std::vector<CampaignManagerCore::GeoCoordInfo>& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_geo_coord_info(item));
      }
    }

    void
    pack_channel_search_result(
      const CampaignManagerCore::ChannelSearchResult& source,
      pb::ChannelSearchResult& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_use_count(source.use_count);
      pack_ids(
        source.matched_simple_channels,
        target.mutable_matched_simple_channels());
      pack_ids(source.ccg_ids, target.mutable_ccg_ids());
      target.set_discover_query(source.discover_query);
      target.set_language(source.language);
    }

    void
    pack_discover_channel_result(
      const CampaignManagerCore::DiscoverChannelResult& source,
      pb::DiscoverChannelResult& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_name(source.name);
      target.set_query(source.query);
      target.set_annotation(source.annotation);
      target.set_weight(source.weight);
      pack_ids(source.categories, target.mutable_categories());
      target.set_country_code(source.country_code);
      target.set_language(source.language);
    }

    void
    pack_category_channel_node(
      const CampaignManagerCore::CategoryChannelNodeInfo& source,
      pb::CategoryChannelNode& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_name(source.name);
      target.set_flags(source.flags);

      for(const auto& child : source.child_category_channels)
      {
        pack_category_channel_node(child, *target.add_child_category_channels());
      }
    }

    CampaignManagerCore::ChannelTriggerMatchInfo
    unpack_channel_trigger_match(const pb::ChannelTriggerMatchInfo& source)
    {
      return {source.channel_trigger_id(), source.channel_id()};
    }

    void
    unpack_channel_trigger_matches(
      const google::protobuf::RepeatedPtrField<pb::ChannelTriggerMatchInfo>& source,
      CampaignManagerCore::ChannelTriggerMatchVector& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_channel_trigger_match(item));
      }
    }

    CampaignManagerCore::CCGKeywordInfo
    unpack_ccg_keyword(const pb::CcgKeywordInfo& source)
    {
      return {
        source.ccg_keyword_id(),
        source.ccg_id(),
        source.channel_id(),
        unpack_revenue_decimal(source.max_cpc()),
        unpack_revenue_decimal(source.ctr()),
        source.click_url(),
        source.original_keyword()};
    }

    void
    unpack_ccg_keywords(
      const google::protobuf::RepeatedPtrField<pb::CcgKeywordInfo>& source,
      CampaignManagerCore::CCGKeywordVector& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_ccg_keyword(item));
      }
    }

    CampaignManagerCore::TrackCreativeInfo
    unpack_track_creative(const pb::TrackCreativeInfo& source)
    {
      return {
        source.ccid(),
        source.ccg_keyword_id(),
        unpack_request_id(source.request_id()),
        unpack_revenue_decimal(source.ctr())};
    }

    void
    unpack_track_creatives(
      const google::protobuf::RepeatedPtrField<pb::TrackCreativeInfo>& source,
      CampaignManagerCore::TrackCreativeVector& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_track_creative(item));
      }
    }

    void
    unpack_common_ad_request_info(
      const pb::CommonAdRequestInfo& source,
      CampaignManagerCore::CommonAdRequestInfo& target)
    {
      target.time = unpack_time(source.time());
      target.request_id = unpack_request_id(source.request_id());
      target.creative_instantiate_type = source.creative_instantiate_type();
      target.request_type = source.request_type();
      target.random = source.random();
      target.test_request = source.test_request();
      target.log_as_test = source.log_as_test();
      target.colo_id = source.colo_id();
      target.external_user_id = source.external_user_id();
      target.source_id = source.source_id();
      unpack_geo_info_seq(source.location(), target.location);
      unpack_geo_coord_info_seq(source.coord_location(), target.coord_location);
      target.full_referer = source.full_referer();
      target.referer = source.referer();
      unpack_strings(source.urls(), target.urls);
      target.security_token = source.security_token();
      target.pub_impr_track_url = source.pub_impr_track_url();
      target.pub_param = source.pub_param();
      target.preclick_url = source.preclick_url();
      target.click_prefix_url = source.click_prefix_url();
      target.original_url = source.original_url();
      target.track_user_id = unpack_user_id(source.track_user_id());
      target.user_id = unpack_user_id(source.user_id());
      target.user_status = source.user_status();
      target.signed_user_id = source.signed_user_id();
      target.peer_ip = source.peer_ip();
      target.user_agent = source.user_agent();
      target.cohort = source.cohort();
      target.hpos = source.hpos();
      target.ext_track_params = source.ext_track_params();
      unpack_tokens(source.tokens(), target.tokens);
      target.set_cookie = source.set_cookie();
      target.passback_type = source.passback_type();
      target.passback_url = source.passback_url();
    }

    CampaignManagerCore::ContextAdRequestInfo
    unpack_context_ad_request_info(const pb::ContextAdRequestInfo& source)
    {
      CampaignManagerCore::ContextAdRequestInfo target;
      target.enabled_notice = source.enabled_notice();
      target.client = source.client();
      target.client_version = source.client_version();
      target.platform_ids = unpack_ids(source.platform_ids());
      target.geo_channels = unpack_ids(source.geo_channels());
      target.platform = source.platform();
      target.full_platform = source.full_platform();
      target.web_browser = source.web_browser();
      target.ip_hash = source.ip_hash();
      target.profile_referer = source.profile_referer();
      target.page_load_id = source.page_load_id();
      target.full_referer_hash = source.full_referer_hash();
      target.short_referer_hash = source.short_referer_hash();
      return target;
    }

    CampaignManagerCore::TraceAdSlotInfo
    unpack_ad_slot_info(const pb::AdSlotInfo& source)
    {
      CampaignManagerCore::TraceAdSlotInfo target;
      target.ad_slot_id = source.ad_slot_id();
      target.format = source.format();
      target.tag_id = source.tag_id();
      unpack_strings(source.sizes(), target.sizes);
      target.ext_tag_id = source.ext_tag_id();
      target.min_ecpm = unpack_revenue_decimal(source.min_ecpm());
      target.min_ecpm_currency_code = source.min_ecpm_currency_code();
      unpack_strings(source.currency_codes(), target.currency_codes);
      target.passback = source.passback();
      target.up_expand_space = source.up_expand_space();
      target.right_expand_space = source.right_expand_space();
      target.left_expand_space = source.left_expand_space();
      target.down_expand_space = source.down_expand_space();
      target.tag_visibility = source.tag_visibility();
      target.tag_predicted_viewability = source.tag_predicted_viewability();
      target.video_min_duration = source.video_min_duration();
      target.video_max_duration = source.video_max_duration();
      target.video_skippable_max_duration = source.video_skippable_max_duration();
      target.video_allow_skippable = source.video_allow_skippable();
      target.video_allow_unskippable = source.video_allow_unskippable();
      target.video_width = source.video_width();
      target.video_height = source.video_height();
      unpack_strings(source.exclude_categories(), target.exclude_categories);
      unpack_strings(source.required_categories(), target.required_categories);
      target.debug_ccg = source.debug_ccg();
      target.allowed_durations = unpack_ids(source.allowed_durations());
      target.native_data_tokens.reserve(source.native_data_tokens_size());
      for(const auto& token : source.native_data_tokens())
      {
        target.native_data_tokens.push_back({token.name(), token.required()});
      }
      target.native_image_tokens.reserve(source.native_image_tokens_size());
      for(const auto& token : source.native_image_tokens())
      {
        target.native_image_tokens.push_back({
          token.name(),
          token.required(),
          token.width(),
          token.height()});
      }
      target.native_ads_impression_tracker_type =
        source.native_ads_impression_tracker_type();
      target.fill_track_html = source.fill_track_html();
      unpack_tokens(source.tokens(), target.tokens);
      return target;
    }

    CampaignManagerCore::CreativeRequestInfo
    unpack_request_params(const pb::RequestParams& source)
    {
      CampaignManagerCore::CreativeRequestInfo target;
      unpack_common_ad_request_info(source.common_info(), target.common_info);
      target.context_info = unpack_context_ad_request_info(source.context_info());
      target.publisher_site_id = source.publisher_site_id();
      target.publisher_account_ids = unpack_ids(source.publisher_account_ids());
      target.fill_track_pixel = source.fill_track_pixel();
      target.fill_iurl = source.fill_iurl();
      target.ad_instantiate_type = source.ad_instantiate_type();
      target.only_display_ad = source.only_display_ad();
      target.full_freq_caps = unpack_ids(source.full_freq_caps());
      target.seq_orders.reserve(source.seq_orders_size());
      for(const auto& seq_order : source.seq_orders())
      {
        target.seq_orders.push_back({
          seq_order.ccg_id(),
          seq_order.set_id(),
          seq_order.imps()});
      }
      target.campaign_freqs.reserve(source.campaign_freqs_size());
      for(const auto& campaign_freq : source.campaign_freqs())
      {
        target.campaign_freqs.push_back({
          campaign_freq.campaign_id(),
          campaign_freq.imps()});
      }
      target.household_id = unpack_user_id(source.household_id());
      target.merged_user_id = unpack_user_id(source.merged_user_id());
      target.search_engine_id = source.search_engine_id();
      target.search_words = source.search_words();
      target.page_keywords_present = source.page_keywords_present();
      target.profiling_available = source.profiling_available();
      target.fraud = source.fraud();
      target.channels = unpack_ids(source.channels());
      target.hid_channels = unpack_ids(source.hid_channels());
      unpack_ccg_keywords(source.ccg_keywords(), target.ccg_keywords);
      unpack_ccg_keywords(source.hid_ccg_keywords(), target.hid_ccg_keywords);
      unpack_channel_trigger_matches(
        source.trigger_match_result().url_channels(),
        target.trigger_match_result.url_channels);
      unpack_channel_trigger_matches(
        source.trigger_match_result().pkw_channels(),
        target.trigger_match_result.pkw_channels);
      unpack_channel_trigger_matches(
        source.trigger_match_result().skw_channels(),
        target.trigger_match_result.skw_channels);
      unpack_channel_trigger_matches(
        source.trigger_match_result().ukw_channels(),
        target.trigger_match_result.ukw_channels);
      target.trigger_match_result.uid_channels =
        unpack_ids(source.trigger_match_result().uid_channels());
      target.client_create_time = unpack_time(source.client_create_time());
      target.session_start = unpack_time(source.session_start());
      target.exclude_pubpixel_accounts =
        unpack_ids(source.exclude_pubpixel_accounts());
      target.tag_delivery_factor = source.tag_delivery_factor();
      target.ccg_delivery_factor = source.ccg_delivery_factor();
      target.preview_ccid = source.preview_ccid();
      target.ad_slots.reserve(source.ad_slots_size());
      for(const auto& ad_slot : source.ad_slots())
      {
        target.ad_slots.push_back(unpack_ad_slot_info(ad_slot));
      }
      target.required_passback = source.required_passback();
      target.profiling_type = source.profiling_type();
      target.disable_fraud_detection = source.disable_fraud_detection();
      target.need_debug_info = source.need_debug_info();
      target.page_keywords = source.page_keywords();
      target.url_keywords = source.url_keywords();
      target.ssp_location = source.ssp_location();
      target.additional_info = source.additional_info();
      return target;
    }

    CampaignManagerCore::TraceRequestInfo
    unpack_trace_request_params(const pb::RequestParams& source)
    {
      CampaignManagerCore::TraceRequestInfo target;
      unpack_common_ad_request_info(source.common_info(), target.common_info);
      target.context_info = unpack_context_ad_request_info(source.context_info());
      target.publisher_site_id = source.publisher_site_id();
      target.publisher_account_ids = unpack_ids(source.publisher_account_ids());
      target.profiling_available = source.profiling_available();
      target.full_freq_caps = unpack_ids(source.full_freq_caps());
      target.channels = unpack_ids(source.channels());
      target.hid_channels = unpack_ids(source.hid_channels());
      target.client_create_time = unpack_time(source.client_create_time());
      target.tag_delivery_factor = source.tag_delivery_factor();
      target.ccg_delivery_factor = source.ccg_delivery_factor();
      return target;
    }

    void
    pack_creative_select_result(
      const CampaignManagerCore::CreativeSelectResultInfo& source,
      pb::CreativeSelectResult& target)
    {
      target.set_request_id(pack_request_id(source.request_id));
      target.set_ccid(source.ccid);
      target.set_cmp_id(source.cmp_id);
      target.set_campaign_group_id(source.campaign_group_id);
      target.set_order_set_id(source.order_set_id);
      target.set_advertiser_id(source.advertiser_id);
      target.set_advertiser_name(source.advertiser_name);
      target.set_creative_size(source.creative_size);
      pack_revenue_decimal(source.revenue, *target.mutable_revenue());
      pack_revenue_decimal(source.ecpm, *target.mutable_ecpm());
      pack_revenue_decimal(source.pub_ecpm, *target.mutable_pub_ecpm());
      target.set_click_url(source.click_url);
      target.set_destination_url(source.destination_url);
      target.set_creative_version_id(source.creative_version_id);
      target.set_creative_id(source.creative_id);
      target.set_https_safe_flag(source.https_safe_flag);
      target.set_expanding(source.expanding);
    }

    void
    pack_creative_select_debug_info(
      const CampaignManagerCore::CreativeSelectDebugInfo& source,
      pb::CreativeSelectDebugInfo& target)
    {
      pack_revenue_decimal(source.imp_revenue, *target.mutable_imp_revenue());
      pack_revenue_decimal(source.click_revenue, *target.mutable_click_revenue());
      pack_revenue_decimal(source.action_revenue, *target.mutable_action_revenue());
      pack_revenue_decimal(source.ecpm_bid, *target.mutable_ecpm_bid());
      target.set_action_adv_url(source.action_adv_url);
      target.set_html_url(source.html_url);
      target.set_triggered_expression(source.triggered_expression);
      target.set_full_expression(source.full_expression);
    }

    void
    pack_ad_slot_result(
      const CampaignManagerCore::AdSlotResultInfo& source,
      pb::AdSlotResult& target)
    {
      target.set_ad_slot_id(source.ad_slot_id);
      target.set_request_id(pack_request_id(source.request_id));
      target.set_passback(source.passback);
      target.set_passback_url(source.passback_url);
      target.set_creative_body(source.creative_body);
      target.set_notice_url(source.notice_url);
      pack_strings(source.track_pixel_urls, target.mutable_track_pixel_urls());
      target.set_yandex_track_params(source.yandex_track_params);
      target.set_creative_url(source.creative_url);
      target.set_track_pixel_params(source.track_pixel_params);
      target.set_click_params(source.click_params);
      target.set_mime_format(source.mime_format);
      target.set_iurl(source.iurl);
      target.set_test_request(source.test_request);
      for(const auto& creative : source.selected_creatives)
      {
        pack_creative_select_result(creative, *target.add_selected_creatives());
      }
      pack_strings(
        source.external_visual_categories,
        target.mutable_external_visual_categories());
      pack_strings(
        source.external_content_categories,
        target.mutable_external_content_categories());
      target.set_pub_currency_code(source.pub_currency_code);
      target.set_overlay_width(source.overlay_width);
      target.set_overlay_height(source.overlay_height);
      pack_tokens(source.tokens, target.mutable_tokens());
      pack_tokens(source.ext_tokens, target.mutable_ext_tokens());
      target.set_track_impr(source.track_impr);
      target.set_tag_size(source.tag_size);
      pack_ids(source.freq_caps, target.mutable_freq_caps());
      pack_ids(source.uc_freq_caps, target.mutable_uc_freq_caps());

      auto* debug_info = target.mutable_debug_info();
      debug_info->set_tag_id(source.debug_info.tag_id);
      debug_info->set_tag_size_id(source.debug_info.tag_size_id);
      debug_info->set_site_id(source.debug_info.site_id);
      debug_info->set_site_rate_id(source.debug_info.site_rate_id);
      debug_info->set_min_no_adv_ecpm(source.debug_info.min_no_adv_ecpm);
      debug_info->set_min_text_ecpm(source.debug_info.min_text_ecpm);
      debug_info->set_auction_type(source.debug_info.auction_type);
      debug_info->set_track_pixel_url(source.debug_info.track_pixel_url);
      pack_revenue_decimal(
        source.debug_info.cpm_threshold,
        *debug_info->mutable_cpm_threshold());
      debug_info->set_walled_garden(source.debug_info.walled_garden);
      for(const auto& creative : source.debug_info.selected_creatives)
      {
        pack_creative_select_debug_info(
          creative,
          *debug_info->add_selected_creatives());
      }
      debug_info->set_trace_ccg(source.debug_info.trace_ccg);

      pack_tokens(source.native_data_tokens, target.mutable_native_data_tokens());
      pack_token_images(
        source.native_image_tokens,
        target.mutable_native_image_tokens());
      target.set_track_html_body(source.track_html_body);
      target.set_erid(source.erid);
      for(const auto& contract : source.contracts)
      {
        auto* target_contract = target.add_contracts();
        target_contract->set_parent_contract_id(contract.parent_contract_id);
        target_contract->set_contract_id(contract.contract_info.contract_id);
        target_contract->set_number(contract.contract_info.number);
        target_contract->set_date(contract.contract_info.date);
        target_contract->set_type(contract.contract_info.type);
        target_contract->set_vat_included(contract.contract_info.vat_included);
        target_contract->set_ord_contract_id(
          contract.contract_info.ord_contract_id);
        target_contract->set_ord_ado_id(contract.contract_info.ord_ado_id);
        target_contract->set_subject_type(contract.contract_info.subject_type);
        target_contract->set_action_type(contract.contract_info.action_type);
        target_contract->set_agent_acting_for_publisher(
          contract.contract_info.agent_acting_for_publisher);
        target_contract->set_contract_parent_contract_id(
          contract.contract_info.parent_contract_id);
        target_contract->set_client_id(contract.contract_info.client_id);
        target_contract->set_client_name(contract.contract_info.client_name);
        target_contract->set_client_legal_form(
          contract.contract_info.client_legal_form);
        target_contract->set_contractor_id(
          contract.contract_info.contractor_id);
        target_contract->set_contractor_name(
          contract.contract_info.contractor_name);
        target_contract->set_contractor_legal_form(
          contract.contract_info.contractor_legal_form);
        target_contract->set_timestamp(
          pack_time(contract.contract_info.timestamp));
      }
    }

    template<typename SourceSeq>
    void
    pack_config_ids(
      const SourceSeq& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      for(const auto& id : source)
      {
        target->Add(id);
      }
    }

    template<typename DecimalType>
    void
    pack_decimal_into_config_ids(
      google::protobuf::RepeatedField<google::protobuf::uint64>* target,
      const DecimalType& value)
    {
      const unsigned long EL_NUMBER = DecimalType::PACK_SIZE / 4 +
        (DecimalType::PACK_SIZE % 4 ? 1 : 0);
      uint32_t buf[EL_NUMBER];
      ::memset(buf, 0, EL_NUMBER * 4);
      value.pack(buf);

      target->Add(0);
      for(unsigned long i = 0; i < EL_NUMBER; ++i)
      {
        target->Add(buf[i]);
      }
    }

    void
    pack_config_option_value(
      const OptionValue& source,
      pb::ConfigOptionValue& target)
    {
      target.set_option_id(source.option_id);
      target.set_value(source.value);
    }

    void
    pack_config_option_values(
      const OptionTokenValueMap& source,
      google::protobuf::RepeatedPtrField<pb::ConfigOptionValue>* target)
    {
      for(const auto& item : source)
      {
        auto* value = target->Add();
        value->set_option_id(item.second.option_id);
        value->set_value(item.second.value);
      }
    }

    void
    pack_config_expression(
      const ExpressionChannel::Expression& source,
      pb::ConfigExpression& target)
    {
      target.set_operation(static_cast<unsigned char>(source.op));
      if(source.op == ExpressionChannel::NOP)
      {
        if(source.channel.in() &&
          (source.channel->expression_channel().in() ||
            source.channel->simple_channel().in()))
        {
          target.set_channel_id(source.channel->params().channel_id);
        }
        else
        {
          target.set_channel_id(0);
        }
      }
      else
      {
        for(const auto& sub_channel : source.sub_channels)
        {
          pack_config_expression(sub_channel, *target.add_sub_channels());
        }
      }
    }

    void
    pack_config_delivery_limits(
      const CampaignDeliveryLimits& source,
      pb::ConfigDeliveryLimits& target)
    {
      target.set_date_start(pack_time(source.date_start));
      target.set_date_end(pack_time(source.date_end));
      if(source.budget.present())
      {
        pack_revenue_decimal(*source.budget, *target.mutable_budget());
      }
      if(source.daily_budget.present())
      {
        pack_revenue_decimal(
          *source.daily_budget,
          *target.mutable_daily_budget());
      }
      if(source.imps.present())
      {
        target.set_imps(*source.imps);
      }
      if(source.clicks.present())
      {
        target.set_clicks(*source.clicks);
      }
      target.set_delivery_pacing(
        static_cast<unsigned char>(source.delivery_pacing));
    }

    void
    pack_config_creative(
      const Creative& source,
      pb::ConfigCreative& target)
    {
      target.set_ccid(source.ccid);
      target.set_creative_id(source.creative_id);
      target.set_fc_id(source.fc_id);
      target.set_weight(source.weight);
      for(const auto& src_size : source.sizes)
      {
        auto* dst_size = target.add_sizes();
        dst_size->set_size_id(src_size.first);
        dst_size->set_up_expand_space(src_size.second.up_expand_space);
        dst_size->set_right_expand_space(src_size.second.right_expand_space);
        dst_size->set_down_expand_space(src_size.second.down_expand_space);
        dst_size->set_left_expand_space(src_size.second.left_expand_space);
        pack_config_option_values(
          src_size.second.tokens,
          dst_size->mutable_tokens());
      }
      target.set_creative_format(source.creative_format);
      pack_config_option_value(source.click_url, *target.mutable_click_url());
      target.set_order_set_id(source.order_set_id);
      target.set_initial_contract_id(
        source.initial_contract ? source.initial_contract->contract_id : 0);
      pack_config_ids(source.categories, target.mutable_categories());
      pack_config_option_values(source.tokens, target.mutable_tokens());
      target.set_status(static_cast<unsigned char>(source.status));
      target.set_version_id(source.version_id);
    }

    void
    pack_config_campaign(
      const Campaign& source,
      pb::ConfigCampaign& target)
    {
      target.set_campaign_id(source.campaign_id);
      target.set_campaign_group_id(source.campaign_group_id);
      target.set_ccg_rate_id(source.ccg_rate_id);
      target.set_ccg_rate_type(static_cast<unsigned char>(source.ccg_rate_type));
      target.set_fc_id(source.fc_id);
      target.set_group_fc_id(source.group_fc_id);
      target.set_flags(source.flags);
      target.set_marketplace(static_cast<unsigned char>(source.marketplace));
      pack_config_expression(
        source.channel.in() ?
          source.channel->expression() : ExpressionChannel::Expression::EMPTY,
        *target.mutable_expression());
      pack_config_expression(
        source.stat_channel.in() ?
          source.stat_channel->expression() : ExpressionChannel::Expression::EMPTY,
        *target.mutable_stat_expression());
      target.set_country(source.country);
      pack_config_ids(source.sites, target.mutable_sites());
      target.set_status(static_cast<unsigned char>(source.status));
      target.set_eval_status(static_cast<unsigned char>(source.eval_status));
      for(const auto& interval : source.weekly_run_intervals)
      {
        auto* dst_interval = target.add_weekly_run_intervals();
        dst_interval->set_min(interval.min);
        dst_interval->set_max(interval.max);
      }
      for(const auto& creative : source.get_creatives())
      {
        pack_config_creative(*creative, *target.add_creatives());
      }
      target.set_account_id(source.account->account_id);
      target.set_advertiser_id(source.advertiser->account_id);
      pack_config_ids(
        source.exclude_pub_accounts,
        target.mutable_exclude_pub_accounts());
      for(const auto& src_tag : source.exclude_tags)
      {
        auto* dst_tag = target.add_exclude_tags();
        dst_tag->set_tag_id(src_tag.first);
        dst_tag->set_delivery_value(src_tag.second);
      }
      target.set_delivery_coef(source.delivery_coef);
      pack_revenue_decimal(source.imp_revenue, *target.mutable_imp_revenue());
      pack_revenue_decimal(source.click_revenue, *target.mutable_click_revenue());
      pack_revenue_decimal(
        source.action_revenue,
        *target.mutable_action_revenue());
      pack_revenue_decimal(source.commision, *target.mutable_commision());
      target.set_ccg_type(static_cast<unsigned char>(source.ccg_type));
      target.set_target_type(static_cast<unsigned char>(source.targeting_type));
      pack_config_delivery_limits(
        source.campaign_delivery_limits,
        *target.mutable_campaign_delivery_limits());
      pack_config_delivery_limits(
        source.ccg_delivery_limits,
        *target.mutable_ccg_delivery_limits());
      target.set_start_user_group_id(source.start_user_group_id);
      target.set_end_user_group_id(source.end_user_group_id);
      pack_revenue_decimal(source.max_pub_share, *target.mutable_max_pub_share());
      target.set_ctr_reset_id(source.ctr_reset_id);
      target.set_mode(source.mode);
      target.set_seq_set_rotate_imps(source.seq_set_rotate_imps);
      target.set_min_uid_age(pack_time(source.min_uid_age));
      pack_config_ids(source.colocations, target.mutable_colocations());
      target.set_bid_strategy(source.bid_strategy);
      pack_revenue_decimal(source.min_ctr_goal(), *target.mutable_min_ctr_goal());
      target.set_timestamp(pack_time(source.timestamp));
    }

    void
    pack_config_tag(
      const Tag& source,
      pb::ConfigTag& target)
    {
      target.set_tag_id(source.tag_id);
      target.set_site_id(source.site->site_id);
      target.set_status(static_cast<unsigned char>(source.site->status));
      for(const auto& src_size : source.sizes)
      {
        auto* dst_size = target.add_sizes();
        dst_size->set_size_id(src_size.first);
        dst_size->set_max_text_creatives(src_size.second->max_text_creatives);
        pack_config_option_values(
          src_size.second->tokens,
          dst_size->mutable_tokens());
      }
      target.set_imp_track_pixel(source.imp_track_pixel);
      target.set_passback(source.passback);
      target.set_passback_type(source.passback_type);
      target.set_flags(source.flags);
      target.set_marketplace(static_cast<unsigned char>(source.marketplace));
      pack_revenue_decimal(source.adjustment, *target.mutable_adjustment());
      for(const auto& src_pricing : source.tag_pricings)
      {
        auto* dst_pricing = target.add_tag_pricings();
        dst_pricing->set_country_code(src_pricing.first.country_code);
        dst_pricing->set_ccg_type(
          static_cast<unsigned char>(src_pricing.first.ccg_type));
        dst_pricing->set_ccg_rate_type(
          static_cast<unsigned char>(src_pricing.first.ccg_rate_type));
        dst_pricing->set_site_rate_id(src_pricing.second.site_rate_id);
        pack_revenue_decimal(
          src_pricing.second.imp_revenue,
          *dst_pricing->mutable_imp_revenue());
        pack_revenue_decimal(
          src_pricing.second.revenue_share,
          *dst_pricing->mutable_revenue_share());
      }
      pack_config_ids(source.accepted_categories, target.mutable_accepted_categories());
      pack_config_ids(source.rejected_categories, target.mutable_rejected_categories());
      target.set_allow_expandable(source.allow_expandable);
      pack_config_option_values(source.tokens, target.mutable_tokens());
      pack_config_option_values(
        source.hidden_tokens,
        target.mutable_hidden_tokens());
      pack_config_option_values(
        source.passback_tokens,
        target.mutable_passback_tokens());
      for(const auto& src_template : source.template_tokens)
      {
        auto* dst_template = target.add_template_tokens();
        dst_template->set_template_name(src_template.first);
        pack_config_option_values(
          src_template.second,
          dst_template->mutable_tokens());
      }
      pack_revenue_decimal(
        source.auction_max_ecpm_share,
        *target.mutable_auction_max_ecpm_share());
      pack_revenue_decimal(
        source.auction_prop_probability_share,
        *target.mutable_auction_prop_probability_share());
      pack_revenue_decimal(
        source.auction_random_share,
        *target.mutable_auction_random_share());
      pack_revenue_decimal(source.cost_coef, *target.mutable_cost_coef());
      target.set_tag_pricings_timestamp(pack_time(source.tag_pricings_timestamp));
      target.set_timestamp(pack_time(source.timestamp));
    }

    CreativeTemplateType
    adopt_config_template_type(CreativeTemplateFactory::Handler::Type type)
    {
      if(type == CreativeTemplateFactory::Handler::CTT_TEXT)
      {
        return CTT_TEXT;
      }
      if(type == CreativeTemplateFactory::Handler::CTT_XSLT)
      {
        return CTT_XSLT;
      }
      Stream::Error ostr;
      ostr << "Unknown template type: " << type;
      throw CampaignManagerCore::Exception(ostr);
    }

    void
    pack_config_expression_channel(
      const ExpressionChannelBase& source,
      pb::ConfigExpressionChannel& target)
    {
      const ChannelParams& params = source.params();
      target.set_channel_id(params.channel_id);
      target.set_type(static_cast<unsigned char>(params.type));
      target.set_country_code(params.country);
      target.set_status(static_cast<unsigned char>(params.status));
      target.set_action_id(params.action_id);
      target.set_timestamp(pack_time(params.timestamp));

      if(params.common_params.in())
      {
        target.set_account_id(params.common_params->account_id);
        target.set_flags(params.common_params->flags);
        target.set_is_public(params.common_params->is_public);
        target.set_freq_cap_id(params.common_params->freq_cap_id);
        target.set_language(params.common_params->language);
      }
      else
      {
        target.set_is_public(true);
      }

      if(params.descriptive_params.in())
      {
        target.set_name(params.descriptive_params->name);
        target.set_parent_channel_id(
          params.descriptive_params->parent_channel_id);
      }

      if(params.discover_params.in())
      {
        target.set_discover_query(params.discover_params->query);
        target.set_discover_annotation(params.discover_params->annotation);
      }

      if(params.cmp_params.in())
      {
        target.set_channel_rate_id(params.cmp_params->channel_rate_id);
        pack_revenue_decimal(
          params.cmp_params->imp_revenue,
          *target.mutable_imp_revenue());
        pack_revenue_decimal(
          params.cmp_params->click_revenue,
          *target.mutable_click_revenue());
      }

      ConstSimpleChannel_var simple_channel = source.simple_channel();
      if(simple_channel.in())
      {
        target.mutable_expression()->set_operation('S');
      }
      else
      {
        ConstExpressionChannel_var expression_channel =
          source.expression_channel();
        assert(expression_channel.in());
        pack_config_expression(
          expression_channel->expression(),
          *target.mutable_expression());
      }
    }

    void
    pack_config_contract(
      const Contract& source,
      pb::ConfigContract& target)
    {
      target.set_contract_id(source.contract_id);
      target.set_number(source.number);
      target.set_date(source.date);
      target.set_type(source.type);
      target.set_vat_included(source.vat_included);
      target.set_ord_contract_id(source.ord_contract_id);
      target.set_ord_ado_id(source.ord_ado_id);
      target.set_subject_type(source.subject_type);
      target.set_action_type(source.action_type);
      target.set_agent_acting_for_publisher(source.agent_acting_for_publisher);
      target.set_parent_contract_id(
        source.parent_contract ? source.parent_contract->contract_id : 0);
      target.set_client_id(source.client_id);
      target.set_client_name(source.client_name);
      target.set_client_legal_form(source.client_legal_form);
      target.set_contractor_id(source.contractor_id);
      target.set_contractor_name(source.contractor_name);
      target.set_contractor_legal_form(source.contractor_legal_form);
      target.set_timestamp(pack_time(source.timestamp));
    }

    void
    pack_config(
      const CampaignConfig& source,
      bool geo_channels,
      pb::CampaignConfig& target)
    {
      for(const auto& src : source.app_formats)
      {
        auto* item = target.add_app_formats();
        item->set_app_format(src.first);
        item->set_mime_format(src.second.mime_format);
        item->set_timestamp(pack_time(src.second.timestamp));
      }
      for(const auto& src : source.sizes)
      {
        auto* item = target.add_sizes();
        item->set_size_id(src.first);
        item->set_protocol_name(src.second->protocol_name);
        item->set_size_type_id(src.second->size_type_id);
        item->set_width(src.second->width);
        item->set_height(src.second->height);
        item->set_timestamp(pack_time(src.second->timestamp));
      }
      for(const auto& src : source.accounts)
      {
        const auto& account = *src.second;
        auto* item = target.add_accounts();
        item->set_account_id(src.first);
        item->set_agency_account_id(
          account.agency_account ? account.agency_account->account_id : 0);
        item->set_internal_account_id(account.internal_account_id);
        item->set_legal_name(account.legal_name);
        item->set_flags(account.flags);
        item->set_at_flags(account.at_flags);
        item->set_text_adserving(static_cast<unsigned char>(account.text_adserving));
        item->set_currency_id(account.currency->currency_id);
        item->set_country(account.country);
        item->set_time_offset(pack_time(account.time_offset));
        pack_revenue_decimal(account.commision, *item->mutable_commision());
        pack_revenue_decimal(account.budget, *item->mutable_budget());
        pack_revenue_decimal(account.paid_amount, *item->mutable_paid_amount());
        pack_config_ids(
          account.walled_garden_accounts,
          item->mutable_walled_garden_accounts());
        item->set_auction_rate(static_cast<unsigned long>(account.auction_rate));
        item->set_use_pub_pixels(account.use_pub_pixels);
        item->set_pub_pixel_optin(account.pub_pixel_optin);
        item->set_pub_pixel_optout(account.pub_pixel_optout);
        pack_revenue_decimal(
          account.self_service_commission,
          *item->mutable_self_service_commission());
        item->set_status(static_cast<unsigned char>(account.status));
        item->set_eval_status(static_cast<unsigned char>(account.eval_status));
        item->set_timestamp(pack_time(account.timestamp));
      }
      for(const auto& src : source.creative_options)
      {
        auto* item = target.add_creative_options();
        item->set_option_id(src.first);
        item->set_token(src.second.token);
        item->set_type(static_cast<unsigned char>(src.second.type));
        for(const auto& relation : src.second.token_relations)
        {
          *item->add_token_relations() = relation;
        }
        item->set_timestamp(pack_time(src.second.timestamp));
      }
      for(const auto& src : source.campaigns)
      {
        auto* item = target.add_campaigns();
        pack_config_campaign(*src.second, *item->mutable_info());
        pack_config_expression(
          src.second->channel.in() ?
            src.second->channel->expression() : ExpressionChannel::Expression::EMPTY,
          *item->mutable_expression());
        pack_revenue_decimal(src.second->ecpm_, *item->mutable_ecpm());
        pack_revenue_decimal(src.second->ctr, *item->mutable_ctr());
      }
      for(const auto& src : source.sites)
      {
        const auto& site = *src.second;
        auto* item = target.add_sites();
        item->set_site_id(src.first);
        item->set_status(static_cast<unsigned char>(site.status));
        item->set_freq_cap_id(site.freq_cap_id);
        item->set_noads_timeout(site.noads_timeout);
        pack_config_ids(
          site.approved_creative_categories,
          item->mutable_approved_creative_categories());
        pack_config_ids(
          site.rejected_creative_categories,
          item->mutable_rejected_creative_categories());
        pack_config_ids(site.approved_creatives, item->mutable_approved_creatives());
        pack_config_ids(site.rejected_creatives, item->mutable_rejected_creatives());
        item->set_flags(site.flags);
        item->set_account_id(site.account->account_id);
        item->set_timestamp(pack_time(site.timestamp));
      }
      for(const auto& src : source.tags)
      {
        auto* item = target.add_tags();
        pack_config_tag(*src.second, *item->mutable_info());
        for(const auto& pricing : src.second->tag_pricings)
        {
          pack_revenue_decimal(pricing.second.cpm, *item->add_cpms());
        }
      }
      for(const auto& src : source.currencies)
      {
        const auto& currency = *src.second;
        auto* item = target.add_currencies();
        pack_revenue_decimal(currency.rate, *item->mutable_rate());
        item->set_currency_id(src.first);
        item->set_currency_exchange_id(currency.currency_exchange_id);
        item->set_effective_date(currency.effective_date);
        item->set_fraction_digits(currency.fraction);
        item->set_currency_code(currency.currency_code);
        item->set_timestamp(pack_time(currency.timestamp));
      }
      for(const auto& src : source.colocations)
      {
        const auto& colocation = *src.second;
        auto* item = target.add_colocations();
        item->set_colo_id(src.first);
        item->set_colo_name(colocation.colo_name);
        item->set_colo_rate_id(colocation.colo_rate_id);
        item->set_at_flags(colocation.at_flags);
        item->set_ad_serving(colocation.ad_serving);
        item->set_hid_profile(colocation.hid_profile);
        item->set_account_id(colocation.account->account_id);
        pack_revenue_decimal(
          colocation.revenue_share,
          *item->mutable_revenue_share());
        pack_config_option_values(colocation.tokens, item->mutable_tokens());
        item->set_timestamp(pack_time(colocation.timestamp));
      }
      for(const auto& src : source.countries)
      {
        auto* item = target.add_countries();
        item->set_country_code(src.first);
        pack_config_option_values(src.second->tokens, item->mutable_tokens());
        item->set_timestamp(pack_time(src.second->timestamp));
      }
      for(const auto& src : source.freq_caps)
      {
        const auto& freq_cap = src.second;
        auto* item = target.add_frequency_caps();
        item->set_fc_id(freq_cap.fc_id);
        item->set_lifelimit(freq_cap.lifelimit);
        item->set_period(freq_cap.period.tv_sec);
        item->set_window_limit(freq_cap.window_limit);
        item->set_window_time(freq_cap.window_time.tv_sec);
        item->set_timestamp(pack_time(freq_cap.timestamp));
      }
      for(const auto& src : source.creative_templates)
      {
        auto* item = target.add_creative_template_files();
        item->set_creative_format(src.first.creative_format);
        item->set_creative_size(src.first.creative_size);
        item->set_app_format(src.first.app_format);
        item->set_mime_format(src.second.mime_format);
        item->set_track_impr(src.second.track_impressions);
        item->set_type(adopt_config_template_type(src.second.type));
        item->set_template_file(src.second.file);
        item->set_timestamp(pack_time(src.second.timestamp));
        pack_config_option_values(*src.second.tokens, item->mutable_tokens());
        pack_config_option_values(
          *src.second.hidden_tokens,
          item->mutable_hidden_tokens());
        item->set_status(static_cast<unsigned char>(src.second.status));
      }
      for(const auto& src : source.ccg_keyword_click_info_map)
      {
        auto* item = target.add_campaign_keywords();
        item->set_ccg_keyword_id(src.first);
        item->set_original_keyword(src.second.original_keyword);
        item->set_click_url(src.second.click_url);
        item->set_timestamp(pack_time(Generics::Time::ZERO));
      }
      for(const auto& src : source.expression_channels)
      {
        if(src.second->channel.in())
        {
          pack_config_expression_channel(
            *src.second->channel,
            *target.add_expression_channels());
        }
      }
      for(const auto& src : source.creative_categories)
      {
        auto* item = target.add_creative_categories();
        item->set_creative_category_id(src.first);
        item->set_cct_id(src.second.cct_id);
        item->set_name(src.second.name);
        for(const auto& src_category : src.second.external_categories)
        {
          auto* external_category = item->add_external_categories();
          external_category->set_ad_request_type(src_category.first);
          for(const auto& name : src_category.second)
          {
            *external_category->add_names() = name;
          }
        }
        item->set_timestamp(pack_time(src.second.timestamp));
      }
      for(const auto& src : source.adv_actions)
      {
        auto* item = target.add_adv_actions();
        item->set_action_id(src.second.action_id);
        item->set_timestamp(pack_time(src.second.timestamp));
        pack_config_ids(src.second.ccg_ids, item->mutable_ccg_ids());
        pack_decimal_into_config_ids(
          item->mutable_ccg_ids(),
          src.second.cur_value);
      }
      for(const auto& src : source.category_channels)
      {
        const auto& category_channel = *src.second;
        auto* item = target.add_category_channels();
        item->set_channel_id(src.first);
        item->set_name(category_channel.name);
        item->set_newsgate_name(category_channel.newsgate_name);
        for(const auto& localization : category_channel.localizations)
        {
          auto* loc = item->add_localizations();
          loc->set_language(localization.first);
          loc->set_name(localization.second);
        }
        item->set_parent_channel_id(category_channel.parent_channel_id);
        item->set_flags(category_channel.flags);
        item->set_timestamp(pack_time(category_channel.timestamp));
      }
      if(geo_channels)
      {
        for(const auto& src : source.geo_channels->channels())
        {
          auto* item = target.add_geo_channels();
          item->set_channel_id(src.second);
          item->set_country(src.first.country());
          auto* geoip_target = item->add_geoip_targets();
          geoip_target->set_region(src.first.region());
          geoip_target->set_city(src.first.city());
          item->set_timestamp(pack_time(Generics::Time::ZERO));
        }
        for(const auto& src : source.geo_coord_channels->channels())
        {
          for(const auto channel_id : src.second->channels)
          {
            auto* item = target.add_geo_coord_channels();
            item->set_channel_id(channel_id);
            pack_revenue_decimal(src.first.longitude, *item->mutable_longitude());
            pack_revenue_decimal(src.first.latitude, *item->mutable_latitude());
            pack_revenue_decimal(src.first.accuracy, *item->mutable_radius());
            item->set_timestamp(pack_time(Generics::Time::ZERO));
          }
        }
      }
      for(WebOperationHash::const_iterator it = source.web_operations.begin();
        it != source.web_operations.end(); ++it)
      {
        auto* item = target.add_web_operations();
        item->set_id(it->second->id);
        item->set_app(it->second->app);
        item->set_source(it->second->source);
        item->set_operation(it->second->operation);
        item->set_flags(it->second->flags);
        item->set_timestamp(pack_time(Generics::Time::ZERO));
      }
      for(const auto& src : source.contracts)
      {
        pack_config_contract(*src.second, *target.add_contracts());
      }
      target.set_currency_exchange_id(source.currency_exchange_id);
      target.set_fraud_user_deactivate_period(
        pack_time(source.fraud_user_deactivate_period));
      pack_revenue_decimal(source.cost_limit, *target.mutable_cost_limit());
      target.set_google_publisher_account_id(source.google_publisher_account_id);
      target.set_master_stamp(pack_time(source.master_stamp));
      target.set_first_load_stamp(pack_time(source.first_load_stamp));
      target.set_finish_load_stamp(pack_time(source.finish_load_stamp));
      target.set_global_params_timestamp(pack_time(source.global_params_timestamp));
    }

  }

  struct CampaignManagerGrpc::AtomicStats
  {
    std::atomic<std::uint64_t> call_in_progress{0};
    std::atomic<std::uint64_t> call_total{0};
    std::atomic<std::uint64_t> call_time{0};
    std::atomic<std::uint64_t> ready_in_progress{0};
    std::atomic<std::uint64_t> ready_total{0};
    std::atomic<std::uint64_t> ready_time{0};
    std::atomic<std::uint64_t> progress_comment_in_progress{0};
    std::atomic<std::uint64_t> progress_comment_total{0};
    std::atomic<std::uint64_t> progress_comment_time{0};
    std::atomic<std::uint64_t> match_geo_channels_in_progress{0};
    std::atomic<std::uint64_t> match_geo_channels_total{0};
    std::atomic<std::uint64_t> match_geo_channels_time{0};
    std::atomic<std::uint64_t> get_file_in_progress{0};
    std::atomic<std::uint64_t> get_file_total{0};
    std::atomic<std::uint64_t> get_file_time{0};
    std::atomic<std::uint64_t> get_campaign_creative_in_progress{0};
    std::atomic<std::uint64_t> get_campaign_creative_total{0};
    std::atomic<std::uint64_t> get_campaign_creative_time{0};
    std::atomic<std::uint64_t> process_match_request_in_progress{0};
    std::atomic<std::uint64_t> process_match_request_total{0};
    std::atomic<std::uint64_t> process_match_request_time{0};
    std::atomic<std::uint64_t> process_anonymous_request_in_progress{0};
    std::atomic<std::uint64_t> process_anonymous_request_total{0};
    std::atomic<std::uint64_t> process_anonymous_request_time{0};
    std::atomic<std::uint64_t> instantiate_ad_in_progress{0};
    std::atomic<std::uint64_t> instantiate_ad_total{0};
    std::atomic<std::uint64_t> instantiate_ad_time{0};
    std::atomic<std::uint64_t> trace_campaign_selection_index_in_progress{0};
    std::atomic<std::uint64_t> trace_campaign_selection_index_total{0};
    std::atomic<std::uint64_t> trace_campaign_selection_index_time{0};
    std::atomic<std::uint64_t> trace_campaign_selection_in_progress{0};
    std::atomic<std::uint64_t> trace_campaign_selection_total{0};
    std::atomic<std::uint64_t> trace_campaign_selection_time{0};
    std::atomic<std::uint64_t> get_campaign_creative_by_ccid_in_progress{0};
    std::atomic<std::uint64_t> get_campaign_creative_by_ccid_total{0};
    std::atomic<std::uint64_t> get_campaign_creative_by_ccid_time{0};
    std::atomic<std::uint64_t> get_channel_links_in_progress{0};
    std::atomic<std::uint64_t> get_channel_links_total{0};
    std::atomic<std::uint64_t> get_channel_links_time{0};
    std::atomic<std::uint64_t> get_discover_channels_in_progress{0};
    std::atomic<std::uint64_t> get_discover_channels_total{0};
    std::atomic<std::uint64_t> get_discover_channels_time{0};
    std::atomic<std::uint64_t> get_category_channels_in_progress{0};
    std::atomic<std::uint64_t> get_category_channels_total{0};
    std::atomic<std::uint64_t> get_category_channels_time{0};
    std::atomic<std::uint64_t> get_colocation_flags_in_progress{0};
    std::atomic<std::uint64_t> get_colocation_flags_total{0};
    std::atomic<std::uint64_t> get_colocation_flags_time{0};
    std::atomic<std::uint64_t> get_pub_pixels_in_progress{0};
    std::atomic<std::uint64_t> get_pub_pixels_total{0};
    std::atomic<std::uint64_t> get_pub_pixels_time{0};
    std::atomic<std::uint64_t> consider_passback_in_progress{0};
    std::atomic<std::uint64_t> consider_passback_total{0};
    std::atomic<std::uint64_t> consider_passback_time{0};
    std::atomic<std::uint64_t> consider_passback_track_in_progress{0};
    std::atomic<std::uint64_t> consider_passback_track_total{0};
    std::atomic<std::uint64_t> consider_passback_track_time{0};
    std::atomic<std::uint64_t> get_click_url_in_progress{0};
    std::atomic<std::uint64_t> get_click_url_total{0};
    std::atomic<std::uint64_t> get_click_url_time{0};
    std::atomic<std::uint64_t> verify_impression_in_progress{0};
    std::atomic<std::uint64_t> verify_impression_total{0};
    std::atomic<std::uint64_t> verify_impression_time{0};
    std::atomic<std::uint64_t> action_taken_in_progress{0};
    std::atomic<std::uint64_t> action_taken_total{0};
    std::atomic<std::uint64_t> action_taken_time{0};
    std::atomic<std::uint64_t> verify_opt_operation_in_progress{0};
    std::atomic<std::uint64_t> verify_opt_operation_total{0};
    std::atomic<std::uint64_t> verify_opt_operation_time{0};
    std::atomic<std::uint64_t> consider_web_operation_in_progress{0};
    std::atomic<std::uint64_t> consider_web_operation_total{0};
    std::atomic<std::uint64_t> consider_web_operation_time{0};
    std::atomic<std::uint64_t> get_config_in_progress{0};
    std::atomic<std::uint64_t> get_config_total{0};
    std::atomic<std::uint64_t> get_config_time{0};
  };

  class CampaignManagerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      CampaignManagerGrpc::ServiceImpl,
      pb::CampaignManagerGrpc,
      pb::CampaignManagerGrpc::AsyncService>
  {
    using AsyncService = pb::CampaignManagerGrpc::AsyncService;

  public:
    ServiceImpl(
      CampaignManagerCore* core,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      std::shared_ptr<AtomicStats> stats);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CORO_CALL(
          pb::ReadyRequest,
          pb::ReadyResponse,
          ready,
          co_ready),
        MAKE_GRPC_CORO_CALL(
          pb::ProgressCommentRequest,
          pb::ProgressCommentResponse,
          progress_comment,
          co_progress_comment),
        MAKE_GRPC_CORO_CALL(
          pb::MatchGeoChannelsRequest,
          pb::MatchGeoChannelsResponse,
          match_geo_channels,
          co_match_geo_channels),
        MAKE_GRPC_CORO_CALL(
          pb::GetFileRequest,
          pb::GetFileResponse,
          get_file,
          co_get_file),
        MAKE_GRPC_CORO_CALL(
          pb::GetCampaignCreativeRequest,
          pb::GetCampaignCreativeResponse,
          get_campaign_creative,
          co_get_campaign_creative),
        MAKE_GRPC_CORO_CALL(
          pb::ProcessMatchRequestRequest,
          pb::ProcessMatchRequestResponse,
          process_match_request,
          co_process_match_request),
        MAKE_GRPC_CORO_CALL(
          pb::ProcessAnonymousRequestRequest,
          pb::ProcessAnonymousRequestResponse,
          process_anonymous_request,
          co_process_anonymous_request),
        MAKE_GRPC_CORO_CALL(
          pb::InstantiateAdRequest,
          pb::InstantiateAdResponse,
          instantiate_ad,
          co_instantiate_ad),
        MAKE_GRPC_CORO_CALL(
          pb::TraceCampaignSelectionIndexRequest,
          pb::TraceCampaignSelectionIndexResponse,
          trace_campaign_selection_index,
          co_trace_campaign_selection_index),
        MAKE_GRPC_CORO_CALL(
          pb::TraceCampaignSelectionRequest,
          pb::TraceCampaignSelectionResponse,
          trace_campaign_selection,
          co_trace_campaign_selection),
        MAKE_GRPC_CORO_CALL(
          pb::GetCampaignCreativeByCcidRequest,
          pb::GetCampaignCreativeByCcidResponse,
          get_campaign_creative_by_ccid,
          co_get_campaign_creative_by_ccid),
        MAKE_GRPC_CORO_CALL(
          pb::GetChannelLinksRequest,
          pb::GetChannelLinksResponse,
          get_channel_links,
          co_get_channel_links),
        MAKE_GRPC_CORO_CALL(
          pb::GetDiscoverChannelsRequest,
          pb::GetDiscoverChannelsResponse,
          get_discover_channels,
          co_get_discover_channels),
        MAKE_GRPC_CORO_CALL(
          pb::GetCategoryChannelsRequest,
          pb::GetCategoryChannelsResponse,
          get_category_channels,
          co_get_category_channels),
        MAKE_GRPC_CORO_CALL(
          pb::GetColocationFlagsRequest,
          pb::GetColocationFlagsResponse,
          get_colocation_flags,
          co_get_colocation_flags),
        MAKE_GRPC_CORO_CALL(
          pb::GetPubPixelsRequest,
          pb::GetPubPixelsResponse,
          get_pub_pixels,
          co_get_pub_pixels),
        MAKE_GRPC_CORO_CALL(
          pb::ConsiderPassbackRequest,
          pb::ConsiderPassbackResponse,
          consider_passback,
          co_consider_passback),
        MAKE_GRPC_CORO_CALL(
          pb::ConsiderPassbackTrackRequest,
          pb::ConsiderPassbackTrackResponse,
          consider_passback_track,
          co_consider_passback_track),
        MAKE_GRPC_CORO_CALL(
          pb::GetClickUrlRequest,
          pb::GetClickUrlResponse,
          get_click_url,
          co_get_click_url),
        MAKE_GRPC_CORO_CALL(
          pb::VerifyImpressionRequest,
          pb::VerifyImpressionResponse,
          verify_impression,
          co_verify_impression),
        MAKE_GRPC_CORO_CALL(
          pb::ActionTakenRequest,
          pb::ActionTakenResponse,
          action_taken,
          co_action_taken),
        MAKE_GRPC_CORO_CALL(
          pb::VerifyOptOperationRequest,
          pb::VerifyOptOperationResponse,
          verify_opt_operation,
          co_verify_opt_operation),
        MAKE_GRPC_CORO_CALL(
          pb::ConsiderWebOperationRequest,
          pb::ConsiderWebOperationResponse,
          consider_web_operation,
          co_consider_web_operation),
        MAKE_GRPC_CORO_CALL(
          pb::GetConfigRequest,
          pb::GetConfigResponse,
          get_config,
          co_get_config));
    }

    AdServer::Grpc::GrpcCoroutine co_ready(
      const pb::ReadyRequest& request,
      pb::ReadyResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_progress_comment(
      const pb::ProgressCommentRequest& request,
      pb::ProgressCommentResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_match_geo_channels(
      const pb::MatchGeoChannelsRequest& request,
      pb::MatchGeoChannelsResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_file(
      const pb::GetFileRequest& request,
      pb::GetFileResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_campaign_creative(
      const pb::GetCampaignCreativeRequest& request,
      pb::GetCampaignCreativeResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_process_match_request(
      const pb::ProcessMatchRequestRequest& request,
      pb::ProcessMatchRequestResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_process_anonymous_request(
      const pb::ProcessAnonymousRequestRequest& request,
      pb::ProcessAnonymousRequestResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_instantiate_ad(
      const pb::InstantiateAdRequest& request,
      pb::InstantiateAdResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_trace_campaign_selection_index(
      const pb::TraceCampaignSelectionIndexRequest& request,
      pb::TraceCampaignSelectionIndexResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_trace_campaign_selection(
      const pb::TraceCampaignSelectionRequest& request,
      pb::TraceCampaignSelectionResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_campaign_creative_by_ccid(
      const pb::GetCampaignCreativeByCcidRequest& request,
      pb::GetCampaignCreativeByCcidResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_channel_links(
      const pb::GetChannelLinksRequest& request,
      pb::GetChannelLinksResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_discover_channels(
      const pb::GetDiscoverChannelsRequest& request,
      pb::GetDiscoverChannelsResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_category_channels(
      const pb::GetCategoryChannelsRequest& request,
      pb::GetCategoryChannelsResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_colocation_flags(
      const pb::GetColocationFlagsRequest& request,
      pb::GetColocationFlagsResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_pub_pixels(
      const pb::GetPubPixelsRequest& request,
      pb::GetPubPixelsResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_consider_passback(
      const pb::ConsiderPassbackRequest& request,
      pb::ConsiderPassbackResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_consider_passback_track(
      const pb::ConsiderPassbackTrackRequest& request,
      pb::ConsiderPassbackTrackResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_click_url(
      const pb::GetClickUrlRequest& request,
      pb::GetClickUrlResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_verify_impression(
      const pb::VerifyImpressionRequest& request,
      pb::VerifyImpressionResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_action_taken(
      const pb::ActionTakenRequest& request,
      pb::ActionTakenResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_verify_opt_operation(
      const pb::VerifyOptOperationRequest& request,
      pb::VerifyOptOperationResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_consider_web_operation(
      const pb::ConsiderWebOperationRequest& request,
      pb::ConsiderWebOperationResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_config(
      const pb::GetConfigRequest& request,
      pb::GetConfigResponse& response,
      ::grpc::Status& result_status) const;

  private:
    void ready(
      const pb::ReadyRequest& request,
      pb::ReadyResponse& response,
      ::grpc::Status& result_status) const;

    void progress_comment(
      const pb::ProgressCommentRequest& request,
      pb::ProgressCommentResponse& response,
      ::grpc::Status& result_status) const;

    void match_geo_channels(
      const pb::MatchGeoChannelsRequest& request,
      pb::MatchGeoChannelsResponse& response,
      ::grpc::Status& result_status) const;

    void get_file(
      const pb::GetFileRequest& request,
      pb::GetFileResponse& response,
      ::grpc::Status& result_status) const;

    void process_match_request(
      const pb::ProcessMatchRequestRequest& request,
      pb::ProcessMatchRequestResponse& response,
      ::grpc::Status& result_status) const;

    void process_anonymous_request(
      const pb::ProcessAnonymousRequestRequest& request,
      pb::ProcessAnonymousRequestResponse& response,
      ::grpc::Status& result_status) const;

    void trace_campaign_selection_index(
      const pb::TraceCampaignSelectionIndexRequest& request,
      pb::TraceCampaignSelectionIndexResponse& response,
      ::grpc::Status& result_status) const;

    void trace_campaign_selection(
      const pb::TraceCampaignSelectionRequest& request,
      pb::TraceCampaignSelectionResponse& response,
      ::grpc::Status& result_status) const;

    void get_campaign_creative_by_ccid(
      const pb::GetCampaignCreativeByCcidRequest& request,
      pb::GetCampaignCreativeByCcidResponse& response,
      ::grpc::Status& result_status) const;

    void get_channel_links(
      const pb::GetChannelLinksRequest& request,
      pb::GetChannelLinksResponse& response,
      ::grpc::Status& result_status) const;

    void get_discover_channels(
      const pb::GetDiscoverChannelsRequest& request,
      pb::GetDiscoverChannelsResponse& response,
      ::grpc::Status& result_status) const;

    void get_category_channels(
      const pb::GetCategoryChannelsRequest& request,
      pb::GetCategoryChannelsResponse& response,
      ::grpc::Status& result_status) const;

    void get_colocation_flags(
      const pb::GetColocationFlagsRequest& request,
      pb::GetColocationFlagsResponse& response,
      ::grpc::Status& result_status) const;

    void get_pub_pixels(
      const pb::GetPubPixelsRequest& request,
      pb::GetPubPixelsResponse& response,
      ::grpc::Status& result_status) const;

    void consider_passback(
      const pb::ConsiderPassbackRequest& request,
      pb::ConsiderPassbackResponse& response,
      ::grpc::Status& result_status) const;

    void consider_passback_track(
      const pb::ConsiderPassbackTrackRequest& request,
      pb::ConsiderPassbackTrackResponse& response,
      ::grpc::Status& result_status) const;

    void action_taken(
      const pb::ActionTakenRequest& request,
      pb::ActionTakenResponse& response,
      ::grpc::Status& result_status) const;

    void verify_opt_operation(
      const pb::VerifyOptOperationRequest& request,
      pb::VerifyOptOperationResponse& response,
      ::grpc::Status& result_status) const;

    void consider_web_operation(
      const pb::ConsiderWebOperationRequest& request,
      pb::ConsiderWebOperationResponse& response,
      ::grpc::Status& result_status) const;

    void get_config(
      const pb::GetConfigRequest& request,
      pb::GetConfigResponse& response,
      ::grpc::Status& result_status) const;

    class ProcessControlService final:
      public pc::ProcessControl::Service
    {
    public:
      explicit ProcessControlService(const ServiceImpl& owner);

      ::grpc::Status get_status(
        ::grpc::ServerContext* context,
        const pc::GetStatusRequest* request,
        pc::GetStatusResponse* response) override;

    private:
      const ServiceImpl& owner_;
    };

    void get_status_(pc::GetStatusResponse& response) const;

  private:
    ProcessControlService process_control_service_;
    CampaignManagerCore_var core_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::shared_ptr<AtomicStats> stats_;
  };

  CampaignManagerGrpc::ServiceImpl::ProcessControlService::
  ProcessControlService(const ServiceImpl& owner)
    : owner_(owner)
  {}

  ::grpc::Status
  CampaignManagerGrpc::ServiceImpl::ProcessControlService::get_status(
    ::grpc::ServerContext*,
    const pc::GetStatusRequest*,
    pc::GetStatusResponse* response)
  {
    owner_.get_status_(*response);
    return ::grpc::Status::OK;
  }

  CampaignManagerGrpc::ServiceImpl::ServiceImpl(
    CampaignManagerCore* core,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    std::shared_ptr<AtomicStats> stats)
    : process_control_service_(*this),
      core_(ReferenceCounting::add_ref(core)),
      executor_pool_(std::move(executor_pool)),
      stats_(std::move(stats))
  {
    add_grpc_service(&process_control_service_);
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_status_(
    pc::GetStatusResponse& response) const
  {
    try
    {
      const bool ready = core_->ready();
      response.set_ready(ready);
      std::string comment;
      core_->progress_comment(comment);
      response.set_description(std::move(comment));
    }
    catch(const eh::Exception& ex)
    {
      response.set_ready(false);
      response.set_description(ex.what());
    }
  }

#define DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(MethodName, RequestType, ResponseType) \
  AdServer::Grpc::GrpcCoroutine \
  CampaignManagerGrpc::ServiceImpl::co_##MethodName( \
    const RequestType& request, \
    ResponseType& response, \
    ::grpc::Status& result_status) const \
  { \
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_); \
    MethodName(request, response, result_status); \
    co_return; \
  }

  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    ready,
    pb::ReadyRequest,
    pb::ReadyResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    progress_comment,
    pb::ProgressCommentRequest,
    pb::ProgressCommentResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    match_geo_channels,
    pb::MatchGeoChannelsRequest,
    pb::MatchGeoChannelsResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_file,
    pb::GetFileRequest,
    pb::GetFileResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    process_match_request,
    pb::ProcessMatchRequestRequest,
    pb::ProcessMatchRequestResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    process_anonymous_request,
    pb::ProcessAnonymousRequestRequest,
    pb::ProcessAnonymousRequestResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    trace_campaign_selection_index,
    pb::TraceCampaignSelectionIndexRequest,
    pb::TraceCampaignSelectionIndexResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    trace_campaign_selection,
    pb::TraceCampaignSelectionRequest,
    pb::TraceCampaignSelectionResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_campaign_creative_by_ccid,
    pb::GetCampaignCreativeByCcidRequest,
    pb::GetCampaignCreativeByCcidResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_channel_links,
    pb::GetChannelLinksRequest,
    pb::GetChannelLinksResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_discover_channels,
    pb::GetDiscoverChannelsRequest,
    pb::GetDiscoverChannelsResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_category_channels,
    pb::GetCategoryChannelsRequest,
    pb::GetCategoryChannelsResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_colocation_flags,
    pb::GetColocationFlagsRequest,
    pb::GetColocationFlagsResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_pub_pixels,
    pb::GetPubPixelsRequest,
    pb::GetPubPixelsResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    consider_passback,
    pb::ConsiderPassbackRequest,
    pb::ConsiderPassbackResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    consider_passback_track,
    pb::ConsiderPassbackTrackRequest,
    pb::ConsiderPassbackTrackResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    action_taken,
    pb::ActionTakenRequest,
    pb::ActionTakenResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    verify_opt_operation,
    pb::VerifyOptOperationRequest,
    pb::VerifyOptOperationResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    consider_web_operation,
    pb::ConsiderWebOperationRequest,
    pb::ConsiderWebOperationResponse)
  DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER(
    get_config,
    pb::GetConfigRequest,
    pb::GetConfigResponse)

#undef DEFINE_CAMPAIGN_MANAGER_GRPC_CORO_WRAPPER

  void
  CampaignManagerGrpc::ServiceImpl::ready(
    const pb::ReadyRequest&,
    pb::ReadyResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->ready_in_progress,
      stats_->ready_total,
      stats_->ready_time);

    try
    {
      response.set_ready(core_->ready());
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::progress_comment(
    const pb::ProgressCommentRequest&,
    pb::ProgressCommentResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->progress_comment_in_progress,
      stats_->progress_comment_total,
      stats_->progress_comment_time);

    try
    {
      std::string comment;
      core_->progress_comment(comment);
      response.set_comment(std::move(comment));
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::match_geo_channels(
    const pb::MatchGeoChannelsRequest& request,
    pb::MatchGeoChannelsResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->match_geo_channels_in_progress,
      stats_->match_geo_channels_total,
      stats_->match_geo_channels_time);

    try
    {
      std::vector<CampaignManagerCore::GeoInfo> location;
      std::vector<CampaignManagerCore::GeoCoordInfo> coord_location;
      unpack_geo_info_seq(request.location(), location);
      unpack_geo_coord_info_seq(request.coord_location(), coord_location);

      CampaignManagerCore::IdVector geo_channels;
      CampaignManagerCore::IdVector coord_channels;
      core_->match_geo_channels(
        location,
        coord_location,
        geo_channels,
        coord_channels);

      pack_ids(geo_channels, response.mutable_geo_channels());
      pack_ids(coord_channels, response.mutable_coord_channels());
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_file(
    const pb::GetFileRequest& request,
    pb::GetFileResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_file_in_progress,
      stats_->get_file_total,
      stats_->get_file_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto file = core_->get_file(request.file_name());
      response.set_file(file.data(), file.size());
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::trace_campaign_selection_index(
    const pb::TraceCampaignSelectionIndexRequest&,
    pb::TraceCampaignSelectionIndexResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->trace_campaign_selection_index_in_progress,
      stats_->trace_campaign_selection_index_total,
      stats_->trace_campaign_selection_index_time);

    try
    {
      response.set_trace_xml(core_->trace_campaign_selection_index());
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  CampaignManagerGrpc::ServiceImpl::co_get_campaign_creative(
    const pb::GetCampaignCreativeRequest& request,
    pb::GetCampaignCreativeResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_campaign_creative_in_progress,
      stats_->get_campaign_creative_total,
      stats_->get_campaign_creative_time);

    try
    {

      const auto result = co_await core_->co_get_campaign_creative(
        unpack_request_params(request.request_params()));

      response.set_hostname(result.hostname);
      response.mutable_request_result()->set_process_time(
        pack_time(result.process_time));
      auto* debug_info = response.mutable_request_result()->mutable_debug_info();
      debug_info->set_colo_id(result.debug_info.colo_id);
      pack_ids(
        result.debug_info.geo_channels,
        debug_info->mutable_geo_channels());
      pack_ids(
        result.debug_info.platform_channels,
        debug_info->mutable_platform_channels());
      debug_info->set_last_platform_channel_id(
        result.debug_info.last_platform_channel_id);
      debug_info->set_user_group_id(result.debug_info.user_group_id);

      for(const auto& ad_slot : result.ad_slots)
      {
        pack_ad_slot_result(
          ad_slot,
          *response.mutable_request_result()->add_ad_slots());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }

    co_return;
  }

  void
  CampaignManagerGrpc::ServiceImpl::process_match_request(
    const pb::ProcessMatchRequestRequest& request,
    pb::ProcessMatchRequestResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->process_match_request_in_progress,
      stats_->process_match_request_total,
      stats_->process_match_request_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto& source = request.match_request_info();
      CampaignManagerCore::MatchRequestInfo info;
      info.user_id = unpack_user_id(source.user_id());
      info.household_id = unpack_user_id(source.household_id());
      info.request_time = unpack_time(source.request_time());
      info.match_info.channels = unpack_ids(source.match_info().channels());
      unpack_channel_trigger_matches(
        source.match_info().pkw_channels(),
        info.match_info.pkw_channels);
      info.match_info.hid_channels = unpack_ids(
        source.match_info().hid_channels());
      info.match_info.colo_id = source.match_info().colo_id();
      unpack_geo_info_seq(
        source.match_info().location(),
        info.match_info.location);
      unpack_geo_coord_info_seq(
        source.match_info().coord_location(),
        info.match_info.coord_location);
      info.match_info.full_referer = source.match_info().full_referer();
      info.source = source.source();

      core_->process_match_request(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::process_anonymous_request(
    const pb::ProcessAnonymousRequestRequest& request,
    pb::ProcessAnonymousRequestResponse&,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->process_anonymous_request_in_progress,
      stats_->process_anonymous_request_total,
      stats_->process_anonymous_request_time);

    try
    {
      const auto& source = request.anonymous_request_info();
      CampaignManagerCore::AnonymousRequestInfo info;
      info.time = unpack_time(source.time());
      info.colo_id = source.colo_id();
      info.user_status = source.user_status();
      info.test_request = source.test_request();
      info.search_engine_id = source.search_engine_id();
      info.search_words = source.search_words();
      info.client = source.client();
      info.client_version = source.client_version();
      info.platform_ids = unpack_ids(source.platform_ids());
      info.full_platform = source.full_platform();
      info.web_browser = source.web_browser();
      info.user_agent = source.user_agent();
      info.search_engine_host = source.search_engine_host();
      info.country_code = source.country_code();
      info.page_keywords_present = source.page_keywords_present();

      core_->process_anonymous_request(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  CampaignManagerGrpc::ServiceImpl::co_instantiate_ad(
    const pb::InstantiateAdRequest& request,
    pb::InstantiateAdResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->instantiate_ad_in_progress,
      stats_->instantiate_ad_total,
      stats_->instantiate_ad_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto& source = request.instantiate_ad_info();
      CampaignManagerCore::InstantiateAdInfo info;
      unpack_common_ad_request_info(source.common_info(), info.common_info);
      info.context_info.reserve(source.context_info_size());
      for(const auto& context : source.context_info())
      {
        info.context_info.push_back(unpack_context_ad_request_info(context));
      }
      info.format = source.format();
      info.publisher_site_id = source.publisher_site_id();
      info.publisher_account_id = source.publisher_account_id();
      info.tag_id = source.tag_id();
      info.tag_size_id = source.tag_size_id();
      unpack_track_creatives(source.creatives(), info.creatives);
      info.creative_id = source.creative_id();
      if(source.has_user_id_hash_mod())
      {
        set_optional_uint64(source.user_id_hash_mod(), info.user_id_hash_mod);
      }
      info.merged_user_id = unpack_user_id(source.merged_user_id());
      info.pubpixel_accounts = unpack_ids(source.pubpixel_accounts());
      info.open_price = source.open_price();
      info.openx_price = source.openx_price();
      info.liverail_price = source.liverail_price();
      info.google_price = source.google_price();
      info.ext_tag_id = source.ext_tag_id();
      info.video_width = source.video_width();
      info.video_height = source.video_height();
      info.consider_request = source.consider_request();
      info.enabled_notice = source.enabled_notice();
      info.emulate_click = source.emulate_click();
      if(source.has_pub_imp_revenue())
      {
        info.pub_imp_revenue = unpack_revenue_decimal(source.pub_imp_revenue());
      }

      const auto result = co_await core_->co_instantiate_ad(info);
      auto* target = response.mutable_instantiate_ad_result();
      target->set_creative_body(result.creative_body);
      target->set_mime_format(result.mime_format);
      for(const auto& request_id : result.request_ids)
      {
        target->add_request_ids(pack_request_id(request_id));
      }
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }

    co_return;
  }

  void
  CampaignManagerGrpc::ServiceImpl::trace_campaign_selection(
    const pb::TraceCampaignSelectionRequest& request,
    pb::TraceCampaignSelectionResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->trace_campaign_selection_in_progress,
      stats_->trace_campaign_selection_total,
      stats_->trace_campaign_selection_time);

    try
    {
      response.set_trace_xml(core_->trace_campaign_selection(
        request.campaign_id(),
        unpack_trace_request_params(request.request_params()),
        unpack_ad_slot_info(request.ad_slot()),
        request.auction_type(),
        request.test_request()));
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_campaign_creative_by_ccid(
    const pb::GetCampaignCreativeByCcidRequest& request,
    pb::GetCampaignCreativeByCcidResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_campaign_creative_by_ccid_in_progress,
      stats_->get_campaign_creative_by_ccid_total,
      stats_->get_campaign_creative_by_ccid_time);

    try
    {
      CampaignManagerCore::PreviewCreativeParams params;
      params.ccid = request.ccid();
      params.tag_id = request.tag_id();
      params.format = request.format();
      params.original_url = request.original_url();
      params.peer_ip = request.peer_ip();

      std::string creative_body;
      response.set_found(
        core_->get_campaign_creative_by_ccid(params, creative_body));
      response.set_creative_body(std::move(creative_body));
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_channel_links(
    const pb::GetChannelLinksRequest& request,
    pb::GetChannelLinksResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_channel_links_in_progress,
      stats_->get_channel_links_total,
      stats_->get_channel_links_time);

    try
    {
      const auto result = core_->get_channel_links(
        unpack_ids(request.channels()),
        request.match());

      for(const auto& channel : result)
      {
        pack_channel_search_result(channel, *response.add_channels());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_discover_channels(
    const pb::GetDiscoverChannelsRequest& request,
    pb::GetDiscoverChannelsResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_discover_channels_in_progress,
      stats_->get_discover_channels_total,
      stats_->get_discover_channels_time);

    try
    {
      std::vector<CampaignManagerCore::ChannelWeight> channels;
      channels.reserve(request.channels_size());
      for(const auto& channel : request.channels())
      {
        channels.push_back({channel.channel_id(), channel.weight()});
      }

      const auto result = core_->get_discover_channels(
        channels,
        request.country(),
        request.language(),
        request.all());

      for(const auto& channel : result)
      {
        pack_discover_channel_result(channel, *response.add_channels());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_category_channels(
    const pb::GetCategoryChannelsRequest& request,
    pb::GetCategoryChannelsResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_category_channels_in_progress,
      stats_->get_category_channels_total,
      stats_->get_category_channels_time);

    try
    {
      const auto result = core_->get_category_channels(request.language());
      for(const auto& channel : result)
      {
        pack_category_channel_node(channel, *response.add_channels());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_colocation_flags(
    const pb::GetColocationFlagsRequest&,
    pb::GetColocationFlagsResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_colocation_flags_in_progress,
      stats_->get_colocation_flags_total,
      stats_->get_colocation_flags_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto result = core_->get_colocation_flags();
      for(const auto& colocation : result)
      {
        auto* target = response.add_colocations();
        target->set_colo_id(colocation.colo_id);
        target->set_flags(colocation.flags);
        target->set_hid_profile(colocation.hid_profile);
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_pub_pixels(
    const pb::GetPubPixelsRequest& request,
    pb::GetPubPixelsResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_pub_pixels_in_progress,
      stats_->get_pub_pixels_total,
      stats_->get_pub_pixels_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto result = core_->get_pub_pixels(
        request.country(),
        request.user_status(),
        unpack_ids(request.publisher_account_ids()));

      for(const auto& pixel : result)
      {
        response.add_pixels(pixel);
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::consider_passback(
    const pb::ConsiderPassbackRequest& request,
    pb::ConsiderPassbackResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->consider_passback_in_progress,
      stats_->consider_passback_total,
      stats_->consider_passback_time);

    response.set_hostname(service_hostname_());

    try
    {
      CampaignManagerCore::PassbackInfo info;
      info.request_id = CorbaAlgs::unpack_request_id(
        unpack_oct_seq(request.request_id()));
      info.time = unpack_time(request.time());
      if(request.has_user_id_hash_mod() &&
        request.user_id_hash_mod().defined())
      {
        info.user_id_hash_mod = AdServer::Commons::Optional<unsigned long>(
          request.user_id_hash_mod().value());
      }

      core_->consider_passback(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::consider_passback_track(
    const pb::ConsiderPassbackTrackRequest& request,
    pb::ConsiderPassbackTrackResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->consider_passback_track_in_progress,
      stats_->consider_passback_track_total,
      stats_->consider_passback_track_time);

    response.set_hostname(service_hostname_());

    try
    {
      CampaignManagerCore::PassbackTrackInfo info;
      info.time = unpack_time(request.time());
      info.country = request.country();
      info.colo_id = request.colo_id();
      info.tag_id = request.tag_id();
      info.user_status = request.user_status();

      core_->consider_passback_track(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::verify_opt_operation(
    const pb::VerifyOptOperationRequest& request,
    pb::VerifyOptOperationResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->verify_opt_operation_in_progress,
      stats_->verify_opt_operation_total,
      stats_->verify_opt_operation_time);

    response.set_hostname(service_hostname_());

    try
    {
      CampaignManagerCore::OptOperation operation =
        CampaignManagerCore::OptOperation::STATUS;
      switch(request.operation())
      {
      case pb::OPT_OPERATION_IN:
        operation = CampaignManagerCore::OptOperation::IN;
        break;
      case pb::OPT_OPERATION_OUT:
        operation = CampaignManagerCore::OptOperation::OUT;
        break;
      case pb::OPT_OPERATION_FORCED_IN:
        operation = CampaignManagerCore::OptOperation::FORCED_IN;
        break;
      case pb::OPT_OPERATION_STATUS:
      default:
        operation = CampaignManagerCore::OptOperation::STATUS;
        break;
      }

      core_->verify_opt_operation(
        request.time(),
        request.colo_id(),
        request.referer(),
        operation,
        request.status(),
        request.user_status(),
        request.log_as_test(),
        request.browser(),
        request.os(),
        request.ct(),
        request.curct(),
        CorbaAlgs::unpack_user_id(unpack_oct_seq(request.user_id())));
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  CampaignManagerGrpc::ServiceImpl::co_get_click_url(
    const pb::GetClickUrlRequest& request,
    pb::GetClickUrlResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_click_url_in_progress,
      stats_->get_click_url_total,
      stats_->get_click_url_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto& source = request.click_info();
      CampaignManagerCore::ClickInfo info;
      info.time = unpack_time(source.time());
      info.bid_time = unpack_time(source.bid_time());
      info.colo_id = source.colo_id();
      info.tag_id = source.tag_id();
      info.tag_size_id = source.tag_size_id();
      info.ccid = source.ccid();
      info.ccg_keyword_id = source.ccg_keyword_id();
      info.creative_id = source.creative_id();
      info.match_user_id = unpack_user_id(source.match_user_id());
      info.cookie_user_id = unpack_user_id(source.cookie_user_id());
      info.request_id = unpack_request_id(source.request_id());
      if(source.has_user_id_hash_mod())
      {
        set_optional_uint64(source.user_id_hash_mod(), info.user_id_hash_mod);
      }
      info.relocate = source.relocate();
      info.referer = source.referer();
      info.log_click = source.log_click();
      info.ctr = unpack_revenue_decimal(source.ctr());
      for(const auto& token : source.tokens())
      {
        info.tokens[token.name()] = token.value();
      }

      CampaignManagerCore::ClickResultInfo result;
      response.set_found(co_await core_->co_get_click_url(info, result));
      auto* target = response.mutable_click_result_info();
      target->set_url(result.url);
      target->set_campaign_id(result.campaign_id);
      target->set_advertiser_id(result.advertiser_id);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }

    co_return;
  }

  AdServer::Grpc::GrpcCoroutine
  CampaignManagerGrpc::ServiceImpl::co_verify_impression(
    const pb::VerifyImpressionRequest& request,
    pb::VerifyImpressionResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->verify_impression_in_progress,
      stats_->verify_impression_total,
      stats_->verify_impression_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto& source = request.impression_info();
      CampaignManagerCore::ImpressionInfo info;
      info.time = unpack_time(source.time());
      info.bid_time = unpack_time(source.bid_time());
      if(source.has_user_id_hash_mod())
      {
        set_optional_uint64(source.user_id_hash_mod(), info.user_id_hash_mod);
      }
      unpack_track_creatives(source.creatives(), info.creatives);
      info.pub_imp_revenue_type = source.pub_imp_revenue_type();
      info.pub_imp_revenue = unpack_revenue_decimal(source.pub_imp_revenue());
      info.request_type = source.request_type();
      info.verify_type = source.verify_type();
      info.user_id = unpack_user_id(source.user_id());
      info.referer = source.referer();
      info.viewability = source.viewability();
      info.action_name = source.action_name();

      const auto result = co_await core_->co_verify_impression(info);
      for(const auto& creative : result)
      {
        auto* target =
          response.mutable_impression_result_info()->add_creatives();
        target->set_campaign_id(creative.campaign_id);
        target->set_advertiser_id(creative.advertiser_id);
      }
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }

    co_return;
  }

  void
  CampaignManagerGrpc::ServiceImpl::action_taken(
    const pb::ActionTakenRequest& request,
    pb::ActionTakenResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->action_taken_in_progress,
      stats_->action_taken_total,
      stats_->action_taken_time);

    response.set_hostname(service_hostname_());

    try
    {
      const auto& source = request.action_info();
      CampaignManagerCore::ActionInfo info;
      info.time = unpack_time(source.time());
      info.test_request = source.test_request();
      info.log_as_test = source.log_as_test();
      if(source.has_campaign_id() && source.campaign_id().defined())
      {
        info.campaign_id.emplace(source.campaign_id().value());
      }
      if(source.has_action_id() && source.action_id().defined())
      {
        info.action_id.emplace(source.action_id().value());
      }
      info.order_id = source.order_id();
      if(source.has_action_value())
      {
        info.action_value.emplace(unpack_revenue_decimal(source.action_value()));
      }
      info.referer = source.referer();
      info.user_status = source.user_status();
      info.user_id = unpack_user_id(source.user_id());
      info.ip_hash = source.ip_hash();
      info.platform_ids = unpack_ids(source.platform_ids());
      info.peer_ip = source.peer_ip();
      unpack_geo_info_seq(source.location(), info.location);

      core_->action_taken(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::consider_web_operation(
    const pb::ConsiderWebOperationRequest& request,
    pb::ConsiderWebOperationResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->consider_web_operation_in_progress,
      stats_->consider_web_operation_total,
      stats_->consider_web_operation_time);

    response.set_hostname(service_hostname_());

    try
    {
      CampaignManagerCore::WebOperationInfo info;
      info.time = unpack_time(request.time());
      info.colo_id = request.colo_id();
      info.tag_id = request.tag_id();
      info.cc_id = request.cc_id();
      info.ct = request.ct();
      info.curct = request.curct();
      info.browser = request.browser();
      info.os = request.os();
      info.app = request.app();
      info.source = request.source();
      info.operation = request.operation();
      info.user_bind_src = request.user_bind_src();
      info.result = static_cast<char>(request.result());
      info.user_status = request.user_status();
      info.test_request = request.test_request();
      info.request_ids.reserve(request.request_ids_size());
      for(const auto& request_id : request.request_ids())
      {
        info.request_ids.emplace_back(
          CorbaAlgs::unpack_request_id(unpack_oct_seq(request_id)));
      }
      info.global_request_id = CorbaAlgs::unpack_request_id(
        unpack_oct_seq(request.global_request_id()));
      info.referer = request.referer();
      info.ip_address = request.ip_address();
      info.external_user_id = request.external_user_id();
      info.user_agent = request.user_agent();

      core_->consider_web_operation(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::InvalidArgument& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INVALID_ARGUMENT,
        ex.what());
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_config(
    const pb::GetConfigRequest& request,
    pb::GetConfigResponse& response,
    ::grpc::Status& result_status) const
  {
    CallStatsGuard call_stats(
      stats_->call_in_progress,
      stats_->call_total,
      stats_->call_time,
      stats_->get_config_in_progress,
      stats_->get_config_total,
      stats_->get_config_time);

    try
    {
      CampaignManagerCore::ConfigRequestInfo get_config_info;
      get_config_info.geo_channels = request.geo_channels();
      CampaignConfig_var core_config = core_->get_config(get_config_info);
      pack_config(
        *core_config,
        get_config_info.geo_channels,
        *response.mutable_config());
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  CampaignManagerGrpc::CampaignManagerGrpc(
    CampaignManagerCore* core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t process_threads,
    std::size_t cq_threads)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      stats_(std::make_shared<AtomicStats>()),
      executor_pool_(std::make_shared<AdServer::Commons::ExecutorPool>(
        Generics::ActiveObjectCallback_var(
          new Logging::ActiveObjectCallbackImpl(
            logger,
            "",
            campaign_manager_grpc_aspect)),
        std::max<std::size_t>(1, process_threads),
        "cm-grpc-pool")),
      impl_(std::make_shared<Impl>(
        logger,
        campaign_manager_grpc_aspect,
        bind_address_,
        cq_threads,
        std::make_unique<ServiceImpl>(core, executor_pool_, stats_)))
  {
    add_child_object(executor_pool_);
    add_child_object(impl_);
  }

  CampaignManagerGrpc::Stats
  CampaignManagerGrpc::stats() const noexcept
  {
    Stats result;
#define LOAD_STAT_(name) \
    result.name = stats_->name.load(std::memory_order_relaxed)

    LOAD_STAT_(call_in_progress);
    LOAD_STAT_(call_total);
    LOAD_STAT_(call_time);
    LOAD_STAT_(ready_in_progress);
    LOAD_STAT_(ready_total);
    LOAD_STAT_(ready_time);
    LOAD_STAT_(progress_comment_in_progress);
    LOAD_STAT_(progress_comment_total);
    LOAD_STAT_(progress_comment_time);
    LOAD_STAT_(match_geo_channels_in_progress);
    LOAD_STAT_(match_geo_channels_total);
    LOAD_STAT_(match_geo_channels_time);
    LOAD_STAT_(get_file_in_progress);
    LOAD_STAT_(get_file_total);
    LOAD_STAT_(get_file_time);
    LOAD_STAT_(get_campaign_creative_in_progress);
    LOAD_STAT_(get_campaign_creative_total);
    LOAD_STAT_(get_campaign_creative_time);
    LOAD_STAT_(process_match_request_in_progress);
    LOAD_STAT_(process_match_request_total);
    LOAD_STAT_(process_match_request_time);
    LOAD_STAT_(process_anonymous_request_in_progress);
    LOAD_STAT_(process_anonymous_request_total);
    LOAD_STAT_(process_anonymous_request_time);
    LOAD_STAT_(instantiate_ad_in_progress);
    LOAD_STAT_(instantiate_ad_total);
    LOAD_STAT_(instantiate_ad_time);
    LOAD_STAT_(trace_campaign_selection_index_in_progress);
    LOAD_STAT_(trace_campaign_selection_index_total);
    LOAD_STAT_(trace_campaign_selection_index_time);
    LOAD_STAT_(trace_campaign_selection_in_progress);
    LOAD_STAT_(trace_campaign_selection_total);
    LOAD_STAT_(trace_campaign_selection_time);
    LOAD_STAT_(get_campaign_creative_by_ccid_in_progress);
    LOAD_STAT_(get_campaign_creative_by_ccid_total);
    LOAD_STAT_(get_campaign_creative_by_ccid_time);
    LOAD_STAT_(get_channel_links_in_progress);
    LOAD_STAT_(get_channel_links_total);
    LOAD_STAT_(get_channel_links_time);
    LOAD_STAT_(get_discover_channels_in_progress);
    LOAD_STAT_(get_discover_channels_total);
    LOAD_STAT_(get_discover_channels_time);
    LOAD_STAT_(get_category_channels_in_progress);
    LOAD_STAT_(get_category_channels_total);
    LOAD_STAT_(get_category_channels_time);
    LOAD_STAT_(get_colocation_flags_in_progress);
    LOAD_STAT_(get_colocation_flags_total);
    LOAD_STAT_(get_colocation_flags_time);
    LOAD_STAT_(get_pub_pixels_in_progress);
    LOAD_STAT_(get_pub_pixels_total);
    LOAD_STAT_(get_pub_pixels_time);
    LOAD_STAT_(consider_passback_in_progress);
    LOAD_STAT_(consider_passback_total);
    LOAD_STAT_(consider_passback_time);
    LOAD_STAT_(consider_passback_track_in_progress);
    LOAD_STAT_(consider_passback_track_total);
    LOAD_STAT_(consider_passback_track_time);
    LOAD_STAT_(get_click_url_in_progress);
    LOAD_STAT_(get_click_url_total);
    LOAD_STAT_(get_click_url_time);
    LOAD_STAT_(verify_impression_in_progress);
    LOAD_STAT_(verify_impression_total);
    LOAD_STAT_(verify_impression_time);
    LOAD_STAT_(action_taken_in_progress);
    LOAD_STAT_(action_taken_total);
    LOAD_STAT_(action_taken_time);
    LOAD_STAT_(verify_opt_operation_in_progress);
    LOAD_STAT_(verify_opt_operation_total);
    LOAD_STAT_(verify_opt_operation_time);
    LOAD_STAT_(consider_web_operation_in_progress);
    LOAD_STAT_(consider_web_operation_total);
    LOAD_STAT_(consider_web_operation_time);
    LOAD_STAT_(get_config_in_progress);
    LOAD_STAT_(get_config_total);
    LOAD_STAT_(get_config_time);

#undef LOAD_STAT_
    return result;
  }

  CampaignManagerGrpc::~CampaignManagerGrpc() noexcept = default;
}
