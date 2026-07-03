
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Uuid.hpp>
#include <String/StringManip.hpp>
#include <Generics/RandomSelect.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "CampaignManagerCore.hpp"
#include "CreativeInstantiator.hpp"
#include "CampaignSelector.hpp"

namespace
{
  const char EQL[] = "*eql*";

  AdServer::CampaignSvcs::CreativeInstantiator::Config
  make_creative_instantiator_config(
    const AdServer::CampaignSvcs::CampaignManagerCore::CampaignManagerConfig&
      config)
  {
    AdServer::CampaignSvcs::CreativeInstantiator::Config result;
    result.service_index = config.service_index().c_str();
    result.post_instantiate_script_mime_format =
      config.Creative().post_instantiate_script_mime_format();
    result.post_instantiate_iframe_mime_format =
      config.Creative().post_instantiate_iframe_mime_format();
    result.post_instantiate_script_template_file =
      config.Creative().post_instantiate_script_template_file();
    result.post_instantiate_iframe_template_file =
      config.Creative().post_instantiate_iframe_template_file();
    result.instantiate_track_html_file =
      config.Creative().instantiate_track_html_file();
    return result;
  }
}

namespace AdServer::CampaignSvcs
{
  void
  CampaignManagerCore::get_channel_targeting_info_(
    CampaignSelectionData& select_params,
    const ChannelIdHashSet& simple_channels,
    const Campaign* campaign_candidate,
    const CampaignKeyword* campaign_keyword)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "CampaignManagerCore::select_creative()";

    if(campaign_candidate->targeted())
    {
      try
      {
        select_params.responded_channels.clear();

        if(campaign_candidate->channel.in())
        {
          ChannelIdSet responded_channels;
          campaign_candidate->channel->triggered_named_channels(
            responded_channels,
            simple_channels);

          std::copy(responded_channels.begin(),
            responded_channels.end(),
            std::back_inserter(select_params.responded_channels));

          if(!campaign_keyword)
          {
            std::string responded_expression;

            if (campaign_candidate->stat_channel.in() &&
              campaign_candidate->stat_channel->triggered_expression(
                responded_expression,
                simple_channels))
            {
              select_params.responded_expression = std::move(responded_expression);
            }
          }
        }
      }
      catch (const ExpressionChannelBase::Exception& e)
      {
        logger_->sstream(Logging::Logger::WARNING,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-186") <<
          FUN << ": Caught ExpressionChannelBase::Exception while triing "
          "to get_triggered_channel_info"
          " (cmpid: " << campaign_candidate->campaign_id << "). "
          "responded_expression and responded_channels "
          "will be left empty. : " << e.what();
      }
    }

    if(campaign_keyword)
    {
      select_params.responded_channels.push_back(
        select_params.campaign_keyword->channel_id);
      select_params.responded_expression =
        std::to_string(select_params.campaign_keyword->channel_id);
    }
  }

  bool
  CampaignManagerCore::instantiate_display_creative(
    const CampaignConfig* config,
    const Colocation* colocation,
    const CreativeRequestInfo& request_params,
    const TraceAdSlotInfo& ad_slot,
    const CampaignSelector::WeightedCampaign& weighted_campaign,
    AdSelectionResult& ad_selection_result,
    RequestResultParams& request_result_params,
    CreativeParams& creative_params,
    AdSlotDebugInfo* ad_slot_debug_info,
    std::string& creative_body,
    std::string& creative_url,
    AdSlotContext& ad_slot_context)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "CampaignManagerCore::instantiate_display_creative()";

    assert(weighted_campaign.tag_size);

    const Campaign* campaign_candidate = weighted_campaign.campaign;
    const Creative* creative_candidate = weighted_campaign.creative;

    assert(campaign_candidate);
    assert(creative_candidate);

    if(ad_slot_debug_info)
    {
      ad_slot_debug_info->selected_creatives.resize(1);
    }

    ad_selection_result.tag = weighted_campaign.tag;
    ad_selection_result.tag_size = weighted_campaign.tag_size;
    ad_selection_result.tag_pricing = weighted_campaign.tag_pricing;

    CampaignSelectionData select_params;
    select_params.campaign = campaign_candidate;
    select_params.creative = creative_candidate;
    select_params.ecpm_bid = weighted_campaign.ecpm;
    select_params.ecpm = weighted_campaign.ecpm;
    select_params.ctr = weighted_campaign.ctr;
    select_params.conv_rate = weighted_campaign.conv_rate;
    select_params.request_id = Commons::RequestId::create_random_based();

    // Find responded expression and responded channels
    ChannelIdHashSet simple_channels(
      request_params.channels.begin(),
      request_params.channels.end());
    get_channel_targeting_info_(
      select_params,
      simple_channels,
      campaign_candidate,
      0); // campaign keyword

    ad_selection_result.selected_campaigns.push_back(select_params);

    // instantiating creative
    try
    {
      CreativeParamsList creative_params_list;

      AdInstantiateType ad_instantiate_type =
        static_cast<AdInstantiateType>(request_params.ad_instantiate_type);

      CreativeInstantiator creative_instantiator(
        make_creative_instantiator_config(campaign_manager_config_),
        creative_instantiate_,
        passback_templates_,
        token_to_parameters_,
        ip_crypter_,
        rid_signer_,
        logger_,
        country_whitelist_);
      creative_instantiator.instantiate_creative_body(
        ad_instantiate_type,
        request_params,
        config,
        colocation,
        weighted_campaign.tag_size->size->protocol_name.c_str(),
        ad_slot,
        ad_selection_result,
        request_result_params,
        creative_params_list,
        creative_body,
        creative_url,
        ad_slot_context,
        String::SubString(ad_slot.ext_tag_id));

      assert(!creative_params_list.empty());

      CreativeParams& upd_creative_params = *creative_params_list.begin();
      ad_selection_result.selected_campaigns.front().click_url = upd_creative_params.click_url;

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->site_rate_id =
          ad_selection_result.tag_pricing ? ad_selection_result.tag_pricing->site_rate_id : 0;
      }

