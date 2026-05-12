#include "CampaignManagerImpl.hpp"

#include <Commons/CorbaAlgs.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace
    {
      template<typename Vector, typename Seq>
      void
      unpack_id_seq(Vector& out, const Seq& in)
      {
        out.assign(in.get_buffer(), in.get_buffer() + in.length());
      }

      template<typename Seq, typename Vector>
      void
      pack_id_seq_local(Seq& out, const Vector& in)
      {
        out.length(in.size());
        for(CORBA::ULong i = 0; i < in.size(); ++i)
        {
          out[i] = in[i];
        }
      }

      template<typename Vector, typename Seq>
      void
      unpack_string_seq(Vector& out, const Seq& in)
      {
        out.clear();
        out.reserve(in.length());
        for(CORBA::ULong i = 0; i < in.length(); ++i)
        {
          out.emplace_back(in[i].in());
        }
      }

      template<typename Seq, typename Vector>
      void
      pack_string_seq(Seq& out, const Vector& in)
      {
        out.length(in.size());
        for(CORBA::ULong i = 0; i < in.size(); ++i)
        {
          out[i] = in[i].c_str();
        }
      }

      template<typename Vector, typename Seq>
      void
      unpack_token_seq(Vector& out, const Seq& in)
      {
        out.clear();
        out.reserve(in.length());
        for(CORBA::ULong i = 0; i < in.length(); ++i)
        {
          out.push_back({in[i].name.in(), in[i].value.in()});
        }
      }

      template<typename Seq, typename Vector>
      void
      pack_token_seq(Seq& out, const Vector& in)
      {
        out.length(in.size());
        for(CORBA::ULong i = 0; i < in.size(); ++i)
        {
          out[i].name << in[i].name;
          out[i].value << in[i].value;
        }
      }
    }

    CampaignManagerImpl::CampaignManagerImpl(CampaignManagerCore* core)
      : core_(ReferenceCounting::add_ref(core))
    {}

    CampaignManagerImpl::~CampaignManagerImpl() noexcept
    {}

    bool
    CampaignManagerImpl::ready() /*throw(eh::Exception)*/
    {
      return core_->ready();
    }

    void
    CampaignManagerImpl::progress_comment(std::string& res)
      /*throw(eh::Exception)*/
    {
      core_->progress_comment(res);
    }

    void
    CampaignManagerImpl::get_campaign_creative(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      CORBA::String_out hostname,
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out request_result)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::CreativeRequestInfo core_request;
        core_request.common_info.time =
          CorbaAlgs::unpack_time(request_params.common_info.time);
        core_request.common_info.request_id =
          CorbaAlgs::unpack_request_id(request_params.common_info.request_id);
        core_request.common_info.creative_instantiate_type =
          request_params.common_info.creative_instantiate_type.in();
        core_request.common_info.request_type =
          request_params.common_info.request_type;
        core_request.common_info.random = request_params.common_info.random;
        core_request.common_info.test_request =
          request_params.common_info.test_request;
        core_request.common_info.log_as_test =
          request_params.common_info.log_as_test;
        core_request.common_info.colo_id = request_params.common_info.colo_id;
        core_request.common_info.external_user_id =
          request_params.common_info.external_user_id.in();
        core_request.common_info.source_id =
          request_params.common_info.source_id.in();
        core_request.common_info.location.reserve(
          request_params.common_info.location.length());
        for(CORBA::ULong i = 0;
          i < request_params.common_info.location.length();
          ++i)
        {
          core_request.common_info.location.push_back({
            request_params.common_info.location[i].country.in(),
            request_params.common_info.location[i].region.in(),
            request_params.common_info.location[i].city.in()});
        }
        core_request.common_info.coord_location.reserve(
          request_params.common_info.coord_location.length());
        for(CORBA::ULong i = 0;
          i < request_params.common_info.coord_location.length();
          ++i)
        {
          core_request.common_info.coord_location.push_back({
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              request_params.common_info.coord_location[i].longitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              request_params.common_info.coord_location[i].latitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              request_params.common_info.coord_location[i].accuracy)});
        }
        core_request.common_info.full_referer =
          request_params.common_info.full_referer.in();
        core_request.common_info.referer =
          request_params.common_info.referer.in();
        unpack_string_seq(core_request.common_info.urls, request_params.common_info.urls);
        core_request.common_info.security_token =
          request_params.common_info.security_token.in();
        core_request.common_info.pub_impr_track_url =
          request_params.common_info.pub_impr_track_url.in();
        core_request.common_info.pub_param =
          request_params.common_info.pub_param.in();
        core_request.common_info.preclick_url =
          request_params.common_info.preclick_url.in();
        core_request.common_info.click_prefix_url =
          request_params.common_info.click_prefix_url.in();
        core_request.common_info.original_url =
          request_params.common_info.original_url.in();
        core_request.common_info.track_user_id =
          CorbaAlgs::unpack_user_id(request_params.common_info.track_user_id);
        core_request.common_info.user_id =
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id);
        core_request.common_info.user_status =
          request_params.common_info.user_status;
        core_request.common_info.signed_user_id =
          request_params.common_info.signed_user_id.in();
        core_request.common_info.peer_ip =
          request_params.common_info.peer_ip.in();
        core_request.common_info.user_agent =
          request_params.common_info.user_agent.in();
        core_request.common_info.cohort =
          request_params.common_info.cohort.in();
        core_request.common_info.hpos = request_params.common_info.hpos;
        core_request.common_info.ext_track_params =
          request_params.common_info.ext_track_params.in();
        unpack_token_seq(core_request.common_info.tokens, request_params.common_info.tokens);
        core_request.common_info.set_cookie = request_params.common_info.set_cookie;
        core_request.common_info.passback_type =
          request_params.common_info.passback_type.in();
        core_request.common_info.passback_url =
          request_params.common_info.passback_url.in();

        core_request.context_info.enabled_notice =
          request_params.context_info.enabled_notice;
        core_request.context_info.client = request_params.context_info.client.in();
        core_request.context_info.client_version =
          request_params.context_info.client_version.in();
        unpack_id_seq(
          core_request.context_info.platform_ids,
          request_params.context_info.platform_ids);
        unpack_id_seq(
          core_request.context_info.geo_channels,
          request_params.context_info.geo_channels);
        core_request.context_info.platform =
          request_params.context_info.platform.in();
        core_request.context_info.full_platform =
          request_params.context_info.full_platform.in();
        core_request.context_info.web_browser =
          request_params.context_info.web_browser.in();
        core_request.context_info.ip_hash =
          request_params.context_info.ip_hash.in();
        core_request.context_info.profile_referer =
          request_params.context_info.profile_referer;
        core_request.context_info.page_load_id =
          request_params.context_info.page_load_id;
        core_request.context_info.full_referer_hash =
          request_params.context_info.full_referer_hash;
        core_request.context_info.short_referer_hash =
          request_params.context_info.short_referer_hash;

        core_request.publisher_site_id = request_params.publisher_site_id;
        unpack_id_seq(core_request.publisher_account_ids, request_params.publisher_account_ids);
        core_request.fill_track_pixel = request_params.fill_track_pixel;
        core_request.fill_iurl = request_params.fill_iurl;
        core_request.ad_instantiate_type = request_params.ad_instantiate_type;
        core_request.only_display_ad = request_params.only_display_ad;
        unpack_id_seq(core_request.full_freq_caps, request_params.full_freq_caps);
        core_request.seq_orders.reserve(request_params.seq_orders.length());
        for(CORBA::ULong i = 0; i < request_params.seq_orders.length(); ++i)
        {
          core_request.seq_orders.push_back({
            request_params.seq_orders[i].ccg_id,
            request_params.seq_orders[i].set_id,
            request_params.seq_orders[i].imps});
        }
        core_request.campaign_freqs.reserve(request_params.campaign_freqs.length());
        for(CORBA::ULong i = 0; i < request_params.campaign_freqs.length(); ++i)
        {
          core_request.campaign_freqs.push_back({
            request_params.campaign_freqs[i].campaign_id,
            request_params.campaign_freqs[i].imps});
        }
        core_request.household_id =
          CorbaAlgs::unpack_user_id(request_params.household_id);
        core_request.merged_user_id =
          CorbaAlgs::unpack_user_id(request_params.merged_user_id);
        core_request.search_engine_id = request_params.search_engine_id;
        core_request.search_words = request_params.search_words.in();
        core_request.page_keywords_present = request_params.page_keywords_present;
        core_request.profiling_available = request_params.profiling_available;
        core_request.fraud = request_params.fraud;
        unpack_id_seq(core_request.channels, request_params.channels);
        unpack_id_seq(core_request.hid_channels, request_params.hid_channels);
        core_request.ccg_keywords.reserve(request_params.ccg_keywords.length());
        for(CORBA::ULong i = 0; i < request_params.ccg_keywords.length(); ++i)
        {
          core_request.ccg_keywords.push_back({
            request_params.ccg_keywords[i].ccg_keyword_id,
            request_params.ccg_keywords[i].ccg_id,
            request_params.ccg_keywords[i].channel_id,
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              request_params.ccg_keywords[i].max_cpc),
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              request_params.ccg_keywords[i].ctr),
            request_params.ccg_keywords[i].click_url.in(),
            request_params.ccg_keywords[i].original_keyword.in()});
        }
        core_request.hid_ccg_keywords.reserve(
          request_params.hid_ccg_keywords.length());
        for(CORBA::ULong i = 0; i < request_params.hid_ccg_keywords.length(); ++i)
        {
          core_request.hid_ccg_keywords.push_back({
            request_params.hid_ccg_keywords[i].ccg_keyword_id,
            request_params.hid_ccg_keywords[i].ccg_id,
            request_params.hid_ccg_keywords[i].channel_id,
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              request_params.hid_ccg_keywords[i].max_cpc),
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              request_params.hid_ccg_keywords[i].ctr),
            request_params.hid_ccg_keywords[i].click_url.in(),
            request_params.hid_ccg_keywords[i].original_keyword.in()});
        }
        auto unpack_triggers = [](
          CampaignManagerCore::ChannelTriggerMatchVector& out,
          const auto& in)
        {
          out.reserve(in.length());
          for(CORBA::ULong i = 0; i < in.length(); ++i)
          {
            out.push_back({in[i].channel_trigger_id, in[i].channel_id});
          }
        };
        unpack_triggers(
          core_request.trigger_match_result.url_channels,
          request_params.trigger_match_result.url_channels);
        unpack_triggers(
          core_request.trigger_match_result.pkw_channels,
          request_params.trigger_match_result.pkw_channels);
        unpack_triggers(
          core_request.trigger_match_result.skw_channels,
          request_params.trigger_match_result.skw_channels);
        unpack_triggers(
          core_request.trigger_match_result.ukw_channels,
          request_params.trigger_match_result.ukw_channels);
        unpack_id_seq(
          core_request.trigger_match_result.uid_channels,
          request_params.trigger_match_result.uid_channels);
        core_request.client_create_time =
          CorbaAlgs::unpack_time(request_params.client_create_time);
        core_request.session_start =
          CorbaAlgs::unpack_time(request_params.session_start);
        unpack_id_seq(
          core_request.exclude_pubpixel_accounts,
          request_params.exclude_pubpixel_accounts);
        core_request.tag_delivery_factor = request_params.tag_delivery_factor;
        core_request.ccg_delivery_factor = request_params.ccg_delivery_factor;
        core_request.preview_ccid = request_params.preview_ccid;
        core_request.required_passback = request_params.required_passback;
        core_request.profiling_type = request_params.profiling_type;
        core_request.disable_fraud_detection =
          request_params.disable_fraud_detection;
        core_request.need_debug_info = request_params.need_debug_info;
        core_request.page_keywords = request_params.page_keywords.in();
        core_request.url_keywords = request_params.url_keywords.in();
        core_request.ssp_location = request_params.ssp_location.in();
        core_request.additional_info = request_params.additional_info.in();

        core_request.ad_slots.reserve(request_params.ad_slots.length());
        for(CORBA::ULong i = 0; i < request_params.ad_slots.length(); ++i)
        {
          const auto& source = request_params.ad_slots[i];
          CampaignManagerCore::TraceAdSlotInfo slot;
          slot.ad_slot_id = source.ad_slot_id;
          slot.format = source.format.in();
          slot.tag_id = source.tag_id;
          unpack_string_seq(slot.sizes, source.sizes);
          slot.ext_tag_id = source.ext_tag_id.in();
          slot.min_ecpm = CorbaAlgs::unpack_decimal<RevenueDecimal>(source.min_ecpm);
          slot.min_ecpm_currency_code = source.min_ecpm_currency_code.in();
          unpack_string_seq(slot.currency_codes, source.currency_codes);
          slot.passback = source.passback;
          slot.up_expand_space = source.up_expand_space;
          slot.right_expand_space = source.right_expand_space;
          slot.left_expand_space = source.left_expand_space;
          slot.tag_visibility = source.tag_visibility;
          slot.tag_predicted_viewability = source.tag_predicted_viewability;
          slot.down_expand_space = source.down_expand_space;
          slot.video_min_duration = source.video_min_duration;
          slot.video_max_duration = source.video_max_duration;
          slot.video_skippable_max_duration = source.video_skippable_max_duration;
          slot.video_allow_skippable = source.video_allow_skippable;
          slot.video_allow_unskippable = source.video_allow_unskippable;
          slot.video_width = source.video_width;
          slot.video_height = source.video_height;
          unpack_string_seq(slot.exclude_categories, source.exclude_categories);
          unpack_string_seq(slot.required_categories, source.required_categories);
          slot.debug_ccg = source.debug_ccg;
          unpack_id_seq(slot.allowed_durations, source.allowed_durations);
          slot.native_data_tokens.reserve(source.native_data_tokens.length());
          for(CORBA::ULong j = 0; j < source.native_data_tokens.length(); ++j)
          {
            slot.native_data_tokens.push_back({
              source.native_data_tokens[j].name.in(),
              source.native_data_tokens[j].required});
          }
          slot.native_image_tokens.reserve(source.native_image_tokens.length());
          for(CORBA::ULong j = 0; j < source.native_image_tokens.length(); ++j)
          {
            slot.native_image_tokens.push_back({
              source.native_image_tokens[j].name.in(),
              source.native_image_tokens[j].required,
              source.native_image_tokens[j].width,
              source.native_image_tokens[j].height});
          }
          slot.native_ads_impression_tracker_type =
            source.native_ads_impression_tracker_type;
          slot.fill_track_html = source.fill_track_html;
          unpack_token_seq(slot.tokens, source.tokens);
          core_request.ad_slots.push_back(slot);
        }

        const auto core_result = core_->get_campaign_creative(core_request);
        hostname << core_result.hostname;
        request_result =
          new AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult();
        request_result->process_time =
          CorbaAlgs::pack_time(core_result.process_time);
        request_result->debug_info.colo_id = core_result.debug_info.colo_id;
        pack_id_seq_local(
          request_result->debug_info.geo_channels,
          core_result.debug_info.geo_channels);
        pack_id_seq_local(
          request_result->debug_info.platform_channels,
          core_result.debug_info.platform_channels);
        request_result->debug_info.last_platform_channel_id =
          core_result.debug_info.last_platform_channel_id;
        request_result->debug_info.user_group_id =
          core_result.debug_info.user_group_id;
        request_result->ad_slots.length(core_result.ad_slots.size());
        for(CORBA::ULong i = 0; i < core_result.ad_slots.size(); ++i)
        {
          const auto& source = core_result.ad_slots[i];
          auto& dest = request_result->ad_slots[i];
          dest.ad_slot_id = source.ad_slot_id;
          dest.request_id = CorbaAlgs::pack_request_id(source.request_id);
          dest.passback = source.passback;
          dest.passback_url << source.passback_url;
          dest.creative_body << source.creative_body;
          dest.notice_url << source.notice_url;
          pack_string_seq(dest.track_pixel_urls, source.track_pixel_urls);
          dest.yandex_track_params << source.yandex_track_params;
          dest.creative_url << source.creative_url;
          dest.track_pixel_params << source.track_pixel_params;
          dest.click_params << source.click_params;
          dest.mime_format << source.mime_format;
          dest.iurl << source.iurl;
          dest.test_request = source.test_request;
          dest.selected_creatives.length(source.selected_creatives.size());
          for(CORBA::ULong j = 0; j < source.selected_creatives.size(); ++j)
          {
            const auto& sc = source.selected_creatives[j];
            auto& dc = dest.selected_creatives[j];
            dc.request_id = CorbaAlgs::pack_request_id(sc.request_id);
            dc.ccid = sc.ccid;
            dc.cmp_id = sc.cmp_id;
            dc.campaign_group_id = sc.campaign_group_id;
            dc.order_set_id = sc.order_set_id;
            dc.advertiser_id = sc.advertiser_id;
            dc.advertiser_name << sc.advertiser_name;
            dc.creative_size << sc.creative_size;
            dc.revenue = CorbaAlgs::pack_decimal<RevenueDecimal>(sc.revenue);
            dc.ecpm = CorbaAlgs::pack_decimal<RevenueDecimal>(sc.ecpm);
            dc.pub_ecpm = CorbaAlgs::pack_decimal<RevenueDecimal>(sc.pub_ecpm);
            dc.click_url << sc.click_url;
            dc.destination_url << sc.destination_url;
            dc.creative_version_id << sc.creative_version_id;
            dc.creative_id = sc.creative_id;
            dc.https_safe_flag = sc.https_safe_flag;
            dc.expanding = sc.expanding;
          }
          pack_string_seq(
            dest.external_visual_categories,
            source.external_visual_categories);
          pack_string_seq(
            dest.external_content_categories,
            source.external_content_categories);
          dest.pub_currency_code << source.pub_currency_code;
          dest.overlay_width = source.overlay_width;
          dest.overlay_height = source.overlay_height;
          pack_token_seq(dest.tokens, source.tokens);
          pack_token_seq(dest.ext_tokens, source.ext_tokens);
          dest.track_impr = source.track_impr;
          dest.tag_size << source.tag_size;
          pack_id_seq_local(dest.freq_caps, source.freq_caps);
          pack_id_seq_local(dest.uc_freq_caps, source.uc_freq_caps);
          dest.debug_info.tag_id = source.debug_info.tag_id;
          dest.debug_info.tag_size_id = source.debug_info.tag_size_id;
          dest.debug_info.site_id = source.debug_info.site_id;
          dest.debug_info.site_rate_id = source.debug_info.site_rate_id;
          dest.debug_info.min_no_adv_ecpm = source.debug_info.min_no_adv_ecpm;
          dest.debug_info.min_text_ecpm = source.debug_info.min_text_ecpm;
          dest.debug_info.auction_type = source.debug_info.auction_type;
          dest.debug_info.track_pixel_url << source.debug_info.track_pixel_url;
          dest.debug_info.cpm_threshold =
            CorbaAlgs::pack_decimal<RevenueDecimal>(
              source.debug_info.cpm_threshold);
          dest.debug_info.walled_garden = source.debug_info.walled_garden;
          dest.debug_info.selected_creatives.length(
            source.debug_info.selected_creatives.size());
          for(CORBA::ULong j = 0;
            j < source.debug_info.selected_creatives.size();
            ++j)
          {
            const auto& sc = source.debug_info.selected_creatives[j];
            auto& dc = dest.debug_info.selected_creatives[j];
            dc.imp_revenue =
              CorbaAlgs::pack_decimal<RevenueDecimal>(sc.imp_revenue);
            dc.click_revenue =
              CorbaAlgs::pack_decimal<RevenueDecimal>(sc.click_revenue);
            dc.action_revenue =
              CorbaAlgs::pack_decimal<RevenueDecimal>(sc.action_revenue);
            dc.ecpm_bid =
              CorbaAlgs::pack_decimal<RevenueDecimal>(sc.ecpm_bid);
            dc.action_adv_url << sc.action_adv_url;
            dc.html_url << sc.html_url;
            dc.triggered_expression << sc.triggered_expression;
            dc.full_expression << sc.full_expression;
          }
          dest.debug_info.trace_ccg << source.debug_info.trace_ccg;
          pack_token_seq(dest.native_data_tokens, source.native_data_tokens);
          dest.native_image_tokens.length(source.native_image_tokens.size());
          for(CORBA::ULong j = 0; j < source.native_image_tokens.size(); ++j)
          {
            dest.native_image_tokens[j].name <<
              source.native_image_tokens[j].name;
            dest.native_image_tokens[j].value <<
              source.native_image_tokens[j].value;
            dest.native_image_tokens[j].width =
              source.native_image_tokens[j].width;
            dest.native_image_tokens[j].height =
              source.native_image_tokens[j].height;
          }
          dest.track_html_body << source.track_html_body;
          dest.erid << source.erid;
          dest.contracts.length(source.contracts.size());
          for(CORBA::ULong j = 0; j < source.contracts.size(); ++j)
          {
            const auto& sc = source.contracts[j];
            auto& dc = dest.contracts[j];
            dc.contract_info.contract_id = sc.contract_info.contract_id;
            dc.contract_info.number << sc.contract_info.number;
            dc.contract_info.date << sc.contract_info.date;
            dc.contract_info.type << sc.contract_info.type;
            dc.contract_info.vat_included = sc.contract_info.vat_included;
            dc.contract_info.ord_contract_id << sc.contract_info.ord_contract_id;
            dc.contract_info.ord_ado_id << sc.contract_info.ord_ado_id;
            dc.contract_info.subject_type << sc.contract_info.subject_type;
            dc.contract_info.action_type << sc.contract_info.action_type;
            dc.contract_info.agent_acting_for_publisher =
              sc.contract_info.agent_acting_for_publisher;
            dc.contract_info.parent_contract_id =
              sc.contract_info.parent_contract_id;
            dc.contract_info.client_id << sc.contract_info.client_id;
            dc.contract_info.client_name << sc.contract_info.client_name;
            dc.contract_info.client_legal_form <<
              sc.contract_info.client_legal_form;
            dc.contract_info.contractor_id << sc.contract_info.contractor_id;
            dc.contract_info.contractor_name << sc.contract_info.contractor_name;
            dc.contract_info.contractor_legal_form <<
              sc.contract_info.contractor_legal_form;
            dc.contract_info.timestamp =
              CorbaAlgs::pack_time(sc.contract_info.timestamp);
            dc.parent_contract_id << sc.parent_contract_id;
          }
        }
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::match_geo_channels(
      const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
      const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
      AdServer::CampaignSvcs::ChannelIdSeq_out geo_channels_result,
      AdServer::CampaignSvcs::ChannelIdSeq_out coord_channels_result)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        std::vector<CampaignManagerCore::GeoInfo> core_location;
        core_location.reserve(location.length());
        for(CORBA::ULong i = 0; i < location.length(); ++i)
        {
          core_location.push_back({
            location[i].country.in(),
            location[i].region.in(),
            location[i].city.in()});
        }

        std::vector<CampaignManagerCore::GeoCoordInfo> core_coord_location;
        core_coord_location.reserve(coord_location.length());
        for(CORBA::ULong i = 0; i < coord_location.length(); ++i)
        {
          core_coord_location.push_back({
            CorbaAlgs::unpack_decimal<CoordDecimal>(coord_location[i].longitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(coord_location[i].latitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(coord_location[i].accuracy)});
        }

        CampaignManagerCore::IdVector geo_channels;
        CampaignManagerCore::IdVector coord_channels;
        core_->match_geo_channels(
          core_location,
          core_coord_location,
          geo_channels,
          coord_channels);

        geo_channels_result = new AdServer::CampaignSvcs::ChannelIdSeq();
        geo_channels_result->length(geo_channels.size());
        for(CORBA::ULong i = 0; i < geo_channels.size(); ++i)
        {
          (*geo_channels_result)[i] = geo_channels[i];
        }

        coord_channels_result = new AdServer::CampaignSvcs::ChannelIdSeq();
        coord_channels_result->length(coord_channels.size());
        for(CORBA::ULong i = 0; i < coord_channels.size(); ++i)
        {
          (*coord_channels_result)[i] = coord_channels[i];
        }
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::process_match_request(
      const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
        match_request_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::MatchRequestInfo core_info;
        core_info.user_id = CorbaAlgs::unpack_user_id(match_request_info.user_id);
        core_info.household_id =
          CorbaAlgs::unpack_user_id(match_request_info.household_id);
        core_info.request_time =
          CorbaAlgs::unpack_time(match_request_info.request_time);
        core_info.match_info.channels.assign(
          match_request_info.match_info.channels.get_buffer(),
          match_request_info.match_info.channels.get_buffer() +
            match_request_info.match_info.channels.length());
        core_info.match_info.pkw_channels.reserve(
          match_request_info.match_info.pkw_channels.length());
        for(CORBA::ULong i = 0;
          i < match_request_info.match_info.pkw_channels.length();
          ++i)
        {
          CampaignManagerCore::ChannelTriggerMatchInfo trigger;
          trigger.channel_trigger_id =
            match_request_info.match_info.pkw_channels[i].channel_trigger_id;
          trigger.channel_id =
            match_request_info.match_info.pkw_channels[i].channel_id;
          core_info.match_info.pkw_channels.push_back(trigger);
        }
        core_info.match_info.hid_channels.assign(
          match_request_info.match_info.hid_channels.get_buffer(),
          match_request_info.match_info.hid_channels.get_buffer() +
            match_request_info.match_info.hid_channels.length());
        core_info.match_info.colo_id = match_request_info.match_info.colo_id;
        core_info.match_info.location.reserve(
          match_request_info.match_info.location.length());
        for(CORBA::ULong i = 0;
          i < match_request_info.match_info.location.length();
          ++i)
        {
          core_info.match_info.location.push_back({
            match_request_info.match_info.location[i].country.in(),
            match_request_info.match_info.location[i].region.in(),
            match_request_info.match_info.location[i].city.in()});
        }
        core_info.match_info.coord_location.reserve(
          match_request_info.match_info.coord_location.length());
        for(CORBA::ULong i = 0;
          i < match_request_info.match_info.coord_location.length();
          ++i)
        {
          core_info.match_info.coord_location.push_back({
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              match_request_info.match_info.coord_location[i].longitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              match_request_info.match_info.coord_location[i].latitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              match_request_info.match_info.coord_location[i].accuracy)});
        }
        core_info.match_info.full_referer =
          match_request_info.match_info.full_referer.in();
        core_info.source = match_request_info.source.in();

        core_->process_match_request(core_info);
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::process_anonymous_request(
      const AdServer::CampaignSvcs::CampaignManager::AnonymousRequestInfo&
        anon_request_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::AnonymousRequestInfo core_info;
        core_info.time = CorbaAlgs::unpack_time(anon_request_info.time);
        core_info.colo_id = anon_request_info.colo_id;
        core_info.user_status = anon_request_info.user_status;
        core_info.test_request = anon_request_info.test_request;
        core_info.search_engine_id = anon_request_info.search_engine_id;
        core_info.search_words = anon_request_info.search_words.in();
        core_info.client = anon_request_info.client.in();
        core_info.client_version = anon_request_info.client_version.in();
        core_info.platform_ids.reserve(anon_request_info.platform_ids.length());
        for(CORBA::ULong i = 0; i < anon_request_info.platform_ids.length(); ++i)
        {
          core_info.platform_ids.emplace_back(anon_request_info.platform_ids[i]);
        }
        core_info.full_platform = anon_request_info.full_platform.in();
        core_info.web_browser = anon_request_info.web_browser.in();
        core_info.user_agent = anon_request_info.user_agent.in();
        core_info.search_engine_host = anon_request_info.search_engine_host.in();
        core_info.country_code = anon_request_info.country_code.in();
        core_info.page_keywords_present = anon_request_info.page_keywords_present;

        core_->process_anonymous_request(core_info);
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::get_file(
      const char* file_name,
      CORBACommons::OctSeq_out file)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        const auto core_file = core_->get_file(file_name ? file_name : "");
        file = new CORBACommons::OctSeq();
        file->length(core_file.size());
        for(CORBA::ULong i = 0; i < core_file.size(); ++i)
        {
          (*file)[i] = core_file[i];
        }
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::instantiate_ad(
      const AdServer::CampaignSvcs::CampaignManager::
        InstantiateAdInfo& instantiate_ad_info,
      AdServer::CampaignSvcs::CampaignManager::
        InstantiateAdResult_out instantiate_ad_result)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::InstantiateAdInfo core_info;

        core_info.common_info.time =
          CorbaAlgs::unpack_time(instantiate_ad_info.common_info.time);
        core_info.common_info.request_id =
          CorbaAlgs::unpack_request_id(
            instantiate_ad_info.common_info.request_id);
        core_info.common_info.creative_instantiate_type =
          instantiate_ad_info.common_info.creative_instantiate_type.in();
        core_info.common_info.request_type =
          instantiate_ad_info.common_info.request_type;
        core_info.common_info.random = instantiate_ad_info.common_info.random;
        core_info.common_info.test_request =
          instantiate_ad_info.common_info.test_request;
        core_info.common_info.log_as_test =
          instantiate_ad_info.common_info.log_as_test;
        core_info.common_info.colo_id = instantiate_ad_info.common_info.colo_id;
        core_info.common_info.external_user_id =
          instantiate_ad_info.common_info.external_user_id.in();
        core_info.common_info.source_id =
          instantiate_ad_info.common_info.source_id.in();
        core_info.common_info.location.reserve(
          instantiate_ad_info.common_info.location.length());
        for(CORBA::ULong i = 0;
          i < instantiate_ad_info.common_info.location.length();
          ++i)
        {
          core_info.common_info.location.push_back({
            instantiate_ad_info.common_info.location[i].country.in(),
            instantiate_ad_info.common_info.location[i].region.in(),
            instantiate_ad_info.common_info.location[i].city.in()});
        }
        core_info.common_info.coord_location.reserve(
          instantiate_ad_info.common_info.coord_location.length());
        for(CORBA::ULong i = 0;
          i < instantiate_ad_info.common_info.coord_location.length();
          ++i)
        {
          core_info.common_info.coord_location.push_back({
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              instantiate_ad_info.common_info.coord_location[i].longitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              instantiate_ad_info.common_info.coord_location[i].latitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(
              instantiate_ad_info.common_info.coord_location[i].accuracy)});
        }
        core_info.common_info.full_referer =
          instantiate_ad_info.common_info.full_referer.in();
        core_info.common_info.referer =
          instantiate_ad_info.common_info.referer.in();
        core_info.common_info.urls.reserve(
          instantiate_ad_info.common_info.urls.length());
        for(CORBA::ULong i = 0;
          i < instantiate_ad_info.common_info.urls.length();
          ++i)
        {
          core_info.common_info.urls.emplace_back(
            instantiate_ad_info.common_info.urls[i].in());
        }
        core_info.common_info.security_token =
          instantiate_ad_info.common_info.security_token.in();
        core_info.common_info.pub_impr_track_url =
          instantiate_ad_info.common_info.pub_impr_track_url.in();
        core_info.common_info.pub_param =
          instantiate_ad_info.common_info.pub_param.in();
        core_info.common_info.preclick_url =
          instantiate_ad_info.common_info.preclick_url.in();
        core_info.common_info.click_prefix_url =
          instantiate_ad_info.common_info.click_prefix_url.in();
        core_info.common_info.original_url =
          instantiate_ad_info.common_info.original_url.in();
        core_info.common_info.track_user_id =
          CorbaAlgs::unpack_user_id(
            instantiate_ad_info.common_info.track_user_id);
        core_info.common_info.user_id =
          CorbaAlgs::unpack_user_id(instantiate_ad_info.common_info.user_id);
        core_info.common_info.user_status =
          instantiate_ad_info.common_info.user_status;
        core_info.common_info.signed_user_id =
          instantiate_ad_info.common_info.signed_user_id.in();
        core_info.common_info.peer_ip =
          instantiate_ad_info.common_info.peer_ip.in();
        core_info.common_info.user_agent =
          instantiate_ad_info.common_info.user_agent.in();
        core_info.common_info.cohort =
          instantiate_ad_info.common_info.cohort.in();
        core_info.common_info.hpos = instantiate_ad_info.common_info.hpos;
        core_info.common_info.ext_track_params =
          instantiate_ad_info.common_info.ext_track_params.in();
        core_info.common_info.tokens.reserve(
          instantiate_ad_info.common_info.tokens.length());
        for(CORBA::ULong i = 0;
          i < instantiate_ad_info.common_info.tokens.length();
          ++i)
        {
          core_info.common_info.tokens.push_back({
            instantiate_ad_info.common_info.tokens[i].name.in(),
            instantiate_ad_info.common_info.tokens[i].value.in()});
        }
        core_info.common_info.set_cookie =
          instantiate_ad_info.common_info.set_cookie;
        core_info.common_info.passback_type =
          instantiate_ad_info.common_info.passback_type.in();
        core_info.common_info.passback_url =
          instantiate_ad_info.common_info.passback_url.in();

        core_info.context_info.reserve(instantiate_ad_info.context_info.length());
        for(CORBA::ULong i = 0; i < instantiate_ad_info.context_info.length(); ++i)
        {
          CampaignManagerCore::ContextAdRequestInfo context_info;
          context_info.enabled_notice =
            instantiate_ad_info.context_info[i].enabled_notice;
          context_info.client = instantiate_ad_info.context_info[i].client.in();
          context_info.client_version =
            instantiate_ad_info.context_info[i].client_version.in();
          context_info.platform_ids.assign(
            instantiate_ad_info.context_info[i].platform_ids.get_buffer(),
            instantiate_ad_info.context_info[i].platform_ids.get_buffer() +
              instantiate_ad_info.context_info[i].platform_ids.length());
          context_info.geo_channels.assign(
            instantiate_ad_info.context_info[i].geo_channels.get_buffer(),
            instantiate_ad_info.context_info[i].geo_channels.get_buffer() +
              instantiate_ad_info.context_info[i].geo_channels.length());
          context_info.platform =
            instantiate_ad_info.context_info[i].platform.in();
          context_info.full_platform =
            instantiate_ad_info.context_info[i].full_platform.in();
          context_info.web_browser =
            instantiate_ad_info.context_info[i].web_browser.in();
          context_info.ip_hash =
            instantiate_ad_info.context_info[i].ip_hash.in();
          context_info.profile_referer =
            instantiate_ad_info.context_info[i].profile_referer;
          context_info.page_load_id =
            instantiate_ad_info.context_info[i].page_load_id;
          context_info.full_referer_hash =
            instantiate_ad_info.context_info[i].full_referer_hash;
          context_info.short_referer_hash =
            instantiate_ad_info.context_info[i].short_referer_hash;
          core_info.context_info.push_back(context_info);
        }

        core_info.format = instantiate_ad_info.format.in();
        core_info.publisher_site_id = instantiate_ad_info.publisher_site_id;
        core_info.publisher_account_id = instantiate_ad_info.publisher_account_id;
        core_info.tag_id = instantiate_ad_info.tag_id;
        core_info.tag_size_id = instantiate_ad_info.tag_size_id;
        core_info.creatives.reserve(instantiate_ad_info.creatives.length());
        for(CORBA::ULong i = 0; i < instantiate_ad_info.creatives.length(); ++i)
        {
          CampaignManagerCore::TrackCreativeInfo creative;
          creative.ccid = instantiate_ad_info.creatives[i].ccid;
          creative.ccg_keyword_id =
            instantiate_ad_info.creatives[i].ccg_keyword_id;
          creative.request_id =
            CorbaAlgs::unpack_request_id(
              instantiate_ad_info.creatives[i].request_id);
          creative.ctr =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.creatives[i].ctr);
          core_info.creatives.push_back(creative);
        }
        core_info.creative_id = instantiate_ad_info.creative_id;
        if(instantiate_ad_info.user_id_hash_mod.defined)
        {
          core_info.user_id_hash_mod =
            instantiate_ad_info.user_id_hash_mod.value;
        }
        core_info.merged_user_id =
          CorbaAlgs::unpack_user_id(instantiate_ad_info.merged_user_id);
        core_info.pubpixel_accounts.assign(
          instantiate_ad_info.pubpixel_accounts.get_buffer(),
          instantiate_ad_info.pubpixel_accounts.get_buffer() +
            instantiate_ad_info.pubpixel_accounts.length());
        core_info.open_price = instantiate_ad_info.open_price.in();
        core_info.openx_price = instantiate_ad_info.openx_price.in();
        core_info.liverail_price = instantiate_ad_info.liverail_price.in();
        core_info.google_price = instantiate_ad_info.google_price.in();
        core_info.ext_tag_id = instantiate_ad_info.ext_tag_id.in();
        core_info.video_width = instantiate_ad_info.video_width;
        core_info.video_height = instantiate_ad_info.video_height;
        core_info.consider_request = instantiate_ad_info.consider_request;
        core_info.enabled_notice = instantiate_ad_info.enabled_notice;
        core_info.emulate_click = instantiate_ad_info.emulate_click;
        if(instantiate_ad_info.pub_imp_revenue_defined)
        {
          core_info.pub_imp_revenue =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.pub_imp_revenue);
        }

        const CampaignManagerCore::InstantiateAdResult core_result =
          core_->instantiate_ad(core_info);

        instantiate_ad_result =
          new AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult();
        instantiate_ad_result->creative_body << core_result.creative_body;
        instantiate_ad_result->mime_format << core_result.mime_format;
        instantiate_ad_result->request_ids.length(core_result.request_ids.size());
        for(CORBA::ULong i = 0; i < core_result.request_ids.size(); ++i)
        {
          instantiate_ad_result->request_ids[i] =
            CorbaAlgs::pack_request_id(core_result.request_ids[i]);
        }
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::trace_campaign_selection(
      CORBA::ULong campaign_id,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      CORBA::ULong auction_type,
      CORBA::Boolean test_request,
      CORBA::String_out trace_xml)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        CampaignManagerCore::TraceRequestInfo core_request_params;
        core_request_params.common_info.time =
          CorbaAlgs::unpack_time(request_params.common_info.time);
        core_request_params.common_info.creative_instantiate_type =
          request_params.common_info.creative_instantiate_type.in();
        core_request_params.common_info.request_type =
          request_params.common_info.request_type;
        core_request_params.common_info.random = request_params.common_info.random;
        core_request_params.common_info.colo_id =
          request_params.common_info.colo_id;
        core_request_params.common_info.user_id =
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id);
        core_request_params.common_info.user_status =
          request_params.common_info.user_status;
        core_request_params.common_info.location.reserve(
          request_params.common_info.location.length());
        for(CORBA::ULong i = 0;
          i < request_params.common_info.location.length();
          ++i)
        {
          core_request_params.common_info.location.push_back({
            request_params.common_info.location[i].country.in(),
            request_params.common_info.location[i].region.in(),
            request_params.common_info.location[i].city.in()});
        }
        core_request_params.publisher_site_id = request_params.publisher_site_id;
        core_request_params.publisher_account_ids.assign(
          request_params.publisher_account_ids.get_buffer(),
          request_params.publisher_account_ids.get_buffer() +
            request_params.publisher_account_ids.length());
        core_request_params.profiling_available =
          request_params.profiling_available;
        core_request_params.full_freq_caps.assign(
          request_params.full_freq_caps.get_buffer(),
          request_params.full_freq_caps.get_buffer() +
            request_params.full_freq_caps.length());
        core_request_params.channels.assign(
          request_params.channels.get_buffer(),
          request_params.channels.get_buffer() + request_params.channels.length());
        core_request_params.hid_channels.assign(
          request_params.hid_channels.get_buffer(),
          request_params.hid_channels.get_buffer() +
            request_params.hid_channels.length());
        core_request_params.context_info.geo_channels.assign(
          request_params.context_info.geo_channels.get_buffer(),
          request_params.context_info.geo_channels.get_buffer() +
            request_params.context_info.geo_channels.length());
        core_request_params.client_create_time =
          CorbaAlgs::unpack_time(request_params.client_create_time);
        core_request_params.tag_delivery_factor =
          request_params.tag_delivery_factor;
        core_request_params.ccg_delivery_factor =
          request_params.ccg_delivery_factor;

        CampaignManagerCore::TraceAdSlotInfo core_ad_slot;
        core_ad_slot.format = ad_slot.format.in();
        core_ad_slot.tag_id = ad_slot.tag_id;
        core_ad_slot.sizes.reserve(ad_slot.sizes.length());
        for(CORBA::ULong i = 0; i < ad_slot.sizes.length(); ++i)
        {
          core_ad_slot.sizes.emplace_back(ad_slot.sizes[i].in());
        }
        core_ad_slot.min_ecpm =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(ad_slot.min_ecpm);
        core_ad_slot.min_ecpm_currency_code =
          ad_slot.min_ecpm_currency_code.in();
        core_ad_slot.exclude_categories.reserve(
          ad_slot.exclude_categories.length());
        for(CORBA::ULong i = 0; i < ad_slot.exclude_categories.length(); ++i)
        {
          core_ad_slot.exclude_categories.emplace_back(
            ad_slot.exclude_categories[i].in());
        }
        core_ad_slot.required_categories.reserve(
          ad_slot.required_categories.length());
        for(CORBA::ULong i = 0; i < ad_slot.required_categories.length(); ++i)
        {
          core_ad_slot.required_categories.emplace_back(
            ad_slot.required_categories[i].in());
        }
        core_ad_slot.up_expand_space = ad_slot.up_expand_space;
        core_ad_slot.right_expand_space = ad_slot.right_expand_space;
        core_ad_slot.left_expand_space = ad_slot.left_expand_space;
        core_ad_slot.down_expand_space = ad_slot.down_expand_space;
        core_ad_slot.tag_visibility = ad_slot.tag_visibility;
        core_ad_slot.video_min_duration = ad_slot.video_min_duration;
        core_ad_slot.video_max_duration = ad_slot.video_max_duration;
        core_ad_slot.video_skippable_max_duration =
          ad_slot.video_skippable_max_duration;
        core_ad_slot.video_allow_skippable = ad_slot.video_allow_skippable;
        core_ad_slot.video_allow_unskippable = ad_slot.video_allow_unskippable;
        core_ad_slot.allowed_durations.assign(
          ad_slot.allowed_durations.get_buffer(),
          ad_slot.allowed_durations.get_buffer() +
            ad_slot.allowed_durations.length());

        trace_xml << core_->trace_campaign_selection(
          campaign_id,
          core_request_params,
          core_ad_slot,
          auction_type,
          test_request);
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::trace_campaign_selection_index(
      CORBA::String_out trace_xml)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        trace_xml << core_->trace_campaign_selection_index();
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    CORBA::Boolean
    CampaignManagerImpl::get_campaign_creative_by_ccid(
      const ::AdServer::CampaignSvcs::CampaignManager::CreativeParams& params,
      CORBA::String_out creative_body)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        CampaignManagerCore::PreviewCreativeParams core_params;
        core_params.ccid = params.ccid;
        core_params.tag_id = params.tag_id;
        core_params.format = params.format.in();
        core_params.original_url = params.original_url.in();
        core_params.peer_ip = params.peer_ip.in();

        std::string core_creative_body;
        const bool result =
          core_->get_campaign_creative_by_ccid(core_params, core_creative_body);
        creative_body << core_creative_body;
        return result;
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }

      return false;
    }

    void
    CampaignManagerImpl::consider_passback(
      const AdServer::CampaignSvcs::CampaignManager::PassbackInfo& in)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        CampaignManagerCore::PassbackInfo core_info;
        core_info.request_id = CorbaAlgs::unpack_request_id(in.request_id);
        core_info.time = CorbaAlgs::unpack_time(in.time);
        core_info.user_id_hash_mod = in.user_id_hash_mod.defined ?
          AdServer::Commons::Optional<unsigned long>(
            in.user_id_hash_mod.value) :
          AdServer::Commons::Optional<unsigned long>();

        core_->consider_passback(core_info);
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    void
    CampaignManagerImpl::consider_passback_track(
      const AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo& in)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::PassbackTrackInfo core_info;
        core_info.time = CorbaAlgs::unpack_time(in.time);
        core_info.country = in.country;
        core_info.colo_id = in.colo_id;
        core_info.tag_id = in.tag_id;
        core_info.user_status = in.user_status;

        core_->consider_passback_track(core_info);
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    CORBA::Boolean
    CampaignManagerImpl::get_click_url(
      const ::AdServer::CampaignSvcs::CampaignManager::ClickInfo& click_info,
      ::AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_out click_result_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::ClickInfo core_click_info;
        core_click_info.time = CorbaAlgs::unpack_time(click_info.time);
        core_click_info.bid_time = CorbaAlgs::unpack_time(click_info.bid_time);
        core_click_info.colo_id = click_info.colo_id;
        core_click_info.tag_id = click_info.tag_id;
        core_click_info.tag_size_id = click_info.tag_size_id;
        core_click_info.ccid = click_info.ccid;
        core_click_info.ccg_keyword_id = click_info.ccg_keyword_id;
        core_click_info.creative_id = click_info.creative_id;
        core_click_info.match_user_id =
          CorbaAlgs::unpack_user_id(click_info.match_user_id);
        core_click_info.cookie_user_id =
          CorbaAlgs::unpack_user_id(click_info.cookie_user_id);
        core_click_info.request_id =
          CorbaAlgs::unpack_request_id(click_info.request_id);
        if(click_info.user_id_hash_mod.defined)
        {
          core_click_info.user_id_hash_mod = click_info.user_id_hash_mod.value;
        }
        core_click_info.relocate = click_info.relocate.in();
        core_click_info.referer = click_info.referer.in();
        core_click_info.log_click = click_info.log_click;
        core_click_info.ctr =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(click_info.ctr);
        for(CORBA::ULong tok_i = 0; tok_i < click_info.tokens.length(); ++tok_i)
        {
          core_click_info.tokens[click_info.tokens[tok_i].name.in()] =
            click_info.tokens[tok_i].value.in();
        }

        CampaignManagerCore::ClickResultInfo core_result;
        const bool result = core_->get_click_url(core_click_info, core_result);

        click_result_info = new AdServer::CampaignSvcs::
          CampaignManager::ClickResultInfo();
        click_result_info->url << core_result.url;
        click_result_info->campaign_id = core_result.campaign_id;
        click_result_info->advertiser_id = core_result.advertiser_id;

        return result;
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const eh::Exception& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::ImplementationException(
          ex.what());
      }
    }

    void
    CampaignManagerImpl::verify_impression(
      const AdServer::CampaignSvcs::CampaignManager::ImpressionInfo& impression_info,
      ::AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_out impression_result_info)
      /*throw(
        AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::ImpressionInfo core_info;
        core_info.time = CorbaAlgs::unpack_time(impression_info.time);
        core_info.bid_time = CorbaAlgs::unpack_time(impression_info.bid_time);
        if(impression_info.user_id_hash_mod.defined)
        {
          core_info.user_id_hash_mod = impression_info.user_id_hash_mod.value;
        }
        core_info.creatives.reserve(impression_info.creatives.length());
        for(CORBA::ULong i = 0; i < impression_info.creatives.length(); ++i)
        {
          CampaignManagerCore::TrackCreativeInfo creative;
          creative.ccid = impression_info.creatives[i].ccid;
          creative.ccg_keyword_id = impression_info.creatives[i].ccg_keyword_id;
          creative.request_id =
            CorbaAlgs::unpack_request_id(impression_info.creatives[i].request_id);
          creative.ctr =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              impression_info.creatives[i].ctr);
          core_info.creatives.push_back(creative);
        }
        core_info.pub_imp_revenue_type = impression_info.pub_imp_revenue_type;
        core_info.pub_imp_revenue =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(
            impression_info.pub_imp_revenue);
        core_info.request_type = impression_info.request_type;
        core_info.verify_type = impression_info.verify_type;
        core_info.user_id = CorbaAlgs::unpack_user_id(impression_info.user_id);
        core_info.referer = impression_info.referer.in();
        core_info.viewability = impression_info.viewability;
        core_info.action_name = impression_info.action_name.in();

        const CampaignManagerCore::ImpressionResultInfo core_result =
          core_->verify_impression(core_info);

        impression_result_info =
          new AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo();
        impression_result_info->creatives.length(core_result.size());
        for(CORBA::ULong i = 0; i < core_result.size(); ++i)
        {
          impression_result_info->creatives[i].campaign_id =
            core_result[i].campaign_id;
          impression_result_info->creatives[i].advertiser_id =
            core_result[i].advertiser_id;
        }
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const eh::Exception& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::ImplementationException(
          ex.what());
      }
    }

    void
    CampaignManagerImpl::action_taken(
      const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
      /*throw(
        AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::ActionInfo core_action_info;
        core_action_info.time = CorbaAlgs::unpack_time(action_info.time);
        core_action_info.test_request = action_info.test_request;
        core_action_info.log_as_test = action_info.log_as_test;
        if(action_info.campaign_id_defined)
        {
          core_action_info.campaign_id.emplace(action_info.campaign_id);
        }
        if(action_info.action_id_defined)
        {
          core_action_info.action_id.emplace(action_info.action_id);
        }
        core_action_info.order_id = action_info.order_id.in();
        if(action_info.action_value_defined)
        {
          core_action_info.action_value.emplace(
            CorbaAlgs::unpack_decimal<RevenueDecimal>(action_info.action_value));
        }
        core_action_info.referer = action_info.referer.in();
        core_action_info.user_status = action_info.user_status;
        core_action_info.user_id = CorbaAlgs::unpack_user_id(action_info.user_id);
        core_action_info.ip_hash = action_info.ip_hash.in();
        core_action_info.platform_ids.assign(
          action_info.platform_ids.get_buffer(),
          action_info.platform_ids.get_buffer() + action_info.platform_ids.length());
        core_action_info.peer_ip = action_info.peer_ip.in();
        core_action_info.location.reserve(action_info.location.length());
        for(CORBA::ULong loc_i = 0; loc_i < action_info.location.length(); ++loc_i)
        {
          CampaignManagerCore::GeoInfo geo_info;
          geo_info.country = action_info.location[loc_i].country.in();
          geo_info.region = action_info.location[loc_i].region.in();
          geo_info.city = action_info.location[loc_i].city.in();
          core_action_info.location.push_back(geo_info);
        }

        core_->action_taken(core_action_info);
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(
          ex.what());
      }
      catch(const eh::Exception& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::ImplementationException(
          ex.what());
      }
    }

    AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq*
    CampaignManagerImpl::get_channel_links(
      const AdServer::CampaignSvcs::ChannelIdSeq& channels,
      bool match)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        CampaignManagerCore::IdVector core_channels(
          channels.get_buffer(),
          channels.get_buffer() + channels.length());
        const auto core_result = core_->get_channel_links(core_channels, match);

        AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq_var
          result = new AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq();
        result->length(core_result.size());
        for(CORBA::ULong i = 0; i < core_result.size(); ++i)
        {
          const auto& source = core_result[i];
          (*result)[i].channel_id = source.channel_id;
          (*result)[i].use_count = source.use_count;
          pack_id_seq(source.matched_simple_channels, (*result)[i].matched_simple_channels);
          pack_id_seq(source.ccg_ids, (*result)[i].ccg_ids);
          (*result)[i].discover_query << source.discover_query;
          (*result)[i].language << source.language;
        }

        return result._retn();
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }

      return 0;
    }

    AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq*
    CampaignManagerImpl::get_discover_channels(
      const AdServer::CampaignSvcs::ChannelWeightSeq& channels,
      const char* country,
      const char* language,
      bool all)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        std::vector<CampaignManagerCore::ChannelWeight> core_channels;
        core_channels.reserve(channels.length());
        for(CORBA::ULong i = 0; i < channels.length(); ++i)
        {
          core_channels.push_back({channels[i].channel_id, channels[i].weight});
        }

        const auto core_result = core_->get_discover_channels(
          core_channels,
          country ? country : "",
          language ? language : "",
          all);

        AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq_var
          result = new AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq();
        result->length(core_result.size());
        for(CORBA::ULong i = 0; i < core_result.size(); ++i)
        {
          const auto& source = core_result[i];
          (*result)[i].channel_id = source.channel_id;
          (*result)[i].name << source.name;
          (*result)[i].query << source.query;
          (*result)[i].annotation << source.annotation;
          (*result)[i].weight = source.weight;
          pack_id_seq(source.categories, (*result)[i].categories);
          (*result)[i].country_code << source.country_code;
          (*result)[i].language << source.language;
        }

        return result._retn();
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }

      return 0;
    }

    AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq*
    CampaignManagerImpl::get_category_channels(const char* language)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        const auto core_result = core_->get_category_channels(
          language ? language : "");

        AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq_var
          result = new AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq();
        pack_category_channel_nodes(core_result, *result);
        return result._retn();
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }

      return 0;
    }

    void
    CampaignManagerImpl::verify_opt_operation(
      ::CORBA::ULong time,
      ::CORBA::Long colo_id,
      const char* referer,
      AdServer::CampaignSvcs::CampaignManager::OptOperation operation,
      ::CORBA::ULong status,
      ::CORBA::ULong user_status,
      bool log_as_test,
      const char* browser,
      const char* os,
      const char* ct,
      const char* curct,
      const CORBACommons::UserIdInfo& user_id)
      /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      CampaignManagerCore::OptOperation core_operation =
        CampaignManagerCore::OptOperation::STATUS;
      switch(operation)
      {
      case AdServer::CampaignSvcs::CampaignManager::OO_IN:
        core_operation = CampaignManagerCore::OptOperation::IN;
        break;
      case AdServer::CampaignSvcs::CampaignManager::OO_OUT:
        core_operation = CampaignManagerCore::OptOperation::OUT;
        break;
      case AdServer::CampaignSvcs::CampaignManager::OO_FORCED_IN:
        core_operation = CampaignManagerCore::OptOperation::FORCED_IN;
        break;
      case AdServer::CampaignSvcs::CampaignManager::OO_STATUS:
      default:
        core_operation = CampaignManagerCore::OptOperation::STATUS;
        break;
      }

      core_->verify_opt_operation(
        time,
        colo_id,
        referer ? referer : "",
        core_operation,
        status,
        user_status,
        log_as_test,
        browser ? browser : "",
        os ? os : "",
        ct ? ct : "",
        curct ? curct : "",
        CorbaAlgs::unpack_user_id(user_id));
    }

    void
    CampaignManagerImpl::consider_web_operation(
      const AdServer::CampaignSvcs::CampaignManager::WebOperationInfo& web_op_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::IncorrectArgument,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        CampaignManagerCore::WebOperationInfo core_info;
        core_info.time = CorbaAlgs::unpack_time(web_op_info.time);
        core_info.colo_id = web_op_info.colo_id;
        core_info.tag_id = web_op_info.tag_id;
        core_info.cc_id = web_op_info.cc_id;
        core_info.ct = web_op_info.ct.in();
        core_info.curct = web_op_info.curct.in();
        core_info.browser = web_op_info.browser.in();
        core_info.os = web_op_info.os.in();
        core_info.app = web_op_info.app.in();
        core_info.source = web_op_info.source.in();
        core_info.operation = web_op_info.operation.in();
        core_info.user_bind_src = web_op_info.user_bind_src.in();
        core_info.result = web_op_info.result;
        core_info.user_status = web_op_info.user_status;
        core_info.test_request = web_op_info.test_request;
        core_info.request_ids.reserve(web_op_info.request_ids.length());
        for(CORBA::ULong i = 0; i < web_op_info.request_ids.length(); ++i)
        {
          core_info.request_ids.emplace_back(
            CorbaAlgs::unpack_request_id(web_op_info.request_ids[i]));
        }
        core_info.global_request_id = CorbaAlgs::unpack_request_id(
          web_op_info.global_request_id);
        core_info.referer = web_op_info.referer.in();
        core_info.ip_address = web_op_info.ip_address.in();
        core_info.external_user_id = web_op_info.external_user_id.in();
        core_info.user_agent = web_op_info.user_agent.in();

        core_->consider_web_operation(core_info);
      }
      catch(const CampaignManagerCore::InvalidArgument&)
      {
        throw AdServer::CampaignSvcs::CampaignManager::IncorrectArgument();
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
    }

    ColocationFlagsSeq*
    CampaignManagerImpl::get_colocation_flags()
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      try
      {
        const auto core_result = core_->get_colocation_flags();
        ColocationFlagsSeq_var result = new ColocationFlagsSeq();
        result->length(core_result.size());
        for(CORBA::ULong i = 0; i < core_result.size(); ++i)
        {
          result[i].colo_id = core_result[i].colo_id;
          result[i].flags = core_result[i].flags;
          result[i].hid_profile = core_result[i].hid_profile;
        }
        return result._retn();
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
      return 0;
    }

    AdServer::CampaignSvcs::StringSeq*
    CampaignManagerImpl::get_pub_pixels(
      const char* country,
      CORBA::ULong user_status,
      const AdServer::CampaignSvcs::PublisherAccountIdSeq& publisher_account_ids)
      /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady,
        AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      try
      {
        CampaignManagerCore::IdVector core_publisher_account_ids;
        core_publisher_account_ids.reserve(publisher_account_ids.length());
        for(CORBA::ULong i = 0; i < publisher_account_ids.length(); ++i)
        {
          core_publisher_account_ids.emplace_back(publisher_account_ids[i]);
        }

        const auto core_result = core_->get_pub_pixels(
          country ? country : "",
          user_status,
          core_publisher_account_ids);

        AdServer::CampaignSvcs::StringSeq_var result =
          new AdServer::CampaignSvcs::StringSeq();
        result->length(core_result.size());
        for(CORBA::ULong i = 0; i < core_result.size(); ++i)
        {
          (*result)[i] << core_result[i];
        }
        return result._retn();
      }
      catch(const CampaignManagerCore::NotReady& ex)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(ex.what());
      }
      catch(const CampaignManagerCore::Exception& ex)
      {
        CORBACommons::throw_desc<
          AdServer::CampaignSvcs::CampaignManager::ImplementationException>(
            String::SubString(ex.what()));
      }
      return 0;
    }

    template<typename Seq>
    void
    CampaignManagerImpl::pack_id_seq(
      const CampaignManagerCore::IdVector& source,
      Seq& target)
    {
      target.length(source.size());
      for(CORBA::ULong i = 0; i < source.size(); ++i)
      {
        target[i] = source[i];
      }
    }

    void
    CampaignManagerImpl::pack_category_channel_nodes(
      const std::vector<CampaignManagerCore::CategoryChannelNodeInfo>& source,
      AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq& target)
    {
      target.length(source.size());
      for(CORBA::ULong i = 0; i < source.size(); ++i)
      {
        target[i].channel_id = source[i].channel_id;
        target[i].name << source[i].name;
        target[i].flags = source[i].flags;
        pack_category_channel_nodes(
          source[i].child_category_channels,
          target[i].child_category_channels);
      }
    }
  }
}
