#include <sstream>
#include <algorithm>
#include <set>
#include <utility>
#include <zlib.h>
#include <unistd.h>

#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/TaskPool.hpp>
#include "Generics/CompositeMetricsProvider.hpp"

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Commons/ExternalUserIdUtils.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/BidStatisticsPrometheus.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <UserInfoSvcs/UserInfoClient/UserInfoCorbaClient.hpp>

#include "OpenRtbBidRequestTask.hpp"
#include "GoogleBidRequestTask.hpp"
#include "AdXmlBidRequestTask.hpp"
#include "ClickStarBidRequestTask.hpp"
#include "AdJsonBidRequestTask.hpp"
#include "DAOBidRequestTask.hpp"
#include "RequestMetricsProvider.hpp"
#include "Utils.hpp"

#include "BiddingFrontend.hpp"

namespace Config
{
  const char ENABLE[] = "BiddingFrontend_Enable";
  const char CONFIG_FILES[] = "BiddingFrontend_Config";
  const char CONFIG_FILE[] = "BiddingFrontend_ConfigFile";
}

namespace Aspect
{
  extern const char BIDDING_FRONTEND[] = "BiddingFrontend";
}

namespace Response
{
  namespace Header
  {
    const String::SubString CONTENT_TYPE("Content-Type");
  }
}

namespace AdServer::Bidding
{
  namespace
  {
    namespace PB = adserver::campaign_svcs::campaign_manager;

    const CampaignSvcs::RevenueDecimal MAX_CPM_CONF_MULTIPLIER(false, 100, 0);
    static const UserInfoSvcs::CampaignIdSeq EMPTY_CAMPAIGN_ID_SEQ;

    template<typename CorbaOctSeq>
    std::string pack_oct_seq(const CorbaOctSeq& seq)
    {
      return std::string(
        reinterpret_cast<const char*>(seq.get_buffer()),
        reinterpret_cast<const char*>(seq.get_buffer()) + seq.length());
    }

    template<typename CorbaOctSeq>
    void unpack_oct_seq(const std::string& value, CorbaOctSeq& target)
    {
      target.length(value.size());
      std::copy(value.begin(), value.end(), target.get_buffer());
    }

    template<typename SourceSeq>
    void pack_ids(
      const SourceSeq& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        target->Add(source[i]);
      }
    }

