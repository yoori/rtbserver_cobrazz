
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Uuid.hpp>
#include <String/StringManip.hpp>
#include <Generics/RandomSelect.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "CampaignManagerImpl.hpp"
#include "CampaignSelector.hpp"

namespace
{
  const char EQL[] = "*eql*";
  const String::SubString NATIVE_FORMAT("native");
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    void
    CampaignManagerImpl::get_channel_targeting_info_(
      CampaignSelectionData& select_params,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const Campaign* campaign_candidate,
      const CampaignKeyword* campaign_keyword,
      AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo*
        creative_debug_info)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::select_creative()";

      if(campaign_candidate->targeted())
      {
        ChannelIdHashSet simple_channels;
        CorbaAlgs::convert_sequence(request_params.channels, simple_channels);

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
              std::ostringstream responded_expression;

              if (campaign_candidate->stat_channel.in() &&
                  campaign_candidate->stat_channel->triggered_expression(
                    responded_expression,
                    simple_channels))
              {
                select_params.responded_expression = responded_expression.str();
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
        std::ostringstream responded_expression;
        responded_expression << select_params.campaign_keyword->channel_id;
        select_params.responded_expression = responded_expression.str();
      }

      if (creative_debug_info)
      {
        /* fill triggered expression in debug info */
        creative_debug_info->triggered_expression << select_params.responded_expression;

        if(campaign_candidate->channel.in())
        {
          std::ostringstream full_expr;
          print(full_expr, campaign_candidate->channel);
          creative_debug_info->full_expression << full_expr.str();
        }
      }
    }

