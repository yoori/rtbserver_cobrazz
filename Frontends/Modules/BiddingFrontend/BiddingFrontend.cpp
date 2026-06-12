#include <sstream>
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <zlib.h>
#include <unistd.h>

#include <google/protobuf/arena.h>

#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include "Generics/CompositeMetricsProvider.hpp"

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Commons/ExternalUserIdUtils.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/BidStatisticsPrometheus.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "OpenRtbBidRequestState.hpp"
#include "GoogleBidRequestState.hpp"
#include "AdXmlBidRequestState.hpp"
#include "ClickStarBidRequestState.hpp"
#include "AdJsonBidRequestState.hpp"
#include "DAOBidRequestState.hpp"
#include "GrpcClientMetricsProvider.hpp"
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

namespace Response::Header
{
    const String::SubString CONTENT_TYPE("Content-Type");
  }

namespace AdServer::Bidding
{
  namespace
  {
    namespace PB = adserver::campaign_svcs::campaign_manager;

    const CampaignSvcs::RevenueDecimal MAX_CPM_CONF_MULTIPLIER(false, 100, 0);

    class InProgressGuard
    {
    public:
      using Method = void (StatHolder::*)() noexcept;
      using TimeMethod = void (StatHolder::*)(const Generics::Time&) noexcept;

      InProgressGuard(
        StatHolder* stats,
        Method add,
        Method complete,
        TimeMethod add_time = nullptr) noexcept
        : stats_(ReferenceCounting::add_ref(stats)),
          complete_(stats ? complete : nullptr),
          add_time_(stats ? add_time : nullptr),
          started_at_(add_time_ ? Generics::Time::get_time_of_day() :
            Generics::Time::ZERO)
      {
        if(stats_.in())
        {
          (stats_.in()->*add)();
        }
      }

      ~InProgressGuard() noexcept
      {
        reset();
      }

      InProgressGuard(const InProgressGuard&) = delete;
      InProgressGuard& operator=(const InProgressGuard&) = delete;

      void
      reset() noexcept
      {
        if(stats_.in() && complete_)
        {
          (stats_.in()->*complete_)();
          if(add_time_)
          {
            (stats_.in()->*add_time_)(
              Generics::Time::get_time_of_day() - started_at_);
          }
          complete_ = nullptr;
          add_time_ = nullptr;
        }
      }

    private:
      StatHolder_var stats_;
      Method complete_;
      TimeMethod add_time_;
      Generics::Time started_at_;
    };

    template<typename ByteSeq>
    std::string pack_oct_seq(const ByteSeq& seq)
    {
      return std::string(
        reinterpret_cast<const char*>(seq.data()),
        reinterpret_cast<const char*>(seq.data()) + seq.size());
    }

    template<typename Type>
    std::shared_ptr<Type>
    make_shared_reference(Type* ptr)
    {
      if(ptr)
      {
        ptr->add_ref();
      }

      return std::shared_ptr<Type>(
        ptr,
        [](Type* value)
        {
          if(value)
          {
            value->remove_ref();
          }
        });
    }

    template<typename ByteSeq>
    void unpack_oct_seq(const std::string& value, ByteSeq& target)
    {
      target.resize(value.size());
      std::copy(value.begin(), value.end(), target.data());
    }

    template<typename SourceSeq>
    void pack_ids(
      const SourceSeq& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      for(std::size_t i = 0; i < source.size(); ++i)
      {
        target->Add(source[i]);
      }
    }

    template<typename SourceSeq>
    void pack_strings(
      const SourceSeq& source,
      google::protobuf::RepeatedPtrField<std::string>* target)
    {
      for(std::size_t i = 0; i < source.size(); ++i)
      {
        *target->Add() = source[i];
      }
    }

    template<typename TargetSeq>
    void unpack_ids(
      const google::protobuf::RepeatedField<google::protobuf::uint64>& source,
      TargetSeq& target)
    {
      target.resize(source.size());
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
      target.resize(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i] = source[i];
      }
    }

    void pack_tokens(
      const CampaignManager::TokenSeq& source,
      google::protobuf::RepeatedPtrField<PB::TokenInfo>* target)
    {
      for(std::size_t i = 0; i < source.size(); ++i)
      {
        auto* token = target->Add();
        token->set_name(source[i].name);
        token->set_value(source[i].value);
      }
    }

    void unpack_tokens(
      const google::protobuf::RepeatedPtrField<PB::TokenInfo>& source,
      CampaignManager::TokenSeq& target)
    {
      target.resize(source.size());
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
      target.set_creative_instantiate_type(source.creative_instantiate_type);
      target.set_request_type(source.request_type);
      target.set_random(source.random);
      target.set_test_request(source.test_request);
      target.set_log_as_test(source.log_as_test);
      target.set_colo_id(source.colo_id);
      target.set_external_user_id(source.external_user_id);
      target.set_source_id(source.source_id);
      for(std::size_t i = 0; i < source.location.size(); ++i)
      {
        auto* location = target.add_location();
        location->set_country(source.location[i].country);
        location->set_region(source.location[i].region);
        location->set_city(source.location[i].city);
      }
      for(std::size_t i = 0; i < source.coord_location.size(); ++i)
      {
        auto* coord = target.add_coord_location();
        coord->set_longitude(pack_oct_seq(source.coord_location[i].longitude));
        coord->set_latitude(pack_oct_seq(source.coord_location[i].latitude));
        coord->set_accuracy(pack_oct_seq(source.coord_location[i].accuracy));
      }
      target.set_full_referer(source.full_referer);
      target.set_referer(source.referer);
      pack_strings(source.urls, target.mutable_urls());
      target.set_security_token(source.security_token);
      target.set_pub_impr_track_url(source.pub_impr_track_url);
      target.set_pub_param(source.pub_param);
      target.set_preclick_url(source.preclick_url);
      target.set_click_prefix_url(source.click_prefix_url);
      target.set_original_url(source.original_url);
      target.set_track_user_id(pack_oct_seq(source.track_user_id));
      target.set_user_id(pack_oct_seq(source.user_id));
      target.set_user_status(source.user_status);
      target.set_peer_ip(source.peer_ip);
      target.set_user_agent(source.user_agent);
      target.set_cohort(source.cohort);
      target.set_hpos(source.hpos);
      target.set_ext_track_params(source.ext_track_params);
      pack_tokens(source.tokens, target.mutable_tokens());
      target.set_set_cookie(source.set_cookie);
      target.set_passback_type(source.passback_type);
      target.set_passback_url(source.passback_url);
    }