    template<typename SourceSeq>
    void pack_strings(
      const SourceSeq& source,
      google::protobuf::RepeatedPtrField<std::string>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        *target->Add() = source[i].str;
      }
    }

    template<typename TargetSeq>
    void unpack_ids(
      const google::protobuf::RepeatedField<google::protobuf::uint64>& source,
      TargetSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i] = source[i];
      }
    }

    template<typename TargetSeq>
    void unpack_strings(
      const google::protobuf::RepeatedPtrField<std::string>& source,
      TargetSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i] = source[i];
      }
    }

    void pack_decimal(
      const CampaignManager::DecimalInfo& source,
      PB::DecimalInfo& target)
    {
      target.set_value(pack_oct_seq(source));
    }

    void unpack_decimal(
      const PB::DecimalInfo& source,
      CampaignManager::DecimalInfo& target)
    {
      unpack_oct_seq(source.value(), target);
    }

    void pack_tokens(
      const CampaignManager::TokenSeq& source,
      google::protobuf::RepeatedPtrField<PB::TokenInfo>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        auto* token = target->Add();
        token->set_name(source[i].name.str);
        token->set_value(source[i].value.str);
      }
    }

    void unpack_tokens(
      const google::protobuf::RepeatedPtrField<PB::TokenInfo>& source,
      CampaignManager::TokenSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i].name = source[i].name();
        target[i].value = source[i].value();
      }
    }

    void pack_common_ad_request_info(
      const CampaignManager::CommonAdRequestInfo& source,
      PB::CommonAdRequestInfo& target)
    {
      target.set_time(pack_oct_seq(source.time));
      target.set_request_id(pack_oct_seq(source.request_id));
      target.set_creative_instantiate_type(source.creative_instantiate_type.str);
      target.set_request_type(source.request_type);
      target.set_random(source.random);
      target.set_test_request(source.test_request);
      target.set_log_as_test(source.log_as_test);
      target.set_colo_id(source.colo_id);
      target.set_external_user_id(source.external_user_id.str);
      target.set_source_id(source.source_id.str);
      for(CORBA::ULong i = 0; i < source.location.length(); ++i)
      {
        auto* location = target.add_location();
        location->set_country(source.location[i].country.str);
        location->set_region(source.location[i].region.str);
        location->set_city(source.location[i].city.str);
      }
      for(CORBA::ULong i = 0; i < source.coord_location.length(); ++i)
      {
        auto* coord = target.add_coord_location();
        coord->set_longitude(pack_oct_seq(source.coord_location[i].longitude));
        coord->set_latitude(pack_oct_seq(source.coord_location[i].latitude));
        coord->set_accuracy(pack_oct_seq(source.coord_location[i].accuracy));
      }
      target.set_full_referer(source.full_referer.str);
      target.set_referer(source.referer.str);
      pack_strings(source.urls, target.mutable_urls());
      target.set_security_token(source.security_token.str);
      target.set_pub_impr_track_url(source.pub_impr_track_url.str);
      target.set_pub_param(source.pub_param.str);
      target.set_preclick_url(source.preclick_url.str);
      target.set_click_prefix_url(source.click_prefix_url.str);
      target.set_original_url(source.original_url.str);
      target.set_track_user_id(pack_oct_seq(source.track_user_id));
      target.set_user_id(pack_oct_seq(source.user_id));
      target.set_user_status(source.user_status);
      target.set_peer_ip(source.peer_ip.str);
      target.set_user_agent(source.user_agent.str);
      target.set_cohort(source.cohort.str);
      target.set_hpos(source.hpos);
      target.set_ext_track_params(source.ext_track_params.str);
      pack_tokens(source.tokens, target.mutable_tokens());
      target.set_set_cookie(source.set_cookie);
      target.set_passback_type(source.passback_type.str);
      target.set_passback_url(source.passback_url.str);
    }

    void pack_context_ad_request_info(
      const CampaignManager::ContextAdRequestInfo& source,
      PB::ContextAdRequestInfo& target)
    {
      target.set_enabled_notice(source.enabled_notice);
      target.set_client(source.client.str);
      target.set_client_version(source.client_version.str);
      pack_ids(source.platform_ids, target.mutable_platform_ids());
      pack_ids(source.geo_channels, target.mutable_geo_channels());
      target.set_platform(source.platform.str);
      target.set_full_platform(source.full_platform.str);
      target.set_web_browser(source.web_browser.str);
      target.set_ip_hash(source.ip_hash.str);
      target.set_profile_referer(source.profile_referer);
      target.set_page_load_id(source.page_load_id);
      target.set_full_referer_hash(source.full_referer_hash);
      target.set_short_referer_hash(source.short_referer_hash);
    }

    void pack_request_params(
      const CampaignManager::RequestParams& source,
      PB::RequestParams& target)
    {
      pack_common_ad_request_info(source.common_info, *target.mutable_common_info());
      pack_context_ad_request_info(source.context_info, *target.mutable_context_info());
      target.set_publisher_site_id(source.publisher_site_id);
      pack_ids(source.publisher_account_ids, target.mutable_publisher_account_ids());
      target.set_fill_track_pixel(source.fill_track_pixel);
      target.set_fill_iurl(source.fill_iurl);
      target.set_ad_instantiate_type(source.ad_instantiate_type);
      target.set_only_display_ad(source.only_display_ad);
      pack_ids(source.full_freq_caps, target.mutable_full_freq_caps());
      for(CORBA::ULong i = 0; i < source.seq_orders.length(); ++i)
      {
        auto* seq_order = target.add_seq_orders();
        seq_order->set_ccg_id(source.seq_orders[i].ccg_id);
        seq_order->set_set_id(source.seq_orders[i].set_id);
        seq_order->set_imps(source.seq_orders[i].imps);
      }
      for(CORBA::ULong i = 0; i < source.campaign_freqs.length(); ++i)
      {
        auto* campaign_freq = target.add_campaign_freqs();
        campaign_freq->set_campaign_id(source.campaign_freqs[i].campaign_id);
        campaign_freq->set_imps(source.campaign_freqs[i].imps);
      }
      target.set_household_id(pack_oct_seq(source.household_id));
      target.set_merged_user_id(pack_oct_seq(source.merged_user_id));
      target.set_search_engine_id(source.search_engine_id);
      target.set_search_words(source.search_words.str);
      target.set_page_keywords_present(source.page_keywords_present);
      target.set_profiling_available(source.profiling_available);
      target.set_fraud(source.fraud);
      pack_ids(source.channels, target.mutable_channels());
      pack_ids(source.hid_channels, target.mutable_hid_channels());
      for(CORBA::ULong i = 0; i < source.ccg_keywords.length(); ++i)
      {
        auto* kw = target.add_ccg_keywords();
        kw->set_ccg_keyword_id(source.ccg_keywords[i].ccg_keyword_id);
        kw->set_ccg_id(source.ccg_keywords[i].ccg_id);
        kw->set_channel_id(source.ccg_keywords[i].channel_id);
        pack_decimal(source.ccg_keywords[i].max_cpc, *kw->mutable_max_cpc());
        pack_decimal(source.ccg_keywords[i].ctr, *kw->mutable_ctr());
        kw->set_click_url(source.ccg_keywords[i].click_url.str);
        kw->set_original_keyword(source.ccg_keywords[i].original_keyword.str);
      }
      auto* trigger = target.mutable_trigger_match_result();
      auto pack_trigger = [](const auto& source_channels, auto* target_channels)
      {
        for(CORBA::ULong i = 0; i < source_channels.length(); ++i)
        {
          auto* channel = target_channels->Add();
          channel->set_channel_id(source_channels[i].channel_id);
          channel->set_channel_trigger_id(source_channels[i].channel_trigger_id);
        }
      };
      pack_trigger(source.trigger_match_result.url_channels, trigger->mutable_url_channels());
      pack_trigger(source.trigger_match_result.pkw_channels, trigger->mutable_pkw_channels());
      pack_trigger(source.trigger_match_result.skw_channels, trigger->mutable_skw_channels());
      pack_trigger(source.trigger_match_result.ukw_channels, trigger->mutable_ukw_channels());
      pack_ids(source.trigger_match_result.uid_channels, trigger->mutable_uid_channels());
      target.set_client_create_time(pack_oct_seq(source.client_create_time));
      target.set_session_start(pack_oct_seq(source.session_start));
      pack_ids(source.exclude_pubpixel_accounts, target.mutable_exclude_pubpixel_accounts());
      target.set_tag_delivery_factor(source.tag_delivery_factor);
      target.set_ccg_delivery_factor(source.ccg_delivery_factor);
      target.set_preview_ccid(source.preview_ccid);
      for(CORBA::ULong i = 0; i < source.ad_slots.length(); ++i)
      {
        const auto& src = source.ad_slots[i];
        auto* dst = target.add_ad_slots();
        dst->set_ad_slot_id(src.ad_slot_id);
        dst->set_format(src.format.str);
        dst->set_tag_id(src.tag_id);
        pack_strings(src.sizes, dst->mutable_sizes());
        dst->set_ext_tag_id(src.ext_tag_id.str);
        pack_decimal(src.min_ecpm, *dst->mutable_min_ecpm());
        dst->set_min_ecpm_currency_code(src.min_ecpm_currency_code.str);
        pack_strings(src.currency_codes, dst->mutable_currency_codes());
        dst->set_passback(src.passback);
        dst->set_up_expand_space(src.up_expand_space);
        dst->set_right_expand_space(src.right_expand_space);
        dst->set_left_expand_space(src.left_expand_space);
        dst->set_tag_visibility(src.tag_visibility);
        dst->set_tag_predicted_viewability(src.tag_predicted_viewability);
        dst->set_down_expand_space(src.down_expand_space);
        dst->set_video_min_duration(src.video_min_duration);
        dst->set_video_max_duration(src.video_max_duration);
        dst->set_video_skippable_max_duration(src.video_skippable_max_duration);
        dst->set_video_allow_skippable(src.video_allow_skippable);
        dst->set_video_allow_unskippable(src.video_allow_unskippable);
        dst->set_video_width(src.video_width);
        dst->set_video_height(src.video_height);
        pack_strings(src.exclude_categories, dst->mutable_exclude_categories());
        pack_strings(src.required_categories, dst->mutable_required_categories());
        dst->set_debug_ccg(src.debug_ccg);
        pack_ids(src.allowed_durations, dst->mutable_allowed_durations());
        for(CORBA::ULong j = 0; j < src.native_data_tokens.length(); ++j)
        {
          auto* token = dst->add_native_data_tokens();
          token->set_name(src.native_data_tokens[j].name.str);
          token->set_required(src.native_data_tokens[j].required);
        }
        for(CORBA::ULong j = 0; j < src.native_image_tokens.length(); ++j)
        {
          auto* token = dst->add_native_image_tokens();
          token->set_name(src.native_image_tokens[j].name.str);
          token->set_required(src.native_image_tokens[j].required);
          token->set_width(src.native_image_tokens[j].width);
          token->set_height(src.native_image_tokens[j].height);
        }
        dst->set_native_ads_impression_tracker_type(
          src.native_ads_impression_tracker_type);
        dst->set_fill_track_html(src.fill_track_html);
        pack_tokens(src.tokens, dst->mutable_tokens());
      }
      target.set_required_passback(source.required_passback);
      target.set_profiling_type(source.profiling_type);
      target.set_disable_fraud_detection(source.disable_fraud_detection);
      target.set_need_debug_info(source.need_debug_info);
      target.set_page_keywords(source.page_keywords.str);
      target.set_url_keywords(source.url_keywords.str);
      target.set_ssp_location(source.ssp_location.str);
      target.set_additional_info(source.additional_info.str);
    }

    void unpack_request_creative_result(
      const PB::RequestCreativeResult& source,
      CampaignManager::RequestCreativeResult& target)
    {
      target.ad_slots.length(source.ad_slots_size());
      for(int i = 0; i < source.ad_slots_size(); ++i)
      {
        const auto& src = source.ad_slots(i);
        auto& dst = target.ad_slots[i];
        dst.ad_slot_id = src.ad_slot_id();
        unpack_oct_seq(src.request_id(), dst.request_id);
        dst.passback = src.passback();
        dst.passback_url = src.passback_url();
        dst.creative_body = src.creative_body();
        dst.notice_url = src.notice_url();
        unpack_strings(src.track_pixel_urls(), dst.track_pixel_urls);
        dst.yandex_track_params = src.yandex_track_params();
        dst.creative_url = src.creative_url();
        dst.track_pixel_params = src.track_pixel_params();
        dst.click_params = src.click_params();
        dst.mime_format = src.mime_format();
        dst.iurl = src.iurl();
        dst.test_request = src.test_request();

        dst.selected_creatives.length(src.selected_creatives_size());
        for(int j = 0; j < src.selected_creatives_size(); ++j)
        {
          const auto& src_creative = src.selected_creatives(j);
          auto& dst_creative = dst.selected_creatives[j];
          unpack_oct_seq(src_creative.request_id(), dst_creative.request_id);
          dst_creative.ccid = src_creative.ccid();
          dst_creative.cmp_id = src_creative.cmp_id();
          dst_creative.campaign_group_id = src_creative.campaign_group_id();
          dst_creative.order_set_id = src_creative.order_set_id();
          dst_creative.advertiser_id = src_creative.advertiser_id();
          dst_creative.advertiser_name = src_creative.advertiser_name();
          dst_creative.creative_size = src_creative.creative_size();
          unpack_decimal(src_creative.revenue(), dst_creative.revenue);
          unpack_decimal(src_creative.ecpm(), dst_creative.ecpm);
          unpack_decimal(src_creative.pub_ecpm(), dst_creative.pub_ecpm);
          dst_creative.click_url = src_creative.click_url();
          dst_creative.destination_url = src_creative.destination_url();
          dst_creative.creative_version_id = src_creative.creative_version_id();
          dst_creative.creative_id = src_creative.creative_id();
          dst_creative.https_safe_flag = src_creative.https_safe_flag();
          dst_creative.expanding = src_creative.expanding();
        }

        unpack_strings(
          src.external_visual_categories(),
          dst.external_visual_categories);
        unpack_strings(
          src.external_content_categories(),
          dst.external_content_categories);
        dst.pub_currency_code = src.pub_currency_code();
        dst.overlay_width = src.overlay_width();
        dst.overlay_height = src.overlay_height();
        unpack_tokens(src.tokens(), dst.tokens);
        unpack_tokens(src.ext_tokens(), dst.ext_tokens);
        dst.track_impr = src.track_impr();
        dst.tag_size = src.tag_size();
        unpack_ids(src.freq_caps(), dst.freq_caps);
        unpack_ids(src.uc_freq_caps(), dst.uc_freq_caps);

        const auto& src_debug = src.debug_info();
        auto& dst_debug = dst.debug_info;
        dst_debug.tag_id = src_debug.tag_id();
        dst_debug.tag_size_id = src_debug.tag_size_id();
        dst_debug.site_id = src_debug.site_id();
        dst_debug.site_rate_id = src_debug.site_rate_id();
        dst_debug.min_no_adv_ecpm = src_debug.min_no_adv_ecpm();
        dst_debug.min_text_ecpm = src_debug.min_text_ecpm();
        dst_debug.auction_type = src_debug.auction_type();
        dst_debug.track_pixel_url = src_debug.track_pixel_url();
        unpack_decimal(src_debug.cpm_threshold(), dst_debug.cpm_threshold);
        dst_debug.walled_garden = src_debug.walled_garden();
        dst_debug.selected_creatives.length(src_debug.selected_creatives_size());
        for(int j = 0; j < src_debug.selected_creatives_size(); ++j)
        {
          const auto& src_debug_creative = src_debug.selected_creatives(j);
          auto& dst_debug_creative = dst_debug.selected_creatives[j];
          unpack_decimal(src_debug_creative.imp_revenue(), dst_debug_creative.imp_revenue);
          unpack_decimal(src_debug_creative.click_revenue(), dst_debug_creative.click_revenue);
          unpack_decimal(src_debug_creative.action_revenue(), dst_debug_creative.action_revenue);
          unpack_decimal(src_debug_creative.ecpm_bid(), dst_debug_creative.ecpm_bid);
          dst_debug_creative.action_adv_url = src_debug_creative.action_adv_url();
          dst_debug_creative.html_url = src_debug_creative.html_url();
          dst_debug_creative.triggered_expression =
            src_debug_creative.triggered_expression();
          dst_debug_creative.full_expression = src_debug_creative.full_expression();
        }
        dst_debug.trace_ccg = src_debug.trace_ccg();

        unpack_tokens(src.native_data_tokens(), dst.native_data_tokens);
        dst.native_image_tokens.length(src.native_image_tokens_size());
        for(int j = 0; j < src.native_image_tokens_size(); ++j)
        {
          const auto& src_token = src.native_image_tokens(j);
          auto& dst_token = dst.native_image_tokens[j];
          dst_token.name = src_token.name();
          dst_token.value = src_token.value();
          dst_token.width = src_token.width();
          dst_token.height = src_token.height();
        }
        dst.track_html_body = src.track_html_body();
        dst.erid = src.erid();
        dst.contracts.length(src.contracts_size());
        for(int j = 0; j < src.contracts_size(); ++j)
        {
          const auto& src_contract = src.contracts(j);
          auto& dst_contract = dst.contracts[j];
          auto& dst_contract_info = dst_contract.contract_info;
          dst_contract_info.contract_id = src_contract.contract_id();
          dst_contract_info.number = src_contract.number();
          dst_contract_info.date = src_contract.date();
          dst_contract_info.type = src_contract.type();
          dst_contract_info.vat_included = src_contract.vat_included();
          dst_contract_info.ord_contract_id = src_contract.ord_contract_id();
          dst_contract_info.ord_ado_id = src_contract.ord_ado_id();
          dst_contract_info.subject_type = src_contract.subject_type();
          dst_contract_info.action_type = src_contract.action_type();
          dst_contract_info.agent_acting_for_publisher =
            src_contract.agent_acting_for_publisher();
          dst_contract_info.parent_contract_id =
            src_contract.contract_parent_contract_id();
          dst_contract_info.client_id = src_contract.client_id();
          dst_contract_info.client_name = src_contract.client_name();
          dst_contract_info.client_legal_form =
            src_contract.client_legal_form();
          dst_contract_info.contractor_id = src_contract.contractor_id();
          dst_contract_info.contractor_name = src_contract.contractor_name();
          dst_contract_info.contractor_legal_form =
            src_contract.contractor_legal_form();
          unpack_oct_seq(src_contract.timestamp(), dst_contract_info.timestamp);
          dst.contracts[j].parent_contract_id =
            src_contract.parent_contract_id();
        }
      }

      unpack_oct_seq(source.process_time(), target.process_time);
      target.debug_info.colo_id = source.debug_info().colo_id();
      unpack_ids(source.debug_info().geo_channels(), target.debug_info.geo_channels);
      unpack_ids(
        source.debug_info().platform_channels(),
        target.debug_info.platform_channels);
      target.debug_info.last_platform_channel_id =
        source.debug_info().last_platform_channel_id();
      target.debug_info.user_group_id = source.debug_info().user_group_id();
    }

    CampaignManager::RequestCreativeResult
    get_campaign_creative(
      AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient& campaign_manager,
      const CampaignManager::RequestParams& request_params,
      std::string& hostname)
    {
      PB::GetCampaignCreativeRequest request;
      pack_request_params(request_params, *request.mutable_request_params());

      auto response = AdServer::Grpc::sync_call<PB::GetCampaignCreativeResponse>(
        [&](auto callback)
        {
          campaign_manager.get_campaign_creative(request, std::move(callback));
        },
        [](const grpc::Status& status)
        {
          Stream::Error ostr;
          ostr << "CampaignManager::get_campaign_creative(): "
            "gRPC call failed: code=" <<
            static_cast<int>(status.error_code()) <<
            ", message=" << status.error_message();
          return Frontend::Exception(ostr);
        });

      hostname = response.hostname();
      CampaignManager::RequestCreativeResult result;
      unpack_request_creative_result(response.request_result(), result);
      return result;
    }

    class TimeGuard
    {
    public:
      TimeGuard() noexcept
      {
        timer_.start();
      }

      Generics::Time consider() noexcept
      {
        timer_.stop();
        return timer_.elapsed_time();
      }

    private:
      Generics::Timer timer_;
    };

    void
    throw_user_bind_exception_(const grpc::Status& status)
    {
      const std::string message = status.error_message();
      switch(status.error_code())
      {
      case grpc::StatusCode::UNAVAILABLE:
        throw AdServer::UserInfoSvcs::UserBindClient::NotReady(
          message.c_str());
      case grpc::StatusCode::NOT_FOUND:
        throw AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound(
          message.c_str());
      default:
        throw AdServer::UserInfoSvcs::UserBindClient::ImplementationException(
          message.c_str());
      }
    }

    AdServer::Bidding::CampaignManager::ChannelTriggerMatchInfo
    convert_channel_atom(
      const adserver::channel_svcs::channel_server::ChannelAtom& atom)
      noexcept
    {
      AdServer::Bidding::CampaignManager::ChannelTriggerMatchInfo out;
      out.channel_id = atom.id();
      out.channel_trigger_id = atom.trigger_channel_id();
      return out;
    }

    struct ChannelMatch
    {
      ChannelMatch(
        unsigned long channel_id_val,
        unsigned long channel_trigger_id_val)
        : channel_id(channel_id_val),
          channel_trigger_id(channel_trigger_id_val)
      {}

      bool operator<(const ChannelMatch& right) const
      {
        return
          (channel_id < right.channel_id ||
           (channel_id == right.channel_id &&
            channel_trigger_id < right.channel_trigger_id));
      }

      unsigned long channel_id;
      unsigned long channel_trigger_id;
    };

    struct GetChannelTriggerId
    {
      ChannelMatch
      operator() (
        const adserver::channel_svcs::channel_server::ChannelAtom& atom)
        noexcept
      {
        return ChannelMatch(atom.id(), atom.trigger_channel_id());
      }
    };

    struct ContextualChannelConverter
    {
      const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight&
      operator()(const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight)
        const
      {
        return ch_weight;
      }

      AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight
      operator()(
        const adserver::channel_svcs::channel_server::ContentChannelAtom&
          contextual_channel)
        const
      {
        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight res;
        res.channel_id = contextual_channel.id();
        res.weight = contextual_channel.weight();
        return res;
      }
    };

    Commons::Interval<Generics::Time>
    construct_time_interval(const String::SubString& str)
      /*throw(eh::Exception)*/
    {
      const std::size_t pos = str.find('-');

      if (pos == String::SubString::NPOS)
      {
        Stream::Error ostr;
        ostr << "Separator not found in '" << str << "'";
        throw Frontend::Exception(ostr);
      }

      const Generics::Time min_val = Generics::Time(str.substr(0, pos), "%H:%M");
      const Generics::Time max_val = Generics::Time(str.substr(pos + 1), "%H:%M");
      return Commons::Interval<Generics::Time>(min_val, max_val);
    }

  }

  class Frontend::UpdateConfigTask: public Generics::TaskGoal
  {
  public:
    UpdateConfigTask(
      Frontend* bid_frontend,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        bid_frontend_(bid_frontend)
    {}

    virtual void
    execute() noexcept
    {
      bid_frontend_->update_config_();
    }

  private:
    Frontend* bid_frontend_;
  };

  class Frontend::FlushStateTask: public Generics::TaskGoal
  {
  public:
    FlushStateTask(
      Frontend* bid_frontend,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        bid_frontend_(bid_frontend)
    {}

    virtual void
    execute() noexcept
    {
      bid_frontend_->flush_state_();
    }

  private:
    Frontend* bid_frontend_;
  };

  class BidRequestInterruptGoal:
    public Generics::Goal,
    public ReferenceCounting::AtomicImpl
  {
  public:
    BidRequestInterruptGoal(BidRequestTask_var bid_request_task)
      : bid_request_task_(std::move(bid_request_task))
    {}

    virtual void
    deliver() /*throw(eh::Exception)*/
    {
      bid_request_task_->interrupt();
    }

  protected:
    BidRequestTask_var bid_request_task_;
  };

  //
  // Frontend::InterruptPassbackTask
  //
  class Frontend::InterruptPassbackTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    InterruptPassbackTask(
      Frontend* frontend,
      std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
        campaign_manager,
      const RequestParamsHolder* request_params,
      const std::string& hostname)
      /*throw(eh::Exception)*/
      : frontend_(frontend),
        campaign_manager_(std::move(campaign_manager)),
        request_params_var_(ReferenceCounting::add_ref(request_params)),
        hostname_(hostname)
    {}

    virtual void
    execute() noexcept
    {
      try
      {
        frontend_->passback_task_count_ += -1;

        std::string hostname = hostname_;
        (void)get_campaign_creative(
          *campaign_manager_,
          *request_params_var_,
          hostname);
      }
      catch(const eh::Exception&)
      {
        // Skip all CM exceptions
      }
    }

  protected:
    virtual ~InterruptPassbackTask() noexcept {}

  private:
    Frontend* frontend_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    const ConstRequestParamsHolder_var request_params_var_;
    std::string hostname_;
  };

  //
  // Frontend implementation
  //
  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    StatHolder* stats,
    Generics::CompositeMetricsProvider* composite_metrics_provider) /*throw(eh::Exception)*/
    : GroupLogger(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().BidFeConfiguration()->Logger().log_level())),
        "Bidding::Frontend",
        Aspect::BIDDING_FRONTEND,
        0),
      /*
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        frontend_config->get().BidFeConfiguration()->threads(),
        frontend_config->get().BidFeConfiguration()->max_pending_tasks()),
      */
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      colo_id_(0),
      campaign_manager_(),
      stats_(ReferenceCounting::add_ref(stats)),
      bid_task_count_(0),
      passback_task_count_(0),
      reached_max_pending_tasks_(0),
      composite_metrics_provider_(ReferenceCounting::add_ref(composite_metrics_provider))
  {
    char hostname[256];
    if(gethostname(hostname, sizeof(hostname)) == 0)
    {
      hostname[sizeof(hostname) - 1] = 0;
      server_id_ = hostname;
    }
  }

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = false;

    if (!uri.empty())
    {
      result =
        FrontendCommons::find_uri(
          config_->GoogleUriList().Uri(), uri, found_uri) ||
        FrontendCommons::find_uri(
          config_->OpenRtbUriList().Uri(), uri, found_uri) ||
        (config_->AdXmlUriList().present() &&
         FrontendCommons::find_uri(
           config_->AdXmlUriList()->Uri(), uri, found_uri)) ||
        (config_->ClickStarUriList().present() &&
         FrontendCommons::find_uri(
           config_->ClickStarUriList()->Uri(), uri, found_uri)) ||
        (config_->DAOUriList().present() &&
         FrontendCommons::find_uri(
           config_->DAOUriList()->Uri(), uri, found_uri))
        ;
    }

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "Bidding::Frontend::will_handle(" << uri <<
        "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void Frontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "Bidding::Frontend::parse_configs_()";

    /* load common configuration */

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration isn't present");
      }

      common_config_.reset(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      colo_id_ = common_config_->colo_id();

      if(!fe_config.BidFeConfiguration().present())
      {
        throw Exception("BidFeConfiguration isn't present");
      }

      config_.reset(
        new BiddingFeConfiguration(*fe_config.BidFeConfiguration()));

      fill_account_traits_();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "': " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "Bidding::Frontend::init()";

    if(true) // module_used()
    {
      try
      {
        parse_configs_();

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        planner_ = new Generics::Planner(callback());
        add_child_object(planner_);

        task_runner_ = new Generics::TaskPool(
          callback(),
          config_->threads(),
          1024*1024 // stack size
          );
        add_child_object(task_runner_);

        control_task_runner_ = new Generics::TaskRunner(callback(), 4);
        add_child_object(control_task_runner_);

        planner_pool_ = new PlannerPool(this->callback(), 16);
        add_child_object(planner_pool_);

        // ADSC-10554
        // Interrupted requests queue
        passback_task_runner_ = new Generics::TaskPool(
          callback(),
          config_->interrupted_threads(), // threads
          0 // stack_size
          );
        add_child_object(passback_task_runner_);

        Generics::Planner_var task_scheduler(new Generics::Planner(callback()));
        add_child_object(task_scheduler);

        // FlushLoggerTask
        Generics::Time flush_period(config_->flush_period().present() ? *config_->flush_period() : 10);
        Commons::make_goal_task(
          std::bind(
            &Commons::MessagePacker<CellsKey, MessageOut>::dump,
            group_logger(), Logging::Logger::ERROR, "", ""),
          control_task_runner_,
          task_scheduler,
          flush_period)->schedule(flush_period);

        AdServer::UserInfoSvcs::UserInfoCorbaClient::ControllerRefList
          user_info_controller_groups;
        for(const auto& controller_group :
            common_config_->UserInfoManagerControllerGroup())
        {
          AdServer::UserInfoSvcs::UserInfoCorbaClient::ControllerRef
            controller_group_refs;
          Config::CorbaConfigReader::read_multi_corba_ref(
            controller_group,
            controller_group_refs);
          user_info_controller_groups.push_back(controller_group_refs);
        }
        auto user_info_client = std::make_shared<AdServer::UserInfoSvcs::UserInfoCorbaClient>(
          logger(),
          user_info_controller_groups,
          corba_client_adapter_.in());
        user_info_client_ = user_info_client;
        add_child_object(user_info_client);

        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        auto campaign_manager_client =
          std::make_shared<
            AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
              FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
              AdServer::Grpc::BatchingOptions(),
              grpc_executor_);
        campaign_manager_ = campaign_manager_client;
        add_child_object(campaign_manager_client);

        auto user_bind_objects =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            logger());
        if(user_bind_objects.client)
        {
          user_bind_client_ = user_bind_objects.client;
          add_child_object(user_bind_objects.active_object);
        }

        auto channel_client_objects =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_);
        channel_client_ = channel_client_objects.client;
        add_child_object(channel_client_objects.active_object);

        for(BiddingFeConfiguration::Source_sequence::const_iterator
              it = config_->Source().begin();
            it != config_->Source().end(); ++it)
        {
          SourceTraits source_traits;
          if (it->default_account_id().present())
          {
            source_traits.default_account_id = *(it->default_account_id());
          }
          source_traits.instantiate_type = AdServer::CampaignSvcs::AIT_BODY;

          // banner notice : disable notice if notice_url defined
          source_traits.notice_instantiate_type = SourceTraits::NIT_NONE;
          std::string notice_instantiate_type = it->notice();
          if(notice_instantiate_type == "nurl")
          {
            source_traits.notice_instantiate_type = SourceTraits::NIT_NURL;
          }
          else if(notice_instantiate_type == "burl")
          {
            source_traits.notice_instantiate_type = SourceTraits::NIT_BURL;
          }
          else if(notice_instantiate_type == "nurl and burl")
          {
            source_traits.notice_instantiate_type = SourceTraits::NIT_NURL_AND_BURL;
          }

          source_traits.vast_instantiate_type =
            AdServer::CampaignSvcs::AIT_BODY;

          // vast notice
          source_traits.vast_notice_instantiate_type = SourceTraits::NIT_NONE;
          std::string vast_notice_instantiate_type = it->vast_notice();
          if(vast_notice_instantiate_type == "nurl")
          {
            source_traits.vast_notice_instantiate_type = SourceTraits::NIT_NURL;
          }
          else if(vast_notice_instantiate_type == "burl")
          {
            source_traits.vast_notice_instantiate_type = SourceTraits::NIT_BURL;
          }

          // native notice
          source_traits.native_notice_instantiate_type = SourceTraits::NIT_NONE;
          std::string native_notice_instantiate_type = it->native_notice();
          if(native_notice_instantiate_type == "nurl")
          {
            source_traits.native_notice_instantiate_type = SourceTraits::NIT_NURL;
          }
          else if(native_notice_instantiate_type == "burl")
          {
            source_traits.native_notice_instantiate_type = SourceTraits::NIT_BURL;
          }
          else if(native_notice_instantiate_type == "nurl and burl")
          {
            source_traits.native_notice_instantiate_type = SourceTraits::NIT_NURL_AND_BURL;
          }

          source_traits.ipw_extension = it->ipw_extension();
          source_traits.truncate_domain = it->truncate_domain();
          source_traits.fill_adid = it->fill_adid();
          if(it->seat().present())
          {
            source_traits.seat = *(it->seat());
          }

          if(it->request_type().present())
          {
            std::string type = *(it->request_type());
            if(type == "openrtb")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
            }
            else if(type == "openrtb with click url")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_OPENRTB_WITH_CLICKURL;
            }
            else if(type == "openx")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_OPENX;
            }
            else if(type == "liverail")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_LIVERAIL;
            }
            else if(type == "adriver")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_ADRIVER;
            }
            else if(type == "yandex")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_YANDEX;
            }
          }

          source_traits.instantiate_type = adapt_instantiate_type_(
            it->instantiate_type());
          source_traits.vast_instantiate_type = adapt_instantiate_type_(
            it->vast_instantiate_type());
          source_traits.native_ads_instantiate_type = adapt_native_ads_instantiate_type_(
            it->native_instantiate_type());
          if (it->native_impression_tracker_type().present())
          {
            source_traits.native_ads_impression_tracker_type =
              adapt_native_ads_impression_tracker_type_(
                *it->native_impression_tracker_type());
          }

          source_traits.erid_return_type = adapt_erid_return_type_(
            it->erid_return_type());

          if(it->max_bid_time().present())
          {
            source_traits.max_bid_time = Generics::Time(*(it->max_bid_time()));
            (*source_traits.max_bid_time) /= 1000;
          }

          source_traits.skip_ext_category = it->skip_ext_category();
          if(it->notice_url().present())
          {
            source_traits.notice_url = *(it->notice_url());
          }

          sources_.insert(std::make_pair(it->id(), source_traits));
        }

        {
          const String::SubString intervalsBlacklist = config_->intervalsBlacklist();
          String::StringManip::SplitNL tokenizer(intervalsBlacklist);

          for (String::SubString interval; tokenizer.get_token(interval);)
          {
            blacklisted_time_intervals_.insert(construct_time_interval(interval.str()));
          }
        }

        RequestInfoFiller::ExternalUserIdSet skip_external_ids;

        if (common_config_->SkipExternalIds().present())
        {
          for(CommonFeConfiguration::SkipExternalIds_type::Id_sequence::const_iterator
                it = common_config_->SkipExternalIds()->Id().begin();
              it != common_config_->SkipExternalIds()->Id().end(); ++it)
          {
            skip_external_ids.insert(it->value());
          }

          String::SubString skip_ids =
            common_config_->SkipExternalIds()->skip_external_ids();

          if (!skip_ids.empty())
          {
            String::StringManip::SplitNL tokenizer(skip_ids);
            for (String::SubString skip_id; tokenizer.get_token(skip_id);)
            {
              skip_external_ids.insert(skip_id.str());
            }
          }
        }

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_config_->colo_id(),
            common_module_.in(),
            common_config_->GeoIP().present() ?
              common_config_->GeoIP()->path().c_str() : 0,
            "", //user_agent_filter_path.c_str()
            skip_external_ids,
            common_config_->ip_logging_enabled(),
            common_config_->ip_salt().c_str(),
            sources_,
            config_->enable_profile_referer(),
            account_traits_));

        if(config_->request_timeout().present())
        {
          request_timeout_ = Generics::Time(*(config_->request_timeout()));
          request_timeout_ /= 1000;
        }

        activate_object();

        control_task_runner_->enqueue_task(
          Generics::Task_var(new UpdateConfigTask(this, control_task_runner_)));

        control_task_runner_->enqueue_task(
          Generics::Task_var(new FlushStateTask(this, control_task_runner_)));
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "Bidding::Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::BIDDING_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
      clear();

      Stream::Error ostr;
      ostr << "Bidding::Frontend::shutdown(): frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::BIDDING_FRONTEND);

      common_module_->shutdown();
    }
    catch(...)
    {}
  }

  adserver::user_info_svcs::user_bind::GetUserIdResponse
  Frontend::get_user_id_(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request)
  {
    return AdServer::Grpc::sync_call<
      adserver::user_info_svcs::user_bind::GetUserIdResponse>(
        [&](auto callback)
        {
          user_bind_client_->get_user_id(
            request,
            std::move(callback));
        },
        [](const grpc::Status& status)
        {
          throw_user_bind_exception_(status);
        });
  }

  adserver::user_info_svcs::user_bind::AddUserIdResponse
  Frontend::add_user_id_(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request)
  {
    return AdServer::Grpc::sync_call<
      adserver::user_info_svcs::user_bind::AddUserIdResponse>(
        [&](auto callback)
        {
          user_bind_client_->add_user_id(
            request,
            std::move(callback));
        },
        [](const grpc::Status& status)
        {
          throw_user_bind_exception_(status);
        });
  }

  Generics::Time
  Frontend::get_request_timeout_(const FCGI::HttpRequest& request) noexcept
  {
    const HTTP::ParamList& params = request.params();

    for (auto it = params.begin(); it != params.end(); ++it)
    {
      if (it->name == Request::Context::SOURCE_ID)
      {
        const auto source_it = sources_.find(it->value);

        if (source_it != sources_.end() && source_it->second.max_bid_time.present())
        {
          return *(source_it->second.max_bid_time);
        }
      }
    }

    return request_timeout_;
  }

  void
  Frontend::handle_request(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::handle_request_()";

    // create task - push it to task runner
    // and push goal for timeout control
    //
    Generics::Time start_process_time = Generics::Time::get_time_of_day();
    BidRequestTask_var request_task;

    try
    {
      const FCGI::HttpRequest& request = request_holder->request();

      const Generics::Time expire_time(
        start_process_time + get_request_timeout_(request));

      std::string found_uri;

      if(FrontendCommons::find_uri(
        config_->GoogleUriList().Uri(), request.uri(), found_uri))
      {
        // Google request
        request_task = new GoogleBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if(config_->AdXmlUriList().present() &&
        FrontendCommons::find_uri(
          config_->AdXmlUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new AdXmlBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if(config_->ClickStarUriList().present() &&
        FrontendCommons::find_uri(
          config_->ClickStarUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new ClickStarBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if(config_->DAOUriList().present() &&
        FrontendCommons::find_uri(
          config_->DAOUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new AdJsonBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else
      {
        // OpenRTB request
        request_task = new OpenRtbBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }

      unsigned long cur_task_count = bid_task_count_.exchange_and_add(1) + 1;

      if(cur_task_count > config_->max_pending_tasks() + config_->threads())
      {
        bid_task_count_ += -1;

        {
          MaxPendingSyncPolicy::WriteGuard lock(reached_max_pending_tasks_lock_);
          reached_max_pending_tasks_ = std::max(
            reached_max_pending_tasks_, cur_task_count);
        }

        if (stats_.in())
        {
          stats_->add_skipped();
        }

        request_task->write_empty_response(0);
      }
      else
      {
        // delegate response writing to task & schedule timeouter
        Generics::Goal_var interrupt_goal(new BidRequestInterruptGoal(request_task));
        planner_pool_->schedule(interrupt_goal, expire_time);
        task_runner_->enqueue_task(request_task);
      }
    }
    catch(const BidRequestTask::Invalid& e)
    {
      // HTTP_BAD_REQUEST
      if(request_task)
      {
        request_task->write_empty_response(400);
      }
      else
      {
        response_writer->write(
          400,
          FCGI::HttpResponse_var(new FCGI::HttpResponse()));
      }

      Stream::Error ostr;
      ostr << FUN << ": BidRequestTask::Invalid caught: " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::BIDDING_FRONTEND);
    }
    catch(const eh::Exception& e)
    {
      if(request_task)
      {
        request_task->write_empty_response(503);
      }
      else
      {
        response_writer->write(
          503,
          FCGI::HttpResponse_var(new FCGI::HttpResponse()));
      }

      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-109");
    }
  }

  void
  Frontend::resolve_user_id_(
    AdServer::Commons::UserId& match_user_id,
    AdServer::Bidding::CampaignManager::CommonAdRequestInfo& common_info,
    RequestInfo& request_info,
    DebugSink::UserResolvingDebugInfo* user_resolving_debug_info)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::resolve_user_id_()";

    Generics::Time start_process_time;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    if(request_info.filter_request)
    {
      common_info.user_status = static_cast<CORBA::ULong>(
        AdServer::CampaignSvcs::US_FOREIGN);
    }
    else if(!request_info.advertising_id.empty() ||
      !request_info.idfa.empty() ||
      common_info.external_user_id[0])
    {
      if(user_bind_client_)
      {
        // external user ids (first will be used as base)
        std::vector<std::string> external_user_ids;

        // fill by stable ids (ext uids)
        for(auto ext_user_id = request_info.ext_user_ids.begin();
          ext_user_id != request_info.ext_user_ids.end(); ++ext_user_id)
        {
          external_user_ids.emplace_back(*ext_user_id);
        }

        if(!request_info.idfa.empty())
        {
          std::string resolve_idfa = request_info.idfa;
          String::AsciiStringManip::to_lower(resolve_idfa);
          external_user_ids.push_back(std::string("ifa/") + resolve_idfa);
        }

        if(!request_info.advertising_id.empty())
        {
          std::string resolve_idfa = request_info.advertising_id;
          String::AsciiStringManip::to_lower(resolve_idfa);
          external_user_ids.push_back(std::string("ifa/") + resolve_idfa);
        }

        if(common_info.external_user_id[0])
        {
          external_user_ids.push_back(common_info.external_user_id.in());
        }

        assert(!external_user_ids.empty());

        try
        {

          auto base_ext_user_id_it = external_user_ids.begin();

          AdServer::Commons::UserId local_match_user_id;
          adserver::user_info_svcs::user_bind::GetUserIdResponse user_bind_info;

          bool blacklisted = false;
          bool min_age_reached = false;

          for(auto ext_user_id_it = external_user_ids.begin();
            ext_user_id_it != external_user_ids.end();
            ++ext_user_id_it)
          {
            adserver::user_info_svcs::user_bind::GetUserIdRequest
              get_request_info;
            get_request_info.set_id(*ext_user_id_it);
            get_request_info.set_timestamp(
              GrpcAlgs::pack_time(request_info.current_time));
            get_request_info.set_silent(false);
            get_request_info.set_generate_user_id(false);
            get_request_info.set_for_set_cookie(false);
            get_request_info.set_create_timestamp(
              GrpcAlgs::pack_time(request_info.user_create_time));
            // get_request_info.current_user_id is null

            user_bind_info = get_user_id_(get_request_info);

            min_age_reached |= user_bind_info.min_age_reached();
            local_match_user_id =
              GrpcAlgs::unpack_user_id(user_bind_info.user_id());
            if(user_resolving_debug_info)
            {
              user_resolving_debug_info->response_present = true;
              user_resolving_debug_info->user_id =
                local_match_user_id.is_null() ?
                  std::string() : local_match_user_id.to_string();
              user_resolving_debug_info->min_age_reached =
                user_bind_info.min_age_reached();
              user_resolving_debug_info->created =
                user_bind_info.created();
              user_resolving_debug_info->invalid_operation =
                user_bind_info.invalid_operation();
              user_resolving_debug_info->user_found =
                user_bind_info.user_found();
            }

            blacklisted |= common_module_->user_id_controller()->null_blacklisted(match_user_id);

            if(!local_match_user_id.is_null())
            {
              common_info.external_user_id << *ext_user_id_it;
              base_ext_user_id_it = ext_user_id_it;
              break;
            }
            else if(common_info.external_user_id[0] == 0)
            {
              common_info.external_user_id << *ext_user_id_it;
            }
          }

          match_user_id = local_match_user_id;

          common_module_->user_id_controller()->null_blacklisted(match_user_id);

          if (!match_user_id.is_null())
          { //(external_user_id is not found and user_id is not null,
            //  in other words: min_age_reached=true && bind_on_min_age=true -> user_id generated)
            //or
            //(external_user_id is found and user_id is not null)
            // link other external user ids to base
            for(auto ext_user_id_it = external_user_ids.begin();
              ext_user_id_it != external_user_ids.end();
              ++ext_user_id_it)
            {
              if(ext_user_id_it != base_ext_user_id_it)
              {
                adserver::user_info_svcs::user_bind::AddUserIdRequest
                  add_user_request;
                add_user_request.set_id(*ext_user_id_it);
                add_user_request.set_timestamp(
                  GrpcAlgs::pack_time(request_info.current_time));
                add_user_request.set_user_id(GrpcAlgs::pack_user_id(match_user_id));
                auto prev_user_bind_info = add_user_id_(add_user_request);

                (void)prev_user_bind_info;
              }
            }

            common_info.user_status = static_cast<CORBA::ULong>(
              AdServer::CampaignSvcs::US_OPTIN);
          }
          else if (blacklisted)
          {
            common_info.user_status = static_cast<CORBA::ULong>(
              AdServer::CampaignSvcs::US_UNDEFINED);
          }
          else if (user_bind_info.user_found())
          { //external_user_id is found and user_id is null
            common_info.user_status = static_cast<CORBA::ULong>(
              AdServer::CampaignSvcs::US_OPTOUT);
          }
          //external_user_id is not found and user_id is null
          else if(min_age_reached)
          {
            // uid generation on RTB requests disabled (bind_on_min_age=false)
            common_info.user_status = static_cast<CORBA::ULong>(
              AdServer::CampaignSvcs::US_UNDEFINED);
          }
          else
          {
            common_info.user_status = static_cast<CORBA::ULong>(
              AdServer::CampaignSvcs::US_EXTERNALPROBE);
          }
        }
        catch(const AdServer::UserInfoSvcs::UserBindClient::NotReady& )
        {
          Stream::Error ostr;
            ostr << FUN << ": caught UserBindClient::NotReady";

          logger()->log(ostr.str(),
            Logging::Logger::WARNING,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-10681");
        }
        catch(const AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound& )
        {
          Stream::Error ostr;
            ostr << FUN << ": caught UserBindClient::ChunkNotFound";

          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-10681");
        }
        catch(const AdServer::UserInfoSvcs::UserBindClient::ImplementationException& ex)
        {
          Stream::Error ostr;
            ostr << FUN << ": caught UserBindClient::ImplementationException: " <<
            ex.what();

          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-10681");
        }
        catch(const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA::SystemException: " << e;
          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::BIDDING_FRONTEND,
            "ADS-ICON-7800");
        }
      }
    }
    else if(common_info.user_status != AdServer::CampaignSvcs::US_PROBE)
    {
      common_info.user_status = static_cast<CORBA::ULong>(
        AdServer::CampaignSvcs::US_NOEXTERNALID);
    }

    if(common_info.user_status != AdServer::CampaignSvcs::US_OPTIN)
    {
      // US_FOREIGN already filtered - filter US_NOEXTERNALID fully
      // and disable ccg keywords loading for non opt in (US_EXTERNALPROBE actually)
      // if colocation configured to passback for non opt-in
      ExtConfig_var ext_config = get_ext_config_();

      if(ext_config.in())
      {
        ExtConfig::ColocationMap::const_iterator colo_it =
          ext_config->colocations.find(common_info.colo_id);

        if(colo_it != ext_config->colocations.end() &&
           (colo_it->second.flags == CampaignSvcs::CS_NONE ||
            colo_it->second.flags == CampaignSvcs::CS_ONLY_OPTIN))
        {
          if(colo_it->second.flags == CampaignSvcs::CS_NONE ||
            common_info.user_status == AdServer::CampaignSvcs::US_NOEXTERNALID)
          {
            request_info.filter_request = true;
          }
          request_info.skip_ccg_keywords = true;
        }
      }
    }

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FUN << ": user is resolving time = " <<
        (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }

    if (config_->trace_mapping() &&
        logger()->log_level() >= Logging::Logger::DEBUG)
    {
      Stream::Error ostr;
      ostr << FUN << ": SSP user mapping: " << match_user_id.to_string() <<
        " <-> (" << common_info.external_user_id << ", " <<
        request_info.source_id << ')';
      logger()->log(ostr.str(),
        Logging::Logger::DEBUG,
        Aspect::BIDDING_FRONTEND);
    }
  }

  void
  Frontend::trigger_match_(
    adserver::channel_svcs::channel_server::MatchResponse&
      trigger_matched_channels,
    bool& trigger_matched_channels_present,
    AdServer::Bidding::CampaignManager::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& user_id,
    std::string& /*hostname*/,
    const char* keywords)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::trigger_match_()";

    if(!request_info.filter_request)
    {
      try
      {
        adserver::channel_svcs::channel_server::MatchRequest channel_request;
        channel_request.set_non_strict_word_match(false);
        channel_request.set_non_strict_url_match(false);
        channel_request.set_return_negative(false);
        channel_request.set_simplify_page(true);
        channel_request.set_fill_content(true);
        channel_request.set_statuses("A", 2);
        channel_request.set_first_url(request_params.common_info.referer.in());
        try
        {
          std::string ref_words;
          FrontendCommons::extract_url_keywords(
            ref_words,
            String::SubString(request_params.common_info.referer),
            common_module_->segmentor());

          if (!ref_words.empty())
          {
            channel_request.set_first_url_words(ref_words);
          }
        }
        catch (const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": url keywords extracting error: " << e.what();
          logger()->log(ostr.str(),
            Logging::Logger::TRACE,
            Aspect::BIDDING_FRONTEND);
        }

        // check multiline
        {
          std::string urls_str;
          std::string urls_words_str;

          for(CORBA::ULong i = 0; i < request_params.common_info.urls.length(); ++i)
          {
            if(i != 0)
            {
              urls_str += '\n';
            }
            urls_str += request_params.common_info.urls[i];

            std::string url_words_res;
            FrontendCommons::extract_url_keywords(
              url_words_res,
              String::SubString(request_params.common_info.urls[i]),
              common_module_->segmentor());

            if (!url_words_res.empty())
            {
              if(!urls_words_str.empty())
              {
                urls_words_str += '\n';
              }
              urls_words_str += url_words_res;
            }
          }

          channel_request.set_urls(urls_str);
          if(!urls_words_str.empty())
          {
            channel_request.set_urls_words(urls_words_str);
          }
        }

        if(keywords)
        {
          channel_request.set_pwords(keywords);
        }
        channel_request.set_swords(request_info.search_words);
        channel_request.set_uid(GrpcAlgs::pack_user_id(user_id));

        trigger_matched_channels =
          AdServer::ChannelSvcs::GrpcAlgs::channel_match(
            *channel_client_,
            channel_request);
        trigger_matched_channels_present = true;

        const auto& matched_channels =
          trigger_matched_channels.matched_channels();

        request_params.trigger_match_result.pkw_channels.length(
          matched_channels.page_channels_size());
        std::transform(
          matched_channels.page_channels().begin(),
          matched_channels.page_channels().end(),
          request_params.trigger_match_result.pkw_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.url_channels.length(
          matched_channels.url_channels_size());
        std::transform(
          matched_channels.url_channels().begin(),
          matched_channels.url_channels().end(),
          request_params.trigger_match_result.url_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.ukw_channels.length(
          matched_channels.url_keyword_channels_size());
        std::transform(
          matched_channels.url_keyword_channels().begin(),
          matched_channels.url_keyword_channels().end(),
          request_params.trigger_match_result.ukw_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.skw_channels.length(
          matched_channels.search_channels_size());
        std::transform(
          matched_channels.search_channels().begin(),
          matched_channels.search_channels().end(),
          request_params.trigger_match_result.skw_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.uid_channels.length(
          matched_channels.uid_channels_size());
        for(int i = 0; i < matched_channels.uid_channels_size(); ++i)
        {
          request_params.trigger_match_result.uid_channels[i] =
            matched_channels.uid_channels(i);
        }

        if(request_params.common_info.user_status ==
           static_cast<CORBA::ULong>(AdServer::CampaignSvcs::US_OPTIN) &&
           (trigger_matched_channels.no_track() ||
            trigger_matched_channels.no_adv()))
        {
          request_params.common_info.user_status = static_cast<CORBA::ULong>(
            AdServer::CampaignSvcs::US_BLACKLISTED);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught ChannelServerGrpcAsyncClient error: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-117");
      }
    }
  }

  void
  Frontend::history_match_(
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out history_match_result,
    AdServer::Bidding::CampaignManager::RequestParams& request_params,
    const RequestInfo& request_info,
    const adserver::channel_svcs::channel_server::MatchResponse* trigger_match_result,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time,
    std::string& /*hostname*/)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::history_match_()";

    typedef std::set<ChannelMatch> ChannelMatchSet;

    Generics::Time start_process_time;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    request_params.profiling_available = false;

    if(!user_id.is_null())
    {
      if(user_info_client_)
      {
        try
        {
          adserver::user_info_svcs::user_info_manager::MatchRequest
            history_match_request;

          auto* user_info = history_match_request.mutable_user_info();
          user_info->set_user_id(GrpcAlgs::pack_user_id(user_id));
          user_info->set_huser_id(
            GrpcAlgs::pack_user_id(AdServer::Commons::UserId()));
          user_info->set_last_colo_id(colo_id_);
          user_info->set_request_colo_id(colo_id_);
          user_info->set_current_colo_id(-1);
          user_info->set_temporary(false);
          user_info->set_time(time.tv_sec);

          auto* match_params = history_match_request.mutable_match_params();
          match_params->set_use_empty_profile(false);
          match_params->set_silent_match(false);
          match_params->set_no_match(
            trigger_match_result && trigger_match_result->no_track());
          match_params->set_no_result(false);
          match_params->set_ret_freq_caps(true);
          match_params->set_provide_channel_count(false);
          match_params->set_provide_persistent_channels(false);
          match_params->set_change_last_request(true);
          match_params->set_publishers_optin_timeout(GrpcAlgs::pack_time(
            time - Generics::Time::ONE_DAY * 15));
          match_params->set_cohort(
            !request_info.idfa.empty() ?
            request_info.idfa : request_info.advertising_id);

          for(CORBA::ULong i = 0;
            i < request_params.context_info.platform_ids.length(); ++i)
          {
            match_params->add_persistent_channel_ids(
              request_params.context_info.platform_ids[i]);
          }

          if(trigger_match_result && !trigger_match_result->no_track())
          {
            const auto& matched_channels =
              trigger_match_result->matched_channels();
            ChannelMatchSet page_channels;
            ChannelMatchSet url_channels;
            ChannelMatchSet search_channels;
            ChannelMatchSet url_keyword_channels;

            std::transform(
              matched_channels.page_channels().begin(),
              matched_channels.page_channels().end(),
              std::inserter(page_channels, page_channels.end()),
              GetChannelTriggerId());

            std::transform(
              matched_channels.url_channels().begin(),
              matched_channels.url_channels().end(),
              std::inserter(url_channels, url_channels.end()),
              GetChannelTriggerId());

            std::transform(
              matched_channels.search_channels().begin(),
              matched_channels.search_channels().end(),
              std::inserter(search_channels, search_channels.end()),
              GetChannelTriggerId());

            std::transform(
              matched_channels.url_keyword_channels().begin(),
              matched_channels.url_keyword_channels().end(),
              std::inserter(url_keyword_channels, url_keyword_channels.end()),
              GetChannelTriggerId());

            const auto fill_channel_matches =
              [](
                auto* out,
                const ChannelMatchSet& in)
            {
              for(const auto& channel_match : in)
              {
                auto* result = out->Add();
                result->set_channel_id(channel_match.channel_id);
                result->set_channel_trigger_id(
                  channel_match.channel_trigger_id);
              }
            };
            fill_channel_matches(
              match_params->mutable_page_channel_ids(),
              page_channels);
            fill_channel_matches(
              match_params->mutable_url_channel_ids(),
              url_channels);
            fill_channel_matches(
              match_params->mutable_search_channel_ids(),
              search_channels);
            fill_channel_matches(
              match_params->mutable_url_keyword_channel_ids(),
              url_keyword_channels);
          }

          history_match_result =
            AdServer::UserInfoSvcs::GrpcAlgs::history_match(
              *user_info_client_,
              history_match_request);

          request_params.profiling_available = true;
        }
        catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
            e.description;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-112");
        }
        catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
        {
          logger()->log(
            String::SubString("UserInfoManager not ready for matching."),
            TraceLevel::MIDDLE,
            Aspect::BIDDING_FRONTEND);
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't match history channels. Caught CORBA::SystemException: " <<
            ex;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-ICON-2");
        }
      }
    }
    else if(trigger_match_result && !trigger_match_result->no_track())
    {
      // fill history channels with context channels
      history_match_result = get_empty_history_matching_();

      AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
        history_matched_channels = history_match_result->channels;

      history_matched_channels.length(trigger_match_result->content_channels_size());
      for(int i = 0; i < trigger_match_result->content_channels_size(); ++i)
      {
        history_matched_channels[i].channel_id =
          trigger_match_result->content_channels(i).id();
        history_matched_channels[i].weight =
          trigger_match_result->content_channels(i).weight();
      }
    }

    if(!history_match_result.ptr())
    {
      history_match_result = get_empty_history_matching_();

      if(trigger_match_result)
      {
        /* fill history channels with context channels */
        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
          history_matched_channels = history_match_result->channels;

        history_matched_channels.length(trigger_match_result->content_channels_size());

        std::transform(
          trigger_match_result->content_channels().begin(),
          trigger_match_result->content_channels().end(),
          history_matched_channels.get_buffer(),
          ContextualChannelConverter());
      }
    }

    // resolve ip to colo_id
    FrontendCommons::IPMatcher_var ip_matcher =
      common_module_->ip_matcher();

    try
    {
      FrontendCommons::IPMatcher::MatchResult ip_match_result;
      if(ip_matcher.in() &&
         request_params.common_info.peer_ip[0] &&
         ip_matcher->match(
           ip_match_result,
           String::SubString(request_params.common_info.peer_ip),
           String::SubString()))
      {
        request_params.common_info.colo_id = ip_match_result.colo_id;
        request_params.context_info.profile_referer =
           config_->enable_profile_referer() && ip_match_result.profile_referer;
      }
    }
    catch(const FrontendCommons::IPMatcher::InvalidParameter&)
    {}

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FUN << ": history matching time = " <<
        (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }
  }

  void
  Frontend::get_ccg_keywords_(
    AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var& ccg_keywords,
    const RequestInfo& request_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult& history_match_result)
    noexcept
  {
    static const char* FUN = "Frontend::get_ccg_keywords_()";

    try
    {
      if(history_match_result.channels.length() &&
         !request_info.skip_ccg_keywords &&
         !request_info.filter_request)
      {
        AdServer::ChannelSvcs::ChannelIdSeq channel_ids;
        channel_ids.length(history_match_result.channels.length());

        for (CORBA::ULong i = 0;
          i < history_match_result.channels.length(); ++i)
        {
          channel_ids[i] = history_match_result.channels[i].channel_id;
        }

        adserver::channel_svcs::channel_server::GetCcgTraitsRequest
          channel_request;
        AdServer::ChannelSvcs::GrpcAlgs::make_get_ccg_traits_request(
          channel_ids,
          channel_request);
        auto channel_response = AdServer::Grpc::sync_call<
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse>(
            [&](auto callback)
            {
              channel_client_->get_ccg_traits(
                channel_request,
                std::move(callback));
            },
            [](const grpc::Status& status)
            {
              Stream::Error ostr;
              ostr << "ChannelServer grpc get_ccg_traits failed: code=" <<
                static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
              return Exception(ostr);
            });
        ccg_keywords = AdServer::ChannelSvcs::GrpcAlgs::make_ccg_traits_result(
          channel_response);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": caught ChannelServerGrpcAsyncClient error: " <<
        ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-117");
    }
  }

  bool
  Frontend::consider_campaign_selection_(
    const AdServer::Commons::UserId& user_id, // not null
    const Generics::Time& now,
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result,
    std::string& /*hostname*/)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::consider_campaign_selection_()";

    Generics::Time start_process_time;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    bool result = false;
    if (user_id.is_null())
    {
      return true;
    }
    else
    {
      CORBA::ULong seq_order_len = 0;

      for(CORBA::ULong ad_slot_i = 0;
          ad_slot_i < campaign_match_result.ad_slots.length(); ++ad_slot_i)
      {
        const AdServer::Bidding::CampaignManager::
          AdSlotResult& ad_slot = campaign_match_result.ad_slots[ad_slot_i];

        for(CORBA::ULong i = 0;
            i < ad_slot.selected_creatives.length(); ++i)
        {
          if(ad_slot.selected_creatives[i].order_set_id)
          {
            ++seq_order_len;
          }
        }
      }

      try
      {

        if(!user_info_client_)
        {
          logger()->log(
            String::SubString("Bidding::Frontend::user_info_post_match_():"
              " non resolved user info session."),
            Logging::Logger::TRACE,
            Aspect::BIDDING_FRONTEND);

          return false;
        }

        UserInfoSvcs::UserInfoManager::SeqOrderSeq seq_orders;
        seq_orders.length(seq_order_len);
        CORBA::ULong result_seq_order_i = 0;

        for(CORBA::ULong ad_slot_i = 0;
            ad_slot_i < campaign_match_result.ad_slots.length();
            ++ad_slot_i)
        {
          const AdServer::Bidding::CampaignManager::
            AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

          if(ad_slot_result.selected_creatives.length() > 0)
          {
            UserInfoSvcs::CampaignIdSeq campaign_ids;
            campaign_ids.length(ad_slot_result.selected_creatives.length());

            for(CORBA::ULong creative_i = 0;
              creative_i < ad_slot_result.selected_creatives.length();
              ++creative_i)
            {
              const AdServer::Bidding::CampaignManager::
                CreativeSelectResult& creative =
                  ad_slot_result.selected_creatives[creative_i];

              if(creative.order_set_id)
              {

                seq_orders[result_seq_order_i].ccg_id = creative.cmp_id;
                seq_orders[result_seq_order_i].set_id = creative.order_set_id;
                seq_orders[result_seq_order_i].imps = 1;
		////qwerty
		BidStatisticsPrometheusInc(composite_metrics_provider_,seq_orders[result_seq_order_i].ccg_id);
                ++result_seq_order_i;
              }

              campaign_ids[creative_i] = creative.campaign_group_id;
            }

            UserInfoSvcs::FreqCapIdSeq freq_caps;
            UserInfoSvcs::FreqCapIdSeq uc_freq_caps;

            CorbaAlgs::copy_sequence(ad_slot_result.freq_caps, freq_caps);
            CorbaAlgs::copy_sequence(ad_slot_result.uc_freq_caps, uc_freq_caps);

            AdServer::UserInfoSvcs::GrpcAlgs::update_user_freq_caps(*user_info_client_,
              CorbaAlgs::pack_user_id(user_id),
              CorbaAlgs::pack_time(now),
              ad_slot_result.request_id,
              freq_caps,
              uc_freq_caps,
              UserInfoSvcs::FreqCapIdSeq(), // virtual_freq_caps
              seq_orders,
              ad_slot_result.track_impr ? EMPTY_CAMPAIGN_ID_SEQ : campaign_ids,
              ad_slot_result.track_impr ? campaign_ids : EMPTY_CAMPAIGN_ID_SEQ);
          } // ad_slot_result.selected_creatives.length() > 0
        }

        result = true;
      }
      catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
          e.description;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-112");
      }
      catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
      {
        logger()->log(
          String::SubString("UserInfoManager not ready for post match."),
          TraceLevel::MIDDLE,
          Aspect::BIDDING_FRONTEND);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't do post match. Caught CORBA::SystemException: " <<
          ex;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::BIDDING_FRONTEND,
          "ADS-ICON-2");
      }
    }

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FUN << ": campaign selection considering = " <<
        (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }

    return result;
  }

  bool
  Frontend::process_bid_request_(
    const char* fn,
    AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result,
    AdServer::Commons::UserId& user_id,
    BidRequestTask* request_task,
    RequestInfo& request_info,
    const std::string& keywords)
    noexcept
  {
    bool interrupted = false;

    AdServer::Bidding::CampaignManager::RequestParams&
      request_params(*request_task->request_params());
    RequestTimeMetering& request_time_metering =
      request_task->request_time_metering_;

    // map external id to uid
    DebugSink::UserResolvingDebugInfo user_resolving_debug_info;
    {
      request_task->set_current_stage(Stage::UserResolving);
      request_time_metering.user_resolving_started_at =
        Generics::Time::get_time_of_day() - request_task->start_processing_time();
      TimeGuard user_resolving_time_metering;
      resolve_user_id_(
        user_id,
        request_params.common_info,
        request_info,
        &user_resolving_debug_info);
      request_time_metering.user_resolving_time =
        user_resolving_time_metering.consider();
    }

    request_task->debug_sink_.print_request_debug_info(
      request_info,
      request_params,
      user_id,
      keywords);
    request_task->debug_sink_.print_user_resolving_debug_info(
      user_resolving_debug_info);

    if(check_interrupt_(fn, Stage::UserResolving, request_task))
    {
      interrupted = true;
    }

    adserver::channel_svcs::channel_server::MatchResponse trigger_match_result;
    bool trigger_match_result_present = false;

    if (!interrupted)
    {
      {
        request_task->set_current_stage(Stage::TriggerMatching);
        request_time_metering.trigger_match_started_at =
          Generics::Time::get_time_of_day() - request_task->start_processing_time();
        TimeGuard trigger_match_time_metering;
        trigger_match_(
          trigger_match_result,
          trigger_match_result_present,
          request_params,
          request_info,
          user_id,
          request_task->hostname_,
          keywords.c_str());
        request_time_metering.trigger_match_time =
          trigger_match_time_metering.consider();
      }

      if(trigger_match_result_present)
      {
        request_task->debug_sink_.print_channel_matching_debug_info(
          trigger_match_result);
      }

      if(check_interrupt_(fn, Stage::TriggerMatching, request_task))
      {
        interrupted = true;
      }
    }

    // process bid request source independently
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var
      history_match_result;

    if (!interrupted)
    {
      {
        request_task->set_current_stage(Stage::HistoryMatching);
        request_time_metering.history_match_started_at =
          Generics::Time::get_time_of_day() - request_task->start_processing_time();
        TimeGuard history_match_time_metering;
        history_match_(
          history_match_result.out(),
          request_params,
          request_info,
          trigger_match_result_present ? &trigger_match_result : nullptr,
          user_id,
          request_info.current_time,
          request_task->hostname_);
        request_time_metering.history_match_time =
          history_match_time_metering.consider();
      }

      if(history_match_result)
      {
        request_time_metering.history_match_local_time =
          CorbaAlgs::unpack_time(history_match_result->process_time);
        request_task->debug_sink_.print_history_matching_debug_info(
          *history_match_result);
      }

      if(check_interrupt_(fn, Stage::HistoryMatching, request_task))
      {
        interrupted = true;
      }
    }
    else
    {
      history_match_result = get_empty_history_matching_();
    }

    AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var ccg_keywords;

    if(interrupted)
    {
      return false;
    }

    request_task->set_current_stage(Stage::CampaignSelection);

    get_ccg_keywords_(
      ccg_keywords,
      request_info,
      *history_match_result);

    {
      request_time_metering.creative_selection_started_at =
        Generics::Time::get_time_of_day() - request_task->start_processing_time();
      TimeGuard creative_selection_time_metering;
      select_campaign_(
        campaign_match_result,
        *history_match_result,
        trigger_match_result_present ? &trigger_match_result : nullptr,
        ccg_keywords.ptr(),
        request_info,
        request_params,
        user_id,
        (trigger_match_result_present &&
          (trigger_match_result.no_track() || trigger_match_result.no_adv())) ||
        request_info.filter_request,
        request_task->hostname_,
        interrupted);
      request_time_metering.creative_selection_time =
        creative_selection_time_metering.consider();
    }

    if(campaign_match_result.ad_slots.length())
    {
      request_time_metering.creative_selection_local_time =
        CorbaAlgs::unpack_time(campaign_match_result.process_time);
    }

    if (!interrupted)
    {
      if(check_interrupt_(fn, Stage::CampaignSelection, request_task))
      {
        return false;
      }

      if(campaign_match_result.ad_slots.length() > 0 &&
        campaign_match_result.ad_slots[0].debug_info.trace_ccg[0] &&
        request_params.ad_slots.length() > 0 &&
        logger()->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream ostr;
        ostr << fn << ": CCG Trace of " <<
        request_params.ad_slots[0].debug_ccg <<
          " for request:" << std::endl;

        request_task->print_request(ostr);

        ostr << std::endl << campaign_match_result.ad_slots[0].debug_info.trace_ccg;
        std::cout << ostr.str() << std::endl;

        logger()->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::BIDDING_FRONTEND);
      }
    }
    else
    {
      // Interrupted CM selection should be the last call of the RequestTask,
      // RequestTask's request params are invalid after the call.
      interrupted_select_campaign_(
        request_task);

      return false;
    }

    return true;
  }

  void
  Frontend::interrupted_select_campaign_(
    BidRequestTask* request_task) noexcept
  {
    static const char* FUN = "Bidding::Frontend::interrupted_select_campaign_()";

    try
    {
      ConstRequestParamsHolder_var
        request_params(request_task->request_params());

      //request_task->request_params.reset();

      unsigned long cur_task_count = passback_task_count_.exchange_and_add(1) + 1;

      if(cur_task_count > config_->interrupted_max_pending_tasks() + config_->interrupted_threads())
      {
        passback_task_count_ += -1;
      }
      else
      {
        try
        {
          passback_task_runner_->enqueue_task(
            Generics::Task_var(
              new InterruptPassbackTask(
                this,
                campaign_manager_,
                request_params,
                request_task->hostname())));
        }
        catch(...)
        {
          passback_task_count_ += -1;
          throw;
        }
      }
    }
    catch(const Generics::TaskRunner::Overflow&)
    {
      // Skip all TaskRunner overflow errors
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-10554");
    }
  }

  void
  Frontend::select_campaign_(
      AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult&
        history_match_result,
    const adserver::channel_svcs::channel_server::MatchResponse* trigger_match_result,
    const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* ccg_keywords,
    const RequestInfo& request_info,
    AdServer::Bidding::CampaignManager::RequestParams& request_params,
    const AdServer::Commons::UserId& user_id,
    bool passback,
    std::string& hostname,
    bool interrupted)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::request_campaign_manager_()";

    Generics::Time start_process_time;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    // do campaign selection
    try
    {
      // Fill user info
      if(!user_id.is_null())
      {
        request_params.common_info.user_id = CorbaAlgs::pack_user_id(user_id);
        request_params.common_info.track_user_id =
          request_params.common_info.user_id;
      }

      // Fill debug-info & process fraud
      request_params.only_display_ad = false;
      if(request_params.ad_slots.length() > 0)
      {
        if (!interrupted)
        {
          request_params.need_debug_info = DebugSink::require_debug_info(
            String::SubString(request_info.require_debug_info));
          request_params.ad_slots[0].debug_ccg = request_info.debug_ccg;
        }
        for(size_t i = 0; i < request_params.ad_slots.length(); ++i)
        {
          request_params.ad_slots[i].passback |=
            passback || interrupted ||
              history_match_result.fraud_request;
        }
      }

      // Fill user history data
      request_params.client_create_time = history_match_result.create_time;
      request_params.session_start = history_match_result.session_start;
      request_params.full_freq_caps.swap(
        history_match_result.full_freq_caps);
      request_params.exclude_pubpixel_accounts.swap(
        history_match_result.exclude_pubpixel_accounts);
      request_params.campaign_freqs.swap(
        history_match_result.campaign_freqs);

      request_params.seq_orders.length(history_match_result.seq_orders.length());
      for(CORBA::ULong seq_order_i = 0;
          seq_order_i != history_match_result.seq_orders.length();
          ++seq_order_i)
      {
        request_params.seq_orders[seq_order_i].ccg_id =
          history_match_result.seq_orders[seq_order_i].ccg_id;
        request_params.seq_orders[seq_order_i].set_id =
          history_match_result.seq_orders[seq_order_i].set_id;
        request_params.seq_orders[seq_order_i].imps =
          history_match_result.seq_orders[seq_order_i].imps;
      }

      request_params.common_info.coord_location.length(
        history_match_result.geo_data_seq.length());
      for(CORBA::ULong i = 0;
          i < history_match_result.geo_data_seq.length(); ++i)
      {
        AdServer::Bidding::CampaignManager::GeoCoordInfo& res_loc =
          request_params.common_info.coord_location[i];
        res_loc.longitude = history_match_result.geo_data_seq[i].longitude;
        res_loc.latitude = history_match_result.geo_data_seq[i].latitude;
        res_loc.accuracy = history_match_result.geo_data_seq[i].accuracy;
      }

      /* fill input channel sequence for CampaignManager */
      std::size_t uid_channels_length = 0;

      if (trigger_match_result)
      {
        uid_channels_length =
          trigger_match_result->matched_channels().uid_channels_size();
      }

      request_params.channels.length(
        history_match_result.channels.length() +
          uid_channels_length);

      CORBA::ULong j = 0;
      for (CORBA::ULong i = 0; i < history_match_result.channels.length();
           ++i, ++j)
      {
          request_params.channels[j] = history_match_result.channels[i].channel_id;
      }

      if (trigger_match_result)
      {
        const auto& uid_channels =
          trigger_match_result->matched_channels().uid_channels();
        for (int i = 0; i < uid_channels.size(); ++i, ++j)
        {
          request_params.channels[j] = uid_channels[i];
        }
      }

      // Fill CCG keywords
      if(ccg_keywords)
      {
        request_params.ccg_keywords.length(ccg_keywords->length());
        for(CORBA::ULong i = 0; i < ccg_keywords->length(); ++i)
        {
          const AdServer::ChannelSvcs::ChannelServerBase::CCGKeyword&
            src_ccg_kw = (*ccg_keywords)[i];
          AdServer::Bidding::CampaignManager::CCGKeywordInfo&
            res_ccg_kw = request_params.ccg_keywords[i];
          res_ccg_kw.ccg_keyword_id = src_ccg_kw.ccg_keyword_id;
          res_ccg_kw.ccg_id = src_ccg_kw.ccg_id;
          res_ccg_kw.channel_id = src_ccg_kw.channel_id;
          res_ccg_kw.max_cpc = src_ccg_kw.max_cpc;
          res_ccg_kw.ctr = src_ccg_kw.ctr;
          res_ccg_kw.click_url = src_ccg_kw.click_url;
          res_ccg_kw.original_keyword = src_ccg_kw.original_keyword;
        }
      }

      // Process black list users
      const Generics::Time day_time =
        request_info.current_time - request_info.current_time.get_gm_time().get_date();

      if (blacklisted_time_intervals_.contains(day_time))
      {
        if (request_params.common_info.user_status ==
            static_cast<CORBA::ULong>(CampaignSvcs::US_OPTIN))
        {
          request_params.common_info.user_status =
            static_cast<CORBA::ULong>(CampaignSvcs::US_BLACKLISTED);
        }

        for(CORBA::ULong slot_i = 0;
            slot_i < request_params.ad_slots.length(); ++slot_i)
        {
          request_params.ad_slots[slot_i].passback = true;
        }
      }

      {
        // fill special tokens
        if(!request_info.bid_request_id.empty())
        {
          add_token(request_params.common_info.tokens,
            "BR_ID", request_info.bid_request_id);
        }

        if(!request_info.bid_site_id.empty())
        {
          add_token(request_params.common_info.tokens,
            "BS_ID", request_info.bid_site_id);
        }

        if(!request_info.bid_publisher_id.empty())
        {
          add_token(request_params.common_info.tokens,
            "BP_ID", request_info.bid_publisher_id);
        }

        if(!request_info.application_id.empty())
        {
          add_token(request_params.common_info.tokens,
            "APPLICATION_ID", request_info.application_id);
        }

        if(!request_info.idfa.empty())
        {
          add_token(request_params.common_info.tokens,
            "IDFA", request_info.idfa);
        }
        else if(!request_info.advertising_id.empty())
        {
          add_token(request_params.common_info.tokens,
            "ADVERTISING_ID", request_info.advertising_id);
        }
        else if(history_match_result.cohort[0])
        {
          if(request_info.platform_names.find("ipad") !=
              request_info.platform_names.end() ||
            request_info.platform_names.find("iphone") !=
              request_info.platform_names.end() ||
            request_info.platform_names.find("ios") !=
              request_info.platform_names.end())
          {
            add_token(request_params.common_info.tokens,
              "IDFA", std::string(history_match_result.cohort));
          }
          else
          {
            add_token(request_params.common_info.tokens,
              "ADVERTISING_ID", std::string(history_match_result.cohort));
          }
        }

        if(request_params.common_info.ext_track_params[0])
        {
          add_token(request_params.common_info.tokens,
            "EXT_TRACK_PARAMS", std::string(request_params.common_info.ext_track_params));
        }

        if(request_info.location)
        {
          add_token(request_params.common_info.tokens,
            "GEO_REGION",
            request_info.location->region);
        }

        if(!request_info.ssp_devicetype_str.empty())
        {
          add_token(request_params.common_info.tokens,
            "SSP_DEVICETYPE",
            request_info.ssp_devicetype_str);
        }
      }

      if (interrupted)
      {
        request_params.required_passback = false;
      }
      else
      {
        campaign_match_result = get_campaign_creative(
          *campaign_manager_,
          request_params,
          hostname);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-118");
    }

    if(!interrupted && logger()->log_level() >= Logging::Logger::TRACE)
    {
      Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FUN << ": campaign selection time = " <<
        (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }
  }

  void
  Frontend::update_config_()
    noexcept
  {
    static const char* FUN = "Frontend::update_config_()";

    try
    {
      auto colocation_response =
        AdServer::Grpc::sync_call<PB::GetColocationFlagsResponse>(
          [&](auto callback)
          {
            campaign_manager_->get_colocation_flags(
              PB::GetColocationFlagsRequest(),
              std::move(callback));
          },
          [](const grpc::Status& status)
          {
            Stream::Error ostr;
            ostr << "CampaignManager::get_colocation_flags(): "
              "gRPC call failed: code=" <<
              static_cast<int>(status.error_code()) <<
              ", message=" << status.error_message();
            return Exception(ostr);
          });

      ExtConfig_var new_config(new ExtConfig());

      for (const auto& colocation_info : colocation_response.colocations())
      {
        ExtConfig::Colocation colocation;
        colocation.flags = colocation_info.flags();
        new_config->colocations.insert(
          ExtConfig::ColocationMap::value_type(
            colocation_info.colo_id(),
            colocation));
      }

      set_ext_config_(new_config);
    }
    catch (const eh::Exception& e)
    {
      logger()->sstream(Logging::Logger::CRITICAL,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-118") << FUN << ": Can't update colocation flags, "
        "caught eh::Exception: " << e.what();
    }

    try
    {
      planner_->schedule(
        Generics::Goal_var(new UpdateConfigTask(this, control_task_runner_)),
        Generics::Time::get_time_of_day() + common_config_->update_period());
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7605") <<
        FUN << ": schedule failed: " << ex.what();
    }
  }

  void
  Frontend::flush_state_()
    noexcept
  {
    static const char* FUN = "Frontend::flush_state_()";

    unsigned long reached_max_pending_tasks;

    {
      MaxPendingSyncPolicy::WriteGuard lock(reached_max_pending_tasks_lock_);
      reached_max_pending_tasks = reached_max_pending_tasks_;
      reached_max_pending_tasks_ = 0;
    }

    if(reached_max_pending_tasks > 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": reached max pending tasks: " <<
        reached_max_pending_tasks;

      logger()->log(ostr.str(),
        Logging::Logger::WARNING,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7602");
    }

    try
    {
      planner_->schedule(
        Generics::Goal_var(new FlushStateTask(this, control_task_runner_)),
        Generics::Time::get_time_of_day() + (
          config_->flush_period().present() ? *config_->flush_period() : 10));
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7605") <<
        FUN << ": schedule failed: " << ex.what();
    }
  }

  AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
  Frontend::get_empty_history_matching_()
    /*throw(eh::Exception)*/
  {
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var res =
      new AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult();
    res->fraud_request = false;
    res->times_inited = false;
    res->last_request_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->create_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->session_start = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->process_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->colo_id = -1;
    return res._retn();
  }

  void
  Frontend::fill_account_traits_() noexcept
  {
    for(auto account_it = config_->Account().begin();
      account_it != config_->Account().end(); ++account_it)
    {
      RequestInfoFiller::AccountTraits_var& target_account_ptr =
        account_traits_[account_it->account_id()];

      if(!target_account_ptr.in())
      {
        target_account_ptr = new RequestInfoFiller::AccountTraits();
      }

      RequestInfoFiller::AccountTraits& target_account = *target_account_ptr;

      if(account_it->max_cpm_value().present())
      {
        CampaignSvcs::RevenueDecimal limit = CampaignSvcs::RevenueDecimal::mul(
          AdServer::Commons::extract_decimal<CampaignSvcs::RevenueDecimal>(
            account_it->max_cpm_value().get()),
          MAX_CPM_CONF_MULTIPLIER,
          Generics::DMR_FLOOR);

        target_account.max_cpm = limit;
      }

      if(account_it->display_billing_id().present())
      {
        target_account.display_billing_id = *(account_it->display_billing_id());
      }

      if(account_it->video_billing_id().present())
      {
        target_account.video_billing_id = *(account_it->video_billing_id());
      }

      if(account_it->google_encryption_key().present())
      {
        target_account.google_encryption_key_size = String::StringManip::hex_decode(
          *(account_it->google_encryption_key()),
          target_account.google_encryption_key);
      }

      if(account_it->google_integrity_key().present())
      {
        target_account.google_integrity_key_size = String::StringManip::hex_decode(
          *(account_it->google_integrity_key()),
          target_account.google_integrity_key);
      }
    }
  }

  void
  Frontend::limit_max_cpm_(
    AdServer::CampaignSvcs::RevenueDecimal& val,
    const AdServer::Bidding::CampaignManager::IdSeq& account_ids)
    const noexcept
  {
    for(CORBA::ULong i = 0; i < account_ids.length(); ++i)
    {
      unsigned long account_id = account_ids[i];
      auto account_it = account_traits_.find(account_id);
      if(account_it != account_traits_.end() && account_it->second->max_cpm.present())
      {
        val = std::min(val, *(account_it->second->max_cpm));
      }
    }
  }

  inline void
  Frontend::interrupt_(
    const char* fun,
    const Stage stage,
    const BidRequestTask* request_task)
    noexcept
  {
    Generics::Time timeout = Generics::Time::get_time_of_day() - request_task->start_processing_time();
    if (stats_.in())
    {
      stats_->add_timeout(timeout);
    }

    if (logger()->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << fun << ": request processing timed out(" << timeout << "):"
        << std::endl;

      request_task->print_request(ostr);

      logger()->log(
        ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }

    std::string ostr(fun);
    ostr += ": interrupted at ";
    ostr += convert_stage_to_string(stage).str();
    ostr += ", after";

    group_logger()->add_error(
      !request_task->hostname().empty() ?
        request_task->hostname().c_str() : "Undefined host",
      ostr,
      timeout,
      Logging::Logger::ERROR,
      Aspect::BIDDING_FRONTEND,
      "ADS-IMPL-7600");
  }

  bool
  Frontend::check_interrupt_(
    const char* fun,
    const Stage stage,
    BidRequestTask* request_task)
    noexcept
  {
    if(request_task->interrupted())
    {
      interrupt_(fun, stage, request_task);
      return true;
    }

    return false;
  }

  AdServer::CampaignSvcs::AdInstantiateType
  Frontend::adapt_instantiate_type_(const std::string& inst_type_str)
    /*throw(Exception)*/
  {
    if(inst_type_str == "url")
    {
      return AdServer::CampaignSvcs::AIT_URL;
    }
    else if(inst_type_str == "nonsecure url")
    {
      return AdServer::CampaignSvcs::AIT_NONSECURE_URL;
    }
    else if(inst_type_str == "url in body")
    {
      return AdServer::CampaignSvcs::AIT_URL_IN_BODY;
    }
    else if(inst_type_str == "video url")
    {
      return AdServer::CampaignSvcs::AIT_VIDEO_URL;
    }
    else if(inst_type_str == "nonsecure video url")
    {
      return AdServer::CampaignSvcs::AIT_VIDEO_NONSECURE_URL;
    }
    else if(inst_type_str == "video url in body")
    {
      return AdServer::CampaignSvcs::AIT_VIDEO_URL_IN_BODY;
    }
    else if(inst_type_str == "body")
    {
      return AdServer::CampaignSvcs::AIT_BODY;
    }
    else if(inst_type_str == "script with url")
    {
      return AdServer::CampaignSvcs::AIT_SCRIPT_WITH_URL;
    }
    else if(inst_type_str == "iframe with url")
    {
      return AdServer::CampaignSvcs::AIT_IFRAME_WITH_URL;
    }
    else if(inst_type_str == "url parameters")
    {
      return AdServer::CampaignSvcs::AIT_URL_PARAMS;
    }
    else if(inst_type_str == "encoded url parameters")
    {
      return AdServer::CampaignSvcs::AIT_DATA_URL_PARAM;
    }
    else if(inst_type_str == "data parameter value")
    {
      return AdServer::CampaignSvcs::AIT_DATA_PARAM_VALUE;
    }

    Stream::Error ostr;
    ostr << "unknown instantiate type '" << inst_type_str << "'";
    throw Exception(ostr);
  }

  SourceTraits::NativeAdsInstantiateType
  Frontend::adapt_native_ads_instantiate_type_(
    const std::string& inst_type_str)
    /*throw(Exception)*/
  {
    if(inst_type_str == "none")
    {
      return SourceTraits::NAIT_NONE;
    }
    else if(inst_type_str == "adm")
    {
      return SourceTraits::NAIT_ADM;
    }
    else if(inst_type_str == "adm_native")
    {
      return SourceTraits::NAIT_ADM_NATIVE;
    }
    else if(inst_type_str == "ext")
    {
      return SourceTraits::NAIT_EXT;
    }
    else if(inst_type_str == "escape_slash_adm")
    {
      return SourceTraits::NAIT_ESCAPE_SLASH_ADM;
    }
    else if(inst_type_str == "native_as_element-1.2")
    {
      return SourceTraits::NAIT_NATIVE_AS_ELEMENT_1_2;
    }
    else if(inst_type_str == "adm-1.2")
    {
      return SourceTraits::NAIT_ADM_1_2;
    }
    else if(inst_type_str == "adm_native-1.2")
    {
      return SourceTraits::NAIT_ADM_NATIVE_1_2;
    }

    Stream::Error ostr;
    ostr << "unknown native ads instantiate type '" << inst_type_str << "'";
    throw Exception(ostr);
  }

  SourceTraits::ERIDReturnType
  Frontend::adapt_erid_return_type_(
    const std::string& erid_type_str)
  {
    if(erid_type_str == "single")
    {
      return SourceTraits::ERIDRT_SINGLE;
    }
    else if(erid_type_str == "array")
    {
      return SourceTraits::ERIDRT_ARRAY;
    }
    else if(erid_type_str == "ext0")
    {
      return SourceTraits::ERIDRT_EXT0;
    }
    else if(erid_type_str == "buzsape")
    {
      return SourceTraits::ERIDRT_EXT_BUZSAPE;
    }

    Stream::Error ostr;
    ostr << "unknown erid return type '" << erid_type_str << "'";
    throw Exception(ostr);
  }

  AdServer::CampaignSvcs::NativeAdsImpressionTrackerType
  Frontend::adapt_native_ads_impression_tracker_type_(
    const std::string& imp_type_str)
    /*throw(Exception)*/
  {
    if(imp_type_str == "imp")
    {
      return AdServer::CampaignSvcs::NAITT_IMP;
    }

    if(imp_type_str == "js")
    {
      return AdServer::CampaignSvcs::NAITT_JS;
    }

    if(imp_type_str == "resources")
    {
      return AdServer::CampaignSvcs::NAITT_RESOURCES;
    }

    Stream::Error ostr;
    ostr << "unknown native ads impression tracker type '" <<
      imp_type_str << "'";
    throw Exception(ostr);
  }
}
