#include "CampaignManagerLogAdapter.hpp"

#include <HTTP/UrlAddress.hpp>

#include <Commons/StringHolder.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Constants.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer::CampaignSvcs
{
  void
  CampaignManagerLogAdapter::convert_channel_ids_(
    ChannelIdHashSet& all_channels,
    ChannelIdHashSet& channels,
    CampaignManagerLogger::TriggerChannelMap& triggers,
    const CampaignManagerCore::ChannelTriggerMatchArray& channel_trigger_matches)
    noexcept
  {
    triggers.reserve(triggers.size() + channel_trigger_matches.size());
    for(const auto& channel_trigger_match : channel_trigger_matches)
    {
      triggers.emplace_back(
        channel_trigger_match.channel_id,
        channel_trigger_match.channel_trigger_id);
      if(&all_channels != &channels)
      {
        all_channels.insert(channel_trigger_match.channel_id);
      }
      channels.insert(channel_trigger_match.channel_id);
    }
  }

  void
  CampaignManagerLogAdapter::fill_responded_channel_info_(
    CampaignManagerLogger::AdSelectionInfo& ad_info,
    const CampaignSelectionData& cs_data,
    const ChannelIdHashSet* channels)
    noexcept
  {
    try
    {
      if(!channels || !cs_data.selection_done)
      {
        return;
      }

      if(cs_data.campaign_keyword.in())
      {
        ad_info.channels.push_back(cs_data.campaign_keyword->channel_id);
        ad_info.expression = std::to_string(cs_data.campaign_keyword->channel_id);
        return;
      }

      if(!cs_data.campaign->targeted() || !cs_data.campaign->channel.in())
      {
        return;
      }

      ChannelIdSet responded_channels;
      cs_data.campaign->channel->triggered_named_channels(
        responded_channels,
        *channels);

      ad_info.channels.insert(
        ad_info.channels.end(),
        responded_channels.begin(),
        responded_channels.end());

      if(cs_data.campaign->stat_channel.in())
      {
        std::string responded_expression;
        if(cs_data.campaign->stat_channel->triggered_expression(
          responded_expression,
          *channels))
        {
          ad_info.expression = std::move(responded_expression);
        }
      }
    }
    catch(const ExpressionChannelBase::Exception&)
    {}
  }

  void
  CampaignManagerLogAdapter::fill_request_info_by_profiling_(
    CampaignManagerLogger::RequestInfo& request_info,
    const CampaignManagerCore::LogAdRequest& log_request,
    const ChannelIdHashSet& channels,
    const CampaignManagerCore::CommonAdRequest& common_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignManagerLogAdapter::fill_request_info_by_profiling_()";

    try
    {
      request_info.merged_user_id = log_request.merged_user_id;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid user id: " << ex.what();
      throw Exception(ostr);
    }

    convert_channel_ids_(
      request_info.triggered_channels.channels,
      request_info.triggered_channels.url_channels,
      request_info.url_triggers,
      log_request.trigger_match_result.url_channels);

    convert_channel_ids_(
      request_info.triggered_channels.channels,
      request_info.triggered_channels.page_channels,
      request_info.page_triggers,
      log_request.trigger_match_result.pkw_channels);

    convert_channel_ids_(
      request_info.triggered_channels.channels,
      request_info.triggered_channels.search_channels,
      request_info.search_triggers,
      log_request.trigger_match_result.skw_channels);

    convert_channel_ids_(
      request_info.triggered_channels.channels,
      request_info.triggered_channels.url_keyword_channels,
      request_info.url_keyword_triggers,
      log_request.trigger_match_result.ukw_channels);

    request_info.triggered_channels.uid_channels.insert(
      log_request.trigger_match_result.uid_channels.begin(),
      log_request.trigger_match_result.uid_channels.end());

    request_info.history_channels.reserve(channels.size());

    request_info.history_channels.assign(
      channels.begin(),
      channels.end());

    request_info.page_keywords_present =
      log_request.page_keywords_present;

    request_info.search_words = new Commons::StringHolder(
      log_request.search_words);
    request_info.page_keywords = new Commons::StringHolder(
      log_request.page_keywords);
    request_info.url_keywords = new Commons::StringHolder(
      log_request.url_keywords);
    request_info.fraud = log_request.fraud;
    request_info.search_engine_id = log_request.search_engine_id;

    if(log_request.search_engine_id)
    {
      HTTP::BrowserAddress referer;

      if(!common_info.referer.empty())
      {
        try
        {
          referer.url(std::string_view(common_info.referer));
        }
        catch(const eh::Exception&)
        {
          // ignore invalid referer value
        }
      }

      request_info.search_engine_host = referer.host().str();
    }
  }

  void
  CampaignManagerLogAdapter::fill_request_info_by_common_info_(
    CampaignManagerLogger::RequestInfo& request_info,
    const CampaignManagerCore::CommonAdRequest& common_info)
    /*throw(Exception)*/
  {
    request_info.user_status = static_cast<UserStatus>(common_info.user_status);
    request_info.user_id = common_info.user_id;
    request_info.request_id = common_info.request_id;
    request_info.referer = common_info.referer;
    request_info.urls.assign(common_info.urls.begin(), common_info.urls.end());

    if(!common_info.location.empty())
    {
      request_info.country_code = common_info.location[0].country;
    }

    request_info.random = common_info.random;
  }

  void
  CampaignManagerLogAdapter::fill_match_request_info(
    CampaignManagerLogger::MatchRequestInfo& result_match_request,
    const CampaignConfig* campaign_config,
    const CampaignManagerCore::MatchRequestInfo& match_request_info,
    const ChannelIdArray& geo_channels)
    /*throw(Exception)*/
  {
    result_match_request.user_id = match_request_info.user_id;
    result_match_request.household_id = match_request_info.household_id;
    result_match_request.time = match_request_info.request_time;
    result_match_request.match_info.colo_id = match_request_info.match_info.colo_id;

    result_match_request.match_info.channels.insert(
      match_request_info.match_info.channels.begin(),
      match_request_info.match_info.channels.end());
    std::copy(
      geo_channels.begin(),
      geo_channels.end(),
      std::inserter(
        result_match_request.match_info.channels,
        result_match_request.match_info.channels.begin()));

    convert_channel_ids_(
      result_match_request.match_info.triggered_page_channels,
      result_match_request.match_info.triggered_page_channels,
      result_match_request.match_info.page_triggers,
      match_request_info.match_info.pkw_channels);

    CampaignConfig::ColocationMap::const_iterator colo_it =
      match_request_info.match_info.colo_id <= 0 ? campaign_config->colocations.end() :
      campaign_config->colocations.find(match_request_info.match_info.colo_id);

    result_match_request.isp_offset =
      (colo_it == campaign_config->colocations.end()) ?
        Generics::Time::ZERO :
        colo_it->second->account->time_offset;
  }

  void
  CampaignManagerLogAdapter::fill_request_info(
    CampaignManagerLogger::RequestInfo& request_info,
    const CampaignConfig* campaign_config,
    const Colocation* colocation,
    const CampaignManagerCore::CommonAdRequest& common_info,
    const CampaignManagerCore::ContextAdRequest& context_info,
    const CampaignManagerCore::LogAdRequest* log_request,
    const ChannelIdHashSet* channels,
    bool is_ad_request,
    bool track_passback,
    const CampaignManagerCore::AdSlotContext& ad_slot_context)
    /*throw(Exception)*/
  {
    //static const char* FUN = "CampaignManagerLogAdapter::fill_request_info()";

    if(log_request && channels)
    {
      fill_request_info_by_profiling_(
        request_info,
        *log_request,
        *channels,
        common_info);

      request_info.is_ad_request = is_ad_request;
      request_info.disable_fraud_detection = log_request->disable_fraud_detection;
      request_info.track_passback = track_passback;
    }
    else
    {
      // TO CHECK
      request_info.is_ad_request = true;
      request_info.disable_fraud_detection = false;
      request_info.track_passback = false;
      request_info.search_engine_id = 0;
    }

    fill_request_info_by_common_info_(request_info, common_info);
    request_info.additional_info.assign(
      context_info.additional_info.begin(), context_info.additional_info.end());

    Generics::Time request_time = common_info.time;
    request_info.time = request_time;
    request_info.isp_time = request_time;

    if(colocation)
    {
      request_info.colo_id = colocation->colo_id;
      request_info.isp_time_offset = colocation->account->time_offset;
      request_info.isp_time = request_info.time + request_info.isp_time_offset;
    }
    else
    {
      request_info.colo_id = 0;
      request_info.isp_time = request_time;
    }

    if(ad_slot_context.request_blacklisted) // override user status
    {
      request_info.user_status = US_BLACKLISTED;
    }

    request_info.profile_referer = context_info.profile_referer;

    request_info.log_as_test = common_info.log_as_test | ad_slot_context.test_request;

    if(context_info.full_referer_hash)
    {
      request_info.full_referer_hash = context_info.full_referer_hash;
    }

    if (context_info.short_referer_hash)
    {
      request_info.short_referer_hash = context_info.short_referer_hash;
    }

    request_info.client_app.assign(context_info.client.begin(), context_info.client.end());
    request_info.client_app_version.assign(
      context_info.client_version.begin(), context_info.client_version.end());
    request_info.web_browser.assign(
      context_info.web_browser.begin(), context_info.web_browser.end());
    request_info.full_platform.assign(
      context_info.full_platform.begin(), context_info.full_platform.end());
    request_info.ip_hash.assign(context_info.ip_hash.begin(), context_info.ip_hash.end());

    if(!common_info.user_agent.empty())
    {
      request_info.user_agent = new Commons::StringHolder(common_info.user_agent);
    }

    request_info.platforms.clear();
    request_info.platforms.insert(
      context_info.platform_ids.begin(), context_info.platform_ids.end());

    if(campaign_config)
    {
      campaign_config->platform_channels->match(
        request_info.platform_channels,
        request_info.platforms);

      // fill request_info.last_platform_channel_id : device channel with great priority
      unsigned long cur_priority = 0;
      request_info.last_platform_channel_id = 0;

      for(ChannelIdSet::const_iterator pch_it = request_info.platform_channels.begin();
        pch_it != request_info.platform_channels.end();
        ++pch_it)
      {
        auto pr_it = campaign_config->platform_channel_priorities.find(*pch_it);
        if(pr_it != campaign_config->platform_channel_priorities.end())
        {
          if(request_info.last_platform_channel_id == 0 ||
            cur_priority < pr_it->second.priority)
          {
            cur_priority = pr_it->second.priority;
            request_info.last_platform_channel_id = *pch_it;
          }
        }
      }
    }

    request_info.geo_channels.assign(
      context_info.geo_channels.begin(), context_info.geo_channels.end());
  }

  void
  CampaignManagerLogAdapter::fill_ad_request_selection_info(
    CampaignManagerLogger::AdRequestSelectionInfo& ad_request_selection_info,
    const CampaignConfig* campaign_config,
    const Colocation* colocation,
    const CampaignManagerCore::CommonAdRequest& common_info,
    const CampaignManagerCore::ContextAdRequest& context_info,
    const ChannelIdHashSet* channels,
    const CampaignManagerCore::AdSlotRequest& ad_slot,
    const Tag* tag,
    const AdSelectionResult& ad_selection_result,
    const CampaignManagerCore::AdSlotContext& ad_slot_context,
    const CampaignManagerCore::AdSlotMinCpm& ad_slot_min_cpm,
    const Tag::SizeMap& tag_sizes,
    bool disable_impression_tracking)
    /*throw(Exception)*/
  {
    typedef std::vector<DataPricing> DataPricingVector;

    Generics::Time request_time = common_info.time;

    ad_request_selection_info.max_ads = 0;
    ad_request_selection_info.size_id = 0;
    ad_request_selection_info.min_text_ecpm =
      ad_selection_result.min_text_ecpm.integer<unsigned long>();
    ad_request_selection_info.text_campaigns = ad_selection_result.text_campaigns;
    ad_request_selection_info.min_no_adv_ecpm =
      ad_selection_result.min_no_adv_ecpm.integer<unsigned long>();
    ad_request_selection_info.auction_type = ad_selection_result.auction_type;
    ad_request_selection_info.cpm_threshold = RevenueDecimal::ZERO;
    ad_request_selection_info.walled_garden = ad_selection_result.walled_garden;
    ad_request_selection_info.household_based = ad_selection_result.household_based;
    // for RTB input tag_id is 0
    ad_request_selection_info.request_tag_id = ad_slot.tag_id ?
      ad_slot.tag_id : (tag ? tag->tag_id : 0);
    ad_request_selection_info.ext_tag_id = ad_slot.ext_tag_id;
    ad_request_selection_info.floor_cost = RevenueDecimal::div(
      ad_slot_min_cpm.min_pub_ecpm,
      ECPM_FACTOR);

    if(common_info.request_type == AR_NORMAL && context_info.page_load_id)
    {
      ad_request_selection_info.page_load_id = context_info.page_load_id;
    }

    if(ad_slot.up_expand_space >= 0)
    {
      ad_request_selection_info.tag_top_offset = ad_slot.up_expand_space;
    }

    if(ad_slot.left_expand_space >= 0)
    {
      ad_request_selection_info.tag_left_offset = ad_slot.left_expand_space;
    }

    if(ad_slot.tag_visibility >= 0 && ad_slot.tag_visibility <= 100)
    {
      ad_request_selection_info.tag_visibility = ad_slot.tag_visibility;
    }

    if(ad_slot.tag_predicted_viewability >= 0 && ad_slot.tag_predicted_viewability <= 100)
    {
      ad_request_selection_info.tag_predicted_viewability = ad_slot.tag_predicted_viewability;
    }
    else
    {
      ad_request_selection_info.tag_predicted_viewability = -1;
    }

    const Tag::TagPricing* no_imp_tag_pricing =
      tag->select_no_impression_tag_pricing(tag ?
        (!common_info.location.empty() ? common_info.location[0].country.c_str() : "") : 0);

    if(tag)
    {
      const Tag::TagPricing* tag_pricing = (!ad_selection_result.selected_campaigns.empty() ?
        tag->select_country_tag_pricing(
           !common_info.location.empty() ?
           common_info.location[0].country.c_str() : "") :
        no_imp_tag_pricing);

      ad_request_selection_info.min_no_adv_ecpm = std::max(
        tag_pricing ? tag_pricing->cpm : RevenueDecimal::ZERO,
        ad_slot_min_cpm.min_pub_ecpm_system).integer<unsigned long>();

      ad_request_selection_info.site_rate_id = tag_pricing ? tag_pricing->site_rate_id : 0;
      ad_request_selection_info.site_id = tag->site->site_id;
      ad_request_selection_info.pub_time = request_time + tag->site->account->time_offset;
      ad_request_selection_info.tag_id = tag->tag_id;
      ad_request_selection_info.pub_account_id = tag->site->account->account_id;
      // ADSC-10025: don't log stats for sizes blocked by placement channel.
      //   use filtered in Impl and passed tag_sizes instead tag->sizes below.
      for(Tag::SizeMap::const_iterator tag_size_it = tag_sizes.begin();
        tag_size_it != tag_sizes.end(); ++tag_size_it)
      {
        ad_request_selection_info.tag_sizes.insert(tag_size_it->second->size->protocol_name);
      }

      if(ad_selection_result.tag_size)
      {
        ad_request_selection_info.max_ads = ad_selection_result.tag_size->max_text_creatives;
        ad_request_selection_info.tag_size = ad_selection_result.tag_size->size->protocol_name;
        ad_request_selection_info.size_id = ad_selection_result.tag_size->size->size_id;
      }
      else if(!tag_sizes.empty())
      {
        // use max_ads by first size (ADSC-8728)
        ad_request_selection_info.max_ads = tag_sizes.begin()->second->max_text_creatives;

        // tag_size used only for ChannelPerformance and
        // and non actual here (actual only if ad selected)

        // don't log size_id into CreativeStat if it can't be
        // clearly defined
        ad_request_selection_info.size_id = tag_sizes.size() == 1 ?
          tag_sizes.begin()->second->size->size_id :
          0;
      }

      RevenueDecimal div_reminder;
      ad_request_selection_info.cpm_threshold = tag->site->account->currency->from_system_currency(
        RevenueDecimal::div(
          ad_selection_result.cpm_threshold,
          RevenueDecimal(false, 100, 0),
          div_reminder));
    }

    AdSelectionInfoList& ad_info_list = ad_request_selection_info.ad_selection_info_list;

    if(ad_selection_result.selected_campaigns.empty())
    {
      // process no selected ad request
      AdSelectionInfo ad_info;
      DataPricing data_pricing(0, 0);

      CampaignManagerLogAdapter::fill_ad_selection_info_(
        ad_info,
        data_pricing,
        campaign_config,
        colocation,
        common_info,
        context_info,
        channels,
        tag,
        0, // tag_pricing
        ad_selection_result,
        1,
        1,
        ad_slot_context);

      ad_info_list.push_back(ad_info);
    }
    else
    {
      unsigned long num_shown = ad_selection_result.selected_campaigns.size();
      DataPricingVector data_pricings(num_shown);

      unsigned idx = 0;

      for(CampaignSelectionDataList::const_iterator s_it =
          ad_selection_result.selected_campaigns.begin();
        s_it != ad_selection_result.selected_campaigns.end();
        ++s_it, ++idx)
      {
        const CampaignSelectionData* cs_data = &*s_it;
        const Tag::TagPricing* tag_pricing = 0;

        if (tag)
        {
          if(cs_data)
          {
            tag_pricing = tag->select_tag_pricing(
              !common_info.location.empty() ?
                common_info.location[0].country.c_str() : "",
              cs_data->campaign->ccg_type,
              cs_data->campaign->ccg_rate_type);
          }
          else
          {
            tag_pricing = no_imp_tag_pricing;
          }
        }

        DataPricing& dp = data_pricings[idx];
        dp = DataPricing(cs_data, tag_pricing);

        AdSelectionInfo ad_info;

        CampaignManagerLogAdapter::fill_ad_selection_info_(
          ad_info,
          dp,
          campaign_config,
          colocation,
          common_info,
          context_info,
          channels,
          tag,
          tag_pricing,
          ad_selection_result,
          num_shown,
          idx + 1,
          ad_slot_context);

        if (disable_impression_tracking)
        {
          ad_info.enabled_impression_tracking = false;
        }

        ad_info_list.push_back(ad_info);
      } // for (CampaignSelectionDataList)
    }
  }

  void
  CampaignManagerLogAdapter::fill_ad_selection_info_(
    CampaignManagerLogger::AdSelectionInfo& ad_info,
    DataPricing& data_pricing,
    const CampaignConfig* campaign_config,
    const Colocation* colocation,
    const CampaignManagerCore::CommonAdRequest& common_info,
    const CampaignManagerCore::ContextAdRequest& context_info,
    const ChannelIdHashSet* channels,
    const Tag* tag,
    const Tag::TagPricing* tag_pricing,
    const AdSelectionResult& ad_selection_result,
    unsigned long num_shown,
    unsigned long position,
    const CampaignManagerCore::AdSlotContext& ad_slot_context)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignManagerLogAdapter::fill_ad_selection_info_()";

    const CampaignSelectionData* cs_data = data_pricing.cs_data;

    try
    {
      ad_info.log_as_test = common_info.log_as_test | ad_slot_context.test_request;
      ad_info.num_shown = num_shown;
      ad_info.position = position;

      ad_info.ecpm = RevenueDecimal::ZERO;
      ad_info.ecpm_bid = RevenueDecimal::ZERO;
      ad_info.tag_delivery_threshold = TAG_DELIVERY_MAX;
      ad_info.currency_exchange_id = campaign_config->currency_exchange_id;

      assert(colocation);

      ad_info.isp_revenue.rate_id = colocation->colo_rate_id;
      const Currency* isp_currency = colocation->account->currency;
      ad_info.isp_currency_rate = isp_currency->rate;

      const Currency* campaign_currency = 0;

      if(cs_data)
      {
        const Generics::Time curr_time = common_info.time;
        campaign_currency = cs_data->campaign->account->currency;

        ad_info.adv_time = curr_time + cs_data->campaign->account->time_offset;
        ad_info.ad_selected = true;
        ad_info.request_id = cs_data->request_id;

        if(cs_data->campaign_keyword.in())
        {
          ad_info.ccg_keyword_id = cs_data->campaign_keyword->ccg_keyword_id;
          ad_info.keyword_channel_id = cs_data->campaign_keyword->channel_id;
        }
        else
        {
          ad_info.ccg_keyword_id = 0;
          ad_info.keyword_channel_id = 0;
        }

        if(cs_data->campaign->advertiser->use_self_budget())
        {
          ad_info.adv_account_id = cs_data->campaign->advertiser->account_id;
        }
        else
        {
          ad_info.adv_account_id = cs_data->campaign->advertiser->agency_account->account_id;
        }

        ad_info.advertiser_id = cs_data->campaign->advertiser->account_id;
        ad_info.text_campaign = cs_data->campaign->ccg_type == CT_TEXT;
        ad_info.ecpm_bid = cs_data->ecpm_bid;
        ad_info.ecpm = cs_data->ecpm; // max ecpm bid

        // external_ecpm_bid : will be used as bid_..._amount (SiteReferrerStats)
        if(ad_slot_context.pub_imp_revenue.present())
        {
          ad_info.external_ecpm_bid = RevenueDecimal::mul(
            *ad_slot_context.pub_imp_revenue,
            RevenueDecimal(false, 100, 0),
            Generics::DMR_FLOOR);
        }
        else if(ad_selection_result.auction_type == AT_RANDOM)
        {
          ad_info.external_ecpm_bid = (
            position == 1 ? tag->pub_max_random_cpm : RevenueDecimal::ZERO);
        }
        else
        {
          ad_info.external_ecpm_bid =
            tag->site->account->currency->from_system_currency(cs_data->ecpm_bid);
        }

        const RevenueDecimal orig_self_service_commission =
          cs_data->campaign->advertiser->get_self_service_commission();
        const RevenueDecimal adv_commission =
          cs_data->campaign->advertiser->adv_commission();

        ad_info.cc_id = cs_data->creative->ccid;
        ad_info.ccg_id = cs_data->campaign->campaign_id;
        ad_info.campaign_id = cs_data->campaign->campaign_group_id;
        ad_info.ctr_reset_id = cs_data->campaign->ctr_reset_id;
        ad_info.has_custom_actions = cs_data->campaign->has_custom_actions;
        ad_info.adv_commission = adv_commission;
        ad_info.self_service_commission = orig_self_service_commission;
        ad_info.pub_cost_coef = tag->cost_coef;
        ad_info.at_flags = cs_data->campaign->account->at_flags | (
          cs_data->campaign->ccg_rate_type == CR_MAXBID ?
          AccountTypeFlags::AGENCY_PROFIT_BY_PUB_AMOUNT : 0);

        const RevenueDecimal self_service_commission =
          orig_self_service_commission + colocation->revenue_share;

        if(ad_selection_result.ctr_calculation)
        {
          ad_info.ctr_algorithm_id =
            ad_selection_result.ctr_calculation->algorithm_id(
              cs_data->creative);

          ad_selection_result.ctr_calculation->create_context(
            ad_selection_result.tag_size)->get_ctr_details(
              ad_info.model_ctrs,
              cs_data->creative);
        }

        if(ad_selection_result.conv_rate_calculation)
        {
          ad_info.conv_rate_algorithm_id =
            ad_selection_result.conv_rate_calculation->algorithm_id(
              cs_data->creative);
        }

        assert(cs_data->ctr.is_nonnegative());
        ad_info.ctr = cs_data->ctr;
        assert(cs_data->conv_rate.is_nonnegative());
        ad_info.conv_rate = cs_data->conv_rate;
        ad_info.campaign_imps = cs_data->campaign_imps;

        fill_responded_channel_info_(ad_info, *cs_data, channels);
        ad_info.enabled_impression_tracking = !cs_data->count_impression;
        ad_info.enabled_notice = context_info.enabled_notice;
        ad_info.enabled_action_tracking = cs_data->campaign->track_actions();

        // adv fill revenue fields
        ad_info.adv_currency_rate = campaign_currency->rate;
        ad_info.adv_revenue.rate_id = cs_data->campaign->ccg_rate_id;
        ad_info.adv_revenue.request = RevenueDecimal::ZERO;
        ad_info.adv_revenue.impression = cs_data->campaign->imp_revenue;

        // pub fill revenue fields
        RevenueDecimal local_pub_revenue;

        if(common_info.request_type != AR_NORMAL)
        {
          // RTB
          local_pub_revenue = RevenueDecimal::div(
            tag->site->account->currency->from_system_currency(cs_data->ecpm_bid),
            ECPM_FACTOR);
        }
        else
        {
          RevenueDecimal full_pub_revenue = std::max(
            RevenueDecimal::div(
              ad_slot_context.pub_imp_revenue.present() ?
                RevenueDecimal::mul(
                  *ad_slot_context.pub_imp_revenue,
                  RevenueDecimal(false, 100, 0),
                  Generics::DMR_FLOOR) :
                (common_info.request_type != AR_NORMAL && ad_selection_result.auction_type == AT_RANDOM ?
                  tag->pub_max_random_cpm :
                 RevenueDecimal::ZERO),
              ECPM_FACTOR),
            tag_pricing->imp_revenue
            );

          RevenueDecimal div_reminder;
          local_pub_revenue = RevenueDecimal::div(
            full_pub_revenue,
            RevenueDecimal(false, num_shown, 0),
            div_reminder);

          if(position == 1)
          {
            local_pub_revenue += div_reminder;
          }
        }

        if(tag->cost_coef != RevenueDecimal::ZERO)
        {
          local_pub_revenue = RevenueDecimal::mul(
            local_pub_revenue,
            REVENUE_ONE + tag->cost_coef,
            Generics::DMR_FLOOR);
        }

        if(tag_pricing)
        {
          ad_info.pub_revenue.rate_id = tag_pricing->site_rate_id;
          ad_info.pub_commission = tag->site->account->commision;
          ad_info.pub_currency_rate = tag->site->account->currency->rate;

          ad_info.pub_revenue.impression = local_pub_revenue;
          ad_info.pub_comm_revenue = ad_info.pub_revenue;
          ad_info.pub_comm_revenue *= tag->site->account->commision;
          ad_info.pub_revenue -= ad_info.pub_comm_revenue;
        }
        else
        {
          ad_info.pub_revenue = AdSelectionInfo::Revenue(); // fill zero
          ad_info.pub_commission = RevenueDecimal::ZERO;
          ad_info.pub_currency_rate = RevenueDecimal::ZERO;
        }

        if(cs_data->selection_done)
        {
          // use full delivery coef if selection produced not here
          TagDeliveryMap::const_iterator tag_delivery_it =
            cs_data->campaign->exclude_tags.find(tag->tag_id);
          if(tag_delivery_it != cs_data->campaign->exclude_tags.end())
          {
            ad_info.tag_delivery_threshold = tag_delivery_it->second;
            assert(ad_info.tag_delivery_threshold);
          }
        }

        if(cs_data->campaign_keyword.in())
        {
          ad_info.adv_revenue.click = cs_data->actual_cpc;
        }
        else
        {
          ad_info.adv_revenue.click = cs_data->campaign->click_revenue;

          if(cs_data->campaign->channel.in() && channels)
          {
            // fill channel cpm
            ExpressionChannelList cmp_channels;
            ChannelIdHashSet simple_channels;
            simple_channels.insert(
              channels->begin(),
              channels->end());

            cs_data->campaign->channel->get_cmp_channels(cmp_channels, simple_channels);

            for(ExpressionChannelList::const_iterator cmp_ch_it = cmp_channels.begin();
                cmp_ch_it != cmp_channels.end(); ++cmp_ch_it)
            {
              const ChannelParams& ch_params = (*cmp_ch_it)->params();

              if(ch_params.common_params.in())
              {
                AccountMap::const_iterator acc_it =
                  campaign_config->accounts.find(ch_params.common_params->account_id);

                if(acc_it != campaign_config->accounts.end())
                {
                  const AccountDef* channel_account = acc_it->second;
                  CampaignManagerLogger::CMPChannel cmp_channel;
                  cmp_channel.channel_id = ch_params.channel_id;
                  cmp_channel.channel_rate_id = ch_params.cmp_params->channel_rate_id;

                  cmp_channel.imp_revenue = ch_params.cmp_params->imp_revenue;
                  cmp_channel.imp_sys_revenue = channel_account->currency->to_system_currency(
                    ch_params.cmp_params->imp_revenue);
                  cmp_channel.adv_imp_revenue = campaign_currency->convert(
                    channel_account->currency,
                    cmp_channel.imp_revenue);

                  cmp_channel.click_revenue = ch_params.cmp_params->click_revenue;
                  cmp_channel.click_sys_revenue = channel_account->currency->to_system_currency(
                    ch_params.cmp_params->click_revenue);
                  cmp_channel.adv_click_revenue = campaign_currency->convert(
                    channel_account->currency,
                    cmp_channel.click_revenue);

                  ad_info.cmp_channels.push_back(cmp_channel);
                }
              }
            }
          }
        }

        ad_info.adv_revenue.action = cs_data->campaign->action_revenue;
        ad_info.adv_comm_revenue = ad_info.adv_revenue;

        const Currency* pub_currency = tag->site->account->currency;
        ad_info.isp_revenue_share = colocation->revenue_share;

        if(!cs_data->campaign->account->agency_profit_by_pub_amount() &&
           cs_data->campaign->ccg_rate_type != CR_MAXBID)
        {
          // schema #1
          // adv_revenue pass as is
          // adv_comm_revenue (agency profit)
          Revenue ssc_pub_amount = ad_info.pub_revenue;
          ssc_pub_amount *= (REVENUE_ONE + self_service_commission);

          ad_info.adv_comm_revenue -= ssc_pub_amount.convert_currency(
            *pub_currency, *(cs_data->campaign->account->currency));

          // isp_revenue (subscription fee)
          ad_info.isp_revenue = ad_info.pub_revenue.convert_currency(
            *pub_currency, *isp_currency);
          ad_info.isp_revenue *= self_service_commission;
          ad_info.isp_revenue.rate_id = colocation->colo_rate_id;
        }
        else // agency_profit_by_pub_amount || CR_MAXBID
        {
          // schema #2
          // adv_revenue (advertiser budget spending)
          // pass here as is (will be corrected at pub cost change)
          // othewise required specific runtime budget recalculations on imp,click
          /*
          ad_info.adv_revenue = ad_info.pub_revenue.convert_currency(
            *pub_currency, *(cs_data->campaign->account->currency));
          ad_info.adv_revenue *= REVENUE_ONE + self_service_commission;
          ad_info.adv_revenue *= REVENUE_ONE + adv_commission;
          */

          // adv_comm_revenue (agency profit)
          ad_info.adv_comm_revenue = ad_info.pub_revenue.convert_currency(
            *pub_currency, *(cs_data->campaign->account->currency));
          ad_info.adv_comm_revenue *= adv_commission;
          ad_info.adv_comm_revenue *= (
            REVENUE_ONE + self_service_commission);

          // revert rate id after override
          ad_info.adv_revenue.rate_id = cs_data->campaign->ccg_rate_id;
          ad_info.adv_comm_revenue.rate_id = cs_data->campaign->ccg_rate_id;

          // isp_revenue (subscription fee)
          ad_info.isp_revenue = ad_info.pub_revenue.convert_currency(
            *pub_currency, *isp_currency);
          ad_info.isp_revenue *= self_service_commission;
          ad_info.isp_revenue.rate_id = colocation->colo_rate_id;
        }

        Revenue& adv_revenue_sys = data_pricing.adv_revenue_sys;
        adv_revenue_sys.impression = campaign_currency->to_system_currency(
          ad_info.adv_revenue.impression);
        adv_revenue_sys.click = campaign_currency->to_system_currency(
          ad_info.adv_revenue.click);
        adv_revenue_sys.action = campaign_currency->to_system_currency(
          ad_info.adv_revenue.action);

        if(cs_data->campaign->account->invoice_commision())
        {
          ad_info.adv_payable_comm_revenue = ad_info.adv_comm_revenue;
        }
      }
      else // !cs_data
      {
        ad_info.ad_selected = false;
        ad_info.ccg_keyword_id = 0;
        ad_info.keyword_channel_id = 0;
        ad_info.text_campaign = false;
        ad_info.ecpm = RevenueDecimal::ZERO;
        ad_info.ctr = RevenueDecimal::ZERO;
        ad_info.conv_rate = RevenueDecimal::ZERO;
        ad_info.adv_account_id = 0;
        ad_info.campaign_id = 0;
        ad_info.ccg_id = 0;
        ad_info.cc_id = 0;
        ad_info.adv_revenue.rate_id = 0;
        ad_info.adv_revenue.request = RevenueDecimal::ZERO;
        ad_info.adv_revenue.impression = RevenueDecimal::ZERO;
        ad_info.adv_revenue.click = RevenueDecimal::ZERO;
        ad_info.adv_revenue.action = RevenueDecimal::ZERO;
      }
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught(tid = " <<
        (tag ? tag->tag_id : 0) << ", ccid = " <<
        (cs_data ? cs_data->creative->ccid : 0) << "): " <<
        ex.what();
      throw Exception(ostr);
    }
  }
}