    void
    CampaignManagerImpl::get_channel_targeting_info_(
      CampaignSelectionData& select_params,
      const Proto::RequestParams& request_params,
      const Campaign* campaign_candidate,
      const CampaignKeyword* campaign_keyword,
      Proto::CreativeSelectDebugInfo* creative_debug_info)
    {
      static const char* FUN = "CampaignManagerImpl::select_creative()";

      if(campaign_candidate->targeted())
      {
        ChannelIdHashSet simple_channels(
          std::begin(request_params.channels()),
          std::end(request_params.channels()));

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
              std::ostringstream responded_expression;

              if (campaign_candidate->stat_channel.in() &&
                  campaign_candidate->stat_channel->triggered_expression(
                    responded_expression,
                    simple_channels))
              {
                select_params.responded_expression = responded_expression.str();
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
        std::ostringstream responded_expression;
        responded_expression << select_params.campaign_keyword->channel_id;
        select_params.responded_expression = responded_expression.str();
      }

      if (creative_debug_info)
      {
        /* fill triggered expression in debug info */
        creative_debug_info->set_triggered_expression(select_params.responded_expression);

        if(campaign_candidate->channel.in())
        {
          std::ostringstream full_expr;
          print(full_expr, campaign_candidate->channel);
          creative_debug_info->set_full_expression(full_expr.str());
        }
      }
    }

    void
    CampaignManagerImpl::instantiate_creative_body_(
      const AdInstantiateType ad_instantiate_type,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const CampaignConfig* config,
      const Colocation* const colocation,
      const char* cr_size,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      CORBA::String_out& creative_body,
      CORBA::String_out& creative_url,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/
    {
      InstantiateParams instantiate_params(
        request_params.common_info.user_id,
        request_params.context_info.enabled_notice);
      instantiate_params.generate_pubpixel_accounts = true;
      instantiate_params.ext_tag_id = ext_tag_id;

      instantiate_params.video_width = ad_slot.video_width;
      instantiate_params.video_height = ad_slot.video_height;
      instantiate_params.publisher_site_id = request_params.publisher_site_id;
      instantiate_params.publisher_account_id = ad_slot_context.publisher_account_id;

      std::string inst_rule(
        request_params.common_info.request_type == AR_GOOGLE &&
          !ad_selection_result.selected_campaigns.empty() &&
            ad_selection_result.selected_campaigns.front().creative->https_safe_flag ?
              AdInstantiateRule::SECURE.str() : request_params.common_info.creative_instantiate_type.in());

      CreativeInstantiateRuleMap::iterator rule_it =
        creative_instantiate_.creative_rules.find(inst_rule);
      
      if(rule_it == creative_instantiate_.creative_rules.end())
      {
        Stream::Error ostr;
        ostr << __func__ << ": Cannot find creative instantiate rule with name: " << inst_rule;
        throw Exception(ostr);
      }

      const CreativeInstantiateRule& instantiate_info = rule_it->second;

      if ((request_params.fill_iurl) &&
        !ad_selection_result.selected_campaigns.empty())
      {
        // init iurl independent on instantiate type
        fill_iurl_(
          request_result_params.iurl,
          config,
          instantiate_info,
          request_params.common_info,
          ad_selection_result.selected_campaigns.front().creative,
          ad_selection_result.tag_size->size);
      }

      bool is_native = ad_slot.format == NATIVE_FORMAT;
      
      if(ad_instantiate_type == AIT_BODY || is_native)
      {
        instantiate_creative_(
          request_params.common_info,
          config,
          colocation,
          instantiate_params,
          ad_slot.format,
          ad_selection_result,
          request_result_params,
          creative_params_list,
          creative_body,
          ad_slot_context,
          &request_params.exclude_pubpixel_accounts);
      }
      else
      {
        std::string instantiate_url;

        init_instantiate_url_(
          instantiate_url,
          ad_instantiate_type,
          creative_params_list,
          request_result_params,
          config,
          ad_selection_result.tag,
          instantiate_params,
          instantiate_info,
          request_params.common_info,
          ad_selection_result,
          ad_slot_context,
          ad_slot.format,
          request_params.exclude_pubpixel_accounts,
          !request_params.context_info.enabled_notice);

        creative_url << instantiate_url; // REVIEW

        if(ad_instantiate_type == AIT_URL_PARAMS ||
           ad_instantiate_type == AIT_DATA_URL_PARAM ||
           ad_instantiate_type == AIT_DATA_PARAM_VALUE)
        {
          creative_body << instantiate_url;
        }
        else if(ad_instantiate_type == AIT_SCRIPT_WITH_URL ||
           ad_instantiate_type == AIT_IFRAME_WITH_URL)
        {
          // init post template
          instantiate_url_creative_(
            creative_body,
            request_result_params,
            ad_selection_result,
            instantiate_url,
            ad_instantiate_type);
        }

        // fill tracking mode
        bool fill_track_pixel = request_params.fill_track_pixel;
        bool track_impression = false;

        {
          const char* cr_format = ad_selection_result.selected_campaigns.begin()->
            creative->creative_format.c_str();

          // getting template
          CreativeTemplateKey key(
            cr_format,
            cr_size,
            ad_slot.format);

          CreativeTemplate creative_template_descr;
          Template* creative_template = config->creative_templates.get(
            key, creative_template_descr);

          if(!creative_template)
          {
            Stream::Error ostr;
            ostr << "Can't find creative template for (type='" <<
              cr_format <<
              "', size='" << cr_size <<
              "', app_format='" << ad_slot.format << "').";
            throw Exception(ostr);
          }

          track_impression = creative_template_descr.track_impressions;
        }

        for(CampaignSelectionDataList::iterator it =
          ad_selection_result.selected_campaigns.begin();
          it != ad_selection_result.selected_campaigns.end(); ++it)
        {
          it->track_impr = track_impression;
        }

        fill_track_pixel &= track_impression;

        // fill track pixel & notice
        if(request_params.context_info.enabled_notice || fill_track_pixel)
        {
          bool fill_notice_url = request_params.context_info.enabled_notice;

          if(fill_notice_url || fill_track_pixel)
          {
            // don't fill pub pixels:
            //   for yandex it can't be confirmed
            //   for other RTB's it is part of instantiate url
            fill_track_urls_(
              ad_selection_result,
              request_result_params,
              request_params.common_info,
              fill_track_pixel,
              InstantiateParams(
                request_params.common_info.user_id,
                fill_notice_url),
              instantiate_info,
              0 // consider_pub_pixel_accounts
              );
          }
        }

        // fill specific tokens (TODO: fill overlay_width, overlay_height here)
        if(request_params.common_info.request_type == AR_YANDEX)
        {
          init_yandex_tokens_(
            config,
            instantiate_info,
            request_result_params,
            request_params.common_info,
            ad_slot_context,
            ad_selection_result.selected_campaigns.front().creative);

          request_result_params.click_params = init_click_params0_( 
            ad_selection_result.selected_campaigns.front().request_id,
            colocation,
            ad_selection_result.selected_campaigns.front().creative,
            ad_selection_result.tag,
            ad_selection_result.tag_size,
            nullptr,
            ad_selection_result.selected_campaigns.front().ctr,
            instantiate_params,
            request_params.common_info,
            ad_slot_context);
        }
      }

      // ADSC-10918 Native ads
      if (is_native)
      {
        init_native_tokens_(
          config,
          instantiate_info,
          request_result_params,
          request_params.common_info,
          ad_slot,
          ad_slot_context,
          ad_selection_result.selected_campaigns.front().creative);
      }

      if(ad_instantiate_type == AIT_VIDEO_URL ||
         ad_instantiate_type == AIT_VIDEO_URL_IN_BODY ||
         ad_instantiate_type == AIT_VIDEO_NONSECURE_URL)
      {
        init_vast_tokens_(
          request_result_params,
          ad_selection_result.selected_campaigns.front().creative);
      }
    }

    void
    CampaignManagerImpl::instantiate_creative_body_(
      const AdInstantiateType ad_instantiate_type,
      const Proto::RequestParams& request_params,
      const CampaignConfig* config,
      const Colocation* const colocation,
      const char* cr_size,
      const Proto::AdSlotInfo& ad_slot,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      std::string& creative_body,
      std::string& creative_url,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id)
    {
      InstantiateParams instantiate_params(
        request_params.common_info().user_id(),
        request_params.context_info().enabled_notice());
      instantiate_params.generate_pubpixel_accounts = true;
      instantiate_params.ext_tag_id = ext_tag_id;

      instantiate_params.video_width = ad_slot.video_width();
      instantiate_params.video_height = ad_slot.video_height();
      instantiate_params.publisher_site_id = request_params.publisher_site_id();
      instantiate_params.publisher_account_id = ad_slot_context.publisher_account_id;

      std::string inst_rule(
        request_params.common_info().request_type() == AR_GOOGLE &&
          !ad_selection_result.selected_campaigns.empty() &&
            ad_selection_result.selected_campaigns.front().creative->https_safe_flag ?
              AdInstantiateRule::SECURE.str() : request_params.common_info().creative_instantiate_type());

      CreativeInstantiateRuleMap::iterator rule_it =
        creative_instantiate_.creative_rules.find(inst_rule);

      if(rule_it == creative_instantiate_.creative_rules.end())
      {
        Stream::Error ostr;
        ostr << __func__ << ": Cannot find creative instantiate rule with name: " << inst_rule;
        throw Exception(ostr);
      }

      const CreativeInstantiateRule& instantiate_info = rule_it->second;

      if ((request_params.fill_iurl()) &&
        !ad_selection_result.selected_campaigns.empty())
      {
        // init iurl independent on instantiate type
        fill_iurl_(
          request_result_params.iurl,
          config,
          instantiate_info,
          request_params.common_info(),
          ad_selection_result.selected_campaigns.front().creative,
          ad_selection_result.tag_size->size);
      }

      bool is_native = ad_slot.format() == NATIVE_FORMAT;

      if(ad_instantiate_type == AIT_BODY || is_native)
      {
        instantiate_creative_(
          request_params.common_info(),
          config,
          colocation,
          instantiate_params,
          ad_slot.format().c_str(),
          ad_selection_result,
          request_result_params,
          creative_params_list,
          creative_body,
          ad_slot_context,
          &request_params.exclude_pubpixel_accounts());
      }
      else
      {
        std::string instantiate_url;

        init_instantiate_url_(
          instantiate_url,
          ad_instantiate_type,
          creative_params_list,
          request_result_params,
          config,
          ad_selection_result.tag,
          instantiate_params,
          instantiate_info,
          request_params.common_info(),
          ad_selection_result,
          ad_slot_context,
          ad_slot.format().c_str(),
          request_params.exclude_pubpixel_accounts(),
          !request_params.context_info().enabled_notice());

        creative_url = instantiate_url; // REVIEW

        if(ad_instantiate_type == AIT_URL_PARAMS ||
           ad_instantiate_type == AIT_DATA_URL_PARAM ||
           ad_instantiate_type == AIT_DATA_PARAM_VALUE)
        {
          creative_body = instantiate_url;
        }
        else if(ad_instantiate_type == AIT_SCRIPT_WITH_URL ||
           ad_instantiate_type == AIT_IFRAME_WITH_URL)
        {
          // init post template
          instantiate_url_creative_(
            creative_body,
            request_result_params,
            ad_selection_result,
            instantiate_url,
            ad_instantiate_type);
        }

        // fill tracking mode
        bool fill_track_pixel = request_params.fill_track_pixel();
        bool track_impression = false;

        {
          const char* cr_format = ad_selection_result.selected_campaigns.begin()->
            creative->creative_format.c_str();

          // getting template
          CreativeTemplateKey key(
            cr_format,
            cr_size,
            ad_slot.format().c_str());

          CreativeTemplate creative_template_descr;
          Template* creative_template = config->creative_templates.get(
            key, creative_template_descr);

          if(!creative_template)
          {
            Stream::Error ostr;
            ostr << "Can't find creative template for (type='" <<
              cr_format <<
              "', size='" << cr_size <<
              "', app_format='" << ad_slot.format() << "').";
            throw Exception(ostr);
          }

          track_impression = creative_template_descr.track_impressions;
        }

        for(CampaignSelectionDataList::iterator it =
          ad_selection_result.selected_campaigns.begin();
          it != ad_selection_result.selected_campaigns.end(); ++it)
        {
          it->track_impr = track_impression;
        }

        fill_track_pixel &= track_impression;

        // fill track pixel & notice
        if(request_params.context_info().enabled_notice() || fill_track_pixel)
        {
          bool fill_notice_url = request_params.context_info().enabled_notice();

          if(fill_notice_url || fill_track_pixel)
          {
            // don't fill pub pixels:
            //   for yandex it can't be confirmed
            //   for other RTB's it is part of instantiate url
            fill_track_urls_(
              ad_selection_result,
              request_result_params,
              request_params.common_info(),
              fill_track_pixel,
              InstantiateParams(
                request_params.common_info().user_id(),
                fill_notice_url),
              instantiate_info,
              0 // consider_pub_pixel_accounts
              );
          }
        }

        // fill specific tokens (TODO: fill overlay_width, overlay_height here)
        if(request_params.common_info().request_type() == AR_YANDEX)
        {
          init_yandex_tokens_(
            config,
            instantiate_info,
            request_result_params,
            request_params.common_info(),
            ad_slot_context,
            ad_selection_result.selected_campaigns.front().creative);

          request_result_params.click_params = init_click_params0_(
            ad_selection_result.selected_campaigns.front().request_id,
            colocation,
            ad_selection_result.selected_campaigns.front().creative,
            ad_selection_result.tag,
            ad_selection_result.tag_size,
            nullptr,
            ad_selection_result.selected_campaigns.front().ctr,
            instantiate_params,
            request_params.common_info(),
            ad_slot_context);
        }
      }

      // ADSC-10918 Native ads
      if (is_native)
      {
        init_native_tokens_(
          config,
          instantiate_info,
          request_result_params,
          request_params.common_info(),
          ad_slot,
          ad_slot_context,
          ad_selection_result.selected_campaigns.front().creative);
      }

      if(ad_instantiate_type == AIT_VIDEO_URL ||
         ad_instantiate_type == AIT_VIDEO_URL_IN_BODY ||
         ad_instantiate_type == AIT_VIDEO_NONSECURE_URL)
      {
        init_vast_tokens_(
          request_result_params,
          ad_selection_result.selected_campaigns.front().creative);
      }
    }

    bool
    CampaignManagerImpl::instantiate_display_creative(
      const CampaignConfig* config,
      const Colocation* colocation,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      const CampaignSelector::WeightedCampaign& weighted_campaign,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParams& creative_params,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
      CORBA::String_out creative_body,
      CORBA::String_out creative_url,
      AdSlotContext& ad_slot_context)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_display_creative()";

      assert(weighted_campaign.tag_size);

      const Campaign* campaign_candidate = weighted_campaign.campaign;
      const Creative* creative_candidate = weighted_campaign.creative;

      assert(campaign_candidate);
      assert(creative_candidate);

      AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo*
        creative_debug_info = 0;
      
      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->selected_creatives.length(1);
        creative_debug_info = &ad_slot_debug_info->selected_creatives[0];
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
      get_channel_targeting_info_(
        select_params,
        request_params,
        campaign_candidate,
        0, // campaign keyword
        creative_debug_info);

      ad_selection_result.selected_campaigns.push_back(select_params);

      // instantiating creative
      try
      {
        CreativeParamsList creative_params_list;

        AdInstantiateType ad_instantiate_type =
          static_cast<AdInstantiateType>(request_params.ad_instantiate_type);

        instantiate_creative_body_(
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
        ad_selection_result.selected_campaigns.front().click_url =
          upd_creative_params.click_url;

        if(ad_slot_debug_info)
        {
          ad_slot_debug_info->site_rate_id =
            ad_selection_result.tag_pricing ? ad_selection_result.tag_pricing->site_rate_id : 0;
        }
        
        if(creative_debug_info)
        {
          if(campaign_candidate->track_actions())
          {
            std::ostringstream action_adv_url;
            action_adv_url <<
              upd_creative_params.action_adv_url <<
              "/cid" << EQL << campaign_candidate->campaign_id;
            creative_debug_info->action_adv_url << action_adv_url.str();
          }
        }

        ad_selection_result.selected_campaigns.front().campaign =
          campaign_candidate;
        ad_selection_result.selected_campaigns.front().creative =
          creative_candidate;
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
    CampaignManagerImpl::instantiate_display_creative(
      const CampaignConfig* config,
      const Colocation* colocation,
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot,
      const CampaignSelector::WeightedCampaign& weighted_campaign,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParams& creative_params,
      Proto::AdSlotDebugInfo* ad_slot_debug_info,
      std::string& creative_body,
      std::string& creative_url,
      AdSlotContext& ad_slot_context)
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_display_creative()";

      assert(weighted_campaign.tag_size);

      const Campaign* campaign_candidate = weighted_campaign.campaign;
      const Creative* creative_candidate = weighted_campaign.creative;

      assert(campaign_candidate);
      assert(creative_candidate);

      Proto::CreativeSelectDebugInfo* creative_debug_info = nullptr;
      if(ad_slot_debug_info)
      {
        creative_debug_info = ad_slot_debug_info->add_selected_creatives();
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
      get_channel_targeting_info_(
        select_params,
        request_params,
        campaign_candidate,
        0, // campaign keyword
        creative_debug_info);

      ad_selection_result.selected_campaigns.push_back(select_params);

      // instantiating creative
      try
      {
        CreativeParamsList creative_params_list;

        AdInstantiateType ad_instantiate_type =
          static_cast<AdInstantiateType>(request_params.ad_instantiate_type());

        instantiate_creative_body_(
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
          String::SubString(ad_slot.ext_tag_id()));

        assert(!creative_params_list.empty());

        CreativeParams& upd_creative_params = *creative_params_list.begin();
        ad_selection_result.selected_campaigns.front().click_url =
          upd_creative_params.click_url;

        if(ad_slot_debug_info)
        {
          ad_slot_debug_info->set_site_rate_id(
            ad_selection_result.tag_pricing ? ad_selection_result.tag_pricing->site_rate_id : 0);
        }

        if(creative_debug_info)
        {
          if(campaign_candidate->track_actions())
          {
            std::ostringstream action_adv_url;
            action_adv_url <<
              upd_creative_params.action_adv_url <<
              "/cid" << EQL << campaign_candidate->campaign_id;
            creative_debug_info->set_action_adv_url(action_adv_url.str());
          }
        }

        ad_selection_result.selected_campaigns.front().campaign =
          campaign_candidate;
        ad_selection_result.selected_campaigns.front().creative =
          creative_candidate;
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
    CampaignManagerImpl::instantiate_text_creatives(
      const CampaignConfig* config,
      const Colocation* const colocation,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      const CampaignSelector::WeightedCampaignKeywordList& campaign_keywords,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
      CORBA::String_out creative_body,
      CORBA::String_out creative_url,
      AdSlotContext& ad_slot_context)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_text_creatives()";

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->selected_creatives.length(campaign_keywords.size());
      }

      CORBA::ULong i = 0;
      
      for(CampaignSelector::WeightedCampaignKeywordList::
            const_iterator kw_it = campaign_keywords.begin();
          kw_it != campaign_keywords.end();
          ++kw_it)
      {
        CampaignSelectionData select_params;

        const Campaign* campaign_candidate = kw_it->campaign;
        const Creative* creative_candidate = kw_it->creative;

        assert(creative_candidate);

        select_params.campaign = campaign_candidate;
        select_params.creative = creative_candidate;
        select_params.campaign_keyword = kw_it->campaign_keyword;

        AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo*
          creative_debug_info = 0;

        if(ad_slot_debug_info)
        {
          creative_debug_info = &ad_slot_debug_info->selected_creatives[i++];
        }

        select_params.ecpm_bid = kw_it->actual_ecpm;
        select_params.ecpm = kw_it->ecpm;
        select_params.ctr = kw_it->ctr;
        select_params.conv_rate = kw_it->conv_rate;

        get_channel_targeting_info_(
          select_params,
          request_params,
          campaign_candidate,
          kw_it->campaign_keyword,
          creative_debug_info);

        select_params.actual_cpc = kw_it->actual_cpc;
        select_params.track_impr = true;
        select_params.request_id = Commons::RequestId::create_random_based();

        ad_selection_result.selected_campaigns.push_back(select_params);
      }

      try
      {
        AdInstantiateType ad_instantiate_type =
          static_cast<AdInstantiateType>(request_params.ad_instantiate_type);

        instantiate_creative_body_(
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
          assert(ad_selection_result.selected_campaigns.size() ==
            creative_params_list.size());

          CampaignSelectionDataList::iterator select_params_it =
            ad_selection_result.selected_campaigns.begin();

          CORBA::ULong i = 0;
          
          for(CreativeParamsList::iterator creative_params_it =
                creative_params_list.begin();
              creative_params_it != creative_params_list.end();
              ++creative_params_it, ++select_params_it, ++i)
          {
            const Campaign* campaign_candidate = select_params_it->campaign;
            const CreativeParams& creative_params = *creative_params_it;
            
            AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo&
              creative_debug_info = ad_slot_debug_info->selected_creatives[i];

            select_params_it->click_url = creative_params.click_url;

            /*
            creative_debug_info.click_url <<
              creative_params.click_url;
            */

            if(campaign_candidate->track_actions())
            {
              std::ostringstream action_adv_url;
              action_adv_url <<
                creative_params.action_adv_url <<
                "/cid" << EQL << campaign_candidate->campaign_id;
              creative_debug_info.action_adv_url << action_adv_url.str();
            }
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

    bool
    CampaignManagerImpl::instantiate_text_creatives(
      const CampaignConfig* config,
      const Colocation* const colocation,
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot,
      const CampaignSelector::WeightedCampaignKeywordList& campaign_keywords,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      Proto::AdSlotDebugInfo* ad_slot_debug_info,
      std::string& creative_body,
      std::string& creative_url,
      AdSlotContext& ad_slot_context)
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_text_creatives()";

      google::protobuf::RepeatedPtrField<Proto::CreativeSelectDebugInfo>* selected_creatives_proto = nullptr;
      if(ad_slot_debug_info)
      {
        selected_creatives_proto = ad_slot_debug_info->mutable_selected_creatives();
        selected_creatives_proto->Reserve(campaign_keywords.size());
      }

      for(CampaignSelector::WeightedCampaignKeywordList::
          const_iterator kw_it = campaign_keywords.begin();
          kw_it != campaign_keywords.end();
          ++kw_it)
      {
        CampaignSelectionData select_params;

        const Campaign* campaign_candidate = kw_it->campaign;
        const Creative* creative_candidate = kw_it->creative;

        assert(creative_candidate);

        select_params.campaign = campaign_candidate;
        select_params.creative = creative_candidate;
        select_params.campaign_keyword = kw_it->campaign_keyword;

        Proto::CreativeSelectDebugInfo* creative_debug_info = nullptr;
        if(ad_slot_debug_info)
        {
          creative_debug_info = selected_creatives_proto->Add();
        }

        select_params.ecpm_bid = kw_it->actual_ecpm;
        select_params.ecpm = kw_it->ecpm;
        select_params.ctr = kw_it->ctr;
        select_params.conv_rate = kw_it->conv_rate;

        get_channel_targeting_info_(
          select_params,
          request_params,
          campaign_candidate,
          kw_it->campaign_keyword,
          creative_debug_info);

        select_params.actual_cpc = kw_it->actual_cpc;
        select_params.track_impr = true;
        select_params.request_id = Commons::RequestId::create_random_based();

        ad_selection_result.selected_campaigns.push_back(select_params);
      }

      try
      {
        AdInstantiateType ad_instantiate_type =
          static_cast<AdInstantiateType>(request_params.ad_instantiate_type());

        instantiate_creative_body_(
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
          String::SubString(ad_slot.ext_tag_id()));

        if(ad_slot_debug_info)
        {
          assert(ad_selection_result.selected_campaigns.size() ==
                 creative_params_list.size());

          auto select_params_it = ad_selection_result.selected_campaigns.begin();
          std::uint32_t i = 0;
          for(auto creative_params_it = creative_params_list.begin();
              creative_params_it != creative_params_list.end();
              ++creative_params_it, ++select_params_it, ++i)
          {
            const Campaign* campaign_candidate = select_params_it->campaign;
            const CreativeParams& creative_params = *creative_params_it;

            select_params_it->click_url = creative_params.click_url;

            Proto::CreativeSelectDebugInfo* creative_debug_info =
              ad_slot_debug_info->mutable_selected_creatives(i);
            if(campaign_candidate->track_actions())
            {
              std::ostringstream action_adv_url;
              action_adv_url <<
                creative_params.action_adv_url <<
                "/cid" << EQL << campaign_candidate->campaign_id;
              creative_debug_info->set_action_adv_url(action_adv_url.str());
            }
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
  } // namespace CampaignSvcs
} // namespace AdServer

