#include "CampaignManagerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <string>
#include <utility>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include <CampaignSvcs/CampaignManager/CampaignManagerGrpc.grpc.pb.h>
#include <CampaignSvcs/CampaignManager/CampaignManagerImpl.hpp>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    constexpr const char campaign_manager_grpc_aspect[] =
      "CampaignManagerGrpc";

    namespace pb = adserver::campaign_svcs::campaign_manager;

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
      for(const auto id : source)
      {
        target->Add(id);
      }
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

    template<typename Seq>
    void
    pack_corba_ids(
      const Seq& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        target->Add(source[i]);
      }
    }

    template<typename Seq>
    void
    pack_corba_strings(
      const Seq& source,
      google::protobuf::RepeatedPtrField<std::string>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        *target->Add() = source[i].in();
      }
    }

    void
    pack_decimal_info(
      const CORBACommons::DecimalInfo& source,
      pb::DecimalInfo& target)
    {
      target.set_value(pack_oct_seq(source));
    }

    void
    pack_config_option_value(
      const OptionValueInfo& source,
      pb::ConfigOptionValue& target)
    {
      target.set_option_id(source.option_id);
      target.set_value(source.value.in());
    }

    void
    pack_config_option_values(
      const OptionValueSeq& source,
      google::protobuf::RepeatedPtrField<pb::ConfigOptionValue>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        pack_config_option_value(source[i], *target->Add());
      }
    }

    void
    pack_config_expression(
      const ExpressionInfo& source,
      pb::ConfigExpression& target)
    {
      target.set_operation(static_cast<unsigned char>(source.operation));
      target.set_channel_id(source.channel_id);
      for(CORBA::ULong i = 0; i < source.sub_channels.length(); ++i)
      {
        pack_config_expression(source.sub_channels[i], *target.add_sub_channels());
      }
    }

    void
    pack_config_delivery_limits(
      const CampaignDeliveryLimitsInfo& source,
      pb::ConfigDeliveryLimits& target)
    {
      target.set_date_start(pack_oct_seq(source.date_start));
      target.set_date_end(pack_oct_seq(source.date_end));
      pack_decimal_info(source.budget, *target.mutable_budget());
      pack_decimal_info(source.daily_budget, *target.mutable_daily_budget());
      if(source.imps_defined)
      {
        target.set_imps(source.imps);
      }
      if(source.clicks_defined)
      {
        target.set_clicks(source.clicks);
      }
      target.set_delivery_pacing(
        static_cast<unsigned char>(source.delivery_pacing));
    }

    void
    pack_config_creative(
      const CreativeInfo& source,
      pb::ConfigCreative& target)
    {
      target.set_ccid(source.ccid);
      target.set_creative_id(source.creative_id);
      target.set_fc_id(source.fc_id);
      target.set_weight(source.weight);
      for(CORBA::ULong i = 0; i < source.sizes.length(); ++i)
      {
        const auto& src_size = source.sizes[i];
        auto* dst_size = target.add_sizes();
        dst_size->set_size_id(src_size.size_id);
        dst_size->set_up_expand_space(src_size.up_expand_space);
        dst_size->set_right_expand_space(src_size.right_expand_space);
        dst_size->set_down_expand_space(src_size.down_expand_space);
        dst_size->set_left_expand_space(src_size.left_expand_space);
        pack_config_option_values(src_size.tokens, dst_size->mutable_tokens());
      }
      target.set_creative_format(source.creative_format.in());
      pack_config_option_value(source.click_url, *target.mutable_click_url());
      pack_config_option_value(source.html_url, *target.mutable_html_url());
      target.set_order_set_id(source.order_set_id);
      target.set_initial_contract_id(source.initial_contract_id);
      pack_corba_ids(source.categories, target.mutable_categories());
      pack_config_option_values(source.tokens, target.mutable_tokens());
      target.set_status(static_cast<unsigned char>(source.status));
      target.set_version_id(source.version_id.in());
    }

    void
    pack_config_campaign(
      const CampaignInfo& source,
      pb::ConfigCampaign& target)
    {
      target.set_campaign_id(source.campaign_id);
      target.set_campaign_group_id(source.campaign_group_id);
      target.set_ccg_rate_id(source.ccg_rate_id);
      target.set_ccg_rate_type(static_cast<unsigned char>(source.ccg_rate_type));
      target.set_fc_id(source.fc_id);
      target.set_group_fc_id(source.group_fc_id);
      target.set_priority(source.priority);
      target.set_flags(source.flags);
      target.set_marketplace(static_cast<unsigned char>(source.marketplace));
      pack_config_expression(source.expression, *target.mutable_expression());
      pack_config_expression(
        source.stat_expression,
        *target.mutable_stat_expression());
      target.set_country(source.country.in());
      pack_corba_ids(source.sites, target.mutable_sites());
      target.set_status(static_cast<unsigned char>(source.status));
      target.set_eval_status(static_cast<unsigned char>(source.eval_status));
      for(CORBA::ULong i = 0; i < source.weekly_run_intervals.length(); ++i)
      {
        auto* interval = target.add_weekly_run_intervals();
        interval->set_min(source.weekly_run_intervals[i].min);
        interval->set_max(source.weekly_run_intervals[i].max);
      }
      for(CORBA::ULong i = 0; i < source.creatives.length(); ++i)
      {
        pack_config_creative(source.creatives[i], *target.add_creatives());
      }
      target.set_account_id(source.account_id);
      target.set_advertiser_id(source.advertiser_id);
      pack_corba_ids(
        source.exclude_pub_accounts,
        target.mutable_exclude_pub_accounts());
      for(CORBA::ULong i = 0; i < source.exclude_tags.length(); ++i)
      {
        auto* tag = target.add_exclude_tags();
        tag->set_tag_id(source.exclude_tags[i].tag_id);
        tag->set_delivery_value(source.exclude_tags[i].delivery_value);
      }
      target.set_delivery_coef(source.delivery_coef);
      pack_decimal_info(source.imp_revenue, *target.mutable_imp_revenue());
      pack_decimal_info(source.click_revenue, *target.mutable_click_revenue());
      pack_decimal_info(source.action_revenue, *target.mutable_action_revenue());
      pack_decimal_info(source.commision, *target.mutable_commision());
      target.set_ccg_type(static_cast<unsigned char>(source.ccg_type));
      target.set_target_type(static_cast<unsigned char>(source.target_type));
      pack_config_delivery_limits(
        source.campaign_delivery_limits,
        *target.mutable_campaign_delivery_limits());
      pack_config_delivery_limits(
        source.ccg_delivery_limits,
        *target.mutable_ccg_delivery_limits());
      target.set_start_user_group_id(source.start_user_group_id);
      target.set_end_user_group_id(source.end_user_group_id);
      pack_decimal_info(source.max_pub_share, *target.mutable_max_pub_share());
      target.set_ctr_reset_id(source.ctr_reset_id);
      target.set_random_imps(source.random_imps);
      target.set_mode(source.mode);
      target.set_seq_set_rotate_imps(source.seq_set_rotate_imps);
      target.set_min_uid_age(pack_oct_seq(source.min_uid_age));
      pack_corba_ids(source.colocations, target.mutable_colocations());
      target.set_bid_strategy(source.bid_strategy);
      pack_decimal_info(source.min_ctr_goal, *target.mutable_min_ctr_goal());
      target.set_timestamp(pack_oct_seq(source.timestamp));
    }

    void
    pack_config_tag(
      const TagInfo& source,
      pb::ConfigTag& target)
    {
      target.set_tag_id(source.tag_id);
      target.set_site_id(source.site_id);
      target.set_status(static_cast<unsigned char>(source.status));
      for(CORBA::ULong i = 0; i < source.sizes.length(); ++i)
      {
        const auto& src_size = source.sizes[i];
        auto* dst_size = target.add_sizes();
        dst_size->set_size_id(src_size.size_id);
        dst_size->set_max_text_creatives(src_size.max_text_creatives);
        pack_config_option_values(src_size.tokens, dst_size->mutable_tokens());
        pack_config_option_values(
          src_size.hidden_tokens,
          dst_size->mutable_hidden_tokens());
      }
      target.set_imp_track_pixel(source.imp_track_pixel.in());
      target.set_passback(source.passback.in());
      target.set_passback_type(source.passback_type.in());
      target.set_flags(source.flags);
      target.set_marketplace(static_cast<unsigned char>(source.marketplace));
      pack_decimal_info(source.adjustment, *target.mutable_adjustment());
      for(CORBA::ULong i = 0; i < source.tag_pricings.length(); ++i)
      {
        const auto& src_pricing = source.tag_pricings[i];
        auto* dst_pricing = target.add_tag_pricings();
        dst_pricing->set_country_code(src_pricing.country_code.in());
        dst_pricing->set_ccg_type(
          static_cast<unsigned char>(src_pricing.ccg_type));
        dst_pricing->set_ccg_rate_type(
          static_cast<unsigned char>(src_pricing.ccg_rate_type));
        dst_pricing->set_site_rate_id(src_pricing.site_rate_id);
        pack_decimal_info(
          src_pricing.imp_revenue,
          *dst_pricing->mutable_imp_revenue());
        pack_decimal_info(
          src_pricing.revenue_share,
          *dst_pricing->mutable_revenue_share());
      }
      pack_corba_ids(
        source.accepted_categories,
        target.mutable_accepted_categories());
      pack_corba_ids(
        source.rejected_categories,
        target.mutable_rejected_categories());
      target.set_allow_expandable(source.allow_expandable);
      pack_config_option_values(source.tokens, target.mutable_tokens());
      pack_config_option_values(
        source.hidden_tokens,
        target.mutable_hidden_tokens());
      pack_config_option_values(
        source.passback_tokens,
        target.mutable_passback_tokens());
      for(CORBA::ULong i = 0; i < source.template_tokens.length(); ++i)
      {
        auto* template_options = target.add_template_tokens();
        template_options->set_template_name(source.template_tokens[i].template_name.in());
        pack_config_option_values(
          source.template_tokens[i].tokens,
          template_options->mutable_tokens());
      }
      pack_decimal_info(
        source.auction_max_ecpm_share,
        *target.mutable_auction_max_ecpm_share());
      pack_decimal_info(
        source.auction_prop_probability_share,
        *target.mutable_auction_prop_probability_share());
      pack_decimal_info(
        source.auction_random_share,
        *target.mutable_auction_random_share());
      pack_decimal_info(source.cost_coef, *target.mutable_cost_coef());
      target.set_tag_pricings_timestamp(
        pack_oct_seq(source.tag_pricings_timestamp));
      target.set_timestamp(pack_oct_seq(source.timestamp));
    }

    void
    pack_config(
      const CampaignManager::CampaignConfig& source,
      pb::CampaignConfig& target)
    {
      for(CORBA::ULong i = 0; i < source.app_formats.length(); ++i)
      {
        auto* item = target.add_app_formats();
        item->set_app_format(source.app_formats[i].app_format.in());
        item->set_mime_format(source.app_formats[i].mime_format.in());
        item->set_timestamp(pack_oct_seq(source.app_formats[i].timestamp));
      }
      for(CORBA::ULong i = 0; i < source.sizes.length(); ++i)
      {
        auto* item = target.add_sizes();
        item->set_size_id(source.sizes[i].size_id);
        item->set_protocol_name(source.sizes[i].protocol_name.in());
        item->set_size_type_id(source.sizes[i].size_type_id);
        item->set_width(source.sizes[i].width);
        item->set_height(source.sizes[i].height);
        item->set_timestamp(pack_oct_seq(source.sizes[i].timestamp));
      }
      for(CORBA::ULong i = 0; i < source.accounts.length(); ++i)
      {
        const auto& src = source.accounts[i];
        auto* item = target.add_accounts();
        item->set_account_id(src.account_id);
        item->set_agency_account_id(src.agency_account_id);
        item->set_internal_account_id(src.internal_account_id);
        item->set_role_id(src.role_id);
        item->set_legal_name(src.legal_name.in());
        item->set_flags(src.flags);
        item->set_at_flags(src.at_flags);
        item->set_text_adserving(static_cast<unsigned char>(src.text_adserving));
        item->set_currency_id(src.currency_id);
        item->set_country(src.country.in());
        item->set_time_offset(pack_oct_seq(src.time_offset));
        pack_decimal_info(src.commision, *item->mutable_commision());
        pack_decimal_info(src.budget, *item->mutable_budget());
        pack_decimal_info(src.paid_amount, *item->mutable_paid_amount());
        pack_corba_ids(
          src.walled_garden_accounts,
          item->mutable_walled_garden_accounts());
        item->set_auction_rate(src.auction_rate);
        item->set_use_pub_pixels(src.use_pub_pixels);
        item->set_pub_pixel_optin(src.pub_pixel_optin.in());
        item->set_pub_pixel_optout(src.pub_pixel_optout.in());
        pack_decimal_info(
          src.self_service_commission,
          *item->mutable_self_service_commission());
        item->set_status(static_cast<unsigned char>(src.status));
        item->set_eval_status(static_cast<unsigned char>(src.eval_status));
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.creative_options.length(); ++i)
      {
        auto* item = target.add_creative_options();
        item->set_option_id(source.creative_options[i].option_id);
        item->set_token(source.creative_options[i].token.in());
        item->set_type(
          static_cast<unsigned char>(source.creative_options[i].type));
        pack_corba_strings(
          source.creative_options[i].token_relations,
          item->mutable_token_relations());
        item->set_timestamp(
          pack_oct_seq(source.creative_options[i].timestamp));
      }
      for(CORBA::ULong i = 0; i < source.campaigns.length(); ++i)
      {
        auto* item = target.add_campaigns();
        pack_config_campaign(source.campaigns[i].info, *item->mutable_info());
        pack_config_expression(
          source.campaigns[i].expression,
          *item->mutable_expression());
        pack_decimal_info(source.campaigns[i].ecpm, *item->mutable_ecpm());
        pack_decimal_info(source.campaigns[i].ctr, *item->mutable_ctr());
      }
      for(CORBA::ULong i = 0; i < source.campaign_ecpms.length(); ++i)
      {
        auto* item = target.add_campaign_ecpms();
        item->set_ccg_id(source.campaign_ecpms[i].ccg_id);
        pack_decimal_info(source.campaign_ecpms[i].ecpm, *item->mutable_ecpm());
        pack_decimal_info(source.campaign_ecpms[i].ctr, *item->mutable_ctr());
        item->set_timestamp(pack_oct_seq(source.campaign_ecpms[i].timestamp));
      }
      for(CORBA::ULong i = 0; i < source.sites.length(); ++i)
      {
        const auto& src = source.sites[i];
        auto* item = target.add_sites();
        item->set_site_id(src.site_id);
        item->set_status(static_cast<unsigned char>(src.status));
        item->set_freq_cap_id(src.freq_cap_id);
        item->set_noads_timeout(src.noads_timeout);
        pack_corba_ids(
          src.approved_creative_categories,
          item->mutable_approved_creative_categories());
        pack_corba_ids(
          src.rejected_creative_categories,
          item->mutable_rejected_creative_categories());
        pack_corba_ids(src.approved_creatives, item->mutable_approved_creatives());
        pack_corba_ids(src.rejected_creatives, item->mutable_rejected_creatives());
        item->set_flags(src.flags);
        item->set_account_id(src.account_id);
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.tags.length(); ++i)
      {
        auto* item = target.add_tags();
        pack_config_tag(source.tags[i].info, *item->mutable_info());
        for(CORBA::ULong j = 0; j < source.tags[i].cpms.length(); ++j)
        {
          pack_decimal_info(source.tags[i].cpms[j], *item->add_cpms());
        }
      }
      for(CORBA::ULong i = 0; i < source.currencies.length(); ++i)
      {
        const auto& src = source.currencies[i];
        auto* item = target.add_currencies();
        pack_decimal_info(src.rate, *item->mutable_rate());
        item->set_currency_id(src.currency_id);
        item->set_currency_exchange_id(src.currency_exchange_id);
        item->set_effective_date(src.effective_date);
        item->set_fraction_digits(src.fraction_digits);
        item->set_currency_code(src.currency_code.in());
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.colocations.length(); ++i)
      {
        const auto& src = source.colocations[i];
        auto* item = target.add_colocations();
        item->set_colo_id(src.colo_id);
        item->set_colo_name(src.colo_name.in());
        item->set_colo_rate_id(src.colo_rate_id);
        item->set_at_flags(src.at_flags);
        item->set_ad_serving(src.ad_serving);
        item->set_hid_profile(src.hid_profile);
        item->set_account_id(src.account_id);
        pack_decimal_info(src.revenue_share, *item->mutable_revenue_share());
        pack_config_option_values(src.tokens, item->mutable_tokens());
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.countries.length(); ++i)
      {
        auto* item = target.add_countries();
        item->set_country_code(source.countries[i].country_code.in());
        pack_config_option_values(
          source.countries[i].tokens,
          item->mutable_tokens());
        item->set_timestamp(pack_oct_seq(source.countries[i].timestamp));
      }
      for(CORBA::ULong i = 0; i < source.frequency_caps.length(); ++i)
      {
        const auto& src = source.frequency_caps[i];
        auto* item = target.add_frequency_caps();
        item->set_fc_id(src.fc_id);
        item->set_lifelimit(src.lifelimit);
        item->set_period(src.period);
        item->set_window_limit(src.window_limit);
        item->set_window_time(src.window_time);
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.creative_template_files.length(); ++i)
      {
        const auto& src = source.creative_template_files[i];
        auto* item = target.add_creative_template_files();
        item->set_creative_format(src.creative_format.in());
        item->set_creative_size(src.creative_size.in());
        item->set_app_format(src.app_format.in());
        item->set_mime_format(src.mime_format.in());
        item->set_track_impr(src.track_impr);
        item->set_type(src.type);
        item->set_template_file(src.template_file.in());
        item->set_timestamp(pack_oct_seq(src.timestamp));
        pack_config_option_values(src.tokens, item->mutable_tokens());
        pack_config_option_values(
          src.hidden_tokens,
          item->mutable_hidden_tokens());
        item->set_status(static_cast<unsigned char>(src.status));
      }
      for(CORBA::ULong i = 0; i < source.campaign_keywords.length(); ++i)
      {
        const auto& src = source.campaign_keywords[i];
        auto* item = target.add_campaign_keywords();
        item->set_ccg_keyword_id(src.ccg_keyword_id);
        item->set_original_keyword(src.original_keyword.in());
        item->set_click_url(src.click_url.in());
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.expression_channels.length(); ++i)
      {
        const auto& src = source.expression_channels[i];
        auto* item = target.add_expression_channels();
        item->set_channel_id(src.channel_id);
        item->set_name(src.name.in());
        item->set_account_id(src.account_id);
        item->set_country_code(src.country_code.in());
        item->set_flags(src.flags);
        item->set_status(static_cast<unsigned char>(src.status));
        item->set_type(static_cast<unsigned char>(src.type));
        item->set_is_public(src.is_public);
        item->set_language(src.language.in());
        item->set_freq_cap_id(src.freq_cap_id);
        item->set_parent_channel_id(src.parent_channel_id);
        item->set_action_id(src.action_id);
        item->set_timestamp(pack_oct_seq(src.timestamp));
        item->set_discover_query(src.discover_query.in());
        item->set_discover_annotation(src.discover_annotation.in());
        item->set_channel_rate_id(src.channel_rate_id);
        pack_decimal_info(src.imp_revenue, *item->mutable_imp_revenue());
        pack_decimal_info(src.click_revenue, *item->mutable_click_revenue());
        pack_config_expression(src.expression, *item->mutable_expression());
        item->set_threshold(src.threshold);
      }
      for(CORBA::ULong i = 0; i < source.creative_categories.length(); ++i)
      {
        const auto& src = source.creative_categories[i];
        auto* item = target.add_creative_categories();
        item->set_creative_category_id(src.creative_category_id);
        item->set_cct_id(src.cct_id);
        item->set_name(src.name.in());
        for(CORBA::ULong j = 0; j < src.external_categories.length(); ++j)
        {
          auto* external_category = item->add_external_categories();
          external_category->set_ad_request_type(
            src.external_categories[j].ad_request_type);
          pack_corba_strings(
            src.external_categories[j].names,
            external_category->mutable_names());
        }
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.adv_actions.length(); ++i)
      {
        auto* item = target.add_adv_actions();
        item->set_action_id(source.adv_actions[i].action_id);
        item->set_timestamp(pack_oct_seq(source.adv_actions[i].timestamp));
        pack_corba_ids(source.adv_actions[i].ccg_ids, item->mutable_ccg_ids());
      }
      for(CORBA::ULong i = 0; i < source.category_channels.length(); ++i)
      {
        const auto& src = source.category_channels[i];
        auto* item = target.add_category_channels();
        item->set_channel_id(src.channel_id);
        item->set_name(src.name.in());
        item->set_newsgate_name(src.newsgate_name.in());
        for(CORBA::ULong j = 0; j < src.localizations.length(); ++j)
        {
          auto* loc = item->add_localizations();
          loc->set_language(src.localizations[j].language.in());
          loc->set_name(src.localizations[j].name.in());
        }
        item->set_parent_channel_id(src.parent_channel_id);
        item->set_flags(src.flags);
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.geo_channels.length(); ++i)
      {
        const auto& src = source.geo_channels[i];
        auto* item = target.add_geo_channels();
        item->set_channel_id(src.channel_id);
        item->set_country(src.country.in());
        for(CORBA::ULong j = 0; j < src.geoip_targets.length(); ++j)
        {
          auto* geoip_target = item->add_geoip_targets();
          geoip_target->set_region(src.geoip_targets[j].region.in());
          geoip_target->set_city(src.geoip_targets[j].city.in());
        }
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.geo_coord_channels.length(); ++i)
      {
        const auto& src = source.geo_coord_channels[i];
        auto* item = target.add_geo_coord_channels();
        item->set_channel_id(src.channel_id);
        pack_decimal_info(src.longitude, *item->mutable_longitude());
        pack_decimal_info(src.latitude, *item->mutable_latitude());
        pack_decimal_info(src.radius, *item->mutable_radius());
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.web_operations.length(); ++i)
      {
        const auto& src = source.web_operations[i];
        auto* item = target.add_web_operations();
        item->set_id(src.id);
        item->set_app(src.app.in());
        item->set_source(src.source.in());
        item->set_operation(src.operation.in());
        item->set_flags(src.flags);
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      for(CORBA::ULong i = 0; i < source.contracts.length(); ++i)
      {
        const auto& src = source.contracts[i];
        auto* item = target.add_contracts();
        item->set_contract_id(src.contract_id);
        item->set_number(src.number.in());
        item->set_date(src.date.in());
        item->set_type(src.type.in());
        item->set_vat_included(src.vat_included);
        item->set_ord_contract_id(src.ord_contract_id.in());
        item->set_ord_ado_id(src.ord_ado_id.in());
        item->set_subject_type(src.subject_type.in());
        item->set_action_type(src.action_type.in());
        item->set_agent_acting_for_publisher(src.agent_acting_for_publisher);
        item->set_parent_contract_id(src.parent_contract_id);
        item->set_client_id(src.client_id.in());
        item->set_client_name(src.client_name.in());
        item->set_client_legal_form(src.client_legal_form.in());
        item->set_contractor_id(src.contractor_id.in());
        item->set_contractor_name(src.contractor_name.in());
        item->set_contractor_legal_form(src.contractor_legal_form.in());
        item->set_timestamp(pack_oct_seq(src.timestamp));
      }
      target.set_currency_exchange_id(source.currency_exchange_id);
      target.set_fraud_user_deactivate_period(
        pack_oct_seq(source.fraud_user_deactivate_period));
      pack_decimal_info(source.cost_limit, *target.mutable_cost_limit());
      target.set_google_publisher_account_id(
        source.google_publisher_account_id);
      target.set_master_stamp(pack_oct_seq(source.master_stamp));
      target.set_first_load_stamp(pack_oct_seq(source.first_load_stamp));
      target.set_finish_load_stamp(pack_oct_seq(source.finish_load_stamp));
      target.set_global_params_timestamp(
        pack_oct_seq(source.global_params_timestamp));
      target.set_creative_categories_timestamp(
        pack_oct_seq(source.creative_categories_timestamp));
    }
  }

  class CampaignManagerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      CampaignManagerGrpc::ServiceImpl,
      pb::CampaignManagerGrpc,
      pb::CampaignManagerGrpc::AsyncService>
  {
    using AsyncService = pb::CampaignManagerGrpc::AsyncService;

  public:
    explicit ServiceImpl(CampaignManagerCore* core);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CALL(pb::ReadyRequest, pb::ReadyResponse, ready),
        MAKE_GRPC_CALL(
          pb::ProgressCommentRequest,
          pb::ProgressCommentResponse,
          progress_comment),
        MAKE_GRPC_CALL(
          pb::MatchGeoChannelsRequest,
          pb::MatchGeoChannelsResponse,
          match_geo_channels),
        MAKE_GRPC_CALL(pb::GetFileRequest, pb::GetFileResponse, get_file),
        MAKE_GRPC_CALL(
          pb::GetCampaignCreativeRequest,
          pb::GetCampaignCreativeResponse,
          get_campaign_creative),
        MAKE_GRPC_CALL(
          pb::ProcessMatchRequestRequest,
          pb::ProcessMatchRequestResponse,
          process_match_request),
        MAKE_GRPC_CALL(
          pb::ProcessAnonymousRequestRequest,
          pb::ProcessAnonymousRequestResponse,
          process_anonymous_request),
        MAKE_GRPC_CALL(
          pb::InstantiateAdRequest,
          pb::InstantiateAdResponse,
          instantiate_ad),
        MAKE_GRPC_CALL(
          pb::TraceCampaignSelectionIndexRequest,
          pb::TraceCampaignSelectionIndexResponse,
          trace_campaign_selection_index),
        MAKE_GRPC_CALL(
          pb::TraceCampaignSelectionRequest,
          pb::TraceCampaignSelectionResponse,
          trace_campaign_selection),
        MAKE_GRPC_CALL(
          pb::GetCampaignCreativeByCcidRequest,
          pb::GetCampaignCreativeByCcidResponse,
          get_campaign_creative_by_ccid),
        MAKE_GRPC_CALL(
          pb::GetChannelLinksRequest,
          pb::GetChannelLinksResponse,
          get_channel_links),
        MAKE_GRPC_CALL(
          pb::GetDiscoverChannelsRequest,
          pb::GetDiscoverChannelsResponse,
          get_discover_channels),
        MAKE_GRPC_CALL(
          pb::GetCategoryChannelsRequest,
          pb::GetCategoryChannelsResponse,
          get_category_channels),
        MAKE_GRPC_CALL(
          pb::GetColocationFlagsRequest,
          pb::GetColocationFlagsResponse,
          get_colocation_flags),
        MAKE_GRPC_CALL(
          pb::GetPubPixelsRequest,
          pb::GetPubPixelsResponse,
          get_pub_pixels),
        MAKE_GRPC_CALL(
          pb::ConsiderPassbackRequest,
          pb::ConsiderPassbackResponse,
          consider_passback),
        MAKE_GRPC_CALL(
          pb::ConsiderPassbackTrackRequest,
          pb::ConsiderPassbackTrackResponse,
          consider_passback_track),
        MAKE_GRPC_CALL(
          pb::GetClickUrlRequest,
          pb::GetClickUrlResponse,
          get_click_url),
        MAKE_GRPC_CALL(
          pb::VerifyImpressionRequest,
          pb::VerifyImpressionResponse,
          verify_impression),
        MAKE_GRPC_CALL(
          pb::ActionTakenRequest,
          pb::ActionTakenResponse,
          action_taken),
        MAKE_GRPC_CALL(
          pb::VerifyOptOperationRequest,
          pb::VerifyOptOperationResponse,
          verify_opt_operation),
        MAKE_GRPC_CALL(
          pb::ConsiderWebOperationRequest,
          pb::ConsiderWebOperationResponse,
          consider_web_operation),
        MAKE_GRPC_CALL(
          pb::GetConfigRequest,
          pb::GetConfigResponse,
          get_config));
    }

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

    void get_campaign_creative(
      const pb::GetCampaignCreativeRequest& request,
      pb::GetCampaignCreativeResponse& response,
      ::grpc::Status& result_status) const;

    void process_match_request(
      const pb::ProcessMatchRequestRequest& request,
      pb::ProcessMatchRequestResponse& response,
      ::grpc::Status& result_status) const;

    void process_anonymous_request(
      const pb::ProcessAnonymousRequestRequest& request,
      pb::ProcessAnonymousRequestResponse& response,
      ::grpc::Status& result_status) const;

    void instantiate_ad(
      const pb::InstantiateAdRequest& request,
      pb::InstantiateAdResponse& response,
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

    void get_click_url(
      const pb::GetClickUrlRequest& request,
      pb::GetClickUrlResponse& response,
      ::grpc::Status& result_status) const;

    void verify_impression(
      const pb::VerifyImpressionRequest& request,
      pb::VerifyImpressionResponse& response,
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

  private:
    CampaignManagerCore_var core_;
  };

  CampaignManagerGrpc::ServiceImpl::ServiceImpl(CampaignManagerCore* core)
    : core_(ReferenceCounting::add_ref(core))
  {}

  void
  CampaignManagerGrpc::ServiceImpl::ready(
    const pb::ReadyRequest&,
    pb::ReadyResponse& response,
    ::grpc::Status& result_status) const
  {
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

  void
  CampaignManagerGrpc::ServiceImpl::get_campaign_creative(
    const pb::GetCampaignCreativeRequest& request,
    pb::GetCampaignCreativeResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {

      const auto result = core_->get_campaign_creative(
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
  }

  void
  CampaignManagerGrpc::ServiceImpl::process_match_request(
    const pb::ProcessMatchRequestRequest& request,
    pb::ProcessMatchRequestResponse&,
    ::grpc::Status& result_status) const
  {
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

  void
  CampaignManagerGrpc::ServiceImpl::instantiate_ad(
    const pb::InstantiateAdRequest& request,
    pb::InstantiateAdResponse& response,
    ::grpc::Status& result_status) const
  {
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

      const auto result = core_->instantiate_ad(info);
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
  }

  void
  CampaignManagerGrpc::ServiceImpl::trace_campaign_selection(
    const pb::TraceCampaignSelectionRequest& request,
    pb::TraceCampaignSelectionResponse& response,
    ::grpc::Status& result_status) const
  {
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
    pb::ConsiderPassbackResponse&,
    ::grpc::Status& result_status) const
  {
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
    pb::ConsiderPassbackTrackResponse&,
    ::grpc::Status& result_status) const
  {
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
    pb::VerifyOptOperationResponse&,
    ::grpc::Status& result_status) const
  {
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

  void
  CampaignManagerGrpc::ServiceImpl::get_click_url(
    const pb::GetClickUrlRequest& request,
    pb::GetClickUrlResponse& response,
    ::grpc::Status& result_status) const
  {
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
      response.set_found(core_->get_click_url(info, result));
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
  }

  void
  CampaignManagerGrpc::ServiceImpl::verify_impression(
    const pb::VerifyImpressionRequest& request,
    pb::VerifyImpressionResponse& response,
    ::grpc::Status& result_status) const
  {
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

      const auto result = core_->verify_impression(info);
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
  }

  void
  CampaignManagerGrpc::ServiceImpl::action_taken(
    const pb::ActionTakenRequest& request,
    pb::ActionTakenResponse&,
    ::grpc::Status& result_status) const
  {
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
    pb::ConsiderWebOperationResponse&,
    ::grpc::Status& result_status) const
  {
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
    catch(const CampaignManagerCore::InvalidArgument&)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INVALID_ARGUMENT,
        "incorrect argument");
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
    try
    {
      CampaignManager::GetConfigInfo get_config_info;
      get_config_info.geo_channels = request.geo_channels();
      CampaignManagerImpl_var corba_pack_adapter =
        new CampaignManagerImpl(core_.in());
      CampaignManager::CampaignConfig_var config =
        corba_pack_adapter->get_config(get_config_info);
      pack_config(config.in(), *response.mutable_config());
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManager::ImplementationException& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.description.in());
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
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        campaign_manager_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(core)))
  {
    add_child_object(impl_);
  }

  CampaignManagerGrpc::~CampaignManagerGrpc() noexcept = default;
}
