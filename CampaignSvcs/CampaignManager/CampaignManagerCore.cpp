
#include <algorithm>
#include <cstring>
#include <vector>

#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <String/StringManip.hpp>
#include <String/UTF8Case.hpp>
#include <Generics/Uuid.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>
#include <Commons/Gason.hpp>

#include <LogCommons/AdRequestLogger.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>

#include "CampaignManagerCore.hpp"
#include "CampaignManagerDeclarations.hpp"

#include "CampaignManagerLogAdapter.hpp"
#include "CampaignManagerLogger.hpp"

#include "DomainParser.hpp"
#include <LogCommons/CsvUtils.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace
    {
      const RevenueDecimal TOP_ALLOWABLE_LOSE_WIN_PERCENTAGE(
        RevenueDecimal::div(
          RevenueDecimal(false, 95, 0),
          RevenueDecimal(false, 100, 0)));
      const RevenueDecimal LOW_ALLOWABLE_LOSE_WIN_PERCENTAGE(
        RevenueDecimal::div(
          RevenueDecimal(false, 75, 0),
          RevenueDecimal(false, 100, 0)));
      const RevenueDecimal FULL_COMMISION = REVENUE_ONE;
      const unsigned long MAX_PUBPIXELS_PER_REQUEST = 10;

      const unsigned long UPDATE_TASKS_COUNT = 3;

      // REQ-3939. Player size in VAST
      const char VAST_APPLICATION[] = "rtb";
      const char VAST_SOURCE[] = "vast";
      const char VAST_OPERATION[] ="request";

      namespace PlatformNames
      {
        const std::string APPLE_IPADS("apple ipads");
        const std::string APPLE_IPHONES("apple iphones");
        const std::string ANDROID_TABLETS("android tablets");
        const std::string ANDROID_SMARTPHONES("android smartphones");
        const std::string WINDOWS_MOBILE("windows mobile");
        const std::string SMARTTV("smarttv");
        const std::string DVR("dvr");
        const std::string NON_MOBILE_DEVICES("non-mobile devices");
      }

      void
      append(
        FreqCapIdSet& freq_caps,
        const CampaignManagerCore::IdVector& cap_seq)
        /*throw(eh::Exception)*/
      {
        std::copy(cap_seq.begin(),
          cap_seq.end(),
          std::inserter(freq_caps, freq_caps.end()));
      }

      template<typename Seq, typename Vector>
      void
      pack_id_vector(Seq& seq, const Vector& ids)
      {
        seq.length(ids.size());
        for(CORBA::ULong i = 0; i < ids.size(); ++i)
        {
          seq[i] = ids[i];
        }
      }

      template<typename Seq, typename Vector>
      void
      unpack_id_vector(Vector& ids, const Seq& seq)
      {
        ids.assign(seq.get_buffer(), seq.get_buffer() + seq.length());
      }

      template<typename Seq>
      void
      pack_string_vector(Seq& seq, const CampaignManagerCore::StringVector& strings)
      {
        seq.length(strings.size());
        for(CORBA::ULong i = 0; i < strings.size(); ++i)
        {
          seq[i] = strings[i].c_str();
        }
      }

      template<typename Seq>
      void
      unpack_string_vector(CampaignManagerCore::StringVector& strings, const Seq& seq)
      {
        strings.clear();
        strings.reserve(seq.length());
        for(CORBA::ULong i = 0; i < seq.length(); ++i)
        {
          strings.emplace_back(seq[i].in());
        }
      }

      template<typename Seq>
      CampaignManagerCore::StringVector
      unpack_string_vector(const Seq& seq)
      {
        CampaignManagerCore::StringVector strings;
        unpack_string_vector(strings, seq);
        return strings;
      }

      template<typename Seq>
      void
      pack_token_vector(Seq& seq, const CampaignManagerCore::TokenVector& tokens)
      {
        seq.length(tokens.size());
        for(CORBA::ULong i = 0; i < tokens.size(); ++i)
        {
          seq[i].name = tokens[i].name.c_str();
          seq[i].value = tokens[i].value.c_str();
        }
      }

      template<typename Seq>
      void
      unpack_token_vector(CampaignManagerCore::TokenVector& tokens, const Seq& seq)
      {
        tokens.clear();
        tokens.reserve(seq.length());
        for(CORBA::ULong i = 0; i < seq.length(); ++i)
        {
          tokens.push_back({seq[i].name.in(), seq[i].value.in()});
        }
      }

      template<typename Seq>
      void
      pack_channel_trigger_vector(
        Seq& seq,
        const CampaignManagerCore::ChannelTriggerMatchVector& channels)
      {
        seq.length(channels.size());
        for(CORBA::ULong i = 0; i < channels.size(); ++i)
        {
          seq[i].channel_trigger_id = channels[i].channel_trigger_id;
          seq[i].channel_id = channels[i].channel_id;
        }
      }

      template<typename Seq>
      void
      pack_ccg_keywords(
        Seq& seq,
        const CampaignManagerCore::CCGKeywordVector& keywords)
      {
        seq.length(keywords.size());
        for(CORBA::ULong i = 0; i < keywords.size(); ++i)
        {
          seq[i].ccg_keyword_id = keywords[i].ccg_keyword_id;
          seq[i].ccg_id = keywords[i].ccg_id;
          seq[i].channel_id = keywords[i].channel_id;
          seq[i].max_cpc =
            CorbaAlgs::pack_decimal<RevenueDecimal>(keywords[i].max_cpc);
          seq[i].ctr =
            CorbaAlgs::pack_decimal<RevenueDecimal>(keywords[i].ctr);
          seq[i].click_url = keywords[i].click_url.c_str();
          seq[i].original_keyword = keywords[i].original_keyword.c_str();
        }
      }

      template<typename Seq>
      CampaignManagerCore::CCGKeywordVector
      unpack_ccg_keywords(const Seq& keywords)
      {
        CampaignManagerCore::CCGKeywordVector result;
        result.reserve(keywords.length());
        for(CORBA::ULong i = 0; i < keywords.length(); ++i)
        {
          CampaignManagerCore::CCGKeywordInfo keyword;
          keyword.ccg_keyword_id = keywords[i].ccg_keyword_id;
          keyword.ccg_id = keywords[i].ccg_id;
          keyword.channel_id = keywords[i].channel_id;
          keyword.max_cpc =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(keywords[i].max_cpc);
          keyword.ctr =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(keywords[i].ctr);
          keyword.click_url = keywords[i].click_url.in();
          keyword.original_keyword = keywords[i].original_keyword.in();
          result.push_back(keyword);
        }
        return result;
      }

      void
      pack_common_ad_request_info(
        CampaignManager::CommonAdRequestInfo& out,
        const CampaignManagerCore::CommonAdRequestInfo& in)
      {
        out.time = CorbaAlgs::pack_time(in.time);
        out.request_id = CorbaAlgs::pack_request_id(in.request_id);
        out.creative_instantiate_type = in.creative_instantiate_type.c_str();
        out.request_type = in.request_type;
        out.random = in.random;
        out.test_request = in.test_request;
        out.log_as_test = in.log_as_test;
        out.colo_id = in.colo_id;
        out.external_user_id = in.external_user_id.c_str();
        out.source_id = in.source_id.c_str();
        out.location.length(in.location.size());
        for(CORBA::ULong i = 0; i < in.location.size(); ++i)
        {
          out.location[i].country = in.location[i].country.c_str();
          out.location[i].region = in.location[i].region.c_str();
          out.location[i].city = in.location[i].city.c_str();
        }
        out.coord_location.length(in.coord_location.size());
        for(CORBA::ULong i = 0; i < in.coord_location.size(); ++i)
        {
          out.coord_location[i].longitude =
            CorbaAlgs::pack_decimal<CoordDecimal>(in.coord_location[i].longitude);
          out.coord_location[i].latitude =
            CorbaAlgs::pack_decimal<CoordDecimal>(in.coord_location[i].latitude);
          out.coord_location[i].accuracy =
            CorbaAlgs::pack_decimal<CoordDecimal>(in.coord_location[i].accuracy);
        }
        out.full_referer = in.full_referer.c_str();
        out.referer = in.referer.c_str();
        pack_string_vector(out.urls, in.urls);
        out.security_token = in.security_token.c_str();
        out.pub_impr_track_url = in.pub_impr_track_url.c_str();
        out.pub_param = in.pub_param.c_str();
        out.preclick_url = in.preclick_url.c_str();
        out.click_prefix_url = in.click_prefix_url.c_str();
        out.original_url = in.original_url.c_str();
        out.track_user_id = CorbaAlgs::pack_user_id(in.track_user_id);
        out.user_id = CorbaAlgs::pack_user_id(in.user_id);
        out.user_status = in.user_status;
        out.signed_user_id = in.signed_user_id.c_str();
        out.peer_ip = in.peer_ip.c_str();
        out.user_agent = in.user_agent.c_str();
        out.cohort = in.cohort.c_str();
        out.hpos = in.hpos;
        out.ext_track_params = in.ext_track_params.c_str();
        pack_token_vector(out.tokens, in.tokens);
        out.set_cookie = in.set_cookie;
        out.passback_type = in.passback_type.c_str();
        out.passback_url = in.passback_url.c_str();
      }

      void
      pack_context_ad_request_info(
        CampaignManager::ContextAdRequestInfo& out,
        const CampaignManagerCore::ContextAdRequestInfo& in)
      {
        out.enabled_notice = in.enabled_notice;
        out.client = in.client.c_str();
        out.client_version = in.client_version.c_str();
        pack_id_vector(out.platform_ids, in.platform_ids);
        pack_id_vector(out.geo_channels, in.geo_channels);
        out.platform = in.platform.c_str();
        out.full_platform = in.full_platform.c_str();
        out.web_browser = in.web_browser.c_str();
        out.ip_hash = in.ip_hash.c_str();
        out.profile_referer = in.profile_referer;
        out.page_load_id = in.page_load_id;
        out.full_referer_hash = in.full_referer_hash;
        out.short_referer_hash = in.short_referer_hash;
      }

      void
      pack_ad_slot_info(
        CampaignManager::AdSlotInfo& out,
        const CampaignManagerCore::TraceAdSlotInfo& in)
      {
        out.ad_slot_id = in.ad_slot_id;
        out.format = in.format.c_str();
        out.tag_id = in.tag_id;
        pack_string_vector(out.sizes, in.sizes);
        out.ext_tag_id = in.ext_tag_id.c_str();
        out.min_ecpm = CorbaAlgs::pack_decimal<RevenueDecimal>(in.min_ecpm);
        out.min_ecpm_currency_code = in.min_ecpm_currency_code.c_str();
        pack_string_vector(out.currency_codes, in.currency_codes);
        out.passback = in.passback;
        out.up_expand_space = in.up_expand_space;
        out.right_expand_space = in.right_expand_space;
        out.left_expand_space = in.left_expand_space;
        out.tag_visibility = in.tag_visibility;
        out.tag_predicted_viewability = in.tag_predicted_viewability;
        out.down_expand_space = in.down_expand_space;
        out.video_min_duration = in.video_min_duration;
        out.video_max_duration = in.video_max_duration;
        out.video_skippable_max_duration = in.video_skippable_max_duration;
        out.video_allow_skippable = in.video_allow_skippable;
        out.video_allow_unskippable = in.video_allow_unskippable;
        out.video_width = in.video_width;
        out.video_height = in.video_height;
        pack_string_vector(out.exclude_categories, in.exclude_categories);
        pack_string_vector(out.required_categories, in.required_categories);
        out.debug_ccg = in.debug_ccg;
        pack_id_vector(out.allowed_durations, in.allowed_durations);
        out.native_data_tokens.length(in.native_data_tokens.size());
        for(CORBA::ULong i = 0; i < in.native_data_tokens.size(); ++i)
        {
          out.native_data_tokens[i].name = in.native_data_tokens[i].name.c_str();
          out.native_data_tokens[i].required = in.native_data_tokens[i].required;
        }
        out.native_image_tokens.length(in.native_image_tokens.size());
        for(CORBA::ULong i = 0; i < in.native_image_tokens.size(); ++i)
        {
          out.native_image_tokens[i].name =
            in.native_image_tokens[i].name.c_str();
          out.native_image_tokens[i].required =
            in.native_image_tokens[i].required;
          out.native_image_tokens[i].width = in.native_image_tokens[i].width;
          out.native_image_tokens[i].height = in.native_image_tokens[i].height;
        }
        out.native_ads_impression_tracker_type =
          in.native_ads_impression_tracker_type;
        out.fill_track_html = in.fill_track_html;
        pack_token_vector(out.tokens, in.tokens);
      }

      void
      pack_creative_request_info(
        CampaignManager::RequestParams& request_params,
        const CampaignManagerCore::CreativeRequestInfo& core_request_params)
      {
        pack_common_ad_request_info(
          request_params.common_info,
          core_request_params.common_info);
        pack_context_ad_request_info(
          request_params.context_info,
          core_request_params.context_info);
        request_params.publisher_site_id = core_request_params.publisher_site_id;
        pack_id_vector(
          request_params.publisher_account_ids,
          core_request_params.publisher_account_ids);
        request_params.fill_track_pixel = core_request_params.fill_track_pixel;
        request_params.fill_iurl = core_request_params.fill_iurl;
        request_params.ad_instantiate_type =
          core_request_params.ad_instantiate_type;
        request_params.only_display_ad = core_request_params.only_display_ad;
        pack_id_vector(
          request_params.full_freq_caps,
          core_request_params.full_freq_caps);
        request_params.seq_orders.length(core_request_params.seq_orders.size());
        for(CORBA::ULong i = 0; i < core_request_params.seq_orders.size(); ++i)
        {
          request_params.seq_orders[i].ccg_id =
            core_request_params.seq_orders[i].ccg_id;
          request_params.seq_orders[i].set_id =
            core_request_params.seq_orders[i].set_id;
          request_params.seq_orders[i].imps =
            core_request_params.seq_orders[i].imps;
        }
        request_params.campaign_freqs.length(
          core_request_params.campaign_freqs.size());
        for(CORBA::ULong i = 0; i < core_request_params.campaign_freqs.size(); ++i)
        {
          request_params.campaign_freqs[i].campaign_id =
            core_request_params.campaign_freqs[i].campaign_id;
          request_params.campaign_freqs[i].imps =
            core_request_params.campaign_freqs[i].imps;
        }
        request_params.household_id =
          CorbaAlgs::pack_user_id(core_request_params.household_id);
        request_params.merged_user_id =
          CorbaAlgs::pack_user_id(core_request_params.merged_user_id);
        request_params.search_engine_id = core_request_params.search_engine_id;
        request_params.search_words = core_request_params.search_words.c_str();
        request_params.page_keywords_present =
          core_request_params.page_keywords_present;
        request_params.profiling_available =
          core_request_params.profiling_available;
        request_params.fraud = core_request_params.fraud;
        pack_id_vector(request_params.channels, core_request_params.channels);
        pack_id_vector(request_params.hid_channels, core_request_params.hid_channels);
        pack_ccg_keywords(
          request_params.ccg_keywords,
          core_request_params.ccg_keywords);
        pack_ccg_keywords(
          request_params.hid_ccg_keywords,
          core_request_params.hid_ccg_keywords);
        pack_channel_trigger_vector(
          request_params.trigger_match_result.url_channels,
          core_request_params.trigger_match_result.url_channels);
        pack_channel_trigger_vector(
          request_params.trigger_match_result.pkw_channels,
          core_request_params.trigger_match_result.pkw_channels);
        pack_channel_trigger_vector(
          request_params.trigger_match_result.skw_channels,
          core_request_params.trigger_match_result.skw_channels);
        pack_channel_trigger_vector(
          request_params.trigger_match_result.ukw_channels,
          core_request_params.trigger_match_result.ukw_channels);
        pack_id_vector(
          request_params.trigger_match_result.uid_channels,
          core_request_params.trigger_match_result.uid_channels);
        request_params.client_create_time =
          CorbaAlgs::pack_time(core_request_params.client_create_time);
        request_params.session_start =
          CorbaAlgs::pack_time(core_request_params.session_start);
        pack_id_vector(
          request_params.exclude_pubpixel_accounts,
          core_request_params.exclude_pubpixel_accounts);
        request_params.tag_delivery_factor =
          core_request_params.tag_delivery_factor;
        request_params.ccg_delivery_factor =
          core_request_params.ccg_delivery_factor;
        request_params.preview_ccid = core_request_params.preview_ccid;
        request_params.ad_slots.length(core_request_params.ad_slots.size());
        for(CORBA::ULong i = 0; i < core_request_params.ad_slots.size(); ++i)
        {
          pack_ad_slot_info(
            request_params.ad_slots[i],
            core_request_params.ad_slots[i]);
        }
        request_params.required_passback = core_request_params.required_passback;
        request_params.profiling_type = core_request_params.profiling_type;
        request_params.disable_fraud_detection =
          core_request_params.disable_fraud_detection;
        request_params.need_debug_info = core_request_params.need_debug_info;
        request_params.page_keywords = core_request_params.page_keywords.c_str();
        request_params.url_keywords = core_request_params.url_keywords.c_str();
        request_params.ssp_location = core_request_params.ssp_location.c_str();
        request_params.additional_info = core_request_params.additional_info.c_str();
      }

      CampaignManagerCore::CreativeSelectDebugInfo
      unpack_creative_select_debug_info(
        const CampaignManager::CreativeSelectDebugInfo& in)
      {
        CampaignManagerCore::CreativeSelectDebugInfo out;
        out.imp_revenue =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(in.imp_revenue);
        out.click_revenue =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(in.click_revenue);
        out.action_revenue =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(in.action_revenue);
        out.ecpm_bid =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(in.ecpm_bid);
        out.action_adv_url = in.action_adv_url.in();
        out.html_url = in.html_url.in();
        out.triggered_expression = in.triggered_expression.in();
        out.full_expression = in.full_expression.in();
        return out;
      }

      CampaignManagerCore::CreativeSelectResultInfo
      unpack_creative_select_result_info(
        const CampaignManager::CreativeSelectResult& in)
      {
        CampaignManagerCore::CreativeSelectResultInfo out;
        out.request_id = CorbaAlgs::unpack_request_id(in.request_id);
        out.ccid = in.ccid;
        out.cmp_id = in.cmp_id;
        out.campaign_group_id = in.campaign_group_id;
        out.order_set_id = in.order_set_id;
        out.advertiser_id = in.advertiser_id;
        out.advertiser_name = in.advertiser_name.in();
        out.creative_size = in.creative_size.in();
        out.revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(in.revenue);
        out.ecpm = CorbaAlgs::unpack_decimal<RevenueDecimal>(in.ecpm);
        out.pub_ecpm = CorbaAlgs::unpack_decimal<RevenueDecimal>(in.pub_ecpm);
        out.click_url = in.click_url.in();
        out.destination_url = in.destination_url.in();
        out.creative_version_id = in.creative_version_id.in();
        out.creative_id = in.creative_id;
        out.https_safe_flag = in.https_safe_flag;
        out.expanding = in.expanding;
        return out;
      }

      CampaignManagerCore::AdSlotDebugInfo
      unpack_ad_slot_debug_info(
        const CampaignManager::AdSlotDebugInfo& in)
      {
        CampaignManagerCore::AdSlotDebugInfo out;
        out.tag_id = in.tag_id;
        out.tag_size_id = in.tag_size_id;
        out.site_id = in.site_id;
        out.site_rate_id = in.site_rate_id;
        out.min_no_adv_ecpm = in.min_no_adv_ecpm;
        out.min_text_ecpm = in.min_text_ecpm;
        out.auction_type = in.auction_type;
        out.track_pixel_url = in.track_pixel_url.in();
        out.cpm_threshold =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(in.cpm_threshold);
        out.walled_garden = in.walled_garden;
        out.selected_creatives.reserve(in.selected_creatives.length());
        for(CORBA::ULong i = 0; i < in.selected_creatives.length(); ++i)
        {
          out.selected_creatives.push_back(
            unpack_creative_select_debug_info(in.selected_creatives[i]));
        }
        out.trace_ccg = in.trace_ccg.in();
        return out;
      }

      CampaignManagerCore::ContractInfo
      unpack_contract_info(const ::AdServer::CampaignSvcs::ContractInfo& in)
      {
        CampaignManagerCore::ContractInfo out;
        out.contract_id = in.contract_id;
        out.number = in.number.in();
        out.date = in.date.in();
        out.type = in.type.in();
        out.vat_included = in.vat_included;
        out.ord_contract_id = in.ord_contract_id.in();
        out.ord_ado_id = in.ord_ado_id.in();
        out.subject_type = in.subject_type.in();
        out.action_type = in.action_type.in();
        out.agent_acting_for_publisher = in.agent_acting_for_publisher;
        out.parent_contract_id = in.parent_contract_id;
        out.client_id = in.client_id.in();
        out.client_name = in.client_name.in();
        out.client_legal_form = in.client_legal_form.in();
        out.contractor_id = in.contractor_id.in();
        out.contractor_name = in.contractor_name.in();
        out.contractor_legal_form = in.contractor_legal_form.in();
        out.timestamp = CorbaAlgs::unpack_time(in.timestamp);
        return out;
      }

      CampaignManagerCore::AdSlotResultInfo
      unpack_ad_slot_result_info(
        const CampaignManager::AdSlotResult& in,
        bool need_debug_info)
      {
        CampaignManagerCore::AdSlotResultInfo out;
        out.ad_slot_id = in.ad_slot_id;
        out.request_id = CorbaAlgs::unpack_request_id(in.request_id);
        out.passback = in.passback;
        out.passback_url = in.passback_url.in();
        out.creative_body = in.creative_body.in();
        out.notice_url = in.notice_url.in();
        unpack_string_vector(out.track_pixel_urls, in.track_pixel_urls);
        out.yandex_track_params = in.yandex_track_params.in();
        out.creative_url = in.creative_url.in();
        out.track_pixel_params = in.track_pixel_params.in();
        out.click_params = in.click_params.in();
        out.mime_format = in.mime_format.in();
        out.iurl = in.iurl.in();
        out.test_request = in.test_request;
        out.selected_creatives.reserve(in.selected_creatives.length());
        for(CORBA::ULong i = 0; i < in.selected_creatives.length(); ++i)
        {
          out.selected_creatives.push_back(
            unpack_creative_select_result_info(in.selected_creatives[i]));
        }
        unpack_string_vector(
          out.external_visual_categories,
          in.external_visual_categories);
        unpack_string_vector(
          out.external_content_categories,
          in.external_content_categories);
        out.pub_currency_code = in.pub_currency_code.in();
        out.overlay_width = in.overlay_width;
        out.overlay_height = in.overlay_height;
        unpack_token_vector(out.tokens, in.tokens);
        unpack_token_vector(out.ext_tokens, in.ext_tokens);
        out.track_impr = in.track_impr;
        out.tag_size = in.tag_size.in();
        unpack_id_vector(out.freq_caps, in.freq_caps);
        unpack_id_vector(out.uc_freq_caps, in.uc_freq_caps);
        if(need_debug_info)
        {
          out.debug_info = unpack_ad_slot_debug_info(in.debug_info);
        }
        unpack_token_vector(out.native_data_tokens, in.native_data_tokens);
        out.native_image_tokens.reserve(in.native_image_tokens.length());
        for(CORBA::ULong i = 0; i < in.native_image_tokens.length(); ++i)
        {
          out.native_image_tokens.push_back({
            in.native_image_tokens[i].name.in(),
            in.native_image_tokens[i].value.in(),
            in.native_image_tokens[i].width,
            in.native_image_tokens[i].height});
        }
        out.track_html_body = in.track_html_body.in();
        out.erid = in.erid.in();
        out.contracts.reserve(in.contracts.length());
        for(CORBA::ULong i = 0; i < in.contracts.length(); ++i)
        {
          CampaignManagerCore::ExtContractInfo contract;
          contract.contract_info = unpack_contract_info(
            in.contracts[i].contract_info);
          contract.parent_contract_id = in.contracts[i].parent_contract_id.in();
          out.contracts.push_back(contract);
        }
        return out;
      }

      void fill_expanding(
        const AdInstances::Creative::Size& size,
        CORBA::Octet& expanding)
      {
        if (size.expandable)
        {
          if (size.up_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_UP;
          }
          if (size.right_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_RIGHT;
          }
          if (size.down_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_DOWN;
          }
          if (size.left_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_LEFT;
          }
        }
      }

      void
      fill_passback_url_from_tag(
        std::string& passback_url,
        const CreativeInstantiateRule& creative_instantiate_rule,
        const Tag* tag)
      {
        if(tag && !tag->passback.empty())
        {
          passback_url = tag->passback;

          if(!creative_instantiate_rule.instantiate_relative_protocol_url(
               passback_url) &&
             !HTTP::HTTP_PREFIX.start(passback_url) &&
             !HTTP::HTTPS_PREFIX.start(passback_url))
          {
            passback_url =
              creative_instantiate_rule.local_passback_prefix + passback_url;
          }
        }
      }

      void
      init_bid_request_params(
        BidCostProvider::RequestParams& bid_request_params,
        const String::SubString& referer,
        const Tag* tag)
      {
        bid_request_params.tag_id = tag->tag_id;

        try
        {
          HTTP::BrowserAddress addr(referer);

          if (!String::case_change<String::Uniform>(
              addr.host(),
              bid_request_params.domain))
          {
            bid_request_params.domain.clear();
          }
        }
        catch(const eh::Exception&)
        {}
      }

      void
      fill_ssp_features_from_additional_info(
        CampaignSelectParams& campaign_select_params,
        const std::string& additional_info)
      {
        if(additional_info.empty())
        {
          return;
        }

        std::vector<char> json_holder(additional_info.begin(), additional_info.end());
        json_holder.push_back('\0');

        JsonValue root_value;
        JsonAllocator json_allocator;
        char* parse_end = nullptr;
        const auto status = json_parse(
          json_holder.data(),
          &parse_end,
          &root_value,
          json_allocator);

        if(status != JSON_PARSE_OK || root_value.getTag() != JSON_TAG_OBJECT)
        {
          return;
        }

        for(auto it = begin(root_value); it != end(root_value); ++it)
        {
          if(!it->key)
          {
            continue;
          }

          if(::strcmp(it->key, "ssp_tag_id") == 0 &&
            it->value.getTag() == JSON_TAG_STRING)
          {
            campaign_select_params.ssp_tag_id = it->value.toString();
          }
          else if(::strcmp(it->key, "ctr") == 0 &&
            it->value.getTag() == JSON_TAG_NUMBER)
          {
            campaign_select_params.ssp_ctr = static_cast<float>(it->value.toNumber());
          }
          else if(::strcmp(it->key, "viewability") == 0 &&
            it->value.getTag() == JSON_TAG_NUMBER)
          {
            campaign_select_params.ssp_viewability = static_cast<float>(it->value.toNumber());
          }
          else if(::strcmp(it->key, "vtr") == 0 &&
            it->value.getTag() == JSON_TAG_NUMBER)
          {
            campaign_select_params.ssp_vtr = static_cast<float>(it->value.toNumber());
          }
        }
      }
    }

    std::vector<CampaignManagerCore::GeoInfo>
    unpack_geo_info_seq(
      const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& source)
    {
      std::vector<CampaignManagerCore::GeoInfo> result;
      result.reserve(source.length());
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        result.push_back({
          source[i].country.in(),
          source[i].region.in(),
          source[i].city.in()});
      }
      return result;
    }

    std::vector<CampaignManagerCore::GeoCoordInfo>
    unpack_geo_coord_info_seq(
      const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& source)
    {
      std::vector<CampaignManagerCore::GeoCoordInfo> result;
      result.reserve(source.length());
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        result.push_back({
          CorbaAlgs::unpack_decimal<CoordDecimal>(source[i].longitude),
          CorbaAlgs::unpack_decimal<CoordDecimal>(source[i].latitude),
          CorbaAlgs::unpack_decimal<CoordDecimal>(source[i].accuracy)});
      }
      return result;
    }

    class CampaignOps
    {
    public:
      CampaignOps(const Campaign* campaign_val)
        : campaign_(campaign_val)
      {}

      const Creative* search_ccid(unsigned long ccid) noexcept
      {
        const CreativeList& creatives = campaign_->get_creatives();

        for(CreativeList::const_iterator cr_it = creatives.begin();
            cr_it != creatives.end(); ++cr_it)
        {
          if((*cr_it)->ccid == ccid)
          {
            return *cr_it;
          }
        }

        return 0;
      }

    private:
      const Campaign* campaign_;
    };

    /** CampaignManagerCore::AdSlotMinCpm */
    CampaignManagerCore::AdSlotMinCpm::AdSlotMinCpm() noexcept
      : min_pub_ecpm(RevenueDecimal::ZERO),
        min_pub_ecpm_system(RevenueDecimal::ZERO)
    {}

    /** CampaignManagerCore::AdSlotContext */
    CampaignManagerCore::AdSlotContext::AdSlotContext() noexcept
      : test_request(false),
        request_blacklisted(false),
        publisher_account_id(0)
    {}

    /**
     * Use this make-function to produce DebugInfo object and init its
     * members with default values
     */
    /*
    AdServer::CampaignSvcs::CampaignManager::DebugInfo*
    create_debug_info(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params)
    {
      AdServer::CampaignSvcs::CampaignManager::DebugInfo* debug_info =
        new AdServer::CampaignSvcs::CampaignManager::DebugInfo();
      debug_info->tag_id = 0;
      debug_info->site_id = 0;
      debug_info->site_rate_id = 0;
      debug_info->colo_id = request_params.colo_id;
      debug_info->min_no_adv_ecpm = 0;
      debug_info->min_text_ecpm = 0;
      debug_info->cpm_threshold = 0;
      debug_info->walled_garden = 0;
      return debug_info;
    }
    */

    CampaignManagerCore::CampaignManagerCore(
      const CampaignManagerConfig& configuration,
      const DomainParser::DomainConfig& domain_config,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      CampaignManagerLogger* campaign_manager_logger,
      const CreativeInstantiate& creative_instantiate,
      const char* campaigns_types)
      /*throw(InvalidArgument, Exception, eh::Exception)*/
      : campaign_manager_config_(configuration),
        callback_(ReferenceCounting::add_ref(callback)),
        logger_(ReferenceCounting::add_ref(logger)),
        domain_parser_(new DomainParser(domain_config)),
        campaign_manager_logger_(
          ReferenceCounting::add_ref(campaign_manager_logger)),
        creative_instantiate_(creative_instantiate),
        task_runner_(new Generics::TaskRunner(callback_, PARALLEL_TASKS_COUNT)),
        update_task_runner_(new Generics::TaskRunner(callback_, UPDATE_TASKS_COUNT)),
        scheduler_(new Generics::Planner(callback_)),
        rid_signer_(campaign_manager_config_.rid_private_key().c_str()),
        template_files_(new Commons::BoundedFileCache(
          campaign_manager_config_.Creative().ContentCache().size(),
          Generics::Time(campaign_manager_config_.Creative().ContentCache().timeout()),
          Commons::TextTemplateCacheConfiguration<Commons::File>(Generics::Time::ONE_SECOND)))
    {
      static const char* FUN = "CampaignManagerCore::CampaignManagerCore()";

      if(callback == 0 || logger == 0)
      {
        Stream::Error ostr;
        ostr << FUN << ":" << (callback == 0 ? " callback == 0;" : "") <<
          (logger == 0 ? " logger == 0;" : "");
        throw InvalidArgument(ostr);
      }

      token_to_parameters_.insert(std::make_pair(
        String::SubString("APPLICATION_ID"),
        "appid"));
      token_to_parameters_.insert(std::make_pair(
        String::SubString("ADVERTISING_ID"),
        "adid"));
      token_to_parameters_.insert(std::make_pair(
        String::SubString("TNS_COUNTER_DEVICE_TYPE"),
        "tdt"));
      token_to_parameters_.insert(std::make_pair(
        String::SubString("EXT_TRACK_PARAMS"),
        "ep"));

      try
      {
        add_child_object(task_runner_.in());
        add_child_object(update_task_runner_.in());
        add_child_object(scheduler_.in());

        if (campaign_manager_config_.KafkaAdsSpacesStorage().present())
        {
          kafka_producer_ = new Commons::Kafka::Producer(
            logger, callback,
            campaign_manager_config_.KafkaAdsSpacesStorage().get());
          add_child_object(kafka_producer_.in());
        }

        if (campaign_manager_config_.KafkaMatchStorage().present())
        {
          kafka_match_producer_ = new Commons::Kafka::Producer(
            logger, callback,
            campaign_manager_config_.KafkaMatchStorage().get());
          add_child_object(kafka_match_producer_.in());
        }
      }
      catch(const Generics::CompositeActiveObject::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": CompositeActiveObject::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      if (campaign_manager_config_.SecurityToken().present())
      {
        try
        {
          const xsd::AdServer::Configuration::SecurityTokenType& sec_token =
            *campaign_manager_config_.SecurityToken();
          Commons::SecTokenGenerator::Config sec_token_config;
          sec_token_config.aes_keys.push_back(sec_token.key0());
          sec_token_config.aes_keys.push_back(sec_token.key1());
          sec_token_config.aes_keys.push_back(sec_token.key2());
          sec_token_config.aes_keys.push_back(sec_token.key3());
          sec_token_config.aes_keys.push_back(sec_token.key4());
          sec_token_config.aes_keys.push_back(sec_token.key5());
          sec_token_config.aes_keys.push_back(sec_token.key6());

          sec_token_generator_ = new Commons::SecTokenGenerator(
            sec_token_config);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't initialize security token generator: " << e.what();
          throw Exception(ostr);
        }
      }

      if (campaign_manager_config_.IpEncryptConfig().present())
      {
        try
        {
          ip_crypter_ = new Commons::IPCrypter(
            campaign_manager_config_.IpEncryptConfig()->key());
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't initialize ip crypter: " << e.what();
          throw Exception(ostr);
        }
      }

      if (campaign_manager_config_.CountryWhitelist().present())
      {


        for (auto it = campaign_manager_config_.CountryWhitelist()->Country().begin();
             it != campaign_manager_config_.CountryWhitelist()->Country().end(); ++it)
        {
          country_whitelist_.push_back(it->country_code());
        }
      }

      try
      {
        campaign_config_source_ = new CampaignConfigSource(
          logger,
          domain_parser_,
          Config::CorbaConfigReader::read_multi_corba_ref(
            campaign_manager_config_.CampaignServerCorbaRef()),
          campaigns_types,
          campaign_manager_config_.Creative().creative_file_dir().c_str(),
          campaign_manager_config_.Creative().template_file_dir().c_str(),
          campaign_manager_config_.service_index(),
          creative_instantiate_.creative_rules,
          campaign_manager_config_.Creative().drop_https_safe().present() &&
            *campaign_manager_config_.Creative().drop_https_safe()
          );

        add_child_object(campaign_config_source_.in());
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't init CampaignConfigSource: " << e.what();
        throw Exception(ostr);
      }

      if(campaign_manager_config_.Billing().present() && (
           campaign_manager_config_.Billing()->check_bids() ||
           campaign_manager_config_.Billing()->confirm_bids()))
      {
        try
        {
          BillingStateContainer_var billing_state_container = new BillingStateContainer(
            callback_,
            logger,
            Config::CorbaConfigReader::read_multi_corba_ref(
              campaign_manager_config_.Billing()->BillingServerCorbaRef()),
            100, // max use count
            campaign_manager_config_.Billing()->optimize_campaign_ctr());

          if(campaign_manager_config_.Billing()->check_bids())
          {
            check_billing_state_container_ = billing_state_container;
          }

          if(campaign_manager_config_.Billing()->confirm_bids())
          {
            confirm_billing_state_container_ = billing_state_container;
          }

          add_child_object(billing_state_container.in());
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't init BillingStateContainer: " << e.what();
          throw Exception(ostr);
        }
      }

      try
      {
        CampaignManagerTaskMessage_var msg =
          new CheckConfigTaskMessage(this, update_task_runner_);
        update_task_runner_->enqueue_task(msg);

        update_task_runner_->enqueue_task(CampaignManagerTaskMessage_var(
          new UpdateCTRProviderTask(this, update_task_runner_)));

        update_task_runner_->enqueue_task(CampaignManagerTaskMessage_var(
          new UpdateConvRateProviderTask(this, update_task_runner_)));

        update_task_runner_->enqueue_task(CampaignManagerTaskMessage_var(
          new UpdateBidCostProviderTask(this, update_task_runner_)));

        msg = new FlushLogsTaskMessage(this, task_runner_);
        task_runner_->enqueue_task(msg);
      }
      catch (const CampaignManagerLogger::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CampaignManagerLogger::Exception caught: " <<
          ex.what();
        throw Exception(ostr);
      }
      catch (const Generics::Planner::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Generics::Planner::Exception caught: " << e.what();
        throw Exception(ostr);
      }
      catch (const Generics::TaskRunner::Overflow& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Generics::TaskRunner::Overflow caught: " << e.what();
        throw Exception(ostr);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }
    }

    bool CampaignManagerCore::ready() /*throw(eh::Exception)*/
    {
      CampaignIndex_var config_index = configuration_index();
      return config_index.in() != 0;
    }

    bool
    CampaignManagerCore::match_geo_channels_(
      const std::vector<GeoInfo>& location,
      const std::vector<GeoCoordInfo>& coord_location,
      ChannelIdList& geo_channels,
      ChannelIdSet& coord_channels)
      /*throw(NotReady, Exception)*/
    {
      CampaignConfig_var campaign_config = configuration();

      if (!campaign_config)
      {
        throw NotReady("Campaign configuration isn't loaded");
      }

      if(!location.empty() || !coord_location.empty())
      {
        if(!location.empty())
        {
          campaign_config->geo_channels->match(
            geo_channels,
            String::SubString(location[0].country.c_str()),
            String::SubString(location[0].region.c_str()),
            String::SubString(location[0].city.c_str()));
        }

        for(const auto& coord : coord_location)
        {
          campaign_config->geo_coord_channels->match(
            coord_channels,
            coord.longitude,
            coord.latitude,
            coord.accuracy);
        }

        return true;
      }

      return false;
    }

    void
    CampaignManagerCore::match_geo_channels(
      const std::vector<GeoInfo>& location,
      const std::vector<GeoCoordInfo>& coord_location,
      IdVector& geo_channels_result,
      IdVector& coord_channels_result)
      /*throw(NotReady, Exception)*/
    {
      // static const char* FUN = "CampaignManagerCore::match_geo_channels()";

      ChannelIdList geo_channels;
      ChannelIdSet coord_channels;

      match_geo_channels_(location, coord_location, geo_channels, coord_channels);

      geo_channels_result.assign(geo_channels.begin(), geo_channels.end());
      coord_channels_result.assign(coord_channels.begin(), coord_channels.end());
    }

    CampaignManagerCore::CreativeRequestResultInfo
    CampaignManagerCore::get_campaign_creative(
      const CreativeRequestInfo& core_request_params)
      /*throw(Exception, NotReady)*/
    {
      return get_campaign_creative_(core_request_params);
    }

    CampaignManagerCore::CreativeRequestResultInfo
    CampaignManagerCore::get_campaign_creative_(
      const CreativeRequestInfo& core_request_params)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::get_campaign_creative()";

      try
      {
        AdServer::CampaignSvcs::CampaignManager::RequestParams request_params;
        pack_creative_request_info(request_params, core_request_params);

        CreativeRequestResultInfo result;
        result.hostname = campaign_manager_config_.host();
        AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult
          idl_result;
        AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult*
          request_result = &idl_result;

        Generics::Timer process_timer;
        process_timer.start();

        AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo*
        request_debug_info = request_params.need_debug_info ?
            &request_result->debug_info : 0;

        if(request_debug_info)
        {
          request_debug_info->colo_id = request_params.common_info.colo_id;
          request_debug_info->user_group_id = 0;
        }

        CampaignConfig_var campaign_config = configuration();

        if (!campaign_config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        AdServer::CampaignSvcs::CampaignManager::RequestParams
          filtered_request_params(request_params);
        CreativeRequestInfo filtered_core_request_params(core_request_params);

        const Colocation* colocation = 0;

        // check input colo
        if(request_params.common_info.colo_id == 0 ||
           (request_params.common_info.colo_id !=
             campaign_manager_config_.colocation_id() &&
           campaign_config->colocations.find(
             request_params.common_info.colo_id) ==
             campaign_config->colocations.end()))
        {
          filtered_request_params.common_info.colo_id =
            campaign_manager_config_.colocation_id();
          filtered_core_request_params.common_info.colo_id =
            campaign_manager_config_.colocation_id();
        }

        CampaignConfig::ColocationMap::const_iterator colo_it =
          campaign_config->colocations.find(
            filtered_request_params.common_info.colo_id);

        if (colo_it != campaign_config->colocations.end())
        {
          colocation = colo_it->second;

          filtered_request_params.common_info.test_request |=
            colo_it->second->is_test();
          filtered_core_request_params.common_info.test_request |=
            colo_it->second->is_test();
        }

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;
        match_geo_channels_(
          filtered_core_request_params.common_info.location,
          filtered_core_request_params.common_info.coord_location,
          geo_channels,
          coord_channels);

        if(!coord_channels.empty())
        {
          std::copy(coord_channels.begin(),
            coord_channels.end(),
            std::back_inserter(geo_channels));
        }

        if(!geo_channels.empty())
        {
          filtered_core_request_params.context_info.geo_channels.insert(
            filtered_core_request_params.context_info.geo_channels.end(),
            geo_channels.begin(),
            geo_channels.end());
        }

        filtered_core_request_params.channels.insert(
          filtered_core_request_params.channels.end(),
          filtered_core_request_params.context_info.geo_channels.begin(),
          filtered_core_request_params.context_info.geo_channels.end());
        filtered_core_request_params.channels.insert(
          filtered_core_request_params.channels.end(),
          filtered_core_request_params.context_info.platform_ids.begin(),
          filtered_core_request_params.context_info.platform_ids.end());

        pack_id_vector(
          filtered_request_params.context_info.geo_channels,
          filtered_core_request_params.context_info.geo_channels);
        pack_id_vector(
          filtered_request_params.channels,
          filtered_core_request_params.channels);

        ChannelIdHashSet matched_channels(
          filtered_core_request_params.channels.begin(),
          filtered_core_request_params.channels.end());

        unsigned long request_tag_id = 0;

        AdServer::CampaignSvcs::RevenueDecimal adsspace_system_cpm =
          AdServer::CampaignSvcs::RevenueDecimal(false, 100000, 0);

        if(filtered_request_params.ad_slots.length())
        {
          //adsspace_system_cpm = AdServer::CampaignSvcs::RevenueDecimal::ZERO;

          // check global blacklist channel
          bool request_blacklisted = size_blacklisted_(
            campaign_config,
            matched_channels,
            0 // global blacklist
            );

          if(request_blacklisted)
          {
            filtered_request_params.common_info.user_status =
              static_cast<CORBA::ULong>(US_BLACKLISTED);
            filtered_core_request_params.common_info.user_status =
              static_cast<unsigned long>(US_BLACKLISTED);

            for(CORBA::ULong slot_i = 0;
                slot_i < filtered_request_params.ad_slots.length(); ++slot_i)
            {
              filtered_request_params.ad_slots[slot_i].passback = true;
            }
            for(auto& ad_slot : filtered_core_request_params.ad_slots)
            {
              ad_slot.passback = true;
            }
          }

          // ad request
          CreativeInstantiateRuleMap::iterator rule_it =
            creative_instantiate_.creative_rules.find(
              filtered_request_params.common_info.creative_instantiate_type.in());

          if(rule_it == creative_instantiate_.creative_rules.end())
          {
            Stream::Error err;
            err << FUN <<
              ": cannot find creative instantiate rule with name = '" <<
              filtered_request_params.common_info.creative_instantiate_type << "'.";
            throw Exception(err);
          }

          // fill common params
          filtered_request_params.tag_delivery_factor =
            Generics::safe_rand() % TAG_DELIVERY_MAX;
          filtered_request_params.ccg_delivery_factor =
            Generics::safe_rand() % TAG_DELIVERY_MAX;
          filtered_core_request_params.tag_delivery_factor =
            filtered_request_params.tag_delivery_factor;
          filtered_core_request_params.ccg_delivery_factor =
            filtered_request_params.ccg_delivery_factor;

          const Generics::Time session_start =
            filtered_core_request_params.session_start;

          AdSlotContext ad_slot_context;
          ad_slot_context.request_blacklisted = request_blacklisted;

          ad_slot_context.test_request =
            filtered_core_request_params.common_info.test_request;
          append(
            ad_slot_context.full_freq_caps,
            filtered_core_request_params.full_freq_caps);

          // process slots
          request_result->ad_slots.length(filtered_request_params.ad_slots.length());

          for(CORBA::ULong ad_slot_i = 0;
              ad_slot_i < filtered_request_params.ad_slots.length();
              ++ad_slot_i)
          {
            // !!! request_result->test_request = filtered_request_params.test_request;
            AdServer::CampaignSvcs::CampaignManager::
              AdSlotResult& ad_slot_result = request_result->ad_slots[ad_slot_i];
            const AdServer::CampaignSvcs::CampaignManager::
              AdSlotInfo& ad_slot = filtered_request_params.ad_slots[ad_slot_i];
            AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo*
              ad_slot_debug_info = filtered_request_params.need_debug_info ?
                &ad_slot_result.debug_info : 0;

            ad_slot_result.ad_slot_id = ad_slot.ad_slot_id;
            ad_slot_context.passback_url.clear();

            get_adslot_campaign_creative_(
              campaign_config,
              ad_slot_result,
              adsspace_system_cpm,
              filtered_core_request_params,
              filtered_request_params,
              session_start,
              colocation,
              filtered_core_request_params.ad_slots[ad_slot_i],
              ad_slot,
              rule_it->second,
              request_debug_info,
              ad_slot_debug_info,
              ad_slot_context,
              matched_channels,
              request_tag_id);

            if(ad_slot_result.selected_creatives.length() > 0)
            {
              ad_slot_result.tag_size << ad_slot_context.tag_size;
            }
          }
        }
        else
        {
          // profiling request
          CampaignManagerLogger::RequestInfo request_info;
          AdSlotContext ad_slot_context;
          ad_slot_context.test_request =
            filtered_request_params.common_info.test_request;

          CampaignManagerLogAdapter::fill_request_info(
            request_info,
            campaign_config,
            colocation,
            filtered_request_params.common_info,
            filtered_request_params.context_info,
            &filtered_request_params,
            request_debug_info,
            ad_slot_context);

          campaign_manager_logger_->process_request(request_info, request_params.profiling_type);
        }

        // fill request level debug info
        if(request_debug_info)
        {
          pack_id_vector(
            request_result->debug_info.geo_channels,
            filtered_core_request_params.context_info.geo_channels);
        }

        // Log ads space info
        produce_ads_space_(
          filtered_core_request_params,
          request_tag_id,
          adsspace_system_cpm);

        process_timer.stop();
        request_result->process_time = CorbaAlgs::pack_time(
          process_timer.elapsed_time());

        result.ad_slots.reserve(idl_result.ad_slots.length());
        for(CORBA::ULong i = 0; i < idl_result.ad_slots.length(); ++i)
        {
          result.ad_slots.push_back(
            unpack_ad_slot_result_info(
              idl_result.ad_slots[i],
              core_request_params.need_debug_info));
        }
        result.process_time = CorbaAlgs::unpack_time(idl_result.process_time);
        result.debug_info.colo_id = idl_result.debug_info.colo_id;
        unpack_id_vector(
          result.debug_info.geo_channels,
          idl_result.debug_info.geo_channels);
        unpack_id_vector(
          result.debug_info.platform_channels,
          idl_result.debug_info.platform_channels);
        result.debug_info.last_platform_channel_id =
          idl_result.debug_info.last_platform_channel_id;
        result.debug_info.user_group_id = idl_result.debug_info.user_group_id;
        return result;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerCore::process_match_request(
      const MatchRequestInfo& match_request_info)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::process_match_request()";

      try
      {
        CampaignConfig_var campaign_config = configuration();

        if (!campaign_config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        CampaignManagerLogger::MatchRequestInfo mri;

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;

        match_geo_channels_(
           match_request_info.match_info.location,
           match_request_info.match_info.coord_location,
           geo_channels,
           coord_channels);

        std::copy(
          coord_channels.begin(),
          coord_channels.end(),
          std::back_inserter(geo_channels));

        mri.user_id = match_request_info.user_id;
        mri.household_id = match_request_info.household_id;
        mri.time = match_request_info.request_time;
        mri.match_info.colo_id = match_request_info.match_info.colo_id;

        std::copy(
          match_request_info.match_info.channels.begin(),
          match_request_info.match_info.channels.end(),
          std::inserter(
            mri.match_info.channels,
            mri.match_info.channels.begin()));
        std::copy(
          geo_channels.begin(),
          geo_channels.end(),
          std::inserter(
            mri.match_info.channels,
            mri.match_info.channels.begin()));

        mri.match_info.triggered_page_channels.rehash(
          (mri.match_info.triggered_page_channels.size() +
            match_request_info.match_info.pkw_channels.size()) * 3);
        for(const auto& pkw_channel : match_request_info.match_info.pkw_channels)
        {
          CampaignConfig::ChannelMap::const_iterator ch_it =
            campaign_config->expression_channels.find(pkw_channel.channel_id);
          if(ch_it == campaign_config->expression_channels.end() ||
            !ch_it->second->has_params() ||
            (ch_it->second->params().type != 'D' &&
              ch_it->second->params().type != 'K'))
          {
            mri.match_info.page_triggers[pkw_channel.channel_trigger_id] =
              pkw_channel.channel_id;
          }

          mri.match_info.triggered_page_channels.insert(pkw_channel.channel_id);
        }

        std::copy(
          match_request_info.match_info.hid_channels.begin(),
          match_request_info.match_info.hid_channels.end(),
          std::inserter(
            mri.match_info.hid_channels,
            mri.match_info.hid_channels.begin()));

        CampaignConfig::ColocationMap::const_iterator colo_it =
          match_request_info.match_info.colo_id <= 0 ?
            campaign_config->colocations.end() :
            campaign_config->colocations.find(match_request_info.match_info.colo_id);
        mri.isp_offset =
          (colo_it == campaign_config->colocations.end()) ?
            Generics::Time::ZERO :
            colo_it->second->account->time_offset;

        campaign_manager_logger_->process_match_request(mri);

        // Log ads space info
        produce_match_(match_request_info);
      }
      catch (const NotReady&)
      {
        throw;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerCore::process_anonymous_request(
      const AnonymousRequestInfo& anon_request_info)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::process_anonymous_request()";

      try
      {
        CampaignConfig_var campaign_config = configuration();

        if (!campaign_config)
        {
          throw NotReady(
            "Campaign configuration isn't loaded");
        }

        CampaignManagerLogger::AnonymousRequestInfo logger_info;
        logger_info.colo_id = anon_request_info.colo_id;

        if(anon_request_info.colo_id == 0 ||
           (anon_request_info.colo_id !=
             campaign_manager_config_.colocation_id() &&
           campaign_config->colocations.find(
               anon_request_info.colo_id) ==
             campaign_config->colocations.end()))
        {
          logger_info.colo_id = campaign_manager_config_.colocation_id();
        }

        logger_info.time = anon_request_info.time;
        logger_info.user_status = static_cast<UserStatus>(anon_request_info.user_status);
        logger_info.log_as_test = anon_request_info.test_request;
        logger_info.search_engine_id = anon_request_info.search_engine_id;
        logger_info.client_app = anon_request_info.client;
        logger_info.client_app_version = anon_request_info.client_version;
        logger_info.web_browser = anon_request_info.web_browser;
        logger_info.full_platform = anon_request_info.full_platform;

        if(!anon_request_info.user_agent.empty())
        {
          logger_info.user_agent =
            new Commons::StringHolder(anon_request_info.user_agent);
        }

        logger_info.search_engine_host = anon_request_info.search_engine_host;
        logger_info.country_code = anon_request_info.country_code;
        logger_info.page_keywords_present = anon_request_info.page_keywords_present;
        std::copy(
          anon_request_info.platform_ids.begin(),
          anon_request_info.platform_ids.end(),
          std::inserter(logger_info.platforms, logger_info.platforms.begin()));

        campaign_config->platform_channels->match(
          logger_info.platform_channels,
          logger_info.platforms);
        logger_info.search_words =
          new Commons::StringHolder(anon_request_info.search_words);

        CampaignConfig::ColocationMap::const_iterator colo_it =
          campaign_config->colocations.find(anon_request_info.colo_id);

        if(colo_it != campaign_config->colocations.end())
        {
          logger_info.isp_time_offset = colo_it->second->account->time_offset;
          logger_info.isp_time = logger_info.time +
            colo_it->second->account->time_offset;
        }

        campaign_manager_logger_->process_anon_request(logger_info);
      }
      catch(const NotReady&)
      {
        throw;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }
    }

    CampaignManagerCore::ByteVector
    CampaignManagerCore::get_file(
      const std::string& file_name)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::get_file()";

      try
      {
        std::string full_file_name(campaign_manager_config_.Creative().ContentCache().root());
        full_file_name += file_name;
        const auto file = template_files_->get(full_file_name);
        return ByteVector(file->begin(), file->end());
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }
    }

    CampaignManagerCore::InstantiateAdResult
    CampaignManagerCore::instantiate_ad(
      const InstantiateAdInfo& core_info)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::instantiate_ad()";

      try
      {
        AdServer::CampaignSvcs::CampaignManager::InstantiateAdInfo
          instantiate_ad_info;

        instantiate_ad_info.common_info.time =
          CorbaAlgs::pack_time(core_info.common_info.time);
        instantiate_ad_info.common_info.request_id =
          CorbaAlgs::pack_request_id(core_info.common_info.request_id);
        instantiate_ad_info.common_info.creative_instantiate_type =
          core_info.common_info.creative_instantiate_type.c_str();
        instantiate_ad_info.common_info.request_type =
          core_info.common_info.request_type;
        instantiate_ad_info.common_info.random = core_info.common_info.random;
        instantiate_ad_info.common_info.test_request =
          core_info.common_info.test_request;
        instantiate_ad_info.common_info.log_as_test =
          core_info.common_info.log_as_test;
        instantiate_ad_info.common_info.colo_id = core_info.common_info.colo_id;
        instantiate_ad_info.common_info.external_user_id =
          core_info.common_info.external_user_id.c_str();
        instantiate_ad_info.common_info.source_id =
          core_info.common_info.source_id.c_str();
        instantiate_ad_info.common_info.location.length(
          core_info.common_info.location.size());
        for(CORBA::ULong i = 0;
          i < core_info.common_info.location.size();
          ++i)
        {
          instantiate_ad_info.common_info.location[i].country =
            core_info.common_info.location[i].country.c_str();
          instantiate_ad_info.common_info.location[i].region =
            core_info.common_info.location[i].region.c_str();
          instantiate_ad_info.common_info.location[i].city =
            core_info.common_info.location[i].city.c_str();
        }
        instantiate_ad_info.common_info.coord_location.length(
          core_info.common_info.coord_location.size());
        for(CORBA::ULong i = 0;
          i < core_info.common_info.coord_location.size();
          ++i)
        {
          instantiate_ad_info.common_info.coord_location[i].longitude =
            CorbaAlgs::pack_decimal<CoordDecimal>(
              core_info.common_info.coord_location[i].longitude);
          instantiate_ad_info.common_info.coord_location[i].latitude =
            CorbaAlgs::pack_decimal<CoordDecimal>(
              core_info.common_info.coord_location[i].latitude);
          instantiate_ad_info.common_info.coord_location[i].accuracy =
            CorbaAlgs::pack_decimal<CoordDecimal>(
              core_info.common_info.coord_location[i].accuracy);
        }
        instantiate_ad_info.common_info.full_referer =
          core_info.common_info.full_referer.c_str();
        instantiate_ad_info.common_info.referer =
          core_info.common_info.referer.c_str();
        instantiate_ad_info.common_info.urls.length(
          core_info.common_info.urls.size());
        for(CORBA::ULong i = 0; i < core_info.common_info.urls.size(); ++i)
        {
          instantiate_ad_info.common_info.urls[i] =
            core_info.common_info.urls[i].c_str();
        }
        instantiate_ad_info.common_info.security_token =
          core_info.common_info.security_token.c_str();
        instantiate_ad_info.common_info.pub_impr_track_url =
          core_info.common_info.pub_impr_track_url.c_str();
        instantiate_ad_info.common_info.pub_param =
          core_info.common_info.pub_param.c_str();
        instantiate_ad_info.common_info.preclick_url =
          core_info.common_info.preclick_url.c_str();
        instantiate_ad_info.common_info.click_prefix_url =
          core_info.common_info.click_prefix_url.c_str();
        instantiate_ad_info.common_info.original_url =
          core_info.common_info.original_url.c_str();
        instantiate_ad_info.common_info.track_user_id =
          CorbaAlgs::pack_user_id(core_info.common_info.track_user_id);
        instantiate_ad_info.common_info.user_id =
          CorbaAlgs::pack_user_id(core_info.common_info.user_id);
        instantiate_ad_info.common_info.user_status =
          core_info.common_info.user_status;
        instantiate_ad_info.common_info.signed_user_id =
          core_info.common_info.signed_user_id.c_str();
        instantiate_ad_info.common_info.peer_ip =
          core_info.common_info.peer_ip.c_str();
        instantiate_ad_info.common_info.user_agent =
          core_info.common_info.user_agent.c_str();
        instantiate_ad_info.common_info.cohort =
          core_info.common_info.cohort.c_str();
        instantiate_ad_info.common_info.hpos = core_info.common_info.hpos;
        instantiate_ad_info.common_info.ext_track_params =
          core_info.common_info.ext_track_params.c_str();
        instantiate_ad_info.common_info.tokens.length(
          core_info.common_info.tokens.size());
        for(CORBA::ULong i = 0;
          i < core_info.common_info.tokens.size();
          ++i)
        {
          instantiate_ad_info.common_info.tokens[i].name =
            core_info.common_info.tokens[i].name.c_str();
          instantiate_ad_info.common_info.tokens[i].value =
            core_info.common_info.tokens[i].value.c_str();
        }
        instantiate_ad_info.common_info.set_cookie =
          core_info.common_info.set_cookie;
        instantiate_ad_info.common_info.passback_type =
          core_info.common_info.passback_type.c_str();
        instantiate_ad_info.common_info.passback_url =
          core_info.common_info.passback_url.c_str();

        instantiate_ad_info.context_info.length(core_info.context_info.size());
        for(CORBA::ULong i = 0; i < core_info.context_info.size(); ++i)
        {
          instantiate_ad_info.context_info[i].enabled_notice =
            core_info.context_info[i].enabled_notice;
          instantiate_ad_info.context_info[i].client =
            core_info.context_info[i].client.c_str();
          instantiate_ad_info.context_info[i].client_version =
            core_info.context_info[i].client_version.c_str();
          instantiate_ad_info.context_info[i].platform_ids.length(
            core_info.context_info[i].platform_ids.size());
          for(CORBA::ULong j = 0;
            j < core_info.context_info[i].platform_ids.size();
            ++j)
          {
            instantiate_ad_info.context_info[i].platform_ids[j] =
              core_info.context_info[i].platform_ids[j];
          }
          instantiate_ad_info.context_info[i].geo_channels.length(
            core_info.context_info[i].geo_channels.size());
          for(CORBA::ULong j = 0;
            j < core_info.context_info[i].geo_channels.size();
            ++j)
          {
            instantiate_ad_info.context_info[i].geo_channels[j] =
              core_info.context_info[i].geo_channels[j];
          }
          instantiate_ad_info.context_info[i].platform =
            core_info.context_info[i].platform.c_str();
          instantiate_ad_info.context_info[i].full_platform =
            core_info.context_info[i].full_platform.c_str();
          instantiate_ad_info.context_info[i].web_browser =
            core_info.context_info[i].web_browser.c_str();
          instantiate_ad_info.context_info[i].ip_hash =
            core_info.context_info[i].ip_hash.c_str();
          instantiate_ad_info.context_info[i].profile_referer =
            core_info.context_info[i].profile_referer;
          instantiate_ad_info.context_info[i].page_load_id =
            core_info.context_info[i].page_load_id;
          instantiate_ad_info.context_info[i].full_referer_hash =
            core_info.context_info[i].full_referer_hash;
          instantiate_ad_info.context_info[i].short_referer_hash =
            core_info.context_info[i].short_referer_hash;
        }

        instantiate_ad_info.format = core_info.format.c_str();
        instantiate_ad_info.publisher_site_id = core_info.publisher_site_id;
        instantiate_ad_info.publisher_account_id =
          core_info.publisher_account_id;
        instantiate_ad_info.tag_id = core_info.tag_id;
        instantiate_ad_info.tag_size_id = core_info.tag_size_id;
        instantiate_ad_info.creatives.length(core_info.creatives.size());
        for(CORBA::ULong i = 0; i < core_info.creatives.size(); ++i)
        {
          instantiate_ad_info.creatives[i].ccid = core_info.creatives[i].ccid;
          instantiate_ad_info.creatives[i].ccg_keyword_id =
            core_info.creatives[i].ccg_keyword_id;
          instantiate_ad_info.creatives[i].request_id =
            CorbaAlgs::pack_request_id(core_info.creatives[i].request_id);
          instantiate_ad_info.creatives[i].ctr =
            CorbaAlgs::pack_decimal<RevenueDecimal>(
              core_info.creatives[i].ctr);
        }
        instantiate_ad_info.creative_id = core_info.creative_id;
        instantiate_ad_info.user_id_hash_mod.defined =
          core_info.user_id_hash_mod.present();
        instantiate_ad_info.user_id_hash_mod.value =
          core_info.user_id_hash_mod.present() ?
          *core_info.user_id_hash_mod : 0;
        instantiate_ad_info.merged_user_id =
          CorbaAlgs::pack_user_id(core_info.merged_user_id);
        instantiate_ad_info.pubpixel_accounts.length(
          core_info.pubpixel_accounts.size());
        for(CORBA::ULong i = 0; i < core_info.pubpixel_accounts.size(); ++i)
        {
          instantiate_ad_info.pubpixel_accounts[i] =
            core_info.pubpixel_accounts[i];
        }
        instantiate_ad_info.open_price = core_info.open_price.c_str();
        instantiate_ad_info.openx_price = core_info.openx_price.c_str();
        instantiate_ad_info.liverail_price = core_info.liverail_price.c_str();
        instantiate_ad_info.google_price = core_info.google_price.c_str();
        instantiate_ad_info.ext_tag_id = core_info.ext_tag_id.c_str();
        instantiate_ad_info.video_width = core_info.video_width;
        instantiate_ad_info.video_height = core_info.video_height;
        instantiate_ad_info.consider_request = core_info.consider_request;
        instantiate_ad_info.enabled_notice = core_info.enabled_notice;
        instantiate_ad_info.emulate_click = core_info.emulate_click;
        instantiate_ad_info.pub_imp_revenue_defined =
          core_info.pub_imp_revenue.has_value();
        if(core_info.pub_imp_revenue)
        {
          instantiate_ad_info.pub_imp_revenue =
            CorbaAlgs::pack_decimal<RevenueDecimal>(*core_info.pub_imp_revenue);
        }

        ConstCampaignConfig_var campaign_config = configuration();
        const Colocation* colocation = 0;
        const Tag* tag = 0;
        const Tag::Size* tag_size = 0;

        if(instantiate_ad_info.common_info.colo_id)
        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            campaign_config->colocations.find(
              instantiate_ad_info.common_info.colo_id);

          if(colo_it != campaign_config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        if(!colocation && (
             instantiate_ad_info.common_info.colo_id !=
               campaign_manager_config_.colocation_id()))
        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            campaign_config->colocations.find(
              campaign_manager_config_.colocation_id());

          if(colo_it != campaign_config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        if(!colocation)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't resolve colocation identifier defined in config";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-174");
        }

        const Creative* creative = 0;

        if(instantiate_ad_info.creative_id)
        {
          // determine size by creative
          CreativeMap::const_iterator cr_it = campaign_config->creatives.find(
            instantiate_ad_info.creative_id);
          if(cr_it != campaign_config->creatives.end())
          {
            creative = cr_it->second.in();
          }
        }

        if(instantiate_ad_info.tag_id)
        {
          TagMap::const_iterator tag_it =
            campaign_config->tags.find(instantiate_ad_info.tag_id);

          if(tag_it != campaign_config->tags.end())
          {
            tag = tag_it->second.in();
          }
        }
        else if(creative && instantiate_ad_info.publisher_account_id)
        {
          bool tag_size_found = false;

          for(Creative::SizeMap::const_iterator cs_it = creative->sizes.begin();
              cs_it != creative->sizes.end(); ++cs_it)
          {
            CampaignConfig::IdTagMap::const_iterator acc_tag_it =
              campaign_config->account_tags.find(IdTagKey(
                instantiate_ad_info.publisher_account_id,
                cs_it->second.size->protocol_name.c_str()));

            if(acc_tag_it != campaign_config->account_tags.end())
            {
              for(auto tag_it = acc_tag_it->second.begin();
                tag_it != acc_tag_it->second.end(); ++tag_it)
              {
                tag_size = match_creative_by_size_(*tag_it, creative);
                if(tag_size)
                {
                  tag = tag_it->in();
                  tag_size_found = true;
                  break;
                }
              }

              if(tag_size_found)
              {
                break;
              }
            }
          }
        }

        if(!tag)
        {
          Stream::Error ostr;
          ostr << FUN << ": tag not found";
          throw Exception(ostr);
        }

        if(!tag_size)
        {
          Tag::SizeMap::const_iterator tag_size_it = tag->sizes.find(
            instantiate_ad_info.tag_size_id);

          if(tag_size_it == tag->sizes.end())
          {
            tag_size_it = tag->sizes.begin();
          }

          if(tag_size_it != tag->sizes.end())
          {
            tag_size = tag_size_it->second;
          }
        }

        if(!tag_size)
        {
          Stream::Error ostr;
          ostr << FUN << ": tag size not found";
          throw Exception(ostr);
        }

        AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult_var
          instantiate_ad_result = new AdServer::CampaignSvcs::
            CampaignManager::InstantiateAdResult();

        AdSlotContext ad_slot_context;
        ad_slot_context.test_request =
          instantiate_ad_info.common_info.test_request;
        if(instantiate_ad_info.pub_imp_revenue_defined)
        {
          ad_slot_context.pub_imp_revenue =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.pub_imp_revenue);
        }

        CreativeInstantiateRuleMap::iterator rule_it =
          creative_instantiate_.creative_rules.find(
            instantiate_ad_info.common_info.creative_instantiate_type.in());

        if(rule_it == creative_instantiate_.creative_rules.end())
        {
          Stream::Error err;
          err << FUN <<
            ": cannot find creative instantiate rule with name = '" <<
            instantiate_ad_info.common_info.creative_instantiate_type.in() << "'.";

          throw Exception(err);
        }

        fill_passback_url_from_tag(
          ad_slot_context.passback_url,
          rule_it->second,
          tag);

        AdSelectionResult ad_selection_result;
        ad_selection_result.tag = tag;
        ad_selection_result.tag_size = tag_size;

        if(instantiate_ad_info.creatives.length() > 0)
        {
          for(CORBA::ULong creative_i = 0;
              creative_i < instantiate_ad_info.creatives.length();
              ++creative_i)
          {
            CampaignSelectionData campaign_selection_data;

            CcidMap::const_iterator creative_it =
              campaign_config->campaign_creatives.find(
                instantiate_ad_info.creatives[creative_i].ccid);

            if(creative_it != campaign_config->campaign_creatives.end() &&
               creative_it->second->campaign->advertiser)
            {
              campaign_selection_data.request_id = CorbaAlgs::unpack_request_id(
                instantiate_ad_info.creatives[creative_i].request_id);
              if(campaign_selection_data.request_id.is_null())
              {
                campaign_selection_data.request_id =
                  AdServer::Commons::RequestId::create_random_based();
              }

              campaign_selection_data.campaign =
                creative_it->second->campaign;
              campaign_selection_data.creative =
                creative_it->second;

              if(instantiate_ad_info.creatives[creative_i].ccg_keyword_id)
              {
                CCGKeywordPostClickInfoMap::const_iterator ccg_keyword_it =
                  campaign_config->ccg_keyword_click_info_map.find(
                    instantiate_ad_info.creatives[creative_i].ccg_keyword_id);

                if(ccg_keyword_it != campaign_config->ccg_keyword_click_info_map.end())
                {
                  campaign_selection_data.campaign_keyword =
                    new CampaignKeyword();
                  campaign_selection_data.campaign_keyword->channel_id = 0;
                  campaign_selection_data.campaign_keyword->max_cpc =
                    RevenueDecimal::ZERO;
                  campaign_selection_data.campaign_keyword->ctr =
                    RevenueDecimal::ZERO;
                  campaign_selection_data.campaign_keyword->campaign =
                    creative_it->second->campaign;
                  campaign_selection_data.campaign_keyword->ecpm =
                    RevenueDecimal::ZERO;

                  campaign_selection_data.campaign_keyword->ccg_keyword_id =
                    ccg_keyword_it->first;
                  campaign_selection_data.campaign_keyword->click_url =
                    ccg_keyword_it->second.click_url;
                  campaign_selection_data.campaign_keyword->original_keyword =
                    ccg_keyword_it->second.original_keyword;
                }
                else
                {
                  Stream::Error ostr;
                  ostr << FUN << ": campaign keyword not found";
                  throw Exception(ostr);
                }
              }
            }
            else
            {
              Stream::Error ostr;
              ostr << FUN << ": creative not found";
              throw Exception(ostr);
            }

            // instantiate don't use count_impression
            campaign_selection_data.count_impression = false;
            // instantiate don't use track_impr
            campaign_selection_data.track_impr = false;
            campaign_selection_data.selection_done = false;
            campaign_selection_data.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.creatives[creative_i].ctr);

            ad_selection_result.selected_campaigns.push_back(campaign_selection_data);
          }
        }
        else if(creative)
        {
          CampaignSelectionData campaign_selection_data;
          campaign_selection_data.request_id = AdServer::Commons::UserId();
          campaign_selection_data.campaign = creative->campaign;
          campaign_selection_data.creative = creative;
          campaign_selection_data.count_impression = false;
          campaign_selection_data.track_impr = false;
          campaign_selection_data.selection_done = false;
          ad_selection_result.selected_campaigns.push_back(campaign_selection_data);
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << ": creative not found";
          throw Exception(ostr);
        }

        RequestResultParams request_result_params;
        request_result_params.request_id = CorbaAlgs::unpack_request_id(
          instantiate_ad_info.common_info.request_id);

        CreativeParamsList creative_params_list;
        InstantiateParams inst_params(
          instantiate_ad_info.user_id_hash_mod.defined ?
          AdServer::Commons::Optional<unsigned long>(
            instantiate_ad_info.user_id_hash_mod.value) :
          AdServer::Commons::Optional<unsigned long>());
        inst_params.open_price = instantiate_ad_info.open_price;
        inst_params.openx_price = instantiate_ad_info.openx_price;
        inst_params.liverail_price = instantiate_ad_info.liverail_price;
        inst_params.google_price = instantiate_ad_info.google_price;
        inst_params.ext_tag_id =
          String::SubString(instantiate_ad_info.ext_tag_id);
        inst_params.video_width = instantiate_ad_info.video_width;
        inst_params.video_height = instantiate_ad_info.video_height;
        inst_params.publisher_site_id = instantiate_ad_info.publisher_site_id;
        inst_params.publisher_account_id = instantiate_ad_info.publisher_account_id;
        // don't init custom macroses (preclick) by source rule
        inst_params.init_source_macroses = false;

        CorbaAlgs::convert_sequence(
          instantiate_ad_info.pubpixel_accounts,
          inst_params.pubpixel_accounts);

        std::string creative_body;
        instantiate_creative_(
          instantiate_ad_info.common_info,
          campaign_config,
          colocation,
          inst_params,
          instantiate_ad_info.format,
          ad_selection_result,
          request_result_params,
          creative_params_list,
          creative_body,
          ad_slot_context,
          0 // exclude_pubpixel_accounts
          );
        instantiate_ad_result->creative_body << creative_body;

        instantiate_ad_result->request_ids.length(0);

        instantiate_ad_result->mime_format <<
          request_result_params.mime_format;

        if (instantiate_ad_info.emulate_click)
        {
          instantiate_ad_result->request_ids.length(
            ad_selection_result.selected_campaigns.size());

          CORBA::ULong i = 0;

          for(CampaignSelectionDataList::iterator it =
                ad_selection_result.selected_campaigns.begin();
              it != ad_selection_result.selected_campaigns.end();
              ++it, ++i)
          {
            assert(it->creative && it->campaign);

            instantiate_ad_result->request_ids[i] =
              CorbaAlgs::pack_request_id(it->request_id);
          }
        }

        if(instantiate_ad_info.consider_request)
        {
          // TODO: use empty user id
          assert(instantiate_ad_info.context_info.length() == 1);

          ChannelIdList geo_channels;

          {
            ChannelIdSet coord_channels;
            match_geo_channels_(
              unpack_geo_info_seq(instantiate_ad_info.common_info.location),
              unpack_geo_coord_info_seq(instantiate_ad_info.common_info.coord_location),
              geo_channels,
              coord_channels);

            std::copy(coord_channels.begin(),
              coord_channels.end(),
              std::back_inserter(geo_channels));
          }

          // emulate campaign selection
          CampaignManagerLogger::RequestInfo request_info;
          CampaignManagerLogger::AdRequestSelectionInfo
            ad_request_selection_info;

          CampaignManagerLogAdapter::fill_request_info(
            request_info,
            campaign_config,
            colocation,
            instantiate_ad_info.common_info,
            instantiate_ad_info.context_info[0],
            0, // request_params
            0, // ad_request_debug_info
            ad_slot_context
            );

          if(instantiate_ad_info.context_info[0].enabled_notice)
          {
            request_info.request_user_id = AdServer::Commons::UserId();
            request_info.request_user_status = US_UNDEFINED;
          }

          std::copy(geo_channels.begin(),
            geo_channels.end(),
            std::back_inserter(request_info.geo_channels));
          std::copy(geo_channels.begin(),
            geo_channels.end(),
            std::inserter(request_info.history_channels, request_info.history_channels.end()));

          CampaignSvcs::CampaignManager::AdSlotInfo ad_slot_info;
          ad_slot_info.ad_slot_id = 0;
          ad_slot_info.format = instantiate_ad_info.format;
          ad_slot_info.tag_id = instantiate_ad_info.tag_id;
          ad_slot_info.ext_tag_id = instantiate_ad_info.ext_tag_id;
          ad_slot_info.min_ecpm = CorbaAlgs::pack_decimal<RevenueDecimal>(
            RevenueDecimal::ZERO);
          ad_slot_info.passback = false;
          ad_slot_info.up_expand_space = -1;
          ad_slot_info.right_expand_space = -1;
          ad_slot_info.left_expand_space = -1;
          ad_slot_info.down_expand_space = -1;
          ad_slot_info.tag_visibility = -1;
          ad_slot_info.video_min_duration = 0;
          ad_slot_info.video_max_duration = -1;
          ad_slot_info.video_skippable_max_duration = -1;
          ad_slot_info.video_allow_skippable = true;
          ad_slot_info.video_allow_unskippable = true;

          CampaignManagerLogAdapter::fill_ad_request_selection_info(
            ad_request_selection_info,
            campaign_config,
            colocation,
            instantiate_ad_info.common_info,
            instantiate_ad_info.context_info[0],
            0, // request_params
            ad_slot_info,
            tag,
            ad_selection_result,
            ad_slot_context,
            AdSlotMinCpm(),
            tag->sizes, // log all sizes, no blacklisting on inst
            instantiate_ad_info.emulate_click);

          campaign_manager_logger_->process_ad_request(
            request_info, ad_request_selection_info);
        }

        if(instantiate_ad_info.consider_request && instantiate_ad_info.emulate_click)
        {
          ConfirmCreativeAmountArray confirm_creatives;

          const Generics::Time time = CorbaAlgs::unpack_time(
            instantiate_ad_info.common_info.time);

          for(auto it = ad_selection_result.selected_campaigns.begin();
            it != ad_selection_result.selected_campaigns.end(); ++it)
          {
            confirm_creatives.push_back(
              ConfirmCreativeAmount(it->creative->ccid, it->ctr));
          }

          // confirm imps
          confirm_amounts_(
            campaign_config,
            time,
            confirm_creatives,
            CR_CPM);

          // confirm clicks
          confirm_amounts_(
            campaign_config,
            time,
            confirm_creatives,
            CR_CPC);
        }

        InstantiateAdResult result;
        result.creative_body = instantiate_ad_result->creative_body.in();
        result.mime_format = instantiate_ad_result->mime_format.in();
        result.request_ids.reserve(instantiate_ad_result->request_ids.length());
        for(CORBA::ULong i = 0;
          i < instantiate_ad_result->request_ids.length();
          ++i)
        {
          result.request_ids.push_back(
            CorbaAlgs::unpack_request_id(instantiate_ad_result->request_ids[i]));
        }

        return result;
      }
      catch(const NotReady&)
      {
        throw;
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-174");
        throw Exception(ostr);
      }

      return InstantiateAdResult();
    }

    const Tag*
    CampaignManagerCore::resolve_tag(
      std::string* tag_size,
      unsigned long* selected_publisher_account_id,
      unsigned long request_type,
      unsigned long random,
      unsigned long publisher_site_id,
      const IdVector& publisher_account_ids,
      const CampaignConfig& campaign_config,
      unsigned long tag_id,
      const StringVector& sizes,
      const StringVector& currency_codes)
      const noexcept
    {
      if(request_type == AR_NORMAL)
      {
        TagMap::const_iterator tag_it =
          campaign_config.tags.find(tag_id);

        return (tag_it != campaign_config.tags.end()) ?
          tag_it->second.in() :
          0;
      }

      if(publisher_site_id)
      {
        for(StringVector::const_iterator size_it = sizes.begin();
            size_it != sizes.end(); ++size_it)
        {
          const CampaignConfig::IdTagMap::const_iterator site_tag_it =
            campaign_config.site_tags.find(IdTagKey(
              publisher_site_id,
              size_it->c_str()));

          if(site_tag_it != campaign_config.site_tags.end())
          {
            if(tag_size)
            {
              *tag_size = *size_it;
            }

            return site_tag_it->second[
              random % site_tag_it->second.size()];
          }
        }
      }

      if(!publisher_account_ids.empty())
      {
        std::vector<unsigned long> allowed_publisher_account_ids;
        allowed_publisher_account_ids.reserve(publisher_account_ids.size());

        if(!currency_codes.empty())
        {
          for(IdVector::const_iterator account_id_it = publisher_account_ids.begin();
              account_id_it != publisher_account_ids.end(); ++account_id_it)
          {
            auto account_it = campaign_config.accounts.find(*account_id_it);
            if(account_it != campaign_config.accounts.end())
            {
              for(StringVector::const_iterator currency_it = currency_codes.begin();
                  currency_it != currency_codes.end(); ++currency_it)
              {
                if(account_it->second->currency->currency_code == *currency_it)
                {
                  allowed_publisher_account_ids.push_back(*account_id_it);
                }
              }
            }
          }
        }
        else
        {
          allowed_publisher_account_ids = publisher_account_ids;
        }

        for(auto acc_it = allowed_publisher_account_ids.begin();
          acc_it != allowed_publisher_account_ids.end(); ++acc_it)
        {
          const unsigned long res_publisher_account_id = *acc_it;

          for(StringVector::const_iterator size_it = sizes.begin();
              size_it != sizes.end(); ++size_it)
          {
            const CampaignConfig::IdTagMap::const_iterator acc_tag_it =
              campaign_config.account_tags.find(IdTagKey(
                res_publisher_account_id,
                size_it->c_str()));

            if(acc_tag_it != campaign_config.account_tags.end())
            {
              if(tag_size)
              {
                *tag_size = *size_it;
              }

              if(selected_publisher_account_id)
              {
                *selected_publisher_account_id = res_publisher_account_id;
              }

              return acc_tag_it->second[
                random % acc_tag_it->second.size()];
            }
          }
        }

        if(selected_publisher_account_id && !allowed_publisher_account_ids.empty())
        {
          *selected_publisher_account_id = *allowed_publisher_account_ids.begin();
        }
      }

      return 0;
    }

    void
    CampaignManagerCore::get_adslot_campaign_creative_(
      const CampaignConfig* campaign_config,
      AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
      AdServer::CampaignSvcs::RevenueDecimal& adsspace_system_cpm,
      const CreativeRequestInfo& core_request_params,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const Generics::Time& session_start,
      const Colocation* colocation,
      const TraceAdSlotInfo& core_ad_slot,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      const CreativeInstantiateRule& creative_instantiate_rule,
      AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo* ad_request_debug_info,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
      AdSlotContext& ad_slot_context,
      const ChannelIdHashSet& matched_channels,
      unsigned long& request_tag_id)
      noexcept
    {
      //static const char* FUN = "CampaignManagerCore::get_adslot_campaign_creative_()";

      bool passback = core_ad_slot.passback;
      bool process_request = true;

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->cpm_threshold =
          CorbaAlgs::pack_decimal(RevenueDecimal::ZERO);
      }

      const Tag* tag = resolve_tag(
        &ad_slot_context.tag_size,
        &ad_slot_context.publisher_account_id,
        core_request_params.common_info.request_type,
        core_request_params.common_info.random,
        core_request_params.publisher_site_id,
        core_request_params.publisher_account_ids,
        *campaign_config,
        core_ad_slot.tag_id,
        core_ad_slot.sizes,
        core_ad_slot.currency_codes);

      if(tag)
      {
        /*
        if(ad_slot.currency_code[0] &&
           tag->site->account->currency->currency_code !=
             ad_slot.currency_code.in())
        {
          Stream::Error ostr;
          ostr << FUN << ": tag_id = " << tag->tag_id <<
            " publisher_id = ";
          Algs::print(
            ostr,
            request_params.publisher_account_ids.get_buffer(),
            request_params.publisher_account_ids.get_buffer() +
              request_params.publisher_account_ids.length());
          ostr <<
            " source_id = " << request_params.common_info.source_id <<
            ": Incorrect currency in request '" <<
            ad_slot.currency_code << "' instead '" <<
            tag->site->account->currency->currency_code <<
            "'(publisher currency)";

          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::TRAFFICKING_PROBLEM,
            "ADS-TF-1003");

          tag = 0; // skip request
        }
        else
        */

        {
          if (!request_tag_id)
          {
            request_tag_id = tag->tag_id;
          }
          ad_slot_result.pub_currency_code <<
            tag->site->account->currency->currency_code;
        }
      }

      if(!core_request_params.common_info.passback_url.empty())
      {
        ad_slot_context.passback_url =
          core_request_params.common_info.passback_url;
        creative_instantiate_rule.instantiate_relative_protocol_url(
          ad_slot_context.passback_url);
      }
      else
      {
        fill_passback_url_from_tag(
          ad_slot_context.passback_url,
          creative_instantiate_rule,
          tag);
      }

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->tag_id = 0;
        ad_slot_debug_info->site_id = 0;
        ad_slot_debug_info->site_rate_id = 0;
        ad_slot_debug_info->cpm_threshold =
          CorbaAlgs::pack_decimal(RevenueDecimal::ZERO);
      }

      if(tag)
      {
        ad_slot_context.test_request |= tag->is_test();

        if(process_request)
        {
          // Check & fill min_ecpm
          AdSlotMinCpm ad_slot_min_cpm;

          if (!fill_ad_slot_min_cpm_(
                ad_slot_min_cpm,
                campaign_config,
                tag,
                String::SubString(core_ad_slot.min_ecpm_currency_code),
                core_ad_slot.min_ecpm
                ))
          {
            passback = true;
          }
          else
          {
            adsspace_system_cpm = std::min(
              adsspace_system_cpm, ad_slot_min_cpm.min_pub_ecpm_system);
          }

          try
          {
            select_adslot_campaign_creative_(
              core_request_params,
              request_params,
              core_ad_slot,
              ad_slot_context,
              ad_slot_min_cpm,
              session_start,
              ad_slot_result,
              tag,
              colocation,
              passback,
              ad_request_debug_info,
              ad_slot_debug_info,
              matched_channels);
          }
          catch(const eh::Exception& ex)
          {
            logger_->sstream(Logging::Logger::EMERGENCY,
              Aspect::CAMPAIGN_MANAGER,
              "ADS-IMPL-118") <<
              "CampaignManagerCore::get_campaign_creative(): "
              "ad slot campaign selection failed for ad_slot_id=" <<
              core_ad_slot.ad_slot_id <<
              ", tag_id=" << (tag ? tag->tag_id : 0) <<
              ": " << ex.what();

            ad_slot_result.selected_creatives.length(0);

            if(ad_slot_debug_info)
            {
              ad_slot_debug_info->selected_creatives.length(0);
            }
          }

          ad_slot_result.test_request = ad_slot_context.test_request;
        }

        if(ad_request_debug_info)
        {
          pack_id_vector(
            ad_request_debug_info->geo_channels,
            core_request_params.context_info.geo_channels);
        }
      }
      else
      {
        CampaignManagerLogger::RequestInfo request_info;

        CampaignManagerLogAdapter::fill_request_info(
          request_info,
          campaign_config,
          colocation,
          request_params.common_info,
          request_params.context_info,
          &request_params,
          ad_request_debug_info,
          ad_slot_context);

        campaign_manager_logger_->process_request(request_info);
      }

      passback |=
        ad_slot_result.selected_creatives.length() == 0;

      ad_slot_result.passback = passback;

      if (core_request_params.preview_ccid &&
          ad_slot_result.selected_creatives.length())
      {
        passback = false;
      }

      if(passback && core_request_params.required_passback)
      {
        if(!ad_slot_context.passback_url.empty() &&
           core_request_params.common_info.passback_type == "redir")
        {
          std::ostringstream passback_imp_url_ostr;
          std::string mime_passback_url;

          String::StringManip::mime_url_encode(
            ad_slot_context.passback_url, mime_passback_url);

          passback_imp_url_ostr << creative_instantiate_rule.passback_pixel_url <<
            "?requestid=" << core_request_params.common_info.request_id.to_string() <<
            "&random=" << core_request_params.common_info.random <<
            "&passback=" << mime_passback_url;

          AdServer::Commons::UserId user_id =
            core_request_params.common_info.user_id;
          if(!user_id.is_null())
          {
            passback_imp_url_ostr << "&h=" <<
              AdServer::LogProcessing::user_id_distribution_hash(user_id);
          }

          if(core_request_params.common_info.log_as_test)
          {
            passback_imp_url_ostr << "&testrequest=1";
          }

          ad_slot_result.passback_url << passback_imp_url_ostr.str();
        }
        else if(tag)
        {
            std::string mime_format;
            std::string passback_body;
            instantiate_passback(
              mime_format,
              passback_body,
              campaign_config,
              colocation,
              tag,
            ad_slot.format,
              request_params,
              ad_slot_context,
              String::SubString(ad_slot.ext_tag_id));
            ad_slot_result.mime_format << mime_format;
            ad_slot_result.creative_body << passback_body;
        }
      }

      /* trace campaign if need */
      if(ad_slot_debug_info &&
         configuration_index().in() &&
         core_ad_slot.debug_ccg)
      {
        TraceRequestInfo trace_request;
        trace_request.common_info = core_request_params.common_info;
        trace_request.context_info = core_request_params.context_info;
        trace_request.publisher_site_id = core_request_params.publisher_site_id;
        trace_request.publisher_account_ids =
          core_request_params.publisher_account_ids;
        trace_request.profiling_available =
          core_request_params.profiling_available;
        trace_request.full_freq_caps = core_request_params.full_freq_caps;
        trace_request.channels = core_request_params.channels;
        trace_request.hid_channels = core_request_params.hid_channels;
        trace_request.client_create_time =
          core_request_params.client_create_time;
        trace_request.tag_delivery_factor =
          core_request_params.tag_delivery_factor;
        trace_request.ccg_delivery_factor =
          core_request_params.ccg_delivery_factor;

        const std::string trace_out = trace_campaign_selection_(
          core_ad_slot.debug_ccg,
          trace_request,
          core_ad_slot,
          ad_slot_debug_info->auction_type,
          ad_slot_context.test_request);

        ad_slot_debug_info->trace_ccg = trace_out.c_str();
      }
    }

    std::string
    CampaignManagerCore::trace_campaign_selection(
      unsigned long campaign_id,
      const TraceRequestInfo& core_request_params,
      const TraceAdSlotInfo& core_ad_slot,
      unsigned long auction_type,
      bool test_request)
      /*throw(Exception)*/
    {
      return trace_campaign_selection_(
        campaign_id,
        core_request_params,
        core_ad_slot,
        auction_type,
        test_request);
    }

    std::string
    CampaignManagerCore::trace_campaign_selection_(
      unsigned long campaign_id,
      const TraceRequestInfo& request_params,
      const TraceAdSlotInfo& ad_slot,
      unsigned long auction_type,
      bool test_request)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::trace_campaign_selection()";

      try
      {
        CampaignConfig_var used_campaign_config = configuration();
        CampaignIndex_var used_campaign_index = configuration_index();

        if (used_campaign_config.in() == 0 ||
            used_campaign_index.in() == 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't get the configuration.";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-172");

          throw Exception(ostr);
        }

        SyncPolicy::WriteGuard guard(lock_);

        std::ostringstream trace_stream;
        trace_stream << "<request>" << std::endl;

        const Tag* tag = resolve_tag(
          0,
          0, // out publisher account id
          request_params.common_info.request_type,
          request_params.common_info.random,
          request_params.publisher_site_id,
          request_params.publisher_account_ids,
          *used_campaign_config,
          ad_slot.tag_id,
          ad_slot.sizes,
          ad_slot.currency_codes);

        Generics::Time current_time = request_params.common_info.time;

        if(tag)
        {
          Campaign_var campaign;

          CampaignConfig::CampaignMap::const_iterator it =
            used_campaign_config->campaigns.find(campaign_id);

          if(it != used_campaign_config->campaigns.end())
          {
            campaign = it->second;
          }

          if(campaign.in())
          {
            CampaignIndex_var config_index(
              new CampaignIndex(*used_campaign_index, logger_));

            CampaignIndex::Key key(tag);

            key.country_code = request_params.common_info.location.empty() ?
              "" : request_params.common_info.location[0].country;
            key.format = ad_slot.format;
            key.user_status = static_cast<UserStatus>(request_params.common_info.user_status);
            key.test_request = test_request;
            key.tag_delivery_factor = request_params.tag_delivery_factor;
            key.ccg_delivery_factor = request_params.ccg_delivery_factor;

            // for trace we use uid & hid channels union
            ChannelIdHashSet triggered_channels;
            triggered_channels.insert(
              request_params.channels.begin(),
              request_params.channels.end());
            triggered_channels.insert(
              request_params.hid_channels.begin(),
              request_params.hid_channels.end());
            triggered_channels.insert(
              request_params.context_info.geo_channels.begin(),
              request_params.context_info.geo_channels.end());
		    triggered_channels.emplace(TRUE_CHANNEL_ID);

            FreqCapIdSet full_freq_caps;
            std::copy(
              request_params.full_freq_caps.begin(),
              request_params.full_freq_caps.end(),
              std::inserter(full_freq_caps, full_freq_caps.begin()));

            CreativeCategoryIdSet exclude_categories;

            convert_external_categories_(
              exclude_categories,
              *used_campaign_config,
              request_params.common_info.request_type,
              ad_slot.exclude_categories);

            CreativeCategoryIdSet required_categories;

            convert_external_categories_(
              required_categories,
              *used_campaign_config,
              request_params.common_info.request_type,
              ad_slot.required_categories);

            if(!ad_slot.required_categories.empty() && required_categories.empty())
            {
              // some category can't be resolved
              required_categories.insert(0);
            }

            AdSlotMinCpm ad_slot_min_cpm;

            fill_ad_slot_min_cpm_(
              ad_slot_min_cpm,
              used_campaign_config,
              tag,
              String::SubString(ad_slot.min_ecpm_currency_code),
              ad_slot.min_ecpm);

            std::set<unsigned long> allowed_durations;
            allowed_durations.insert(
              ad_slot.allowed_durations.begin(),
              ad_slot.allowed_durations.end());

            config_index->trace_indexing(
              key,
              current_time,
              request_params.profiling_available,
              full_freq_caps,
              request_params.common_info.colo_id,
              request_params.client_create_time,
              triggered_channels,
              request_params.common_info.user_id,
              campaign,
              ad_slot_min_cpm.min_pub_ecpm_system,
              ad_slot.up_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.up_expand_space) : 0,
              ad_slot.right_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.right_expand_space) : 0,
              ad_slot.down_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.down_expand_space) : 0,
              ad_slot.left_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.left_expand_space) : 0,
              ad_slot.tag_visibility,
              ad_slot.video_min_duration,
              ad_slot.video_max_duration >= 0 ?
                AdServer::Commons::Optional<unsigned long>(ad_slot.video_max_duration) :
                AdServer::Commons::Optional<unsigned long>(),
              ad_slot.video_skippable_max_duration >= 0 ?
                AdServer::Commons::Optional<unsigned long>(ad_slot.video_skippable_max_duration) :
                AdServer::Commons::Optional<unsigned long>(),
              ad_slot.video_allow_skippable,
              ad_slot.video_allow_unskippable,
              allowed_durations,
              exclude_categories,
              required_categories,
              AuctionType(auction_type),
              (String::SubString(request_params.common_info.creative_instantiate_type) ==
                AdInstantiateRule::SECURE),
              (request_params.common_info.request_type == AR_GOOGLE),
              trace_stream);
          }
          else
          {
            trace_stream << "  <message>can't find campaign</message>" << std::endl;
          }
        }
        else
        {
          trace_stream << "  <message>can't find tag: "
            "tag_id = " << ad_slot.tag_id <<
            ", sizes = ";
          for(StringVector::const_iterator it = ad_slot.sizes.begin();
              it != ad_slot.sizes.end(); ++it)
          {
            trace_stream << (it != ad_slot.sizes.begin() ? "," : "") << *it;
          }
          trace_stream << ", publisher_account_id = ";
          for(IdVector::const_iterator it = request_params.publisher_account_ids.begin();
              it != request_params.publisher_account_ids.end(); ++it)
          {
            trace_stream <<
              (it != request_params.publisher_account_ids.begin() ? "," : "") << *it;
          }
          trace_stream << "</message>" << std::endl;
        }

        trace_stream << "</request>" << std::endl;

        return trace_stream.str();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-174");
        throw Exception(ostr);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CORBA::SystemException caught: " << ex;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-175");
        throw Exception(ostr);
      }
    }

    std::string
    CampaignManagerCore::trace_campaign_selection_index(
      )
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::trace_campaign_selection_index()";

      try
      {
        CampaignIndex_var config_index = configuration_index();
        if (config_index.in() == 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't get the configuration.";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-176");
          throw Exception(ostr);
        }

        std::ostringstream trace_stream;
        config_index->trace_tree(trace_stream);
        return trace_stream.str();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught. : " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-176");
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerCore::get_bid_costs_(
      RevenueDecimal& low_predicted_pub_ecpm_system,
      RevenueDecimal& top_predicted_pub_ecpm_system,
      const String::SubString& referer,
      const Tag* tag,
      const RevenueDecimal& min_pub_ecpm_system,
      const CampaignSelectionDataList& selected_campaigns)
    {
      RevenueDecimal sum_campaigns_ecpm = RevenueDecimal::ZERO;

      for(CampaignSelectionDataList::const_iterator it = selected_campaigns.begin();
          it != selected_campaigns.end();
          ++it)
      {
        sum_campaigns_ecpm += it->ecpm_bid;
      }

      RevenueDecimal sum_campaigns_imp_cost_system = RevenueDecimal::div(
        sum_campaigns_ecpm,
        ECPM_FACTOR,
        Generics::DDR_FLOOR);

      RevenueDecimal init_predicted_ecpm_system = std::max(
        RevenueDecimal::mul(
          sum_campaigns_imp_cost_system, ECPM_FACTOR, Generics::DMR_CEIL),
        min_pub_ecpm_system);

      low_predicted_pub_ecpm_system = init_predicted_ecpm_system;

      /*
      std::cerr << "CampaignManagerCore::get_bid_costs_(): "
        "sum_campaigns_imp_cost_system = " << sum_campaigns_imp_cost_system <<
        ", sum_campaigns_ecpm = " << sum_campaigns_ecpm <<
        ", min_pub_ecpm_system = " << min_pub_ecpm_system << std::endl;
      */
      Commons::Optional<BidCostProvider::RequestParams> bid_request_params;

      for(CampaignSelectionDataList::const_iterator it = selected_campaigns.begin();
          it != selected_campaigns.end();
          ++it)
      {
        if(it->campaign->ccg_rate_type == CR_MAXBID)
        {
          ConstBidCostProvider_var bid_cost_provider = bid_cost_provider_.get();
          if(bid_cost_provider)
          {
            if(!bid_request_params.has_value())
            {
              bid_request_params = BidCostProvider::RequestParams();
              init_bid_request_params(*bid_request_params, referer, tag);
            }

            AdServer::Commons::Optional<RevenueDecimal> local_low_predicted_pub_ecpm_system =
              bid_cost_provider->get_bid_cost(
                *bid_request_params,
                LOW_ALLOWABLE_LOSE_WIN_PERCENTAGE,
                sum_campaigns_imp_cost_system);
            if(local_low_predicted_pub_ecpm_system.has_value())
            {
              // convert cost per impression to ecpm (0.01 / 1000)
              low_predicted_pub_ecpm_system = std::min(
                std::max(
                  RevenueDecimal::div(
                    RevenueDecimal::mul(
                      *local_low_predicted_pub_ecpm_system, ECPM_FACTOR, Generics::DMR_CEIL),
                    REVENUE_ONE + tag->cost_coef,
                    Generics::DDR_FLOOR
                  ),
                  min_pub_ecpm_system),
                sum_campaigns_ecpm);
            }

            break;
          }
        }
      }

      top_predicted_pub_ecpm_system = init_predicted_ecpm_system;

      if(!selected_campaigns.empty())
      {
        ConstBidCostProvider_var bid_cost_provider = bid_cost_provider_.get();
        if(bid_cost_provider)
        {
          if(!bid_request_params.has_value())
          {
            bid_request_params = BidCostProvider::RequestParams();
            init_bid_request_params(*bid_request_params, referer, tag);
          }

          AdServer::Commons::Optional<RevenueDecimal> local_top_predicted_pub_ecpm_system =
            bid_cost_provider->get_bid_cost(
              *bid_request_params,
              TOP_ALLOWABLE_LOSE_WIN_PERCENTAGE,
              sum_campaigns_imp_cost_system);
          if(local_top_predicted_pub_ecpm_system.has_value())
          {
            // convert cost per impression to ecpm (0.01 / 1000)
            top_predicted_pub_ecpm_system = std::min(
              std::max(
                RevenueDecimal::div(
                  RevenueDecimal::mul(
                    *local_top_predicted_pub_ecpm_system, ECPM_FACTOR, Generics::DMR_CEIL),
                  REVENUE_ONE + tag->cost_coef,
                  Generics::DDR_FLOOR
                ),
                min_pub_ecpm_system),
              sum_campaigns_ecpm);
          }
        }
      }
    }

    void
    CampaignManagerCore::select_adslot_campaign_creative_(
      const CreativeRequestInfo& core_request_params,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const TraceAdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const Generics::Time& session_start,
      AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
      const Tag* tag,
      const Colocation* colocation,
      bool passback,
      AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo* ad_request_debug_info,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
      const ChannelIdHashSet& matched_channels)
      /*throw(eh::Exception)*/
    {
      static const char* FUN =
        "CampaignManagerCore::select_adslot_campaign_creative_()";

      /* configuring response by input parameters */
      if(ad_request_debug_info)
      {
        ad_request_debug_info->colo_id = core_request_params.common_info.colo_id;
        ad_request_debug_info->user_group_id =
          core_request_params.common_info.user_id.hash() %
            MAX_TARGET_USERS_GROUPS;
      }

      CampaignIndex_var config_index = configuration_index();

      ConstCampaignConfig_var const_config = config_index.in() ?
        config_index->configuration() :
        ConstCampaignConfig_var(configuration());

      /* checking noads_timeout period */
      if (tag->site->noads_timeout != 0 &&
          session_start + tag->site->noads_timeout >
            core_request_params.common_info.time)
      {
        passback = true;
      }

      if(!config_index.in())
      {
        passback = true;
      }

      AdSelectionResult ad_selection_result;
      RequestResultParams request_result_params;

      request_result_params.request_id =
        core_request_params.common_info.request_id;

      Tag::SizeMap tag_sizes;
      if (!ad_slot_context.request_blacklisted)
      {
        for(Tag::SizeMap::const_iterator tag_size_it = tag->sizes.begin();
            tag_size_it != tag->sizes.end(); ++tag_size_it)
        {
          if(!size_blacklisted_(
               const_config,
               matched_channels,
               tag_size_it->second->size->size_id))
          {
            tag_sizes.insert(*tag_size_it);
          }
        }
      }

      if(tag_sizes.empty())
      {
        ad_slot_context.request_blacklisted = true;
        passback = true;
      }

      bool is_preview_ccid = false;

      std::string creative_body;
      std::string creative_url;

      if (core_request_params.preview_ccid &&
          preview_ccid_(
            const_config,
            colocation,
            tag,
            request_params,
            ad_slot,
            ad_slot_context,
            request_result_params,
            ad_selection_result,
            creative_body))
      {
        is_preview_ccid = true;
        passback = false;
      }

      if(!passback)
      {
        // fill tokens
        if (!is_preview_ccid)
        {
          SeqOrderMap seq_orders;

          for(const auto& core_seq_order : core_request_params.seq_orders)
          {
            SeqOrder& seq_order = seq_orders[core_seq_order.ccg_id];
            seq_order.set_id = core_seq_order.set_id;
            seq_order.imps = core_seq_order.imps;
          }

          get_site_creative_(
            config_index.in(),
            colocation,
            tag,
            tag_sizes,
            request_params,
            ad_slot,
            ad_slot_context,
            ad_slot_min_cpm,
            ad_slot_context.full_freq_caps,
            seq_orders,
            request_result_params,
            ad_selection_result,
            creative_body,
            creative_url,
            ad_request_debug_info,
            ad_slot_debug_info);
        }

        ad_slot_result.creative_body << creative_body;
        ad_slot_result.creative_url << creative_url;

        if(ad_slot_debug_info)
        {
          ad_slot_debug_info->site_id = tag->site->site_id;
          ad_slot_debug_info->tag_id = tag ? tag->tag_id : 0;
          ad_slot_debug_info->tag_size_id =
            ad_selection_result.tag_size ? ad_selection_result.tag_size->size->size_id : 0;
          ad_slot_debug_info->min_no_adv_ecpm =
            ad_selection_result.min_no_adv_ecpm.integer<unsigned long>();
          ad_slot_debug_info->min_text_ecpm =
            ad_selection_result.min_text_ecpm.integer<unsigned long>();
          ad_slot_debug_info->cpm_threshold = CorbaAlgs::pack_decimal(
            ad_selection_result.cpm_threshold);
          ad_slot_debug_info->track_pixel_url <<
            request_result_params.track_pixel_url;
          ad_slot_debug_info->walled_garden = ad_selection_result.walled_garden;
          ad_slot_debug_info->selected_creatives.length(
            ad_selection_result.selected_campaigns.size());
        }

        CorbaAlgs::fill_sequence(
          ad_selection_result.freq_caps.begin(),
          ad_selection_result.freq_caps.end(),
          ad_slot_result.freq_caps);
        CorbaAlgs::fill_sequence(
          ad_selection_result.uc_freq_caps.begin(),
          ad_selection_result.uc_freq_caps.end(),
          ad_slot_result.uc_freq_caps);

        if(!ad_selection_result.selected_campaigns.empty())
        {
          const Creative* creative =
            ad_selection_result.selected_campaigns.begin()->creative;

          ad_slot_result.erid << creative->erid;

          if(creative->initial_contract)
          {
            fill_campaign_contracts_(ad_slot_result.contracts,
              creative->initial_contract);
          }
        }

        // filtering campaign/creative selection by freq caps,
        // which presents in previously processed slots
        ad_slot_context.full_freq_caps.insert(
          ad_selection_result.freq_caps.begin(),
          ad_selection_result.freq_caps.end());
        ad_slot_context.full_freq_caps.insert(
          ad_selection_result.uc_freq_caps.begin(),
          ad_selection_result.uc_freq_caps.end());

        if(!ad_selection_result.selected_campaigns.empty())
        {
          ad_slot_result.notice_url << request_result_params.notice_url;
          if(request_params.fill_track_pixel)
          {
            if(!(request_result_params.track_pixel_url.empty() || ad_slot.fill_track_html))
            {
              ad_slot_result.track_pixel_urls.length(1);
              ad_slot_result.track_pixel_urls[0] << request_result_params.track_pixel_url;
            }

            if(!request_result_params.track_html_body.empty() && ad_slot.fill_track_html)
            {
              ad_slot_result.track_html_body << request_result_params.track_html_body;
            }

            CorbaAlgs::fill_sequence(
              request_result_params.add_track_pixel_urls.begin(),
              request_result_params.add_track_pixel_urls.end(),
              ad_slot_result.track_pixel_urls,
              true);
          }

          ad_slot_result.track_pixel_params << request_result_params.track_pixel_params;
          ad_slot_result.click_params << request_result_params.click_params;
          ad_slot_result.yandex_track_params << request_result_params.yandex_track_params;
          ad_slot_result.iurl << request_result_params.iurl;
          ad_slot_result.overlay_width = request_result_params.overlay_width;
          ad_slot_result.overlay_height = request_result_params.overlay_height;
          ad_slot_result.tokens.length(request_result_params.tokens.size());
          CORBA::ULong token_i = 0;
          for(std::map<std::string, std::string>::const_iterator token_it =
                request_result_params.tokens.begin();
              token_it != request_result_params.tokens.end();
              ++token_it, ++token_i)
          {
            ad_slot_result.tokens[token_i].name << token_it->first;
            ad_slot_result.tokens[token_i].value << token_it->second;
          }

          ad_slot_result.ext_tokens.length(request_result_params.ext_tokens.size());
          CORBA::ULong ext_token_i = 0;
          for(std::map<std::string, std::string>::const_iterator ext_token_it =
                request_result_params.ext_tokens.begin();
              ext_token_it != request_result_params.ext_tokens.end();
              ++ext_token_it, ++ext_token_i)
          {
            ad_slot_result.ext_tokens[ext_token_i].name << ext_token_it->first;
            ad_slot_result.ext_tokens[ext_token_i].value << ext_token_it->second;
          }

          // ADSC-10918 Native ads
          // Data tokens
          ad_slot_result.native_data_tokens.length(
            ad_slot.native_data_tokens.size());
          for (unsigned long token_i = 0;
               token_i < ad_slot.native_data_tokens.size(); ++token_i)
          {
            const std::string& token_name =
              ad_slot.native_data_tokens[token_i].name;
            ad_slot_result.native_data_tokens[token_i].name <<
              token_name;
            auto token_it = request_result_params.native_data_tokens.find(
              token_name);
            if (token_it != request_result_params.native_data_tokens.end())
            {
              ad_slot_result.native_data_tokens[token_i].value <<
                token_it->second;
            }
          }

          // Image tokens
          ad_slot_result.native_image_tokens.length(
            ad_slot.native_image_tokens.size());
          for (unsigned long token_i = 0;
               token_i < ad_slot.native_image_tokens.size(); ++token_i)
          {
            const std::string& token_name =
              ad_slot.native_image_tokens[token_i].name;
            ad_slot_result.native_image_tokens[token_i].name <<
              token_name;
            auto token_it = request_result_params.native_image_tokens.find(
              token_name);
            if (token_it != request_result_params.native_image_tokens.end())
            {
              ad_slot_result.native_image_tokens[token_i].value <<
                token_it->second.value;
              ad_slot_result.native_image_tokens[token_i].width =
                token_it->second.width;
              ad_slot_result.native_image_tokens[token_i].height =
                token_it->second.height;
            }
          }

          ad_slot_result.request_id = CorbaAlgs::pack_request_id(
            request_result_params.request_id);
          ad_slot_result.mime_format << request_result_params.mime_format;

          ad_slot_result.selected_creatives.length(
            ad_selection_result.selected_campaigns.size());

          CORBA::ULong i = 0;
          StringSet external_visual_categories;
          StringSet external_content_categories;

          // ad_slot_min_cpm.min_pub_ecpm_system
          RevenueDecimal low_predicted_pub_ecpm_system;
          RevenueDecimal top_predicted_pub_ecpm_system;

          {
            get_bid_costs_(
              low_predicted_pub_ecpm_system,
              top_predicted_pub_ecpm_system,
              String::SubString(request_params.common_info.referer.in()),
              tag,
              ad_slot_min_cpm.min_pub_ecpm_system,
              ad_selection_result.selected_campaigns);

            /*
            std::cerr << "get_bid_costs_: "
              "ad_slot_min_cpm.min_pub_ecpm_system = " <<
              ad_slot_min_cpm.min_pub_ecpm_system <<
              ", low_predicted_pub_ecpm_system = " <<
              low_predicted_pub_ecpm_system <<
              ", top_predicted_pub_ecpm_system = " <<
              top_predicted_pub_ecpm_system <<
              std::endl;
            */
          }

          // eval slot pub ecpm's
          RevenueDecimal slot_pub_ecpm = RevenueDecimal::ZERO;
          RevenueDecimal slot_low_predicted_pub_ecpm_system = RevenueDecimal::ZERO;
          RevenueDecimal slot_top_predicted_pub_ecpm_system = RevenueDecimal::ZERO;

          if(!ad_selection_result.selected_campaigns.empty())
          {
            RevenueDecimal selected_campaigns_count(false, ad_selection_result.selected_campaigns.size(), 0),
            slot_pub_ecpm = RevenueDecimal::div(
              ad_slot_min_cpm.min_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            slot_low_predicted_pub_ecpm_system = RevenueDecimal::div(
              low_predicted_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            slot_top_predicted_pub_ecpm_system = RevenueDecimal::div(
              top_predicted_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            (void)slot_pub_ecpm;
          }

          for(CampaignSelectionDataList::iterator it =
                ad_selection_result.selected_campaigns.begin();
              it != ad_selection_result.selected_campaigns.end();
              ++it, ++i)
          {
            assert(it->creative && it->campaign);

            it->count_impression = it->creative && !it->track_impr;

            AdServer::CampaignSvcs::CampaignManager::CreativeSelectResult& cs_result =
              ad_slot_result.selected_creatives[i];

            cs_result.request_id = CorbaAlgs::pack_request_id(it->request_id);
            cs_result.cmp_id = it->campaign->campaign_id;
            cs_result.campaign_group_id = it->campaign->campaign_group_id;
            cs_result.advertiser_id = it->campaign->advertiser->account_id;
            cs_result.advertiser_name << it->campaign->advertiser->legal_name;
            cs_result.order_set_id = it->campaign->seq_set_rotate_imps > 0 ?
              it->creative->order_set_id : 0;
            cs_result.revenue = CorbaAlgs::pack_decimal(
              it->track_impr ? RevenueDecimal::ZERO :
                it->campaign->imp_revenue);
            cs_result.ecpm = CorbaAlgs::pack_decimal(it->ecpm);

            RevenueDecimal cs_result_pub_ecpm;

            if(ad_selection_result.auction_type == AT_RANDOM)
            {
              cs_result_pub_ecpm = (i == 0 ? tag->pub_max_random_cpm :
                RevenueDecimal::ZERO);
            }
            else
            {
              //RevenueDecimal orig_ecpm_bid = it->ecpm_bid;

              // set random cost for 10% bids
              if(request_params.common_info.random % 10 == 0 && it->ecpm_bid > slot_pub_ecpm)
              {
                RevenueDecimal c = slot_pub_ecpm + RevenueDecimal::div(
                  RevenueDecimal::mul(
                    it->ecpm_bid - slot_pub_ecpm,
                    RevenueDecimal(false, Generics::safe_rand(100), 0),
                    Generics::DMR_FLOOR),
                  RevenueDecimal(false, 100, 0));
                it->ecpm_bid = c;
              }
              else
              {
                if(it->campaign->ccg_rate_type == CR_MAXBID)
                {
                  it->ecpm_bid = slot_low_predicted_pub_ecpm_system;
                }
                else
                {
                  it->ecpm_bid = slot_top_predicted_pub_ecpm_system;
                }
              }

              //std::cerr << "Bid on " << it->ecpm_bid << " instead " << orig_ecpm_bid << std::endl;

              cs_result_pub_ecpm = tag->site->account->currency->from_system_currency(
                it->ecpm_bid);
            }

            if(tag->cost_coef != RevenueDecimal::ZERO)
            {
              cs_result_pub_ecpm = RevenueDecimal::mul(
                cs_result_pub_ecpm,
                REVENUE_ONE + tag->cost_coef,
                Generics::DMR_FLOOR);
            }

            cs_result.pub_ecpm = CorbaAlgs::pack_decimal(cs_result_pub_ecpm);

            cs_result.ccid = it->creative->ccid;
            cs_result.creative_size <<
              ad_selection_result.tag_size->size->protocol_name;
            cs_result.click_url << it->click_url;
            cs_result.creative_version_id << it->creative->version_id;
            cs_result.creative_id = it->creative->creative_id;
            cs_result.https_safe_flag = it->creative->https_safe_flag;

            std::string destination_url =
              it->creative->destination_url.url();

            if (destination_url.empty() && it->campaign_keyword.in())
            {
              destination_url = it->campaign_keyword->click_url;
            }

            cs_result.destination_url << destination_url;

            AdRequestType request_type = reduce_request_type_(
              request_params.common_info.request_type);

            auto size_it = it->creative->sizes.find(
              ad_selection_result.tag_size->size->size_id);

            if (size_it != it->creative->sizes.end())
            {
              fill_expanding(
                size_it->second, cs_result.expanding);
            }

            if(request_type != AR_NORMAL)
            {
              for(Creative::CategorySet::const_iterator cat_it =
                    it->creative->categories.begin();
                  cat_it != it->creative->categories.end(); ++cat_it)
              {
                CampaignConfig::CreativeCategoryMap::const_iterator ccat_it =
                  const_config->creative_categories.find(*cat_it);
                if(ccat_it != const_config->creative_categories.end())
                {
                  CreativeCategory::ExternalCategoryMap::
                    const_iterator req_cat_it =
                      ccat_it->second.external_categories.find(request_type);

                  if(req_cat_it != ccat_it->second.external_categories.end())
                  {
                    if(ccat_it->second.cct_id == CCT_VISUAL)
                    {
                      std::copy(req_cat_it->second.begin(),
                        req_cat_it->second.end(),
                        std::inserter(external_visual_categories,
                          external_visual_categories.begin()));
                    }
                    else if(ccat_it->second.cct_id == CCT_CONTENT)
                    {
                      std::copy(req_cat_it->second.begin(),
                        req_cat_it->second.end(),
                        std::inserter(external_content_categories,
                          external_content_categories.begin()));
                    }
                  }
                }
              }
            }

            if(ad_slot_debug_info)
            {
              AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo& cs_debug =
                ad_slot_debug_info->selected_creatives[i];
              cs_debug.ecpm_bid = CorbaAlgs::pack_decimal(it->ecpm_bid);
              cs_debug.imp_revenue = CorbaAlgs::pack_decimal(it->campaign->imp_revenue);
              cs_debug.click_revenue =
                it->campaign_keyword.in() ?
                CorbaAlgs::pack_decimal(it->actual_cpc) :
                CorbaAlgs::pack_decimal(it->campaign->click_revenue);

              cs_debug.action_revenue =
                CorbaAlgs::pack_decimal(it->campaign->action_revenue);
            }
          }

          {
            ad_slot_result.external_visual_categories.length(
              external_visual_categories.size());
            CORBA::ULong res_cat_i = 0;
            for(StringSet::const_iterator cat_it = external_visual_categories.begin();
                cat_it != external_visual_categories.end();
                ++cat_it, ++res_cat_i)
            {
              ad_slot_result.external_visual_categories[res_cat_i] << *cat_it;
            }
          }

          {
            ad_slot_result.external_content_categories.length(
              external_content_categories.size());
            CORBA::ULong res_cat_i = 0;
            for(StringSet::const_iterator cat_it = external_content_categories.begin();
                cat_it != external_content_categories.end();
                ++cat_it, ++res_cat_i)
            {
              ad_slot_result.external_content_categories[res_cat_i] << *cat_it;
            }
          }

          {
            CORBA::ULong i = 0;
            ad_slot_result.tokens.length(request_result_params.tokens.size());
            for(auto it = request_result_params.tokens.begin();
                it != request_result_params.tokens.end(); ++it, ++i)
            {
              ad_slot_result.tokens[i].name << it->first;
              ad_slot_result.tokens[i].value << it->second;
            }
          }

          ad_slot_result.track_impr =
            ad_selection_result.selected_campaigns.front().track_impr;
        }
      } // !passback

      try
      {
        CampaignManagerLogger::RequestInfo request_info;
        CampaignManagerLogger::AdRequestSelectionInfo
          ad_request_selection_info;
        AdServer::CampaignSvcs::CampaignManager::AdSlotInfo log_ad_slot;
        pack_ad_slot_info(log_ad_slot, ad_slot);

        CampaignManagerLogAdapter::fill_request_info(
          request_info,
          const_config,
          colocation,
          request_params.common_info,
          request_params.context_info,
          &request_params,
          ad_request_debug_info,
          ad_slot_context);

        CampaignManagerLogAdapter::fill_ad_request_selection_info(
          ad_request_selection_info,
          const_config,
          colocation,
          request_params.common_info,
          request_params.context_info,
          &request_params,
          log_ad_slot,
          tag,
          ad_selection_result,
          ad_slot_context,
          ad_slot_min_cpm,
          tag_sizes,
          false);

        campaign_manager_logger_->process_ad_request(
          request_info, ad_request_selection_info);
      }
      catch (const CampaignManagerLogger::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-177") << FUN << ": "
          "eh::Exception caught while logging request: " << e.what();
      }
    }

    bool
    CampaignManagerCore::check_request_constraints(
      const String::SubString& referer,
      const String::SubString& original_url,
      std::string& referer_hostname_str,
      std::string& original_url_str)
      /*throw(eh::Exception)*/
    {
      try
      {
        HTTP::BrowserAddress addr(referer);

        if (!String::case_change<String::Uniform>(
              addr.host(),
              referer_hostname_str))
        {
          referer_hostname_str.clear();
        }
      }
      catch(const eh::Exception&)
      {}

      if (!String::case_change<String::Uniform>(
        original_url,
        original_url_str))
      {
        original_url_str = "";
      }

      return true;
    }

    bool
    CampaignManagerCore::get_campaign_creative_by_ccid(
      const PreviewCreativeParams& params,
      std::string& creative_body)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::get_campaign_creative_by_ccid()";

      try
      {
        return get_campaign_creative_by_ccid_impl(params, creative_body);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught. : " << e.what();
        throw Exception(ostr);
      }
    }

    bool
    CampaignManagerCore::get_campaign_creative_by_ccid_impl(
      const PreviewCreativeParams& params,
      std::string& creative_body)
      /*throw(eh::Exception)*/
    {
      // getting configuration
      CampaignConfig_var config = configuration();
      if(config.in() == 0) // can't do anything
      {
        return 0;
      }

      // selecting creative
      const Campaign* campaign = 0;
      const Creative* creative = 0;
      const Tag* tag = 0;

      for(CampaignConfig::CampaignMap::iterator it = config->campaigns.begin();
          it != config->campaigns.end(); ++it)
      {
        const Campaign* cmp = it->second;
        if((creative = CampaignOps(cmp).search_ccid(params.ccid)))
        {
          campaign = cmp;
          break;
        }
      }

      TagMap::const_iterator tag_it = config->tags.find(params.tag_id);
      if(tag_it != config->tags.end())
      {
        tag = tag_it->second;
      }

      if (creative == 0) return 0;

      const Tag::Size* tag_size = match_creative_by_size_(tag, creative);

      if(!tag_size)
      {
        return false;
      }

      return instantiate_creative_preview(
        params, config, campaign, creative, tag, *tag_size, creative_body) ? 1 : 0;
    }

    void CampaignManagerCore::consider_passback(
      const PassbackInfo& in)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::consider_passback()";

      try
      {
        campaign_manager_logger_->process_passback(
          CampaignManagerLogger::PassbackInfo(
            in.request_id,
            in.time,
            in.user_id_hash_mod));
      }
      catch(const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();

        throw Exception(ostr.str());
      }
    }

    void CampaignManagerCore::consider_passback_track(
      const PassbackTrackInfo& in)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::consider_passback_track()";

      CampaignConfig_var config = configuration();
      if (!config)
      {
        throw NotReady(
          "Campaign configuration isn't loaded");
      }

      TagMap::const_iterator tag_it = config->tags.find(in.tag_id);
      CampaignConfig::ColocationMap::const_iterator colo_it =
        config->colocations.find(in.colo_id);

      if(tag_it != config->tags.end() && colo_it != config->colocations.end())
      {
        const Tag::TagPricing* tag_pricing =
          tag_it->second->select_country_tag_pricing(in.country.c_str());

        try
        {
          CampaignManagerLogger::PassbackTrackInfo passback_track_info;
          passback_track_info.time = in.time;
          passback_track_info.country = in.country;
          passback_track_info.currency_exchange_id = config->currency_exchange_id;
          passback_track_info.colo_id = in.colo_id;
          passback_track_info.colo_rate_id = colo_it->second->colo_rate_id;
          passback_track_info.tag_id = in.tag_id;
          passback_track_info.site_rate_id = tag_pricing ? tag_pricing->site_rate_id : 0;
          passback_track_info.user_status = static_cast<UserStatus>(in.user_status);
          passback_track_info.log_as_test =
            colo_it->second->is_test() || tag_it->second->is_test();
          campaign_manager_logger_->process_passback_track(passback_track_info);
        }
        catch(const eh::Exception& ex)
        {
          std::ostringstream ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw Exception(ostr.str());
        }
      }
    }

    bool
    CampaignManagerCore::get_click_url(
      const ClickInfo& click_info,
      ClickResultInfo& click_result_info)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::get_click_url()";

      bool result = false;
      const Generics::Time click_time = click_info.time;
      const Generics::Time bid_time = click_info.bid_time;

      try
      {
        CampaignConfig_var config = configuration(true);

        std::string redirect;
        const CampaignKeywordBase* ckw = 0;

        String::StringManip::trim(click_info.relocate, redirect);

        OptionValue click_url;

        if(redirect.empty())
        {
          CCGKeywordPostClickInfoMap::const_iterator kw_it =
            config->ccg_keyword_click_info_map.find(click_info.ccg_keyword_id);

          if(kw_it != config->ccg_keyword_click_info_map.end())
          {
            ckw = &(kw_it->second);
            click_url.value = ckw->click_url;
          }
        }

        const Colocation* colocation = 0;
        const Tag* tag = 0;
        Tag::ConstSize_var tag_size;
        const Creative* creative = 0;

        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            config->colocations.find(click_info.colo_id);
          if(colo_it != config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        {
          TagMap::const_iterator tag_it = config->tags.find(click_info.tag_id);
          if(tag_it != config->tags.end())
          {
            tag = tag_it->second.in();
            Tag::SizeMap::const_iterator tag_size_it =
              tag->sizes.find(click_info.tag_size_id);
            if(tag_size_it != tag->sizes.end())
            {
              tag_size = tag_size_it->second;
            }
          }
        }

        click_result_info.advertiser_id = 0;

        if (click_info.ccid)
        {
          const CcidMap::const_iterator creative_it =
            config->campaign_creatives.find(click_info.ccid);

          if(creative_it != config->campaign_creatives.end())
          {
            creative = creative_it->second.in();
          }
        }
        else if (click_info.creative_id)
        {
          // Get creative by click_info.creative_id if click_info.ccid not defined.
          const CreativeMap::const_iterator creative_it =
            config->creatives.find(click_info.creative_id);

          if(creative_it != config->creatives.end())
          {
            creative = creative_it->second.in();
          }
        }

        if (creative)
        {
          if(redirect.empty())
          {
            if(click_url.value.empty())
            {
              click_url.value = creative->click_url.value;
            }

            click_url.option_id = creative->click_url.option_id;
          }

          // campaign_group_id is campaign_id in OIX terms
          click_result_info.campaign_id = creative->campaign->campaign_group_id;

          if (creative->campaign->advertiser)
          {
            click_result_info.advertiser_id = creative->campaign->advertiser->account_id;
          }
        }
        else
        {
          click_result_info.campaign_id = 0;
        }

        if(redirect.empty())
        {
          if(!click_url.value.empty())
          {
            try
            {
              instantiate_click_url(
                config,
                click_url,
                redirect,
                colocation ? &colocation->colo_id : 0,
                tag,
                tag_size,
                creative,
                ckw,
                click_info.tokens);
            }
            catch(const eh::Exception& ex)
            {
              logger_->sstream(Logging::Logger::NOTICE,
                Aspect::TRAFFICKING_PROBLEM) <<
                "Can't instantiate click url '" <<
                click_url.value <<
                "' (ccid = " << click_info.ccid <<
                ", creative_id = " << click_info.creative_id <<
                ", ccg_keyword_id = " << click_info.ccg_keyword_id <<
                ", tag_id = " << (tag ? tag->tag_id : 0) <<
                "): " << ex.what();

              throw;
            }
          }
        }

        if(!redirect.empty())
        {
          try
          {
            HTTP::BrowserAddress normalized_redirect(redirect);
            redirect = normalized_redirect.url();
          }
          catch(const eh::Exception& ex)
          {
            logger_->sstream(Logging::Logger::NOTICE,
              Aspect::TRAFFICKING_PROBLEM) <<
              "Invalid click url(after tokens substitution): '" <<
              redirect <<
              "' (ccid = " << click_info.ccid <<
              ", creative_id = " << click_info.creative_id <<
              ", ccg_keyword_id = " << click_info.ccg_keyword_id <<
              "): " << ex.what();

            throw;
          }

          click_result_info.url = redirect;
          result = true;
        }

        if(!click_info.request_id.is_null() && click_info.log_click)
        {
          try
          {
            campaign_manager_logger_->process_click(
              CampaignManagerLogger::ClickInfo(
                click_time,
                click_info.request_id,
                click_info.user_id_hash_mod.present() ?
                  CampaignManagerLogger::UserIdHashMod(*click_info.user_id_hash_mod) :
                  CampaignManagerLogger::UserIdHashMod(),
                click_info.referer.c_str()));
          }
          catch (const CampaignManagerLogger::Exception& e)
          {
            logger_->sstream(Logging::Logger::EMERGENCY,
              Aspect::CAMPAIGN_MANAGER,
              "ADS-IMPL-177") << FUN <<
              ": eh::Exception caught while logging request "
              "for request_id = '" <<
              click_info.request_id <<
              "': " << e.what();
          }
          catch (const AdServer::Commons::RequestId::InvalidArgument& ex)
          {
            if (logger_->log_level() >= Logging::Logger::TRACE)
            {
              logger_->sstream(Logging::Logger::TRACE,
                Aspect::CAMPAIGN_MANAGER) << FUN <<
                ": RequestId::InvalidArgument caught while logging request "
                "for request_id = '" <<
                 click_info.request_id <<
                "': " << ex.what();
            }
          }
        }

        {
          // confirm click amount if other operations success
          // otherwise frontend will call other CampaignManager
          ConfirmCreativeAmountArray creatives;
          creatives.push_back(
            ConfirmCreativeAmount(
              click_info.ccid,
              click_info.ctr));
          confirm_amounts_(config, bid_time, creatives, CR_CPC);
        }

        if(kafka_producer_ &&
          (!click_info.match_user_id.is_null() ||
            !click_info.cookie_user_id.is_null()))
        {
          const AdServer::Commons::UserId* produce_user_ids[] = {0, 0};
          unsigned long produce_user_id_i = 0;
          if(!click_info.match_user_id.is_null())
          {
            produce_user_ids[produce_user_id_i++] = &click_info.match_user_id;
          }

          if(!click_info.cookie_user_id.is_null())
          {
            produce_user_ids[produce_user_id_i++] = &click_info.cookie_user_id;
          }

          const std::vector<GeoInfo> empty_location;
          for(unsigned long i = 0; i < produce_user_id_i; ++i)
          {
            char campaign_id_str[40];
            size_t campaign_id_str_size = String::StringManip::int_to_str(
              click_result_info.campaign_id,
              campaign_id_str,
              sizeof(campaign_id_str));
            produce_ads_space_message_impl_(
              click_info.time,
              *produce_user_ids[i],
              0, // tag_id
              std::string("click-").append(campaign_id_str, campaign_id_str_size), // referer
              0, // ad_slots
              0, // publisher_account_ids
              String::SubString(),
              empty_location,
              String::SubString(),
              RevenueDecimal::ZERO,
              String::SubString());
          }
        }
      }
      catch (const NotReady&)
      {
        throw;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr.str());
      }

      return result;
    }

    void
    CampaignManagerCore::log_incoming_request(
      unsigned long tag_id,
      const String::SubString& referer,
      const IdVector& channels,
      const IdVector& full_freq_caps)
      /*throw(eh::Exception)*/
    {
      std::ostringstream ostr;
      ostr << "Incoming request: " << std::endl <<
        "  tag_id: " << tag_id << std::endl <<
        "  referer_hostname: " << referer << std::endl <<
        "  channels: " << std::endl;

      Algs::print(ostr, channels.begin(), channels.end());
      ostr << "  full freq caps: ";
      Algs::print(ostr, full_freq_caps.begin(), full_freq_caps.end());

      logger_->log(ostr.str(),
        TraceLevel::MIDDLE,
        Aspect::CAMPAIGN_MANAGER);
    }

    const Tag::Size*
    CampaignManagerCore::match_creative_by_size_(
      const Tag* tag,
      const Creative* creative)
      noexcept
    {
      for(Tag::SizeMap::const_iterator tag_size_it =
            tag->sizes.begin();
          tag_size_it != tag->sizes.end(); ++tag_size_it)
      {
        if(!creative->campaign->is_text() ||
           tag_size_it->second->max_text_creatives > 0)
        {
          Creative::SizeMap::const_iterator crs_it =
            creative->sizes.find(tag_size_it->first);
          if(crs_it != creative->sizes.end())
          {
            return tag_size_it->second;
          }
        }
      }

      return 0;
    }

    bool
    CampaignManagerCore::preview_ccid_(
      const CampaignConfig* config,
      const Colocation* colocation,
      const Tag* tag,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const TraceAdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      std::string& creative_body)
      /*throw(eh::Exception)*/
    {
      const CcidMap::const_iterator ccid_it =
        config->campaign_creatives.find(request_params.preview_ccid);

      if(ccid_it == config->campaign_creatives.end())
      {
        return false;
      }

      const Tag::Size* tag_size = match_creative_by_size_(
        tag, ccid_it->second);

      if(!tag_size)
      {
        return false;
      }

      const Creative* creative = ccid_it->second;

      if (!creative->campaign->advertiser)
      {
        return false;
      }

      select_result.tag = tag;
      select_result.tag_pricing = tag->select_tag_pricing(
        creative->campaign->country.c_str(),
        creative->campaign->ccg_type,
        creative->campaign->ccg_rate_type);
      select_result.tag_size = tag_size;

      CampaignSelectionData select_params;
      select_params.campaign = creative->campaign;
      select_params.creative = creative;
      select_params.selection_done = false; // no selection

      // use first intersect size
      select_result.selected_campaigns.push_back(select_params);

      CreativeParamsList creative_params_list;
      InstantiateParams inst_params(
        CorbaAlgs::unpack_user_id(request_params.common_info.user_id),
        request_params.context_info.enabled_notice);
      inst_params.ext_tag_id = String::SubString(ad_slot.ext_tag_id);

      inst_params.video_width = ad_slot.video_width;
      inst_params.video_height = ad_slot.video_height;
      inst_params.publisher_site_id = request_params.publisher_site_id;
      inst_params.publisher_account_id = ad_slot_context.publisher_account_id;

      instantiate_creative_(
        request_params.common_info,
        config,
        colocation,
        inst_params,
        ad_slot.format.c_str(),
        select_result,
        request_result_params,
        creative_params_list,
        creative_body,
        ad_slot_context,
        &request_params.exclude_pubpixel_accounts
        );

      if (!select_result.selected_campaigns.empty() &&
          !creative_params_list.empty())
      {
        select_result.selected_campaigns.front().click_url =
          creative_params_list.front().click_url;
      }

      return true;
    }

    void
    CampaignManagerCore::get_site_creative_(
      CampaignIndex* config_index,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::SizeMap& tag_sizes,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const TraceAdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const FreqCapIdSet& full_freq_caps,
      const SeqOrderMap& seq_orders,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      std::string& creative_body,
      std::string& creative_url,
      AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo*
        /*ad_request_debug_info*/,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo*
        ad_slot_debug_info)
      /*throw(eh::Exception)*/
    {
      ConstCampaignConfig_var config = config_index->configuration();
      std::string referer_hostname_str;
      std::string original_url_str;

      // logging incoming request
      if (logger_ && logger_->log_level() >= TraceLevel::MIDDLE)
      {
        IdVector channels;
        unpack_id_vector(channels, request_params.channels);
        IdVector full_freq_caps;
        unpack_id_vector(full_freq_caps, request_params.full_freq_caps);
        log_incoming_request(
          ad_slot.tag_id,
          String::SubString(request_params.common_info.referer.in()),
          channels,
          full_freq_caps);
      }

      if (!check_request_constraints(
            String::SubString(request_params.common_info.referer.in()),
            String::SubString(request_params.common_info.original_url.in()),
            referer_hostname_str,
            original_url_str))
      {
        return;
      }

      if (logger_->log_level() >= TraceLevel::MIDDLE)
      {
        logger_->log(
          String::SubString(
            "CampaignManagerCore::get_creative(): Weightening campaigns: \n"),
          TraceLevel::MIDDLE,
          Aspect::CAMPAIGN_MANAGER);
      }

      /*
       * Note the same 'tag visibility' checking and 'frequency caps' checking are used
       * in the function CampaignIndex::trace_indexing().
       * Please read comment in CampaignIndex::trace_indexing() if you want to modify this code.
       */
      if (CampaignIndex::check_tag_visibility(ad_slot.tag_visibility, tag) ||
          CampaignIndex::check_site_freq_cap(request_params.profiling_available, full_freq_caps, tag))
      {
        return;
      }

      convert_ccg_keywords_(
        config,
        tag,
        request_result_params.hit_keywords,
        unpack_ccg_keywords(request_params.ccg_keywords),
        request_params.profiling_available,
        full_freq_caps);

      AuctionType auction_type = AT_MAX_ECPM;
      AuctionType second_auction_type = AT_MAX_ECPM;

      // weightening campaigns
      CampaignSelector::WeightedCampaignPtr weighted_campaign;
      CampaignSelector::WeightedCampaignKeywordListPtr weighted_campaign_keywords;

      CampaignSelector campaign_selector(
        config_index, ctr_provider_.get(), conv_rate_provider_.get());
      CampaignSelectParams_var campaign_select_params_ptr(new CampaignSelectParams(
        request_params.profiling_available,
        full_freq_caps,
        seq_orders,
        colocation,
        tag,
        tag_sizes,
        request_params.common_info.request_type == AR_GOOGLE,
        ad_slot.tag_visibility,
        ad_slot.tag_predicted_viewability));

      CampaignSelectParams& campaign_select_params = *campaign_select_params_ptr;

      {
        ChannelIdHashSet triggered_channels(
          request_params.channels.get_buffer(),
          request_params.channels.get_buffer() + request_params.channels.length());
        triggered_channels.emplace(TRUE_CHANNEL_ID);

        campaign_select_params.user_id =
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id);
        campaign_select_params.country_code =
          request_params.common_info.location.length() ?
          request_params.common_info.location[0].country.in() : "";
        campaign_select_params.format = ad_slot.format;
        campaign_select_params.referer_hostname = referer_hostname_str;
        campaign_select_params.original_url = original_url_str;
        campaign_select_params.user_status = static_cast<UserStatus>(
          request_params.common_info.user_status);
        campaign_select_params.test_request = ad_slot_context.test_request;
        campaign_select_params.time = CorbaAlgs::unpack_time(request_params.common_info.time);
        campaign_select_params.tag_delivery_factor = request_params.tag_delivery_factor;
        campaign_select_params.random = request_params.common_info.random;
        campaign_select_params.random2 = Generics::safe_rand(RANDOM_PARAM_MAX);
        campaign_select_params.up_expand_space = ad_slot.up_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.up_expand_space) : 0;
        campaign_select_params.right_expand_space = ad_slot.right_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.right_expand_space) : 0;
        campaign_select_params.down_expand_space = ad_slot.down_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.down_expand_space) : 0;
        campaign_select_params.left_expand_space = ad_slot.left_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.left_expand_space) : 0;
        campaign_select_params.video_min_duration = ad_slot.video_min_duration;
        campaign_select_params.video_max_duration =
          ad_slot.video_max_duration >= 0 ?
            AdServer::Commons::Optional<unsigned long>(ad_slot.video_max_duration) :
            AdServer::Commons::Optional<unsigned long>();
        campaign_select_params.video_skippable_max_duration =
          ad_slot.video_skippable_max_duration >= 0 ?
            AdServer::Commons::Optional<unsigned long>(ad_slot.video_skippable_max_duration) :
            AdServer::Commons::Optional<unsigned long>();
        campaign_select_params.video_allow_skippable = ad_slot.video_allow_skippable;
        campaign_select_params.video_allow_unskippable = ad_slot.video_allow_unskippable;
        campaign_select_params.allowed_durations.insert(
          ad_slot.allowed_durations.begin(),
          ad_slot.allowed_durations.end());
        campaign_select_params.user_create_time = CorbaAlgs::unpack_time(
          request_params.client_create_time);
        campaign_select_params.only_display_ad = request_params.only_display_ad;
        campaign_select_params.min_pub_ecpm = ad_slot_min_cpm.min_pub_ecpm;
        campaign_select_params.min_ecpm = ad_slot_min_cpm.min_pub_ecpm_system;

        convert_external_categories_(
          campaign_select_params.exclude_categories,
          *config,
          request_params.common_info.request_type,
          ad_slot.exclude_categories);

        convert_external_categories_(
          campaign_select_params.required_categories,
          *config,
          request_params.common_info.request_type,
          ad_slot.required_categories);

        if(!ad_slot.required_categories.empty() &&
           campaign_select_params.required_categories.empty())
        {
          // some category can't be resolved
          campaign_select_params.required_categories.insert(0);
        }

        // fill campaign_select_params fields required for CTR calculate
        campaign_select_params.ext_tag_id = ad_slot.ext_tag_id;
        fill_ssp_features_from_additional_info(
          campaign_select_params,
          request_params.additional_info.in());
        campaign_select_params.short_referer_hash = request_params.context_info.short_referer_hash;
        campaign_select_params.channels = triggered_channels;
        //CorbaAlgs::convert_sequence(request_params.channels, campaign_select_params.channels_array);

        campaign_select_params.last_platform_channel_id = 0;
        campaign_select_params.time_hour = campaign_select_params.time.get_gm_time().tm_hour;
        campaign_select_params.time_week_day = ((
          campaign_select_params.time /
          Generics::Time::ONE_DAY.tv_sec).tv_sec + 3) % 7;
        CorbaAlgs::convert_sequence(
          request_params.context_info.geo_channels, campaign_select_params.geo_channels);
        campaign_select_params.secure =
          (request_params.common_info.creative_instantiate_type == AdInstantiateRule::SECURE);
        for(CORBA::ULong i = 0; i < request_params.campaign_freqs.length(); ++i)
        {
          campaign_select_params.campaign_imps.insert(
            std::make_pair(
              request_params.campaign_freqs[i].campaign_id,
              std::min(
                static_cast<unsigned long>(request_params.campaign_freqs[i].imps),
                100ul)));
        }

        StringSet norm_platform_names;

        if(config)
        {
          // TODO: remove equal block from CampaignManagerLogAdapter
          ChannelIdSet platform_channels;
          ChannelIdHashSet platforms(
            request_params.context_info.platform_ids.get_buffer(),
            request_params.context_info.platform_ids.get_buffer() + request_params.context_info.platform_ids.length());

          config->platform_channels->match(
            platform_channels,
            platforms);

          for(auto platform_id_it = platforms.begin();
            platform_id_it != platforms.end();
            ++platform_id_it)
          {
            auto platform_it = config->platforms.find(*platform_id_it);

            if(platform_it != config->platforms.end())
            {
              norm_platform_names.insert(platform_it->second);
            }
          }

          unsigned long cur_priority = 0;

          for(ChannelIdSet::const_iterator pch_it = platform_channels.begin();
              pch_it != platform_channels.end();
              ++pch_it)
          {
            CampaignConfig::PlatformChannelPriorityMap::const_iterator pr_it =
              config->platform_channel_priorities.find(*pch_it);
            if(pr_it != config->platform_channel_priorities.end())
            {
              if(campaign_select_params.last_platform_channel_id == 0 ||
                 cur_priority < pr_it->second.priority)
              {
                cur_priority = pr_it->second.priority;
                campaign_select_params.last_platform_channel_id = *pch_it;
              }

              norm_platform_names.insert(pr_it->second.norm_name);
            }
          }
        }

        fill_tns_counter_device_type_(
          ad_slot_context.tns_counter_device_type,
          norm_platform_names);

        for(TokenVector::const_iterator token_it = ad_slot.tokens.begin();
            token_it != ad_slot.tokens.end(); ++token_it)
        {
          ad_slot_context.tokens.emplace(token_it->name, token_it->value);
        }

        // determine auction type, using order: max ecpm, prop probability, random
        {
          RevenueDecimal auction_offset = RevenueDecimal::div(
            RevenueDecimal(false, campaign_select_params.random % RANDOM_PARAM_MAX, 0),
            RevenueDecimal(false, RANDOM_PARAM_MAX, 0));

          if(auction_offset >= tag->auction_max_ecpm_share)
          {
            if(auction_offset <
               tag->auction_max_ecpm_share + tag->auction_prop_probability_share)
            {
              auction_type = AT_PROPORTIONAL_PROBABILITY;
            }
            else
            {
              auction_type = AT_RANDOM;
              if(tag->auction_prop_probability_share + tag->auction_max_ecpm_share ==
                 RevenueDecimal::ZERO)
              {
                second_auction_type = AT_RANDOM;
              }
              else
              {
                // calculate auction type for second auction
                auction_offset -=
                  (tag->auction_max_ecpm_share +
                   tag->auction_prop_probability_share);
                if(RevenueDecimal::div(RevenueDecimal::mul(
                      auction_offset,
                      tag->auction_prop_probability_share +
                      tag->auction_max_ecpm_share, Generics::DMR_ROUND),
                    tag->auction_random_share) >=
                  tag->auction_max_ecpm_share)
                {
                  second_auction_type = AT_PROPORTIONAL_PROBABILITY;
                }
              }
            }
          }
        }

        campaign_selector.select_campaigns(
          auction_type,
          second_auction_type,
          campaign_select_params_ptr,
          triggered_channels,
          request_result_params.hit_keywords,
          true, // collect lost
          weighted_campaign_keywords,
          weighted_campaign,
          select_result);

        assert(
          (weighted_campaign.get() || (
            weighted_campaign_keywords.get() && !weighted_campaign_keywords->empty())) ?
          (select_result.tag_size != 0) : true);

        /*
        if(!CorbaAlgs::unpack_user_id(request_params.household_id).is_null() &&
           weighted_campaign.get() == 0 &&
           weighted_campaign_keywords.get() == 0)
        {
          // try to select household based campaign
          ChannelIdHashSet hid_triggered_channels;
          CampaignKeywordMap hid_hit_keywords;

          CorbaAlgs::convert_sequence(
            request_params.hid_channels, hid_triggered_channels);
          CorbaAlgs::convert_sequence(
            request_params.context_info.platform_ids, hid_triggered_channels);
          CorbaAlgs::convert_sequence(
            request_params.context_info.geo_channels, hid_triggered_channels);

          convert_ccg_keywords_(
            config,
            tag,
            hid_hit_keywords,
            unpack_ccg_keywords(request_params.hid_ccg_keywords),
            request_params.profiling_available,
            full_freq_caps);

          campaign_selector.select_campaigns(
            auction_type,
            second_auction_type,
            campaign_select_params_ptr,
            hid_triggered_channels,
            hid_hit_keywords,
            false, // collect lost
            weighted_campaign_keywords,
            weighted_campaign,
            select_result);

          if(weighted_campaign.get() != 0 ||
            weighted_campaign_keywords.get() != 0)
          {
            select_result.household_based = true;
          }
        }
        */
      }

      if(check_billing_state_container_)
      {
        if(weighted_campaign_keywords.get())
        {
          // TODO: one call for all keywords
          // clear weighted_campaign_keywords if showing not allowed
          bool available = true;

          for(CampaignSelector::WeightedCampaignKeywordList::const_iterator cmp_it =
              weighted_campaign_keywords->begin();
            cmp_it != weighted_campaign_keywords->end(); ++cmp_it)
          {
            const BillingStateContainer::BidCheckResult check_result =
              check_billing_state_container_->check_available_bid(
                campaign_select_params.time,
                cmp_it->campaign->advertiser->bill_account_id(),
                cmp_it->campaign->advertiser->not_bill_account_id(),
                cmp_it->campaign->campaign_group_id,
                cmp_it->campaign->campaign_id,
                cmp_it->ctr,
                cmp_it->campaign);

            available &= apply_check_available_bid_result_(
              cmp_it->campaign,
              check_result,
              cmp_it->ctr);
          }

          if(!available)
          {
            weighted_campaign_keywords.reset(nullptr);
          }
        }

        if(weighted_campaign.get())
        {
          // clear weighted_campaign if showing not allowed
          const BillingStateContainer::BidCheckResult check_result =
            check_billing_state_container_->check_available_bid(
              campaign_select_params.time,
              weighted_campaign->campaign->advertiser->bill_account_id(),
              weighted_campaign->campaign->advertiser->not_bill_account_id(),
              weighted_campaign->campaign->campaign_group_id,
              weighted_campaign->campaign->campaign_id,
              weighted_campaign->ctr,
              weighted_campaign->campaign);

          if(!apply_check_available_bid_result_(
               weighted_campaign->campaign,
               check_result,
               weighted_campaign->ctr))
          {
            weighted_campaign.reset(nullptr);
          }
        }
      }

      // instantiate creatives
      bool text_creative_selected = false;
      select_result.text_campaigns = false;

      if(weighted_campaign_keywords.get() &&
         !weighted_campaign_keywords->empty())
      {
        assert(select_result.tag_size);

        CreativeParamsList creative_params_list;

        text_creative_selected |= instantiate_text_creatives(
          config,
          colocation,
          request_params,
          ad_slot,
          *weighted_campaign_keywords.get(),
          select_result,
          request_result_params,
          creative_params_list,
          ad_slot_debug_info,
          creative_body,
          creative_url,
          ad_slot_context);

        if(!text_creative_selected)
        {
          select_result.selected_campaigns.clear();
        }

        select_result.text_campaigns = text_creative_selected;
      }

      bool display_creative_selected = false;

      if(weighted_campaign.get() && !text_creative_selected)
      {
        size_t display_try_number = 0;

        while(!display_creative_selected && display_try_number++ < 2)
        {
          CreativeParams creative_params;
          AdSelectionResult display_ad_selection_result(select_result);

          display_creative_selected |= instantiate_display_creative(
            config,
            colocation,
            request_params,
            ad_slot,
            *weighted_campaign.get(),
            display_ad_selection_result,
            request_result_params,
            creative_params,
            ad_slot_debug_info,
            creative_body,
            creative_url,
            ad_slot_context);

          if(display_creative_selected)
          {
            select_result = display_ad_selection_result;
          }
        }
      }

      if(!select_result.selected_campaigns.empty())
      {
        // updating freq_caps
        if(tag->site->freq_cap_id)
        {
          select_result.freq_caps.insert(tag->site->freq_cap_id);
        }
      }

      ConfirmCreativeAmountArray confirm_creatives;

      for(CampaignSelectionDataList::iterator cs_it =
            select_result.selected_campaigns.begin();
          cs_it != select_result.selected_campaigns.end(); ++cs_it)
      {
        const CampaignSelectionData& select_params = *cs_it;

        assert(cs_it->campaign && cs_it->creative);

        const Campaign* campaign = cs_it->campaign;
        const Creative* creative = cs_it->creative;

        if(!select_params.track_impr)
        {
          confirm_creatives.push_back(ConfirmCreativeAmount(creative->ccid, select_params.ctr));
        }

        // fill imp
        {
          CampaignSelectParams::CampaignImpsMap::const_iterator cmp_it =
            campaign_select_params.campaign_imps.find(campaign->campaign_group_id);
          cs_it->campaign_imps = (
            cmp_it != campaign_select_params.campaign_imps.end() ?
            cmp_it->second : 0);
        }

        // updating freq caps
        FreqCapIdSet& result_fc_set =
          select_params.track_impr ? select_result.uc_freq_caps :
          select_result.freq_caps;

        if(creative->fc_id)
        {
          result_fc_set.insert(creative->fc_id);
        }

        if(campaign->fc_id)
        {
          result_fc_set.insert(campaign->fc_id);
        }

        if(campaign->group_fc_id)
        {
          result_fc_set.insert(campaign->group_fc_id);
        }

        if(cs_it->campaign_keyword.in())
        {
          CampaignConfig::ChannelMap::const_iterator ch_it =
            config->expression_channels.find(cs_it->campaign_keyword->channel_id);
          if(ch_it != config->expression_channels.end() &&
             ch_it->second->params().common_params.in() &&
             ch_it->second->params().common_params->freq_cap_id)
          {
            result_fc_set.insert(ch_it->second->params().common_params->freq_cap_id);
          }
        }
      }

      confirm_amounts_(
        config,
        campaign_select_params.time,
        confirm_creatives,
        CR_CPM);

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->auction_type = auction_type;
      }
    }

    void
    CampaignManagerCore::flush_logs() noexcept
    {
      static const char* FUN = "CampaignManagerCore::flush_logs()";

      const Generics::Time LOGS_DUMP_ERROR_RESCHEDULE_PERIOD(2);

      logger_->log(
        String::SubString("flush logs"),
        TraceLevel::MIDDLE,
        Aspect::CAMPAIGN_MANAGER);

      Generics::Time next_flush;

      try
      {
        next_flush = campaign_manager_logger_->flush_if_required(
          Generics::Time::get_time_of_day());
      }
      catch (const eh::Exception& ex)
      {
        next_flush = Generics::Time::get_time_of_day() +
          LOGS_DUMP_ERROR_RESCHEDULE_PERIOD;

        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught on flush logs:" << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-250");
      }

      if(logger_->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << FUN << ": logs flushed, next flush at " << next_flush.get_gm_time();
        logger_->log(
          ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::CAMPAIGN_MANAGER);
      }

      if(next_flush != Generics::Time::ZERO)
      {
        try
        {
          CampaignManagerTaskMessage_var task =
            new FlushLogsTaskMessage(this, task_runner_);

          scheduler_->schedule(task, next_flush);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't schedule next flush task. "
            "eh::Exception caught:" << ex.what();

          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-251");
        }
      }
    }

    std::vector<CampaignManagerCore::ChannelSearchResult>
    CampaignManagerCore::get_channel_links(
      const IdVector& channels,
      bool match)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::get_channel_links()";

      try
      {
        ConstCampaignConfig_var config = configuration();

        if(config.in() == 0)
        {
          throw Exception("Can't receive configuration.");
        }

        ChannelIdHashSet used_simple_channels(channels.begin(), channels.end());
        ChannelUseCountMap uc_tbl;

        for(CampaignConfig::ChannelMap::const_iterator ch_it =
              config->expression_channels.begin();
            ch_it != config->expression_channels.end(); ++ch_it)
        {
          if(match)
          {
            ch_it->second->triggered(&used_simple_channels, 0, "A", &uc_tbl);
          }
          else
          {
            ch_it->second->use(uc_tbl, used_simple_channels, "AI");
          }
        }

        for(CampaignConfig::CampaignMap::const_iterator cmp_it =
              config->campaigns.begin();
            cmp_it != config->campaigns.end(); ++cmp_it)
        {
          if(cmp_it->second->channel.in() && cmp_it->second->channel->has_params())
          {
            ChannelUseCountMap::iterator uc_it = uc_tbl.find(
              cmp_it->second->channel->params().channel_id);
            if(uc_it != uc_tbl.end())
            {
              uc_it->second.ccg_ids.insert(cmp_it->first);
            }
          }
        }

        std::vector<ChannelSearchResult> result;
        result.reserve(uc_tbl.size());
        for(ChannelUseCountMap::const_iterator uc_it = uc_tbl.begin();
            uc_it != uc_tbl.end(); ++uc_it)
        {
          ChannelSearchResult item;
          item.channel_id = uc_it->first.value();
          item.use_count = uc_it->second.count;

          CampaignConfig::ChannelMap::const_iterator discover_it =
            config->discover_channels.find(uc_it->first.value());

          if(discover_it != config->discover_channels.end())
          {
            assert(discover_it->second.in());
            const ChannelParams& ch_params = discover_it->second->params();
            assert(ch_params.discover_params.in());
            item.discover_query = ch_params.discover_params->query;
            if(ch_params.common_params.in())
            {
              item.language = ch_params.common_params->language;
            }
          }

          item.ccg_ids.assign(
            uc_it->second.ccg_ids.begin(),
            uc_it->second.ccg_ids.end());
          item.matched_simple_channels.assign(
            uc_it->second.channel_ids.begin(),
            uc_it->second.channel_ids.end());
          result.emplace_back(std::move(item));
        }

        return result;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-179");
        throw Exception(ostr);
      }
    }

    std::vector<CampaignManagerCore::DiscoverChannelResult>
    CampaignManagerCore::get_discover_channels(
      const std::vector<ChannelWeight>& channels,
      const std::string& country,
      const std::string& language,
      bool all)
      /*throw(NotReady, Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::get_discover_channels()";

      try
      {
        ConstCampaignConfig_var config = configuration();
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        std::vector<DiscoverChannelResult> result;
        result.reserve(config->discover_channels.size());

        ChannelWeightMap triggered_channels;
        fill_triggered_channels_(triggered_channels, channels);

        for(CampaignConfig::ChannelMap::const_iterator ch_it =
              config->discover_channels.begin();
            ch_it != config->discover_channels.end(); ++ch_it)
        {
          unsigned long weight;

          if ((weight = ch_it->second->triggered(0, &triggered_channels)) || (
                all && ch_it->second->params().status == 'A'))
          {
            const ChannelParams& ch_params = ch_it->second->params();

            if((country.empty() || ch_params.country == country) &&
               ch_params.common_params.in() &&
               (language.empty() ||
                ch_params.common_params->language == language))
            {
              DiscoverChannelResult item;
              item.channel_id = ch_it->first;
              item.weight = weight;
              item.country_code = ch_params.country;
              item.language = ch_params.common_params->language;

              if(ch_params.descriptive_params.in())
              {
                item.name = ch_params.descriptive_params->name;
              }

              item.query = ch_params.discover_params->query;
              item.annotation = ch_params.discover_params->annotation;

              CampaignConfig::SimpleChannelMap::const_iterator sit =
                config->simple_channels.find(ch_it->first);
              if(sit != config->simple_channels.end())
              {
                item.categories.assign(
                  sit->second->categories.begin(),
                  sit->second->categories.end());
              }

              result.emplace_back(std::move(item));
            }
          }
        }

        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": result for (";
          for(std::size_t i = 0; i < channels.size(); ++i)
          {
            ostr << (i == 0 ? "" : ",") <<
              channels[i].channel_id << ":" << channels[i].weight;
          }
          ostr << "): ";
          for(std::size_t i = 0; i < result.size(); ++i)
          {
            if(i != 0) ostr << ", ";
            ostr << result[i].channel_id << "->" << result[i].weight;
          }

          logger_->log(
            ostr.str(),
            Logging::Logger::TRACE,
            Aspect::CAMPAIGN_MANAGER);
        }

        return result;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-181");
        throw Exception(ostr);
      }
    }

    std::vector<CampaignManagerCore::CategoryChannelNodeInfo>
    CampaignManagerCore::get_category_channels(const std::string& language)
      /*throw(NotReady, Exception)*/
    {
      static const char* FUN = "CampaignManagerCore::get_category_channels()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        std::vector<CategoryChannelNodeInfo> result;
        result.reserve(config->category_channel_nodes.size());

        for(CategoryChannelNodeMap::const_iterator ch_it =
              config->category_channel_nodes.begin();
            ch_it != config->category_channel_nodes.end(); ++ch_it)
        {
          CategoryChannelNodeInfo item;
          if(fill_category_channel_node_(item, ch_it->second, language))
          {
            result.emplace_back(std::move(item));
          }
        }

        return result;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-181");

        throw Exception(ostr);
      }
    }

    CampaignManagerCore::ImpressionResultInfo
    CampaignManagerCore::verify_impression(
      const ImpressionInfo& impression_info)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::verify_impression()";

      const Generics::Time imp_time = impression_info.time;
      const Generics::Time bid_time = impression_info.bid_time;

      CampaignConfig_var config = configuration();
      if(!config)
      {
        throw NotReady("Campaign configuration isn't loaded");
      }

      RevenueType pub_imp_revenue_type = static_cast<RevenueType>(
        impression_info.pub_imp_revenue_type);

      RevenueDecimal pub_first_slot_revenue;
      RevenueDecimal pub_non_first_slot_revenue;

      if(pub_imp_revenue_type == RT_ABSOLUTE)
      {
        RevenueDecimal pub_imp_revenue = RevenueDecimal::ZERO;

        if(static_cast<AdRequestType>(impression_info.request_type) !=
           AR_NORMAL)
        {
          pub_imp_revenue = impression_info.pub_imp_revenue;
        }

        RevenueDecimal pub_slot_imp_revenue_reminder;
        RevenueDecimal pub_slot_imp_revenue = RevenueDecimal::div(
          pub_imp_revenue,
          RevenueDecimal(false, impression_info.creatives.size(), 0ul),
          pub_slot_imp_revenue_reminder);
        pub_first_slot_revenue = pub_slot_imp_revenue +
          pub_slot_imp_revenue_reminder;
        pub_non_first_slot_revenue = pub_slot_imp_revenue;
      }
      else if (pub_imp_revenue_type == RT_SHARE)
      {
        pub_first_slot_revenue = impression_info.pub_imp_revenue;
        pub_non_first_slot_revenue = pub_first_slot_revenue;
      }

      ImpressionResultInfo impression_result_info;
      impression_result_info.resize(impression_info.creatives.size());

      for(std::size_t i = 0; i < impression_info.creatives.size(); ++i)
      {
        const Creative* creative = 0;

        const AdServer::Commons::RequestId& request_id =
          impression_info.creatives[i].request_id;

        if(!request_id.is_null())
        {
          try
          {
            Commons::Optional<RevenueDecimal> pub_imp_revenue;

            if (pub_imp_revenue_type != RT_NONE)
            {
              pub_imp_revenue =
                (i == 0 ? pub_first_slot_revenue : pub_non_first_slot_revenue);
            }

            campaign_manager_logger_->process_impression(
              CampaignManagerLogger::ImpressionInfo(
                imp_time,
                request_id,
                impression_info.user_id_hash_mod.present() ?
                  CampaignManagerLogger::UserIdHashMod(
                    *impression_info.user_id_hash_mod) :
                  CampaignManagerLogger::UserIdHashMod(),
                static_cast<RequestVerificationType>(impression_info.verify_type),
                pub_imp_revenue_type,
                pub_imp_revenue,
                impression_info.user_id,
                impression_info.referer.c_str(),
                impression_info.viewability,
                String::SubString(impression_info.action_name)));
          }
          catch (const eh::Exception& e)
          {
            logger_->sstream(Logging::Logger::NOTICE,
              Aspect::CAMPAIGN_MANAGER,
              "ADS-IMPL-183") <<
              FUN << ": eh::Exception caught while logging request " <<
              "with request id = '" <<
              request_id.to_string() <<
              "': " << e.what();
          }
        }

        if(impression_info.creatives[i].ccid)
        {
          const CcidMap::const_iterator creative_it =
            config->campaign_creatives.find(impression_info.creatives[i].ccid);

          if(creative_it != config->campaign_creatives.end())
          {
            creative = creative_it->second.in();
          }
        }

        if(creative)
        {
          impression_result_info[i].campaign_id =
            creative->campaign->campaign_group_id;
          impression_result_info[i].advertiser_id =
            creative->campaign->advertiser->account_id;
        }
      }

      if(impression_info.viewability <= 0 &&
        static_cast<RequestVerificationType>(
          impression_info.verify_type) == RVT_IMPRESSION)
      {
        // confirm impression amounts, only if viewability is -1 or 0
        // in other cases we sure that this is duplicate impression tracking request
        ConfirmCreativeAmountArray creatives;
        creatives.reserve(impression_info.creatives.size());
        for(std::size_t cr_i = 0; cr_i < impression_info.creatives.size(); ++cr_i)
        {
          if(impression_info.creatives[cr_i].ccid)
          {
            creatives.push_back(
              ConfirmCreativeAmount(
                impression_info.creatives[cr_i].ccid,
                impression_info.creatives[cr_i].ctr));
          }
        }

        confirm_amounts_(config, bid_time, creatives, CR_CPM);
      }

      return impression_result_info;
    }

    void
    CampaignManagerCore::action_taken(
      const ActionInfo& action_info)
      /*throw(Exception, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::action_taken()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw NotReady(
            "Campaign configuration isn't loaded");
        }

        AdServer::Commons::RequestId ar_id =
          AdServer::Commons::RequestId::create_random_based();

        CampaignManagerLogger::AdvActionInfo adv_action_info;
        adv_action_info.time = action_info.time;
        adv_action_info.action_request_id = ar_id;
        adv_action_info.user_status = static_cast<UserStatus>(action_info.user_status);
        adv_action_info.user_id = action_info.user_id;
        adv_action_info.log_as_test = action_info.log_as_test;
        if(!action_info.location.empty())
        {
          adv_action_info.country = action_info.location[0].country.c_str();
        }
        adv_action_info.referer = action_info.referer;
        adv_action_info.colo_id = campaign_manager_config_.colocation_id();
        adv_action_info.order_id = action_info.order_id;
        adv_action_info.ip_hash = action_info.ip_hash;
        if(action_info.action_value)
        {
          adv_action_info.action_value = *action_info.action_value;
        }
        else
        {
          adv_action_info.action_value = RevenueDecimal::ZERO;
        }

        if(action_info.action_id)
        {
          AdvActionMap::const_iterator adv_act_it =
            config->adv_actions.find(*action_info.action_id);
          if(adv_act_it != config->adv_actions.end())
          {
            adv_action_info.action_id = *action_info.action_id;
            adv_action_info.ccg_ids.assign(
              adv_act_it->second.ccg_ids.begin(),
              adv_act_it->second.ccg_ids.end());
            if(!action_info.action_value)
            {
              adv_action_info.action_value = adv_act_it->second.cur_value;
            }
          }
        }

        {
          ChannelIdSet platform_channels;
          ChannelIdHashSet platforms(
            action_info.platform_ids.begin(),
            action_info.platform_ids.end());

          config->platform_channels->match(
            platform_channels,
            platforms);

          // fill adv_action_info.device_channel_id : device channel with great priority
          unsigned long cur_priority = 0;
          adv_action_info.device_channel_id = 0;

          for(ChannelIdSet::const_iterator pch_it = platform_channels.begin();
              pch_it != platform_channels.end();
              ++pch_it)
          {
            CampaignConfig::PlatformChannelPriorityMap::const_iterator pr_it =
              config->platform_channel_priorities.find(*pch_it);
            if(pr_it != config->platform_channel_priorities.end())
            {
              if(*adv_action_info.device_channel_id == 0 ||
                 cur_priority < pr_it->second.priority)
              {
                cur_priority = pr_it->second.priority;
                adv_action_info.device_channel_id = *pch_it;
              }
            }
          }
        }

        if(action_info.campaign_id)
        {
          adv_action_info.ccg_ids.push_back(*action_info.campaign_id);
        }

        produce_action_message_(action_info);

        campaign_manager_logger_->process_action(adv_action_info);
      }
      catch (const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-184") <<
          FUN << ": eh::Exception caught while logging request " <<
          "with user_id = '" << action_info.user_id.to_string() <<
          "': " << e.what();
      }
    }

    void CampaignManagerCore::verify_opt_operation(
      unsigned long time,
      long colo_id,
      const std::string& /*referer*/,
      OptOperation operation,
      unsigned long status,
      unsigned long user_status,
      bool log_as_test,
      const std::string& browser,
      const std::string& os,
      const std::string& ct,
      const std::string& curct,
      const AdServer::Commons::UserId& user_id)
      /*throw(NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::verify_opt_operation()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw NotReady(
            "Campaign configuration isn't loaded");
        }

        CampaignConfig::ColocationMap::const_iterator colo_it =
          colo_id <= 0 ? config->colocations.end() :
          config->colocations.find(colo_id);

        if (colo_it == config->colocations.end())
        {
          colo_it = config->colocations.find(
            campaign_manager_config_.colocation_id());

          colo_id = campaign_manager_config_.colocation_id();
        }

        if (colo_it != config->colocations.end())
        {
          if(operation == OptOperation::OUT ||
             operation == OptOperation::IN ||
             operation == OptOperation::FORCED_IN)
          {
            char oper;
            switch(operation)
            {
            case OptOperation::OUT:
              oper = 'O';
              break;
            case OptOperation::IN:
              oper = 'I';
              break;
            case OptOperation::FORCED_IN:
              oper = 'F';
              break;
            default:
              oper = 'E';
              break;
            };

            const Generics::Time time_offset = colo_it->second->account->time_offset;

            campaign_manager_logger_->process_oo_operation(
              colo_id,
              Generics::Time(time),
              time_offset,
              user_id,
              log_as_test,
              oper);
          }

          if(operation != OptOperation::FORCED_IN)
          {
            WebOperationHash::const_iterator web_it = config->web_operations.find(
              WebOperationKey(
                "adserver",
                "oo",
                (operation == OptOperation::OUT ?
                 "out" : (
                   operation == OptOperation::IN ?
                   "in" : "status"))));
            if(web_it == config->web_operations.end())
            {
              throw Exception(
                "There isn't identificators for adserver 'oo' operations");
            }

            CampaignManagerLogger::WebOperationInfo web_op;
            web_op.init_by_flags(
              Generics::Time(time),
              ct.c_str(),
              curct.c_str(),
              browser.c_str(),
              os.c_str(),
              web_it->second->flags);
            web_op.colo_id = colo_id;
            web_op.tag_id = 0;
            web_op.cc_id = 0;
            web_op.web_operation_id = web_it->second->id;
            web_op.result = (status == 0 || status == 2 ? 'F' : 'S');
            web_op.user_status = static_cast<UserStatus>(user_status);

            web_op.test_request = log_as_test;

            campaign_manager_logger_->process_web_operation(web_op);
          }
        }
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-185") <<
          FUN << ": Caught eh::Exception: " << e.what();
      }
    }

    void CampaignManagerCore::consider_web_operation(
      const WebOperationInfo& web_op_info)
      /*throw(InvalidArgument, NotReady)*/
    {
      static const char* FUN = "CampaignManagerCore::consider_web_operation()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw NotReady(
            "Campaign configuration isn't loaded");
        }

        WebOperationHash::const_iterator web_it = config->web_operations.find(
          WebOperationKey(
            web_op_info.app.c_str(),
            web_op_info.source.c_str(),
            web_op_info.operation.c_str()));
        if(web_it == config->web_operations.end())
        {
          throw InvalidArgument("Unknown web operation");
        }

        unsigned long colo_id = web_op_info.colo_id;

        CampaignConfig::ColocationMap::const_iterator colo_it =
          colo_id == 0 ? config->colocations.end() :
          config->colocations.find(colo_id);

        if (colo_it == config->colocations.end())
        {
          colo_it = config->colocations.find(
            campaign_manager_config_.colocation_id());

          colo_id = campaign_manager_config_.colocation_id();
        }

        if (colo_it != config->colocations.end())
        {
          CampaignManagerLogger::WebOperationInfo web_op;
          web_op.init_by_flags(
            web_op_info.time,
            web_op_info.ct.c_str(),
            web_op_info.curct.c_str(),
            web_op_info.browser.c_str(),
            web_op_info.os.c_str(),
            web_it->second->flags);
          web_op.global_request_id = web_op_info.global_request_id;
          std::copy(
            web_op_info.request_ids.begin(),
            web_op_info.request_ids.end(),
            std::back_inserter(web_op.request_ids));
          web_op.colo_id = colo_id;

          // filter inactive tags & ccid's
          if(web_op_info.tag_id && (web_it->second->flags &
             CampaignManagerLogger::WebOperationInfo::LOG_TAG_ID) &&
             config->tags.find(web_op_info.tag_id) != config->tags.end())
          {
            web_op.tag_id = web_op_info.tag_id;
          }
          else
          {
            web_op.tag_id = 0;
          }

          if(web_op_info.cc_id &&
             (web_it->second->flags &
               CampaignManagerLogger::WebOperationInfo::LOG_CC_ID) &&
             config->campaign_creatives.find(web_op_info.cc_id) !=
               config->campaign_creatives.end())
          {
            web_op.cc_id = web_op_info.cc_id;
          }
          else
          {
            web_op.cc_id = 0;
          }

          web_op.web_operation_id = web_it->second->id;
          web_op.app = web_op_info.app;
          web_op.source = web_op_info.source;
          web_op.operation = web_op_info.operation;
          web_op.result = web_op_info.result;
          web_op.user_status = static_cast<AdServer::CampaignSvcs::UserStatus>(
            web_op_info.user_status);
          web_op.test_request = web_op_info.test_request;
          web_op.user_bind_src = web_op_info.user_bind_src;
          web_op.referer = web_op_info.referer;
          web_op.ip_address = web_op_info.ip_address;
          web_op.external_user_id = web_op_info.external_user_id;
          web_op.user_agent = web_op_info.user_agent;

          campaign_manager_logger_->process_web_operation(web_op);
        }
      }
      catch(const InvalidArgument&)
      {
        throw;
      }
      catch(const NotReady&)
      {
        throw;
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-185") <<
          FUN << ": Caught eh::Exception: " << e.what();
      }
    }

    bool
    CampaignManagerCore::fill_category_channel_node_(
      CategoryChannelNodeInfo& res,
      const CategoryChannelNode* node,
      const std::string& language)
      noexcept
    {
      res.channel_id = node->channel_id;
      res.flags = node->flags;

      if(language.empty())
      {
        res.name = node->name;
      }
      else
      {
        CategoryChannel::LocalizationMap::const_iterator loc_it =
          node->localizations.find(language);
        if(loc_it != node->localizations.end())
        {
          res.name = loc_it->second;
        }
        else
        {
          return false;
        }
      }

      res.child_category_channels.reserve(
        node->child_category_channels.size());
      for(CategoryChannelNodeMap::const_iterator child_node_it =
            node->child_category_channels.begin();
          child_node_it != node->child_category_channels.end();
          ++child_node_it)
      {
        CategoryChannelNodeInfo child;
        if(fill_category_channel_node_(child, child_node_it->second, language))
        {
          res.child_category_channels.emplace_back(std::move(child));
        }
      }

      return true;
    }

    bool
    CampaignManagerCore::fill_ad_slot_min_cpm_(
      AdSlotMinCpm& ad_slot_min_cpm,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const String::SubString& min_ecpm_currency_code,
      const RevenueDecimal& min_ecpm)
      noexcept
    {
      static const char* FUN = "CampaignManagerCore::fill_ad_slot_min_cpm_()";

      assert(tag);

      try
      {
        // convert min_ecpm to tag currency
        RevenueDecimal adapted_min_ecpm = tag->skip_min_ecpm ?
          RevenueDecimal::ZERO :
          min_ecpm;

        if(!min_ecpm_currency_code.empty() &&
          min_ecpm_currency_code != tag->site->account->currency->currency_code)
        {
          auto cur_it = campaign_config->currency_codes.find(
            min_ecpm_currency_code);

          if(cur_it == campaign_config->currency_codes.end())
          {
            // request with unknown currency can't be processed
            return false;
          }

          const Currency* min_ecpm_currency = cur_it->second.in();

          assert(min_ecpm_currency);

          adapted_min_ecpm = tag->site->account->currency->convert(
            min_ecpm_currency,
            adapted_min_ecpm);
        }

        const RevenueDecimal min_pub_ecpm = RevenueDecimal::div(
          adapted_min_ecpm,
          REVENUE_ONE + tag->cost_coef,
          Generics::DDR_CEIL);

        ad_slot_min_cpm.min_pub_ecpm_system = tag->site->account->currency->to_system_currency(
          min_pub_ecpm,
          Generics::DDR_CEIL);

        ad_slot_min_cpm.min_pub_ecpm = min_pub_ecpm;

        return true;
      }
      catch (const Generics::DecimalException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": tag_id = '" << tag->tag_id <<
          "': min_ecpm processing error: " << e.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::TRAFFICKING_PROBLEM,
          "ADS-IMPL-10340");

        return false;
      }
    }

    CampaignSvcs::AdRequestType
    CampaignManagerCore::reduce_request_type_(
      unsigned long request_type)
      noexcept
    {
      AdRequestType result_request_type =
        static_cast<CampaignSvcs::AdRequestType>(request_type);

      if(result_request_type == AR_OPENRTB_WITH_CLICKURL ||
          result_request_type == AR_LIVERAIL ||
          result_request_type == AR_ADRIVER)
      {
        return AR_OPENRTB;
      }

      return result_request_type;
    }

    void
    CampaignManagerCore::convert_external_categories_(
      CreativeCategoryIdSet& exclude_categories,
      const CampaignConfig& config,
      unsigned long request_type,
      const StringVector& categories)
      noexcept
    {
      AdRequestType reduced_request_type = reduce_request_type_(request_type);

      CampaignConfig::ExternalCategoryMap::const_iterator req_ext_cat_it =
        config.external_creative_categories.find(reduced_request_type);

      if(req_ext_cat_it != config.external_creative_categories.end())
      {
        const CampaignConfig::ExternalCategoryNameMap& ext_categories =
          req_ext_cat_it->second;

        for(StringVector::const_iterator ext_cat_name_it = categories.begin();
            ext_cat_name_it != categories.end(); ++ext_cat_name_it)
        {
          CampaignConfig::ExternalCategoryNameMap::const_iterator ext_cat_it =
            ext_categories.find(*ext_cat_name_it);
          if(ext_cat_it != ext_categories.end())
          {
            std::copy(ext_cat_it->second.begin(),
              ext_cat_it->second.end(),
              std::inserter(exclude_categories,
                exclude_categories.begin()));
          }
        }
      }
    }

    void CampaignManagerCore::convert_ccg_keywords_(
      const CampaignConfig* campaign_config,
      const Tag* tag,
      CampaignKeywordMap& result_keywords,
      const CCGKeywordVector& keywords,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps)
      noexcept
    {
      for(CCGKeywordVector::const_iterator kw_it = keywords.begin();
          kw_it != keywords.end(); ++kw_it)
      {
        const CCGKeywordInfo& src_keyword = *kw_it;
        CampaignConfig::CampaignMap::const_iterator cmp_it =
        campaign_config->campaigns.find(src_keyword.ccg_id);
        if(cmp_it != campaign_config->campaigns.end())
        {
          bool blocked = true;

          CampaignConfig::ChannelMap::const_iterator ch_it =
            campaign_config->expression_channels.find(src_keyword.channel_id);
          if(ch_it != campaign_config->expression_channels.end())
          {
            blocked = ch_it->second->has_params() &&
              ch_it->second->params().common_params.in() &&
              ch_it->second->params().common_params->freq_cap_id && (
                full_freq_caps.find(ch_it->second->params().common_params->freq_cap_id) !=
                  full_freq_caps.end() ||
                !profiling_available);
          }

          if(!blocked)
          {
            blocked = !CampaignIndex::check_tag_domain_exclusion(
              String::SubString(src_keyword.click_url),
              tag);
          }

          if(!blocked)
          {
            CampaignKeyword_var res_keyword(new CampaignKeyword);
            res_keyword->ccg_keyword_id = src_keyword.ccg_keyword_id;
            res_keyword->channel_id = src_keyword.channel_id;
            res_keyword->max_cpc = src_keyword.max_cpc;

            res_keyword->ctr = std::min(
              RevenueDecimal::mul(
                src_keyword.ctr,
                tag->adjustment,
                Generics::DMR_FLOOR),
              REVENUE_ONE);
            res_keyword->original_keyword = src_keyword.original_keyword;
            res_keyword->click_url = src_keyword.click_url;

            res_keyword->max_cpc = cmp_it->second->advertiser->adapt_cost(
              res_keyword->max_cpc,
              cmp_it->second->commision);

            try
            {
              res_keyword->ecpm = RevenueDecimal::mul(
                cmp_it->second->account->currency->to_system_currency(
                  RevenueDecimal::mul(
                    res_keyword->max_cpc,
                    res_keyword->ctr,
                    Generics::DMR_FLOOR)),
                ECPM_FACTOR,
                Generics::DMR_FLOOR);
            }
            catch(const RevenueDecimal::Overflow&)
            {
              res_keyword->ecpm = RevenueDecimal(
                std::numeric_limits<unsigned long>::max());
            }

            res_keyword->campaign = cmp_it->second;

            if(!res_keyword->click_url.empty())
            {
              try
              {
                HTTP::HTTPAddress http_url(res_keyword->click_url);
                domain_parser_->specific_domain(
                  http_url.host(), res_keyword->click_url_domain);
              }
              catch (const HTTP::URLAddress::InvalidURL& e)
              {}
            }

            result_keywords.insert(std::make_pair(src_keyword.ccg_id, res_keyword));
          }
        }
      }
    }

    std::vector<CampaignManagerCore::ColocationFlagsInfo>
    CampaignManagerCore::get_colocation_flags()
      /*throw(NotReady, Exception)*/
    {
      static const char* FUN = "CampaignServerBaseImpl::get_colocation_flags()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw NotReady(
            "Campaign configuration isn't loaded");
        }

        std::vector<ColocationFlagsInfo> result;
        result.reserve(config->colocations.size());

        for (CampaignConfig::ColocationMap::const_iterator it =
          config->colocations.begin();
          it != config->colocations.end(); ++it)
        {
          ColocationFlagsInfo colo;
          colo.colo_id = it->first;
          colo.flags = it->second->ad_serving;
          colo.hid_profile = it->second->hid_profile;
          result.emplace_back(std::move(colo));
        }

        return result;
      }
      catch (const NotReady&)
      {
        throw;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    CampaignManagerCore::StringVector
    CampaignManagerCore::get_pub_pixels(
      const std::string& country,
      unsigned long user_status,
      const IdVector& publisher_account_ids)
      /*throw(NotReady, Exception)*/
    {
      static const char* FUN = "CampaignServerBaseImpl::get_pub_pixels()";

      try
      {
        CampaignConfig_var config = configuration();

        if (!config)
        {
          throw NotReady(
            "Campaign configuration isn't loaded");
        }

        StringVector result;
        AccountList result_accounts;

        if(!publisher_account_ids.empty())
        {
          // select by account id
          const PubPixelAccountMap::const_iterator it =
            find_pub_pixel_accounts_(
              config,
              country.c_str(),
              static_cast<UserStatus>(user_status));

          if(it != config->pub_pixel_accounts.end())
          {
            Account_var test_account = new AccountDef;

            for(IdVector::const_iterator id_it = publisher_account_ids.begin();
                id_it != publisher_account_ids.end(); ++id_it)
            {
              test_account->account_id = *id_it;

              AccountSet::const_iterator acc_it = it->second.find(test_account);
              if(acc_it != it->second.end())
              {
                result_accounts.push_back(*acc_it);
              }
            }
          }
        }
        else
        {
          get_pub_pixel_account_ids_(
            result_accounts,
            config,
            country.c_str(),
            static_cast<UserStatus>(user_status),
            AccountIdSet(),
            MAX_PUBPIXELS_PER_REQUEST);
        }

        result.reserve(result_accounts.size());
        for(AccountList::const_iterator acc_it = result_accounts.begin();
            acc_it != result_accounts.end(); ++acc_it)
        {
          result.emplace_back(
            static_cast<UserStatus>(user_status) == US_OPTIN ?
            (*acc_it)->pub_pixel_optin :
            (*acc_it)->pub_pixel_optout);
        }

        return result;
      }
      catch (const NotReady&)
      {
        throw;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerCore::get_pub_pixel_account_ids_(
      AccountList& result_account_ids,
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status,
      const AccountIdSet& exclude_publisher_account_ids,
      unsigned long limit)
      noexcept
    {
      const PubPixelAccountMap::const_iterator it =
        find_pub_pixel_accounts_(campaign_config, country, user_status);

      if (it != campaign_config->pub_pixel_accounts.end())
      {
        AccountSet::const_iterator acc_it = it->second.begin();
        AccountIdSet::const_iterator excl_acc_it =
          exclude_publisher_account_ids.begin();

        while(acc_it != it->second.end() &&
              excl_acc_it != exclude_publisher_account_ids.end())
        {
          if((*acc_it)->account_id < *excl_acc_it)
          {
            result_account_ids.push_back(*acc_it);
            ++acc_it;
          }
          else if(*excl_acc_it < (*acc_it)->account_id)
          {
            ++excl_acc_it;
          }
          else
          {
            ++acc_it;
            ++excl_acc_it;
          }
        }

        while(acc_it != it->second.end())
        {
          result_account_ids.push_back(*acc_it);
          ++acc_it;
        }
      }

      if (result_account_ids.size() > limit)
      {
        std::random_shuffle(result_account_ids.begin(), result_account_ids.end());
        result_account_ids.resize(limit);
      }
    }

    void
    CampaignManagerCore::get_inst_optin_pub_pixel_account_ids_(
      AccountIdList& result_account_ids,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AdServer::CampaignSvcs::
        PublisherAccountIdSeq& exclude_pubpixel_accounts_seq)
      noexcept
    {
      AccountIdSet exclude_pubpixel_accounts;
      CorbaAlgs::convert_sequence(
        exclude_pubpixel_accounts_seq,
        exclude_pubpixel_accounts);
      exclude_pubpixel_accounts.insert(tag->site->account->account_id);

      AccountList result_accounts;
      get_pub_pixel_account_ids_(
        result_accounts,
        campaign_config,
        request_params.location.length() ? request_params.location[0].country.in() : "",
        US_OPTIN,
        exclude_pubpixel_accounts,
        MAX_PUBPIXELS_PER_REQUEST);

      for(AccountList::const_iterator acc_it = result_accounts.begin();
          acc_it != result_accounts.end(); ++acc_it)
      {
        result_account_ids.push_back((*acc_it)->account_id);
      }
    }

    PubPixelAccountMap::const_iterator
    CampaignManagerCore::find_pub_pixel_accounts_(
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status) const
      noexcept
    {
      /*
       * if country_whitelist_ is set then pub_pixel_accounts is
       * precalculated for countries from whitelist.
       */
      const char* country_key = country_whitelist_.empty() ? country : "";
      return campaign_config->pub_pixel_accounts.find(
        PubPixelAccountKey(country_key, user_status));
    }

    bool
    CampaignManagerCore::size_blacklisted_(
      const CampaignConfig* campaign_config,
      const ChannelIdHashSet& matched_channels,
      unsigned long size_id)
      noexcept
    {
      CampaignConfig::BlockChannelMap::const_iterator block_channel_list_it =
        campaign_config->block_channels.find(size_id);

      if(block_channel_list_it != campaign_config->block_channels.end())
      {
        for(CampaignConfig::ExpressionChannelHolderList::
              const_iterator block_channel_it =
                block_channel_list_it->second.begin();
            block_channel_it != block_channel_list_it->second.end();
            ++block_channel_it)
        {
          if((*block_channel_it)->triggered(&matched_channels, 0))
          {
            return true;
          }
        }
      }

      return false;
    }

    bool
    CampaignManagerCore::apply_check_available_bid_result_(
      const Campaign* campaign,
      const BillingStateContainer::BidCheckResult& check_result,
      const RevenueDecimal& ctr)
      noexcept
    {
      /*
      std::cerr << "apply_check_available_bid_result_(): "
        "deactivate_account = " << check_result.deactivate_account <<
        ", deactivate_advertiser = " << check_result.deactivate_advertiser <<
        ", deactivate_campaign = " << check_result.deactivate_campaign <<
        ", deactivate_ccg = " << check_result.deactivate_ccg <<
        ", available = " << check_result.available <<
        std::endl;
      */

      if(check_result.deactivate_account)
      {
        campaign->account->set_available(false);
      }

      if(check_result.deactivate_advertiser)
      {
        campaign->advertiser->set_available(false);
      }

      /*
      if(check_result.deactivate_account ||
        check_result.deactivate_advertiser ||
        check_result.deactivate_campaign ||
        check_result.deactivate_ccg)
      {
        campaign->set_available(false, check_result.goal_ctr);
      }
      */

      return check_result.available && ctr >= check_result.goal_ctr;
    }

    void
    CampaignManagerCore::confirm_amounts_(
      const CampaignConfig* config,
      const Generics::Time& now,
      const ConfirmCreativeAmountArray& creatives,
      CCGRateType rate_type)
      noexcept
    {
      if(confirm_billing_state_container_)
      {
        // resolve id's and rate
        for(ConfirmCreativeAmountArray::const_iterator cc_it = creatives.begin();
          cc_it != creatives.end(); ++cc_it)
        {
          const Creative* creative = 0;

          CcidMap::const_iterator cr_it = config->campaign_creatives.find(cc_it->cc_id);
          if(cr_it != config->campaign_creatives.end())
          {
            creative = cr_it->second.in();
          }

          // account can be non linked if deactivated directly
          if(creative && creative->campaign && creative->campaign->account.in())
          {
            const Campaign* campaign = creative->campaign;
            assert(campaign);

            RevenueDecimal amount = RevenueDecimal::ZERO;
            if(rate_type == CR_CPM || rate_type == CR_MAXBID)
            {
              amount = campaign->imp_revenue;
            }
            else if(rate_type == CR_CPC)
            {
              amount = campaign->click_revenue;
            }
            else if(rate_type == CR_CPA)
            {
              amount = campaign->action_revenue;
            }
            else
            {
              assert(0);
            }

            // eval comm amount
            RevenueDecimal comm_amount = RevenueDecimal::ZERO;

            if(campaign->commision != RevenueDecimal::ZERO)
            {
              if(campaign->account->cost_is_gross())
              {
                comm_amount = amount - RevenueDecimal::mul(
                  amount,
                  REVENUE_ONE - campaign->commision,
                  Generics::DMR_FLOOR);

                amount -= comm_amount;
              }
              else
              {
                comm_amount = RevenueDecimal::div(
                  RevenueDecimal::mul(
                    amount,
                    campaign->commision,
                    Generics::DMR_FLOOR),
                  REVENUE_ONE - campaign->commision); // += comm_amount
              }
            }

            const BillingStateContainer::BidCheckResult check_result =
              confirm_billing_state_container_->confirm_bid(
                now,
                campaign->advertiser ? campaign->advertiser->bill_account_id() : 0,
                campaign->advertiser ? campaign->advertiser->not_bill_account_id() : 0,
                campaign->campaign_group_id,
                campaign->campaign_id,
                campaign->account && campaign->account->invoice_commision() ?
                  amount + comm_amount : amount, // account amount
                campaign->account && campaign->account->cost_is_gross() ?
                  amount + comm_amount : amount,
                cc_it->ctr,
                RevenueDecimal(false, rate_type == CR_CPM || rate_type == CR_MAXBID ? 1 : 0, 0),
                RevenueDecimal(false, rate_type == CR_CPC ? 1 : 0, 0),
                campaign);

            apply_check_available_bid_result_(
              campaign, check_result, RevenueDecimal::ZERO);
          }
        }
      } // if(confirm_billing_state_container_)
    }

    void
    CampaignManagerCore::fill_tns_counter_device_type_(
      std::string& tns_counter_device_type,
      const StringSet& norm_platform_names)
      noexcept
    {
      if(norm_platform_names.find(PlatformNames::APPLE_IPADS) != norm_platform_names.end() ||
        norm_platform_names.find(PlatformNames::APPLE_IPHONES) != norm_platform_names.end())
      {
        tns_counter_device_type = "2";
      }
      else if(norm_platform_names.find(PlatformNames::ANDROID_TABLETS) != norm_platform_names.end() ||
        norm_platform_names.find(PlatformNames::ANDROID_SMARTPHONES) != norm_platform_names.end())
      {
        tns_counter_device_type = "3";
      }
      else if(norm_platform_names.find(PlatformNames::WINDOWS_MOBILE) != norm_platform_names.end())
      {
        tns_counter_device_type = "4";
      }
      else if(norm_platform_names.find(PlatformNames::SMARTTV) != norm_platform_names.end())
      {
        tns_counter_device_type = "5";
      }
      else if(norm_platform_names.find(PlatformNames::DVR) != norm_platform_names.end())
      {
        tns_counter_device_type = "6";
      }
      else if(norm_platform_names.find(PlatformNames::NON_MOBILE_DEVICES) != norm_platform_names.end())
      {
        tns_counter_device_type = "1";
      }
      else
      {
        tns_counter_device_type = "7";
      }
    }

    void
    CampaignManagerCore::produce_ads_space_message_impl_(
      const Generics::Time& request_time,
      const AdServer::Commons::UserId& user_id_orig,
      unsigned long request_tag_id,
      const String::SubString& referer,
      const std::vector<TraceAdSlotInfo>* ad_slots,
      const IdVector* publisher_account_ids,
      const String::SubString& peer_ip,
      const std::vector<GeoInfo>& location,
      const String::SubString& ssp_location,
      const RevenueDecimal& adsspace_system_cpm,
      const String::SubString& external_user_id)
    {
      std::string user_id(user_id_orig.to_string(false));

      std::string csv_encoded_ref;
      String::StringManip::csv_encode(referer.str().c_str(), csv_encoded_ref);

      char usec_str[40];
      size_t usec_str_size = String::StringManip::int_to_str(
        request_time.tv_usec, usec_str + 6, sizeof(usec_str) - 6);
      ::memset(usec_str + usec_str_size, '0', 6 - usec_str_size);

      char tag_id_str[40];
      size_t tag_id_str_size = String::StringManip::int_to_str(
        request_tag_id, tag_id_str, sizeof(tag_id_str));

      std::string record;
      record.reserve(1024);
      record += request_time.gm_ft();
      record += '.';
      record.append(usec_str + usec_str_size, 6);
      record += ',';
      record += user_id;
      record += ',';
      record.append(tag_id_str, tag_id_str_size);
      record += ',';
      record += csv_encoded_ref;
      record += ',';
      record += adsspace_system_cpm.str();
      record += ',';

      if(publisher_account_ids)
      {
        char acc_id_str[40];
        for(IdVector::const_iterator acc_it = publisher_account_ids->begin();
            acc_it != publisher_account_ids->end(); ++acc_it)
        {
          if(acc_it != publisher_account_ids->begin())
          {
            record += ";";
          }

          size_t acc_id_str_size = String::StringManip::int_to_str(
            *acc_it, acc_id_str, sizeof(acc_id_str));
          record.append(acc_id_str, acc_id_str_size);
        }
      }

      record += ',';

      if(ad_slots)
      {
        bool first_size = true;
        for(std::vector<TraceAdSlotInfo>::const_iterator slot_it = ad_slots->begin();
            slot_it != ad_slots->end(); ++slot_it)
        {
          for(StringVector::const_iterator size_it = slot_it->sizes.begin();
              size_it != slot_it->sizes.end(); ++size_it)
          {
            if(first_size)
            {
              first_size = false;
            }
            else
            {
              record += ';';
            }

            record += *size_it;
          }
        }
      }

      record += ',';
      record += peer_ip.str();
      record += ',';

      for(std::size_t loc_i = 0; loc_i < location.size(); ++loc_i)
      {
        if(loc_i > 0)
        {
          record += ';';
        }
        record += location[loc_i].country;
        record += '/';
        record += location[loc_i].region;
        record += '/';
        record += location[loc_i].city;
      }

      record += ',';
      record += ssp_location.str();
      record += ',';
      record += external_user_id.str();

      kafka_producer_->push_data(user_id, record);
    }

    void
    CampaignManagerCore::produce_action_message_(
      const ActionInfo& action_info)
    {
      if(kafka_producer_)
      {
        if(!action_info.referer.empty())
        {
          produce_ads_space_message_impl_(
            action_info.time,
            action_info.user_id,
            0, // tag_id
            String::SubString(action_info.referer),
            0, // ad_slots
            0, // publisher_account_ids
            String::SubString(action_info.peer_ip),
            action_info.location,
            String::SubString(),
            RevenueDecimal::ZERO,
            String::SubString());
        }

        char action_id_str[40];
        size_t action_id_size = String::StringManip::int_to_str(
          action_info.action_id.value_or(0), action_id_str, sizeof(action_id_str));

        std::string act_ref("act-");
        act_ref.append(action_id_str, action_id_size);

        produce_ads_space_message_impl_(
          action_info.time,
          action_info.user_id,
          0, // tag_id
          act_ref,
          0, // ad_slots
          0, // publisher_account_ids
          String::SubString(action_info.peer_ip),
          action_info.location,
          String::SubString(),
          RevenueDecimal::ZERO,
          String::SubString());
      }
    }

    void
    CampaignManagerCore::produce_ads_space_(
      const CreativeRequestInfo& request_params,
      unsigned long request_tag_id,
      const RevenueDecimal& adsspace_system_cpm)
    {
      if(kafka_producer_)
      {
        std::vector<TraceAdSlotInfo> ad_slots;
        ad_slots.reserve(request_params.ad_slots.size());
        for(const auto& request_ad_slot : request_params.ad_slots)
        {
          TraceAdSlotInfo ad_slot;
          ad_slot.sizes = request_ad_slot.sizes;
          ad_slots.push_back(ad_slot);
        }

        produce_ads_space_message_impl_(
          request_params.common_info.time,
          request_params.common_info.user_id,
          request_tag_id,
          String::SubString(request_params.common_info.referer),
          &ad_slots,
          &request_params.publisher_account_ids,
          String::SubString(request_params.common_info.peer_ip),
          request_params.common_info.location,
          String::SubString(request_params.ssp_location),
          adsspace_system_cpm,
          String::SubString(request_params.common_info.external_user_id));
      }
    }

    void
    CampaignManagerCore::produce_match_(
      const MatchRequestInfo& request_params)
    {
      if (kafka_match_producer_ &&
        !request_params.match_info.full_referer.empty())
      {
        const Generics::Time request_time = request_params.request_time;

        std::string user_id(request_params.user_id.to_string(false));

        std::string csv_encoded_ref;
        String::StringManip::csv_encode(
          request_params.match_info.full_referer.c_str(),
          csv_encoded_ref);

        std::string csv_encoded_source;
        String::StringManip::csv_encode(
          request_params.source.c_str(),
          csv_encoded_source);

        char usec_str[40];
        size_t usec_str_size = String::StringManip::int_to_str(
          request_time.tv_usec, usec_str + 6, sizeof(usec_str) - 6);
        ::memset(usec_str + usec_str_size, '0', 6 - usec_str_size);

        std::string record;
        record.reserve(1024);
        record += request_time.gm_ft();
        record += '.';
        record.append(usec_str + usec_str_size, 6);
        record += ',';
        record += user_id;
        record += ',';
        record += csv_encoded_ref;
        record += ',';
        record += csv_encoded_source;

        kafka_match_producer_->push_data(
          user_id,
          record);
      }
    }
  } // namespace CampaignSvcs
} // namespace AdServer