      ad_selection_result.selected_campaigns.front().campaign = campaign_candidate;
      ad_selection_result.selected_campaigns.front().creative = creative_candidate;
      creative_params = *creative_params_list.begin();

      return true;
    }
    catch(const CreativeTemplateProblem& ex)
    {
      logger_->sstream(Logging::Logger::ERROR,
        Aspect::TRAFFICKING_PROBLEM,
        "ADS-TF-7") <<
        FUN << ": Can't instantiate creative ccid: " <<
        creative_candidate->ccid << ". Caught CreativeTemplateProblem: " <<
        ex.what();
    }
    catch(const CreativeInstantiateProblem& ex)
    {
      logger_->sstream(Logging::Logger::ERROR,
        Aspect::TRAFFICKING_PROBLEM,
        "ADS-TF-1000") <<
        FUN << ": Can't instantiate creative ccid: " <<
        creative_candidate->ccid << ". Caught CreativeInstantiateProblem: " <<
        ex.what();
    }

    return false;
  }

  bool
  CampaignManagerCore::instantiate_text_creatives(
    const CampaignConfig* config,
    const Colocation* const colocation,
    const CreativeRequestInfo& request_params,
    const TraceAdSlotInfo& ad_slot,
    const CampaignSelector::WeightedCampaignKeywordList& campaign_keywords,
    AdSelectionResult& ad_selection_result,
    RequestResultParams& request_result_params,
    CreativeParamsList& creative_params_list,
    AdSlotDebugInfo* ad_slot_debug_info,
    std::string& creative_body,
    std::string& creative_url,
    AdSlotContext& ad_slot_context)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "CampaignManagerCore::instantiate_text_creatives()";

    if(ad_slot_debug_info)
    {
      ad_slot_debug_info->selected_creatives.resize(campaign_keywords.size());
    }

    for(CampaignSelector::WeightedCampaignKeywordList::const_iterator kw_it =
        campaign_keywords.begin();
      kw_it != campaign_keywords.end(); ++kw_it)
    {
      CampaignSelectionData select_params;

      const Campaign* campaign_candidate = kw_it->campaign;
      const Creative* creative_candidate = kw_it->creative;

      assert(creative_candidate);

      select_params.campaign = campaign_candidate;
      select_params.creative = creative_candidate;
      select_params.campaign_keyword = kw_it->campaign_keyword;

      select_params.ecpm_bid = kw_it->actual_ecpm;
      select_params.ecpm = kw_it->ecpm;
      select_params.ctr = kw_it->ctr;
      select_params.conv_rate = kw_it->conv_rate;

      ChannelIdHashSet simple_channels(
        request_params.channels.begin(),
        request_params.channels.end());
      get_channel_targeting_info_(
        select_params,
        simple_channels,
        campaign_candidate,
        kw_it->campaign_keyword);

      select_params.actual_cpc = kw_it->actual_cpc;
      select_params.track_impr = true;
      select_params.request_id = Commons::RequestId::create_random_based();

      ad_selection_result.selected_campaigns.push_back(select_params);
    }

    try
    {
      AdInstantiateType ad_instantiate_type =
        static_cast<AdInstantiateType>(request_params.ad_instantiate_type);

      CreativeInstantiator creative_instantiator(
        make_creative_instantiator_config(campaign_manager_config_),
        creative_instantiate_,
        passback_templates_,
        token_to_parameters_,
        ip_crypter_,
        rid_signer_,
        logger_,
        country_whitelist_);
      creative_instantiator.instantiate_creative_body(
        ad_instantiate_type,
        request_params,
        config,
        colocation,
        ad_selection_result.tag_size->size->protocol_name.c_str(),
        ad_slot,
        ad_selection_result,
        request_result_params,
        creative_params_list,
        creative_body,
        creative_url,
        ad_slot_context,
        String::SubString(ad_slot.ext_tag_id));

      if(ad_slot_debug_info)
      {
        assert(ad_selection_result.selected_campaigns.size() == creative_params_list.size());

        CampaignSelectionDataList::iterator select_params_it =
          ad_selection_result.selected_campaigns.begin();

        CORBA::ULong i = 0;

        for(CreativeParamsList::iterator creative_params_it = creative_params_list.begin();
          creative_params_it != creative_params_list.end();
          ++creative_params_it, ++select_params_it, ++i)
        {
          const CreativeParams& creative_params = *creative_params_it;

          select_params_it->click_url = creative_params.click_url;
        }
      }

      return true;
    }
    catch(const CreativeInstantiateProblem& ex)
    {
      logger_->sstream(Logging::Logger::WARNING,
        Aspect::TRAFFICKING_PROBLEM,
        "ADS-TF-1001") <<
        FUN << ": Can't instantiate text creative. "
        "Caught CreativeInstantiateProblem: " <<
        ex.what();
    }

    return false;
  }
} // namespace AdServer::CampaignSvcs