    void pack_context_ad_request_info(
      const CampaignManager::ContextAdRequestInfo& source,
      PB::ContextAdRequestInfo& target)
    {
      target.set_enabled_notice(source.enabled_notice);
      target.set_client(source.client);
      target.set_client_version(source.client_version);
      pack_ids(source.platform_ids, target.mutable_platform_ids());
      pack_ids(source.geo_channels, target.mutable_geo_channels());
      target.set_platform(source.platform);
      target.set_full_platform(source.full_platform);
      target.set_web_browser(source.web_browser);
      target.set_ip_hash(source.ip_hash);
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
      for(std::size_t i = 0; i < source.seq_orders.size(); ++i)
      {
        auto* seq_order = target.add_seq_orders();
        seq_order->set_ccg_id(source.seq_orders[i].ccg_id);
        seq_order->set_set_id(source.seq_orders[i].set_id);
        seq_order->set_imps(source.seq_orders[i].imps);
      }
      for(std::size_t i = 0; i < source.campaign_freqs.size(); ++i)
      {
        auto* campaign_freq = target.add_campaign_freqs();
        campaign_freq->set_campaign_id(source.campaign_freqs[i].campaign_id);
        campaign_freq->set_imps(source.campaign_freqs[i].imps);
      }
      target.set_merged_user_id(pack_oct_seq(source.merged_user_id));
      target.set_search_engine_id(source.search_engine_id);
      target.set_search_words(source.search_words);
      target.set_page_keywords_present(source.page_keywords_present);
      target.set_profiling_available(source.profiling_available);
      target.set_fraud(source.fraud);
      pack_ids(source.channels, target.mutable_channels());
      for(std::size_t i = 0; i < source.ccg_keywords.size(); ++i)
      {
        auto* kw = target.add_ccg_keywords();
        kw->set_ccg_keyword_id(source.ccg_keywords[i].ccg_keyword_id);
        kw->set_ccg_id(source.ccg_keywords[i].ccg_id);
        kw->set_channel_id(source.ccg_keywords[i].channel_id);
        kw->mutable_max_cpc()->set_value(source.ccg_keywords[i].max_cpc);
        kw->mutable_ctr()->set_value(source.ccg_keywords[i].ctr);
        kw->set_click_url(source.ccg_keywords[i].click_url);
        kw->set_original_keyword(source.ccg_keywords[i].original_keyword);
      }
      auto* trigger = target.mutable_trigger_match_result();
      auto pack_trigger = [](const auto& source_channels, auto* target_channels)
      {
        for(std::size_t i = 0; i < source_channels.size(); ++i)
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
      for(std::size_t i = 0; i < source.ad_slots.size(); ++i)
      {
        const auto& src = source.ad_slots[i];
        auto* dst = target.add_ad_slots();
        dst->set_ad_slot_id(src.ad_slot_id);
        dst->set_format(src.format);
        dst->set_tag_id(src.tag_id);
        pack_strings(src.sizes, dst->mutable_sizes());
        dst->set_ext_tag_id(src.ext_tag_id);
        dst->mutable_min_ecpm()->set_value(src.min_ecpm);
        dst->set_min_ecpm_currency_code(src.min_ecpm_currency_code);
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
        for(std::size_t j = 0; j < src.native_data_tokens.size(); ++j)
        {
          auto* token = dst->add_native_data_tokens();
          token->set_name(src.native_data_tokens[j].name);
          token->set_required(src.native_data_tokens[j].required);
        }
        for(std::size_t j = 0; j < src.native_image_tokens.size(); ++j)
        {
          auto* token = dst->add_native_image_tokens();
          token->set_name(src.native_image_tokens[j].name);
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
      target.set_page_keywords(source.page_keywords);
      target.set_url_keywords(source.url_keywords);
      target.set_ssp_location(source.ssp_location);
      target.set_additional_info(source.additional_info);
    }

    void unpack_request_creative_result(
      const PB::RequestCreativeResult& source,
      CampaignManager::RequestCreativeResult& target)
    {
      target.ad_slots.resize(source.ad_slots_size());
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

        dst.selected_creatives.resize(src.selected_creatives_size());
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
          dst_creative.revenue = src_creative.revenue().value();
          dst_creative.ecpm = src_creative.ecpm().value();
          dst_creative.pub_ecpm = src_creative.pub_ecpm().value();
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
        dst_debug.cpm_threshold = src_debug.cpm_threshold().value();
        dst_debug.walled_garden = src_debug.walled_garden();
        dst_debug.selected_creatives.resize(src_debug.selected_creatives_size());
        for(int j = 0; j < src_debug.selected_creatives_size(); ++j)
        {
          const auto& src_debug_creative = src_debug.selected_creatives(j);
          auto& dst_debug_creative = dst_debug.selected_creatives[j];
          dst_debug_creative.imp_revenue =
            src_debug_creative.imp_revenue().value();
          dst_debug_creative.click_revenue =
            src_debug_creative.click_revenue().value();
          dst_debug_creative.action_revenue =
            src_debug_creative.action_revenue().value();
          dst_debug_creative.ecpm_bid =
            src_debug_creative.ecpm_bid().value();
          dst_debug_creative.action_adv_url = src_debug_creative.action_adv_url();
          dst_debug_creative.html_url = src_debug_creative.html_url();
          dst_debug_creative.triggered_expression =
            src_debug_creative.triggered_expression();
          dst_debug_creative.full_expression = src_debug_creative.full_expression();
        }
        dst_debug.trace_ccg = src_debug.trace_ccg();

        unpack_tokens(src.native_data_tokens(), dst.native_data_tokens);
        dst.native_image_tokens.resize(src.native_image_tokens_size());
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
        dst.contracts.resize(src.contracts_size());
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

    void
    pack_get_campaign_creative_request(
      PB::GetCampaignCreativeRequest& request,
      const CampaignManager::RequestParams& request_params)
    {
      pack_request_params(request_params, *request.mutable_request_params());
    }

    void
    unpack_get_campaign_creative_response(
      const PB::GetCampaignCreativeResponse& response,
      CampaignManager::RequestCreativeResult& result,
      std::string& hostname)
    {
      hostname = response.hostname();
      unpack_request_creative_result(response.request_result(), result);
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

    StageResult*
    start_stage(
      std::optional<StageResult>& stage,
      const Generics::Time& request_started_at,
      const bool enabled)
    {
      if(!enabled)
      {
        return nullptr;
      }

      const auto now = Generics::Time::get_time_of_day();
      stage.emplace();
      stage->started_at = now - request_started_at;
      return &*stage;
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

  //
  // Frontend implementation
  //
  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    StatHolder* stats,
    Generics::CompositeMetricsProvider* composite_metrics_provider,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
    std::shared_ptr<AdServer::Commons::ExecutorPool> timeout_workers,
    unsigned long service_index) /*throw(eh::Exception)*/
    : GroupLogger(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().BidFeConfiguration()->Logger().log_level())),
        "Bidding::Frontend",
        Aspect::BIDDING_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      colo_id_(0),
      campaign_manager_(),
      bid_workers_(std::move(request_workers)),
      timeout_workers_(std::move(timeout_workers)),
      stats_(ReferenceCounting::add_ref(stats)),
      bid_task_count_(0),
      passback_task_count_(0),
      reached_max_pending_tasks_(0),
      composite_metrics_provider_(ReferenceCounting::add_ref(composite_metrics_provider))
  {
    if(!timeout_workers_)
    {
      timeout_workers_ = bid_workers_;
    }

    char hostname[256];
    if(gethostname(hostname, sizeof(hostname)) == 0)
    {
      hostname[sizeof(hostname) - 1] = 0;
      server_id_ = std::string(hostname) + "." + std::to_string(service_index);
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
      ostr << FUN << "': " << e.what();
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

        planner_ = new Generics::Planner(callback());
        add_child_object(planner_);

        control_task_runner_ = new Generics::TaskRunner(callback(), 4);
        add_child_object(control_task_runner_);

        // ADSC-10554
        // Interrupted requests queue
        passback_workers_ = bid_workers_;

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
        grpc_executor_ = common_module_->grpc_executor();

        auto user_info_client =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        user_info_distributed_client_ = user_info_client;
        user_info_client_ = user_info_client;
        user_info_client_coro_ = std::make_shared<
          AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>(
            user_info_client_,
            bid_workers_);
        add_child_object(user_info_client);

        auto campaign_manager_client =
          std::make_shared<
            AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
              FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
              AdServer::Grpc::BatchingOptions(),
              grpc_executor_,
              common_module_->grpc_coalesce_runner());
        campaign_manager_ = campaign_manager_client;
        campaign_manager_coro_ = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(
            campaign_manager_,
            bid_workers_);
        add_child_object(campaign_manager_client);

        auto user_bind_client =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        if(user_bind_client)
        {
          user_bind_client_ = user_bind_client;
          user_bind_client_coro_ = std::make_shared<
            AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>(
              user_bind_client_,
              bid_workers_);
          add_child_object(user_bind_client);
        }

        auto channel_client =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        channel_client_ = channel_client;
        channel_client_coro_ = std::make_shared<
          AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>(
            channel_client_,
            bid_workers_);
        add_child_object(channel_client);

        {
          std::vector<GrpcClientMetricsProvider::ClientSource> client_sources;
          auto add_client_source =
            [&client_sources](const char* prefix, const auto& client)
            {
              if (client)
              {
                client_sources.push_back(GrpcClientMetricsProvider::ClientSource{
                  prefix,
                  std::static_pointer_cast<AdServer::Grpc::Client>(client)
                });
              }
            };

          add_client_source("user_bind_client", user_bind_client_);
          add_client_source("user_info_client", user_info_client_);
          add_client_source("channel_client", channel_client_);
          add_client_source("campaign_client", campaign_manager_);

          composite_metrics_provider_->add_provider(
            new GrpcClientMetricsProvider(std::move(client_sources)));
        }

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
            common_module_->ip_mapper(),
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

    }
    catch(...)
    {}
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
    BidRequestState_var request_task;

    try
    {
      const FCGI::HttpRequest& request = request_holder->request();

      const Generics::Time request_timeout = get_request_timeout_(request);

      std::string found_uri;

      if(FrontendCommons::find_uri(
        config_->GoogleUriList().Uri(), request.uri(), found_uri))
      {
        // Google request
        request_task = new GoogleBidRequestState(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if(config_->AdXmlUriList().present() &&
        FrontendCommons::find_uri(
          config_->AdXmlUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new AdXmlBidRequestState(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if(config_->ClickStarUriList().present() &&
        FrontendCommons::find_uri(
          config_->ClickStarUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new ClickStarBidRequestState(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if(config_->DAOUriList().present() &&
        FrontendCommons::find_uri(
          config_->DAOUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new AdJsonBidRequestState(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else
      {
        // OpenRTB request
        request_task = new OpenRtbBidRequestState(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }

      unsigned long cur_task_count = bid_task_count_.exchange_and_add(1) + 1;
      request_task->init_debug_info();

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

        request_task->write_interrupted_empty_response(
          String::SubString("MaxPendingTasks"));
      }
      else
      {
        // Keep timeout callbacks out of the request continuation pool to avoid interrupt delays
        // when the main pool is busy
        timeout_workers_->schedule(
          request_timeout,
          [request_task]()
          {
            request_task->interrupt();
          });

        bid_workers_->post(
          [request_task]()
          {
            request_task->execute();
          });
      }
    }
    catch(const BidRequestState::Invalid& e)
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
      ostr << FUN << ": BidRequestState::Invalid caught: " << e.what();
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

  FrontendCommons::RequestTask
  Frontend::co_process_bid_request_(
    BidRequestState_var request_task)
    noexcept
  {
    static const char* FUN = "Bidding::Frontend::co_process_bid_request_()";

    struct TriggerMatchResult
    {
      std::shared_ptr<adserver::channel_svcs::channel_server::MatchResponse>
        result;
      bool present = false;
    };

    auto campaign_match_result =
      std::make_shared<AdServer::Bidding::CampaignManager::RequestCreativeResult>();
    bool stats_flushed = false;
    auto flush_stats = [&]() noexcept
    {
      if(stats_.in() && !stats_flushed && request_task->request_params().in())
      {
        stats_->flush(
          *request_task->request_params(),
          campaign_match_result.get(),
          Generics::Time::get_time_of_day() -
            request_task->start_processing_time());
        stats_flushed = true;
      }
    };

    InProgressGuard request_in_progress(
      stats_.in(),
      &StatHolder::add_rtb_request,
      &StatHolder::complete_rtb_request);

    try
    {
      InProgressGuard user_resolving_in_progress(
        stats_.in(),
        &StatHolder::add_rtb_request_user_resolving,
        &StatHolder::complete_rtb_request_user_resolving,
        &StatHolder::add_rtb_request_user_resolving_time);
      request_task->set_current_stage(Stage::UserResolving);

      const bool require_debug_info =
        request_task->debug_sink_.require_debug_info();
      auto& request_time_metering = request_task->request_time_metering_;
      StageResult* user_resolving_stage = start_stage(
        request_time_metering.user_resolving,
        request_task->start_processing_time(),
        require_debug_info);
      auto& request_info = request_task->request_info_;
      auto& common_info = request_task->request_params()->common_info;
      auto& match_user_id = request_task->resolved_user_id_;
      DebugSink::UserResolvingDebugInfo user_resolving_debug_info;

      auto finish_user_resolving = [&]()
      {
        if(common_info.user_status != AdServer::CampaignSvcs::US_OPTIN)
        {
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

        if(config_->trace_mapping() &&
           logger()->log_level() >= Logging::Logger::DEBUG)
        {
          Stream::Error ostr;
          ostr << "Bidding::Frontend::co_process_bid_request_(): "
            "SSP user mapping: " << match_user_id.to_string() <<
            " <-> (" << common_info.external_user_id << ", " <<
            request_info.source_id << ')';
          logger()->log(
            ostr.str(),
            Logging::Logger::DEBUG,
            Aspect::BIDDING_FRONTEND);
        }
      };

      if(request_info.filter_request)
      {
        common_info.user_status = static_cast<std::size_t>(
          AdServer::CampaignSvcs::US_FOREIGN);
      }
      else
      {
        match_user_id = CampaignManager::unpack_user_id(common_info.user_id);
        common_module_->user_id_controller()->null_blacklisted(match_user_id);
        if(!match_user_id.is_null())
        {
          common_info.user_status = static_cast<std::size_t>(
            AdServer::CampaignSvcs::US_OPTIN);
        }
        else if(request_info.advertising_id.empty() &&
          request_info.idfa.empty() &&
          common_info.external_user_id.empty())
        {
          if(common_info.user_status != AdServer::CampaignSvcs::US_PROBE)
          {
            common_info.user_status = static_cast<std::size_t>(
              AdServer::CampaignSvcs::US_NOEXTERNALID);
          }
        }
        else if(user_bind_client_coro_)
        {
          std::vector<std::string> external_user_ids;
          external_user_ids.reserve(
            request_info.ext_user_ids.size() + 3);
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

          if(!common_info.external_user_id.empty())
          {
            external_user_ids.push_back(common_info.external_user_id);
          }

          AdServer::Commons::UserId local_match_user_id;
          adserver::user_info_svcs::user_bind::GetUserIdResponse user_bind_info;
          bool blacklisted = false;
          bool min_age_reached = false;
          std::size_t base_index = 0;

          auto log_user_bind_exception =
            [this, user_resolving_stage](const eh::Exception& ex)
            {
              if(user_resolving_stage)
              {
                user_resolving_stage->set_exception_error(ex);
              }
              Stream::Error ostr;
              ostr << FUN << ": caught UserBindClient exception: " << ex.what();
              logger()->log(
                ostr.str(),
                Logging::Logger::ERROR,
                Aspect::BIDDING_FRONTEND,
                "ADS-IMPL-10681");
            };
          auto log_user_bind_grpc_error =
            [this, user_resolving_stage](
              const char* method,
              const grpc::Status& status)
            {
              if(user_resolving_stage)
              {
                user_resolving_stage->set_grpc_error(status);
              }
              logger()->sstream(
                Logging::Logger::ERROR,
                Aspect::BIDDING_FRONTEND,
                "ADS-IMPL-10681") <<
                "Bidding::Frontend::co_process_bid_request_(): "
                "UserBindServer::" << method <<
                "(): gRPC call failed: code=" <<
                static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
            };

          for(std::size_t get_index = 0;
              get_index < external_user_ids.size();
              ++get_index)
          {
            adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
            get_request.set_id(external_user_ids[get_index]);
            get_request.set_timestamp(
              GrpcAlgs::pack_time(request_info.current_time));
            get_request.set_silent(false);
            get_request.set_generate_user_id(false);
            get_request.set_for_set_cookie(false);
            get_request.set_create_timestamp(
              GrpcAlgs::pack_time(request_info.user_create_time));

            try
            {
              auto get_result = co_await
                user_bind_client_coro_->get_user_id(std::move(get_request));
              if(!get_result.status.ok())
              {
                log_user_bind_grpc_error("get_user_id", get_result.status);
                break;
              }

              user_bind_info = std::move(get_result.response);
              if(user_resolving_stage)
              {
                user_resolving_stage->server_id = user_bind_info.hostname();
              }
              min_age_reached |= user_bind_info.min_age_reached();
              local_match_user_id =
                GrpcAlgs::unpack_user_id(user_bind_info.user_id());

              if(require_debug_info)
              {
                user_resolving_debug_info.response_present = true;
                user_resolving_debug_info.user_id =
                  local_match_user_id.is_null() ?
                    std::string() : local_match_user_id.to_string();
                user_resolving_debug_info.min_age_reached =
                  user_bind_info.min_age_reached();
                user_resolving_debug_info.created = user_bind_info.created();
                user_resolving_debug_info.invalid_operation =
                  user_bind_info.invalid_operation();
                user_resolving_debug_info.user_found = user_bind_info.user_found();
              }

              blacklisted |=
                common_module_->user_id_controller()->null_blacklisted(
                  match_user_id);

              if(!local_match_user_id.is_null())
              {
                common_info.external_user_id = external_user_ids[get_index];
                base_index = get_index;
                break;
              }
              else if(common_info.external_user_id.empty())
              {
                common_info.external_user_id = external_user_ids[get_index];
              }
            }
            catch(const eh::Exception& ex)
            {
              log_user_bind_exception(ex);
              break;
            }
          }

          match_user_id = local_match_user_id;
          common_module_->user_id_controller()->null_blacklisted(match_user_id);

          if(!match_user_id.is_null())
          {
            common_info.user_status = static_cast<std::size_t>(
              AdServer::CampaignSvcs::US_OPTIN);

            for(std::size_t add_index = 0;
                add_index < external_user_ids.size();
                ++add_index)
            {
              if(add_index == base_index)
              {
                continue;
              }

              adserver::user_info_svcs::user_bind::AddUserIdRequest
                add_user_request;
              add_user_request.set_id(external_user_ids[add_index]);
              add_user_request.set_timestamp(
                GrpcAlgs::pack_time(request_info.current_time));
              add_user_request.set_user_id(GrpcAlgs::pack_user_id(match_user_id));

              try
              {
                auto add_result = co_await
                  user_bind_client_coro_->add_user_id(
                    std::move(add_user_request));
                if(!add_result.status.ok())
                {
                  log_user_bind_grpc_error("add_user_id", add_result.status);
                  break;
                }
                if(user_resolving_stage &&
                   user_resolving_stage->server_id.empty())
                {
                  user_resolving_stage->server_id =
                    add_result.response.hostname();
                }
              }
              catch(const eh::Exception& ex)
              {
                log_user_bind_exception(ex);
                break;
              }
            }
          }
          else if(blacklisted)
          {
            common_info.user_status = static_cast<std::size_t>(
              AdServer::CampaignSvcs::US_UNDEFINED);
          }
          else if(user_bind_info.user_found())
          {
            common_info.user_status = static_cast<std::size_t>(
              AdServer::CampaignSvcs::US_OPTOUT);
          }
          else if(min_age_reached)
          {
            common_info.user_status = static_cast<std::size_t>(
              AdServer::CampaignSvcs::US_UNDEFINED);
          }
          else if(!external_user_ids.empty())
          {
            common_info.user_status = static_cast<std::size_t>(
              AdServer::CampaignSvcs::US_EXTERNALPROBE);
          }
        }
      }

      finish_user_resolving();
      if(user_resolving_stage)
      {
        user_resolving_stage->finish(request_task->start_processing_time());
      }
      user_resolving_in_progress.reset();

      if(require_debug_info)
      {
        request_task->print_available_request_debug_info_();
        request_task->debug_sink_.print_user_resolving_debug_info(
          user_resolving_debug_info,
          user_resolving_stage);
      }

      if(check_interrupt_(FUN, Stage::UserResolving, request_task))
      {
        request_task->complete_request_(false, *campaign_match_result);
        co_return FrontendCommons::RequestResult::written();
      }

      InProgressGuard trigger_match_in_progress(
        stats_.in(),
        &StatHolder::add_rtb_request_trigger_match,
        &StatHolder::complete_rtb_request_trigger_match,
        &StatHolder::add_rtb_request_trigger_match_time);
      request_task->set_current_stage(Stage::TriggerMatching);
      StageResult* trigger_match_stage = start_stage(
        request_time_metering.trigger_match,
        request_task->start_processing_time(),
        require_debug_info);

      TriggerMatchResult trigger_match;
      if(!request_info.filter_request)
      {
        try
        {
          adserver::channel_svcs::channel_server::MatchRequest channel_request;
          auto& request_params = *request_task->request_params();

          channel_request.set_non_strict_word_match(false);
          channel_request.set_non_strict_url_match(false);
          channel_request.set_return_negative(false);
          channel_request.set_simplify_page(true);
          channel_request.set_fill_content(true);
          channel_request.set_statuses("A", 2);
          channel_request.set_first_url(request_params.common_info.referer);

          try
          {
            std::string ref_words;
            FrontendCommons::extract_url_keywords(
              ref_words,
              String::SubString(request_params.common_info.referer),
              common_module_->segmentor());

            if(!ref_words.empty())
            {
              channel_request.set_first_url_words(ref_words);
            }
          }
          catch(const eh::Exception& e)
          {
            logger()->sstream(
              Logging::Logger::TRACE,
              Aspect::BIDDING_FRONTEND) <<
              FUN << ": url keywords extracting error: " << e.what();
          }

          std::string urls_str;
          std::string urls_words_str;
          for(std::size_t i = 0;
              i < request_params.common_info.urls.size();
              ++i)
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

            if(!url_words_res.empty())
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

          if(!request_task->keywords_.empty())
          {
            channel_request.set_pwords(request_task->keywords_);
          }
          channel_request.set_swords(request_info.search_words);
          channel_request.set_uid(
            GrpcAlgs::pack_user_id(request_task->resolved_user_id_));

          auto channel_result = co_await
            channel_client_coro_->match(std::move(channel_request));
          if(channel_result.status.ok())
          {
            if(trigger_match_stage)
            {
              trigger_match_stage->server_id =
                channel_result.response.hostname();
            }
            trigger_match.result = std::make_shared<
              adserver::channel_svcs::channel_server::MatchResponse>(
                std::move(channel_result.response));
            trigger_match.present = true;

            const auto& matched_channels =
              trigger_match.result->matched_channels();

            request_params.trigger_match_result.pkw_channels.resize(
              matched_channels.page_channels_size());
            std::transform(
              matched_channels.page_channels().begin(),
              matched_channels.page_channels().end(),
              request_params.trigger_match_result.pkw_channels.data(),
              convert_channel_atom);
            request_params.trigger_match_result.url_channels.resize(
              matched_channels.url_channels_size());
            std::transform(
              matched_channels.url_channels().begin(),
              matched_channels.url_channels().end(),
              request_params.trigger_match_result.url_channels.data(),
              convert_channel_atom);
            request_params.trigger_match_result.ukw_channels.resize(
              matched_channels.url_keyword_channels_size());
            std::transform(
              matched_channels.url_keyword_channels().begin(),
              matched_channels.url_keyword_channels().end(),
              request_params.trigger_match_result.ukw_channels.data(),
              convert_channel_atom);
            request_params.trigger_match_result.skw_channels.resize(
              matched_channels.search_channels_size());
            std::transform(
              matched_channels.search_channels().begin(),
              matched_channels.search_channels().end(),
              request_params.trigger_match_result.skw_channels.data(),
              convert_channel_atom);
            request_params.trigger_match_result.uid_channels.resize(
              matched_channels.uid_channels_size());
            for(int i = 0; i < matched_channels.uid_channels_size(); ++i)
            {
              request_params.trigger_match_result.uid_channels[i] =
                matched_channels.uid_channels(i);
            }

            if(request_params.common_info.user_status ==
                 static_cast<std::size_t>(AdServer::CampaignSvcs::US_OPTIN) &&
               (trigger_match.result->no_track() ||
                trigger_match.result->no_adv()))
            {
              request_params.common_info.user_status =
                static_cast<std::size_t>(
                  AdServer::CampaignSvcs::US_BLACKLISTED);
            }
          }
          else
          {
            if(trigger_match_stage)
            {
              trigger_match_stage->set_grpc_error(channel_result.status);
            }
            logger()->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::BIDDING_FRONTEND,
              "ADS-IMPL-117") <<
              "Bidding::Frontend::co_process_bid_request_(): "
              "ChannelServer grpc match failed: code=" <<
              static_cast<int>(channel_result.status.error_code()) <<
              ", message=" << channel_result.status.error_message();
          }
        }
        catch(const eh::Exception& ex)
        {
          if(trigger_match_stage)
          {
            trigger_match_stage->set_exception_error(ex);
          }
          logger()->sstream(
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-117") <<
            FUN << ": caught ChannelServerGrpcCoroClient match error: " <<
            ex.what();
        }
      }

      if(trigger_match_stage)
      {
        trigger_match_stage->finish(request_task->start_processing_time());
      }

      trigger_match_in_progress.reset();

      if(require_debug_info && trigger_match.present)
      {
        request_task->debug_sink_.print_channel_matching_debug_info(
          *trigger_match.result,
          trigger_match_stage);
      }

      if(check_interrupt_(FUN, Stage::TriggerMatching, request_task))
      {
        request_task->complete_request_(false, *campaign_match_result);
        co_return FrontendCommons::RequestResult::written();
      }

      InProgressGuard history_match_in_progress(
        stats_.in(),
        &StatHolder::add_rtb_request_history_match,
        &StatHolder::complete_rtb_request_history_match,
        &StatHolder::add_rtb_request_history_match_time);
      request_task->set_current_stage(Stage::HistoryMatching);
      StageResult* history_match_stage = start_stage(
        request_time_metering.history_match,
        request_task->start_processing_time(),
        require_debug_info);

      auto init_synthetic_match_result = [](auto* match_result)
      {
        match_result->set_fraud_request(false);
        match_result->set_times_inited(false);
        match_result->set_last_request_time(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        match_result->set_create_time(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        match_result->set_session_start(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        match_result->set_process_time(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        match_result->set_colo_id(-1);
      };

      auto finish_history_match = [&](
        std::shared_ptr<
          adserver::user_info_svcs::user_info_manager::MatchResponse>
            result)
      {
        auto& request_params = *request_task->request_params();

        if(!result && trigger_match.result)
        {
          result = std::make_shared<
            adserver::user_info_svcs::user_info_manager::MatchResponse>();
          auto* match_result = result->mutable_match_result();
          init_synthetic_match_result(match_result);
          for(const auto& content_channel :
              trigger_match.result->content_channels())
          {
            auto* channel = match_result->add_channels();
            channel->set_channel_id(content_channel.id());
            channel->set_weight(content_channel.weight());
          }
        }

        request_params.context_info.profile_referer =
          config_->enable_profile_referer();

        return result;
      };

      request_task->request_params()->profiling_available = false;
      std::shared_ptr<
        adserver::user_info_svcs::user_info_manager::MatchResponse>
          history_match_result;

      if(request_task->resolved_user_id_.is_null())
      {
        if(trigger_match.result && !trigger_match.result->no_track())
        {
          history_match_result = std::make_shared<
            adserver::user_info_svcs::user_info_manager::MatchResponse>();
          auto* match_result = history_match_result->mutable_match_result();
          init_synthetic_match_result(match_result);
          for(int i = 0; i < trigger_match.result->content_channels_size(); ++i)
          {
            auto* channel = match_result->add_channels();
            channel->set_channel_id(
              trigger_match.result->content_channels(i).id());
            channel->set_weight(
              trigger_match.result->content_channels(i).weight());
          }
        }
      }
      else
      {
        try
        {
          typedef std::set<ChannelMatch> ChannelMatchSet;
          adserver::user_info_svcs::user_info_manager::MatchRequest
            history_match_request;
          auto& request_params = *request_task->request_params();

          auto* user_info = history_match_request.mutable_user_info();
          const auto packed_user_id =
            GrpcAlgs::pack_user_id(request_task->resolved_user_id_);
          if(request_task->hostname_.empty() && user_info_distributed_client_)
          {
            request_task->hostname_ =
              user_info_distributed_client_->endpoint_for_user(packed_user_id);
          }
          user_info->set_user_id(packed_user_id);
          user_info->set_last_colo_id(colo_id_);
          user_info->set_request_colo_id(colo_id_);
          user_info->set_current_colo_id(-1);
          user_info->set_temporary(false);
          user_info->set_time(request_info.current_time.tv_sec);

          auto* match_params = history_match_request.mutable_match_params();
          match_params->set_use_empty_profile(false);
          match_params->set_silent_match(false);
          match_params->set_no_match(
            trigger_match.result && trigger_match.result->no_track());
          match_params->set_no_result(false);
          match_params->set_ret_freq_caps(true);
          match_params->set_provide_channel_count(false);
          match_params->set_provide_persistent_channels(false);
          match_params->set_change_last_request(true);
          match_params->set_publishers_optin_timeout(GrpcAlgs::pack_time(
            request_info.current_time - Generics::Time::ONE_DAY * 15));
          match_params->set_cohort(
            !request_info.idfa.empty() ?
              request_info.idfa : request_info.advertising_id);

          for(std::size_t i = 0;
              i < request_params.context_info.platform_ids.size();
              ++i)
          {
            match_params->add_persistent_channel_ids(
              request_params.context_info.platform_ids[i]);
          }

          if(trigger_match.result && !trigger_match.result->no_track())
          {
            const auto& matched_channels =
              trigger_match.result->matched_channels();
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

            const auto fill_channel_matches = [](auto* out, const auto& in)
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

          auto history_result = co_await
            user_info_client_coro_->match(std::move(history_match_request));
          if(history_result.status.ok())
          {
            request_task->request_params()->profiling_available = true;
            if(history_match_stage)
            {
              history_match_stage->server_id =
                history_result.response.hostname();
            }
            history_match_result = std::make_shared<
              adserver::user_info_svcs::user_info_manager::MatchResponse>(
                std::move(history_result.response));
          }
          else
          {
            if(history_match_stage)
            {
              history_match_stage->set_grpc_error(
                history_result.status,
                request_task->hostname_);
            }
            logger()->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::BIDDING_FRONTEND,
              "ADS-IMPL-112") <<
              "Bidding::Frontend::co_process_bid_request_(): "
              "UserInfoManager grpc match failed: code=" <<
              static_cast<int>(history_result.status.error_code()) <<
              ", message=" << history_result.status.error_message();
          }
        }
        catch(const eh::Exception& ex)
        {
          if(history_match_stage)
          {
            history_match_stage->set_exception_error(ex);
          }
          logger()->sstream(
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-112") <<
            "Bidding::Frontend::co_process_bid_request_(): "
            "caught UserInfoManagerGrpcCoroClient match error: " <<
            ex.what();
        }
      }

      history_match_result = finish_history_match(
        std::move(history_match_result));
      if(history_match_stage)
      {
        history_match_stage->finish(request_task->start_processing_time());
      }
      history_match_in_progress.reset();

      if(history_match_result)
      {
        if(history_match_stage)
        {
          history_match_stage->local_time =
            GrpcAlgs::unpack_time(
              history_match_result->match_result().process_time());
        }
        if(require_debug_info)
        {
          request_task->debug_sink_.print_history_matching_debug_info(
            history_match_result->match_result(),
            history_match_stage);
        }
      }

      if(check_interrupt_(FUN, Stage::HistoryMatching, request_task))
      {
        request_task->complete_request_(false, *campaign_match_result);
        co_return FrontendCommons::RequestResult::written();
      }

      InProgressGuard campaign_selection_in_progress(
        stats_.in(),
        &StatHolder::add_rtb_request_campaign_selection,
        &StatHolder::complete_rtb_request_campaign_selection,
        &StatHolder::add_rtb_request_campaign_selection_time);
      request_task->set_current_stage(Stage::CampaignSelection);

      std::shared_ptr<
        adserver::channel_svcs::channel_server::GetCcgTraitsResponse>
          ccg_keywords;
      if(history_match_result &&
         history_match_result->match_result().channels_size() &&
         !request_info.skip_ccg_keywords &&
         !request_info.filter_request)
      {
        adserver::channel_svcs::channel_server::GetCcgTraitsRequest
          ccg_traits_request;
        for(const auto& channel :
            history_match_result->match_result().channels())
        {
          ccg_traits_request.add_ids(channel.channel_id());
        }

        try
        {
          auto ccg_traits_result = co_await
            channel_client_coro_->get_ccg_traits(std::move(ccg_traits_request));
          if(ccg_traits_result.status.ok())
          {
            ccg_keywords = std::make_shared<
              adserver::channel_svcs::channel_server::GetCcgTraitsResponse>(
                std::move(ccg_traits_result.response));
          }
          else
          {
            logger()->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::BIDDING_FRONTEND,
              "ADS-IMPL-117") <<
              "ChannelServer grpc get_ccg_traits failed: code=" <<
              static_cast<int>(ccg_traits_result.status.error_code()) <<
              ", message=" << ccg_traits_result.status.error_message();
          }
        }
        catch(const eh::Exception& ex)
        {
          logger()->sstream(
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-117") <<
            "Frontend::co_process_bid_request_(): "
            "caught ChannelServerGrpcCoroClient get_ccg_traits error: " <<
            ex.what();
        }
      }

      StageResult* creative_selection_stage = start_stage(
        request_time_metering.creative_selection,
        request_task->start_processing_time(),
        require_debug_info);

      auto& request_params = *request_task->request_params();
      const bool passback =
        (trigger_match.present &&
          (trigger_match.result->no_track() || trigger_match.result->no_adv())) ||
        request_info.filter_request;

      select_campaign_(
        history_match_result.get(),
        trigger_match.present ? trigger_match.result.get() : nullptr,
        ccg_keywords.get(),
        request_info,
        request_params,
        request_task->resolved_user_id_,
        passback,
        false);

      PB::GetCampaignCreativeRequest campaign_request;
      pack_get_campaign_creative_request(campaign_request, request_params);

      try
      {
        auto campaign_result = co_await
          campaign_manager_coro_->get_campaign_creative(
            std::move(campaign_request));
        if(campaign_result.status.ok())
        {
          const auto& response = campaign_result.response;
          if(creative_selection_stage)
          {
            creative_selection_stage->server_id = response.hostname();
          }
          if(require_debug_info)
          {
            const auto& request_result = response.request_result();
            bool has_trace = false;
            bool has_selected_creatives = false;
            for(const auto& ad_slot : request_result.ad_slots())
            {
              has_trace |= !ad_slot.debug_info().trace_ccg().empty();
              has_selected_creatives |=
                ad_slot.selected_creatives_size() != 0;
            }

            if(has_trace && !has_selected_creatives)
            {
              logger()->sstream(
                Logging::Logger::NOTICE,
                Aspect::BIDDING_FRONTEND) <<
                "CampaignManager::get_campaign_creative() returned "
                "trace without selected creatives: ad_slots=" <<
                request_result.ad_slots_size();
            }
          }

          unpack_get_campaign_creative_response(
            response,
            *campaign_match_result,
            request_task->hostname_);
        }
        else
        {
          if(creative_selection_stage)
          {
            creative_selection_stage->set_grpc_error(campaign_result.status);
          }
          logger()->sstream(
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-118") <<
            "CampaignManager::get_campaign_creative(): "
            "gRPC call failed: code=" <<
            static_cast<int>(campaign_result.status.error_code()) <<
            ", message=" << campaign_result.status.error_message();
        }
      }
      catch(const eh::Exception& ex)
      {
        if(creative_selection_stage)
        {
          creative_selection_stage->set_exception_error(ex);
        }
        logger()->sstream(
          Logging::Logger::EMERGENCY,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-118") <<
          "CampaignManager::get_campaign_creative(): "
          "caught CampaignManagerGrpcCoroClient error: " << ex.what();
      }

      if(creative_selection_stage)
      {
        creative_selection_stage->finish(request_task->start_processing_time());
      }
      campaign_selection_in_progress.reset();

      if(creative_selection_stage && campaign_match_result->ad_slots.size())
      {
        creative_selection_stage->local_time =
          CampaignManager::unpack_time(campaign_match_result->process_time);
      }

      if(check_interrupt_(FUN, Stage::CampaignSelection, request_task))
      {
        flush_stats();
        request_task->complete_request_(false, *campaign_match_result);
        co_return FrontendCommons::RequestResult::written();
      }

      if(campaign_match_result->ad_slots.size() > 0 &&
        campaign_match_result->ad_slots[0].debug_info.trace_ccg[0] &&
        request_params.ad_slots.size() > 0 &&
        logger()->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream ostr;
        ostr << FUN << ": CCG Trace of " <<
          request_params.ad_slots[0].debug_ccg <<
          " for request:" << std::endl;

        request_task->print_request(ostr);

        ostr << std::endl <<
          campaign_match_result->ad_slots[0].debug_info.trace_ccg;
        std::cout << ostr.str() << std::endl;

        logger()->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::BIDDING_FRONTEND);
      }

      flush_stats();
      request_task->complete_request_(true, *campaign_match_result);
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-109") <<
        FUN << ": eh::Exception caught: " << ex.what();
      flush_stats();
      request_task->complete_request_(false, *campaign_match_result);
    }
    catch(...)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-109") <<
        FUN << ": unknown exception caught";
      flush_stats();
      request_task->complete_request_(false, *campaign_match_result);
    }

    co_return FrontendCommons::RequestResult::written();
  }

  void
  Frontend::interrupted_select_campaign_(
    BidRequestState* request_task) noexcept
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
      else if(!passback_workers_)
      {
        passback_task_count_ += -1;
      }
      else
      {
        try
        {
          co_interrupted_select_campaign_(*request_params).start_detached(nullptr);
        }
        catch(...)
        {
          passback_task_count_ += -1;
          throw;
        }
      }
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

  bool
  Frontend::consider_campaign_selection_(
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result,
    std::string& hostname)
    noexcept
  {
    if(user_id.is_null())
    {
      return true;
    }

    try
    {
      co_consider_campaign_selection_(
        user_id,
        now,
        std::make_shared<
          AdServer::Bidding::CampaignManager::RequestCreativeResult>(
            campaign_match_result),
        hostname).start_detached(nullptr);
    }
    catch(const eh::Exception& e)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-112") <<
        "Bidding::Frontend::consider_campaign_selection_(): "
        "post match failed: " << e.what();
      return false;
    }

    return true;
  }

  FrontendCommons::RequestTask
  Frontend::co_consider_campaign_selection_(
    AdServer::Commons::UserId user_id,
    Generics::Time now,
    std::shared_ptr<
      const AdServer::Bidding::CampaignManager::RequestCreativeResult>
        campaign_match_result,
    std::string /*hostname*/)
    noexcept
  {
    static const char* FUN =
      "Bidding::Frontend::co_consider_campaign_selection_()";

    Generics::Time start_process_time;

    try
    {
      InProgressGuard history_post_match_in_progress(
        stats_.in(),
        &StatHolder::add_rtb_request_history_post_match,
        &StatHolder::complete_rtb_request_history_post_match,
        &StatHolder::add_rtb_request_history_post_match_time);

      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        start_process_time = Generics::Time::get_time_of_day();
      }

      std::size_t seq_order_len = 0;

      for(const auto& ad_slot : campaign_match_result->ad_slots)
      {
        for(const auto& creative : ad_slot.selected_creatives)
        {
          if(creative.order_set_id)
          {
            ++seq_order_len;
          }
        }
      }

      std::vector<adserver::user_info_svcs::user_info_manager::SeqOrderInfo>
        seq_orders(seq_order_len);
      std::size_t result_seq_order_i = 0;

      for(const auto& ad_slot_result : campaign_match_result->ad_slots)
      {
        if(ad_slot_result.selected_creatives.empty())
        {
          continue;
        }

        std::vector<unsigned long> campaign_ids(
          ad_slot_result.selected_creatives.size());

        for(std::size_t creative_i = 0;
            creative_i < ad_slot_result.selected_creatives.size();
            ++creative_i)
        {
          const auto& creative = ad_slot_result.selected_creatives[creative_i];

          if(creative.order_set_id)
          {
            auto& seq_order = seq_orders[result_seq_order_i];
            seq_order.set_ccg_id(creative.cmp_id);
            seq_order.set_set_id(creative.order_set_id);
            seq_order.set_imps(1);
            BidStatisticsPrometheusInc(
              composite_metrics_provider_,
              seq_order.ccg_id());
            ++result_seq_order_i;
          }

          campaign_ids[creative_i] = creative.campaign_group_id;
        }

        adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest
          request;
        request.set_user_id(GrpcAlgs::pack_user_id(user_id));
        request.set_time(GrpcAlgs::pack_time(now));
        request.set_request_id(ad_slot_result.request_id);
        for(const auto freq_cap : ad_slot_result.freq_caps)
        {
          request.add_freq_caps(freq_cap);
        }
        for(const auto uc_freq_cap : ad_slot_result.uc_freq_caps)
        {
          request.add_uc_freq_caps(uc_freq_cap);
        }
        for(const auto& seq_order : seq_orders)
        {
          *request.add_seq_orders() = seq_order;
        }
        for(const auto campaign_id : campaign_ids)
        {
          if(ad_slot_result.track_impr)
          {
            request.add_uc_campaign_ids(campaign_id);
          }
          else
          {
            request.add_campaign_ids(campaign_id);
          }
        }

        auto result = co_await user_info_client_coro_->update_user_freq_caps(
          std::move(request));
        if(!result.status.ok())
        {
          logger()->sstream(
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-112") <<
            "UserInfoManager grpc update_user_freq_caps failed: code=" <<
            static_cast<int>(result.status.error_code()) <<
            ", message=" << result.status.error_message();
        }
      }
    }
    catch(const eh::Exception& e)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-112") <<
        FUN << ": post match failed: " << e.what();
    }

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      const Generics::Time end_process_time = Generics::Time::get_time_of_day();
      logger()->sstream(
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND) <<
        FUN << ": campaign selection considering = " <<
        (end_process_time - start_process_time);
    }

    co_return FrontendCommons::RequestResult::written();
  }

  void
  Frontend::select_campaign_(
    const adserver::user_info_svcs::user_info_manager::MatchResponse*
      history_match_result,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_match_result,
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
      ccg_keywords,
    const RequestInfo& request_info,
    AdServer::Bidding::CampaignManager::RequestParams& request_params,
    const AdServer::Commons::UserId& user_id,
    bool passback,
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
        request_params.common_info.user_id = CampaignManager::pack_user_id(user_id);
        request_params.common_info.track_user_id =
          request_params.common_info.user_id;
      }

      // Fill debug-info & process fraud
      request_params.only_display_ad = false;
      const auto* history_result =
        history_match_result ? &history_match_result->match_result() : nullptr;
      if(request_params.ad_slots.size() > 0)
      {
        if (!interrupted)
        {
          request_params.need_debug_info = true;
          request_params.ad_slots[0].debug_ccg = request_info.debug_ccg;
        }
        for(size_t i = 0; i < request_params.ad_slots.size(); ++i)
        {
          request_params.ad_slots[i].passback |=
            passback || interrupted ||
              (history_result && history_result->fraud_request());
        }
      }

      // Fill user history data
      request_params.client_create_time =
        CampaignManager::pack_time(Generics::Time::ZERO);
      request_params.session_start =
        CampaignManager::pack_time(Generics::Time::ZERO);
      if(history_result)
      {
        request_params.client_create_time = history_result->create_time();
        request_params.session_start = history_result->session_start();

        request_params.full_freq_caps.resize(
          history_result->full_freq_caps_size());
        for(int i = 0; i < history_result->full_freq_caps_size(); ++i)
        {
          request_params.full_freq_caps[i] =
            history_result->full_freq_caps(i);
        }

        request_params.exclude_pubpixel_accounts.resize(
          history_result->exclude_pubpixel_accounts_size());
        for(int i = 0; i < history_result->exclude_pubpixel_accounts_size(); ++i)
        {
          request_params.exclude_pubpixel_accounts[i] =
            history_result->exclude_pubpixel_accounts(i);
        }

        request_params.campaign_freqs.resize(
          history_result->campaign_freqs_size());
        for(int i = 0; i < history_result->campaign_freqs_size(); ++i)
        {
          const auto& source = history_result->campaign_freqs(i);
          auto& target = request_params.campaign_freqs[i];
          target.campaign_id = source.campaign_id();
          target.imps = source.imps();
        }

        request_params.seq_orders.resize(history_result->seq_orders_size());
        for(int i = 0; i < history_result->seq_orders_size(); ++i)
        {
          const auto& source = history_result->seq_orders(i);
          auto& target = request_params.seq_orders[i];
          target.ccg_id = source.ccg_id();
          target.set_id = source.set_id();
          target.imps = source.imps();
        }

        request_params.common_info.coord_location.resize(
          history_result->geo_data_seq_size());
        for(int i = 0; i < history_result->geo_data_seq_size(); ++i)
        {
          const auto& source = history_result->geo_data_seq(i);
          auto& target = request_params.common_info.coord_location[i];
          target.longitude = source.longitude();
          target.latitude = source.latitude();
          target.accuracy = source.accuracy();
        }
      }

      /* fill input channel sequence for CampaignManager */
      std::size_t uid_channels_length = 0;

      if (trigger_match_result)
      {
        uid_channels_length =
          trigger_match_result->matched_channels().uid_channels_size();
      }

      const int history_channels_size =
        history_result ? history_result->channels_size() : 0;
      request_params.channels.resize(history_channels_size + uid_channels_length);

      std::size_t j = 0;
      for (int i = 0; i < history_channels_size; ++i, ++j)
      {
        request_params.channels[j] =
          history_result->channels(i).channel_id();
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
        request_params.ccg_keywords.resize(ccg_keywords->ccg_keywords_size());
        for(int i = 0; i < ccg_keywords->ccg_keywords_size(); ++i)
        {
          const auto& src_ccg_kw = ccg_keywords->ccg_keywords(i);
          AdServer::Bidding::CampaignManager::CCGKeywordInfo&
            res_ccg_kw = request_params.ccg_keywords[i];
          res_ccg_kw.ccg_keyword_id = src_ccg_kw.ccg_keyword_id();
          res_ccg_kw.ccg_id = src_ccg_kw.ccg_id();
          res_ccg_kw.channel_id = src_ccg_kw.channel_id();
          res_ccg_kw.max_cpc = src_ccg_kw.max_cpc();
          res_ccg_kw.ctr = src_ccg_kw.ctr();
          res_ccg_kw.click_url = src_ccg_kw.click_url();
          res_ccg_kw.original_keyword = src_ccg_kw.original_keyword();
        }
      }

      // Process black list users
      const Generics::Time day_time =
        request_info.current_time - request_info.current_time.get_gm_time().get_date();

      if (blacklisted_time_intervals_.contains(day_time))
      {
        if (request_params.common_info.user_status ==
            static_cast<std::size_t>(CampaignSvcs::US_OPTIN))
        {
          request_params.common_info.user_status =
            static_cast<std::size_t>(CampaignSvcs::US_BLACKLISTED);
        }

        for(std::size_t slot_i = 0;
            slot_i < request_params.ad_slots.size(); ++slot_i)
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
        else if(history_result && !history_result->cohort().empty())
        {
          if(request_info.platform_names.find("ipad") !=
              request_info.platform_names.end() ||
            request_info.platform_names.find("iphone") !=
              request_info.platform_names.end() ||
            request_info.platform_names.find("ios") !=
              request_info.platform_names.end())
          {
            add_token(request_params.common_info.tokens,
              "IDFA", history_result->cohort());
          }
          else
          {
            add_token(request_params.common_info.tokens,
              "ADVERTISING_ID", history_result->cohort());
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

    co_update_config_().start_detached(nullptr);

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

  FrontendCommons::RequestTask
  Frontend::co_update_config_()
    noexcept
  {
    static const char* FUN = "Frontend::co_update_config_()";

    try
    {
      auto result = co_await campaign_manager_coro_->get_colocation_flags(
        PB::GetColocationFlagsRequest());
      if(!result.status.ok())
      {
        logger()->sstream(
          Logging::Logger::CRITICAL,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-118") << FUN <<
          ": CampaignManager::get_colocation_flags() failed: code=" <<
          static_cast<int>(result.status.error_code()) <<
          ", message=" << result.status.error_message();
        co_return FrontendCommons::RequestResult::written();
      }

      ExtConfig_var new_config(new ExtConfig());

      for(const auto& colocation_info : result.response.colocations())
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
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::CRITICAL,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-118") << FUN <<
        ": CampaignManager::get_colocation_flags() failed: " << ex.what();
    }

    co_return FrontendCommons::RequestResult::written();
  }

  FrontendCommons::RequestTask
  Frontend::co_interrupted_select_campaign_(
    AdServer::Bidding::CampaignManager::RequestParams request_params)
    noexcept
  {
    try
    {
      co_await AdServer::Commons::ExecutorPool::yield(passback_workers_);
      passback_task_count_ += -1;

      PB::GetCampaignCreativeRequest request;
      pack_get_campaign_creative_request(request, request_params);
      co_await campaign_manager_coro_->get_campaign_creative(std::move(request));
    }
    catch(const eh::Exception&)
    {
      // Interrupted request passback is best-effort.
    }

    co_return FrontendCommons::RequestResult::written();
  }

  void
  Frontend::limit_max_cpm_(
    AdServer::CampaignSvcs::RevenueDecimal& val,
    const AdServer::Bidding::CampaignManager::IdSeq& account_ids)
    const noexcept
  {
    for(std::size_t i = 0; i < account_ids.size(); ++i)
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
    const BidRequestState* request_task)
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
    BidRequestState* request_task)
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
