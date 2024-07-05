#include <iostream>
#include <optional>

#include <HTTP/UrlAddress.hpp>
#include <Generics/Rand.hpp>

#include <Commons/Algs.hpp>
#include "CampaignIndex.hpp"

namespace
{
  const unsigned long INDEXING_PROGRESS_PRECISSION = 10;

  std::string
  campaign_flags_to_str(unsigned long flags) /*throw(eh::Exception)*/
  {
    using namespace AdServer::CampaignSvcs;
    std::string str_flags;

    if (flags & CampaignFlags::US_NONE)
    {
      str_flags = "NONE";
    }
    else
    {
      if (flags & CampaignFlags::US_UNDEFINED)
      {
        str_flags = "UNDEFINED";
      }

      if (flags & CampaignFlags::US_OPTOUT)
      {
        if (!str_flags.empty())
        {
          str_flags += '|';
        }

        str_flags += "OPTOUT";
      }

      if (flags & CampaignFlags::US_OPTIN)
      {
        if (!str_flags.empty())
        {
          str_flags += '|';
        }

        str_flags += "OPTIN";
      }
    }

    return str_flags;
  }
  
  bool
  match(
    unsigned long campaign_flags,
    AdServer::CampaignSvcs::UserStatus st) noexcept
  {
    using namespace AdServer::CampaignSvcs;

    return
      (((campaign_flags & CampaignFlags::US_OPTIN) && (st == US_OPTIN)) ||
       ((campaign_flags & CampaignFlags::US_OPTOUT) &&
           (st == US_OPTOUT || st == US_EXTERNALPROBE || st == US_NOEXTERNALID)) ||
       ((campaign_flags & CampaignFlags::US_UNDEFINED) &&
           (st == US_UNDEFINED || st == US_PROBE)));
  }
}

namespace Aspect
{
  const char CAMPAIGN_SELECTION_INDEX[] = "CampaignSelectionIndex";
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    /** campaign index functions */
    CampaignIndex::CampaignIndex(
      const CampaignConfig* campaign_config,
      Logging::Logger* logger)
      noexcept
      : logger_(ReferenceCounting::add_ref(logger)),
        campaign_config_(ReferenceCounting::add_ref(campaign_config))
    {}

    CampaignIndex::CampaignIndex(
      const CampaignIndex& init,
      Logging::Logger* logger)
      noexcept
      : logger_(ReferenceCounting::add_ref(logger)),
        campaign_config_(ReferenceCounting::add_ref(init.campaign_config_))
    {}
    
    /** campaign index functions */
    bool CampaignIndex::index_campaigns(
      IndexingProgress* indexing_progress,
      Generics::ActiveObject* interrupter)
      /*throw(eh::Exception)*/
    {
      if(indexing_progress)
      {
        IndexingProgress::SyncPolicy::WriteGuard guard(indexing_progress->lock);
        indexing_progress->common_campaign_count = campaign_config_->campaigns.size();
      }

      /* fill main index */
      cell_holder_ = new CampaignSelectionCellListHolder();
      campaign_cell_holder_ = new CampaignCellListHolder();

      std::set<unsigned long> progress_campaigns;
      unsigned long i = 0;

      Generics::Timer timer;
      timer.start();

      for(CampaignConfig::CampaignMap::const_iterator cmp_it =
            campaign_config_->campaigns.begin();
          cmp_it != campaign_config_->campaigns.end(); ++cmp_it, ++i)
      {
        if(indexing_progress && i % INDEXING_PROGRESS_PRECISSION == 0)
        {
          if(interrupter && !interrupter->active())
          {
            return false;
          }

          IndexingProgress::SyncPolicy::WriteGuard guard(indexing_progress->lock);
          indexing_progress->loaded_campaign_count = i;
        }

        // check active outside of index_campaign for correct working of selection trace
        // for inactive campaigns
        if(cmp_it->second->account->is_active() &&
          cmp_it->second->is_active())
        {
          index_campaign(cmp_it->second);
        }

        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          progress_campaigns.insert(cmp_it->first);

          if(i % INDEXING_PROGRESS_PRECISSION == 0)
          {
            Stream::Dynamic ostr(4096);

            ostr << "Campaign index constructed for " << i << "/"
              << campaign_config_->campaigns.size() << " campaigns: ";

            for(std::set<unsigned long>::const_iterator pit =
                  progress_campaigns.begin();
                pit != progress_campaigns.end();
                ++pit)
            {
              ostr << " " << *pit;
            }
            
            logger_->log(
              ostr.str(),
              Logging::Logger::TRACE,
              Aspect::CAMPAIGN_SELECTION_INDEX);

            progress_campaigns.clear();
          }
        }
      }

      timer.stop();

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << "Campaign index construct time : " << timer.elapsed_time();
        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::CAMPAIGN_SELECTION_INDEX);
      }

      cell_holder_.reset();
      campaign_cell_holder_.reset();
      tag_campaign_approve_.clear();

      return true;
    }

    void
    CampaignIndex::index_for_status_(
      const Campaign* campaign,
      UserStatus user_status,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      if(trace_params)
      {
        if(campaign->is_active() &&
           ((!trace_params->key.test_request && !campaign->is_test()) ||
            trace_params->key.test_request))
        {
          trace_params->trace_stream <<
            "  <step name=\"status and test campaign check\" passed=\"yes\"/>" <<
            std::endl;

          index_for_colo_(
            user_status,
            trace_params->key.test_request ? ST_TEST_REAL : ST_REAL,
            campaign,
            trace_params);
        }
        else
        {
          trace_params->trace_stream <<
            "  <step name=\"campaign status and test flags\" passed=\"no\">" << std::endl <<
            "    Campaign status '" << campaign->status << "'" << std::endl <<
            "    Campaign eval status '" << campaign->eval_status << "'" << std::endl <<
            "    Account status '" << (campaign->account->is_active() ? 'A' : 'I') << "'" << std::endl <<
            "    Campaign is " << (campaign->is_test() ? "test" : "non test") << std::endl <<
            "    Request is " << (trace_params->key.test_request ? "test" : "non test") << std::endl <<
            "  </step>" << std::endl;
        }
      }
      else if(campaign->is_active())
      {
        if (campaign->status == 'A')
        {
          index_for_colo_(
            user_status,
            ST_TEST_REAL,
            campaign,
            trace_params);

          if (!campaign->is_test())
          {
            index_for_colo_(
              user_status,
              ST_REAL,
              campaign,
              trace_params);
          }
        }
      }
    }

    bool
    CampaignIndex::colo_precheck_(
      UserStatus user_status,
      const Colocation* colocation)
      noexcept
    {
      if(colocation->ad_serving == CS_NONE)
      {
        return false;
      }
      else if(colocation->ad_serving == CS_ONLY_OPTIN)
      {
        return user_status == US_OPTIN;
      }
      else if(colocation->ad_serving == CS_NON_OPTOUT)
      {
        return user_status != US_OPTOUT;
      }

      return false;
    }

    void
    CampaignIndex::index_for_colo_(
      UserStatus user_status,
      StatusType status_type,
      const Campaign* campaign,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      if(trace_params)
      {
        trace_params->trace_stream <<
          "  <step name=\"include colocations check\" passed=\"yes\"/>" <<
          std::endl;
      }

      index_for_tags_(
        user_status,
        status_type,
        campaign,
        trace_params);
    }

    void
    CampaignIndex::index_for_tags_(
      UserStatus user_status,
      StatusType status_type,
      const Campaign* campaign,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      /* index only for tags linked to included to campaign sites */
      if(!trace_params)
      {
        TagCampaignApproveMap::const_iterator tag_cmp_it =
          tag_campaign_approve_.lower_bound(
            TagCampaignApprove(campaign->campaign_id, 0));

        for(; tag_cmp_it != tag_campaign_approve_.end() &&
              tag_cmp_it->first.campaign_id == campaign->campaign_id;
              ++tag_cmp_it)
        {
          index_for_tag_i_(
            user_status,
            status_type,
            campaign,
            tag_cmp_it->first.tag,
            tag_cmp_it->second,
            0);
        }
      }
      else
      {
        TagCampaignApproveMap::const_iterator tag_cmp_it =
          tag_campaign_approve_.find(
            TagCampaignApprove(campaign->campaign_id, trace_params->tag));

        if(tag_cmp_it != tag_campaign_approve_.end())
        {
          index_for_tag_i_(
            user_status,
            status_type,
            campaign,
            trace_params->tag,
            tag_cmp_it->second,
            trace_params);
        }
      }
    }

    bool
    CampaignIndex::creative_available_by_templates_(
      const Campaign* /*campaign*/,
      const Creative* creative,
      const Tag* tag,
      const char* app_format)
      const
      noexcept
    {
      Template_var templ;
      CreativeTemplate c_template;

      const CreativeTemplateMap& creative_templates =
        campaign_config_->creative_templates;

      for(Creative::SizeMap::const_iterator size_it =
            creative->sizes.begin();
          size_it != creative->sizes.end(); ++size_it)
      {
        try
        {
          if(tag->sizes.find(size_it->first) != tag->sizes.end())
          {
            templ = creative_templates.get(
              CreativeTemplateKey(
                creative->creative_format.c_str(),
                size_it->second.size->protocol_name.c_str(),
                app_format),
              c_template);

            if(templ.in() && c_template.status == 'A')
            {
              return true;
            }
          }
        }
        catch(const Template::FileNotExists& e)
        {}
        catch(const Template::InvalidTemplate& ex)
        {
          logger_->sstream(Logging::Logger::WARNING,
            Aspect::TRAFFICKING_PROBLEM,
            "ADS-TF-0") <<
            "incorrect template file for template (format = '" <<
            creative->creative_format << "', size = '" <<
            size_it->second.size->protocol_name << "', app-format = '" <<
            app_format << "'): all creatives that uses it will be ignored: " <<
            ex.what();
        }
        catch(const eh::Exception& ex)
        {
          logger_->sstream(Logging::Logger::ERROR,
            Aspect::CAMPAIGN_SELECTION_INDEX,
            "ADS-IMPL-5089") <<
            "caught eh::Exception at receiving template (format = '" <<
            creative->creative_format << "', size = '" <<
            size_it->second.size->protocol_name << "', app-format = '" <<
            app_format << "'): all creatives that uses it will be ignored: " <<
            ex.what();
        }
      }

      return false;
    }
    
    bool
    CampaignIndex::check_categories_(
      const Tag* tag,
      bool site_approve_creative,
      bool tag_exclusion,
      bool site_exclusion,
      const Creative::CategorySet& creative_categories)
      noexcept
    {
      for(Creative::CategorySet::const_iterator cat_it =
            creative_categories.begin();
          cat_it != creative_categories.end(); ++cat_it)
      {
        if(tag_exclusion)
        {
          if(tag->rejected_categories.find(*cat_it) !=
             tag->rejected_categories.end())
          {
            return false;
          }
          if(tag->accepted_categories.find(*cat_it) !=
             tag->accepted_categories.end())
          {
            continue;
          }
        }
        if(site_exclusion)
        {
          if(tag->site->rejected_creative_categories.find(*cat_it) !=
             tag->site->rejected_creative_categories.end())
          {
            return false;
          }

          if(!site_approve_creative)
          {
            //Really, this is not approved, but pending categories
            if(tag->site->approved_creative_categories.find(*cat_it) !=
               tag->site->approved_creative_categories.end())
            {
              return false;
            }
          }
        }
      }

      return true;
    }

    bool
    CampaignIndex::creative_available_by_exclude_categories_(
      const Creative* creative,
      const CreativeCategoryIdSet& exclude_categories)
      noexcept
    {
      if(!exclude_categories.empty())
      {
        const Creative::CategorySet& creative_categories =
          creative->categories;

        for(Creative::CategorySet::const_iterator cat_it =
              creative_categories.begin();
            cat_it != creative_categories.end(); ++cat_it)
        {
          if(exclude_categories.find(*cat_it) !=
             exclude_categories.end())
          {
            return false;
          }
        }
      }

      return true;
    }

    bool
    CampaignIndex::creative_available_by_required_categories_(
      const Creative* creative,
      const CreativeCategoryIdSet& required_categories)
      noexcept
    {
      if(!required_categories.empty())
      {
        const Creative::CategorySet& creative_categories =
          creative->categories;

        for(Creative::CategorySet::const_iterator cat_it =
              creative_categories.begin();
            cat_it != creative_categories.end(); ++cat_it)
        {
          if(required_categories.find(*cat_it) != required_categories.end())
          {
            return true;
          }
        }

        return false;
      }

      return true;
    }

    bool
    CampaignIndex::creative_preavailable_by_sizes_(
      const Tag* tag,
      const Creative* creative)
      noexcept
    {
      for(Tag::SizeMap::const_iterator tag_size_it =
            tag->sizes.begin();
          tag_size_it != tag->sizes.end(); ++tag_size_it)
      {
        Creative::SizeMap::const_iterator crs_it =
          creative->sizes.find(tag_size_it->first);

        if(crs_it != creative->sizes.end())
        {
          if(!crs_it->second.expandable || tag->allow_expandable)
          {
            return true;
          }
        }
      }

      return false;
    }

    bool
    CampaignIndex::creative_available_by_size(
      const Tag* tag,
      const Tag::Size* tag_size,
      const Creative* creative,
      unsigned long up_expand_space,
      unsigned long right_expand_space,
      unsigned long down_expand_space,
      unsigned long left_expand_space)
      noexcept
    {
      Creative::SizeMap::const_iterator crs_it =
        creative->sizes.find(tag_size->size->size_id);

      if(crs_it != creative->sizes.end())
      {
        if(!crs_it->second.expandable ||
           (tag->allow_expandable &&
            up_expand_space >= crs_it->second.up_expand_space &&
            right_expand_space >= crs_it->second.right_expand_space &&
            down_expand_space >= crs_it->second.down_expand_space &&
            left_expand_space >= crs_it->second.left_expand_space))
        {
          return true;
        }
      }

      return false;
    }

    bool
    CampaignIndex::creative_available_by_sizes_(
      const Tag* tag,
      const Tag::SizeMap* tag_sizes,
      const Creative* creative,
      unsigned long up_expand_space,
      unsigned long right_expand_space,
      unsigned long down_expand_space,
      unsigned long left_expand_space)
      noexcept
    {
      const Tag::SizeMap* check_tag_sizes =
        tag_sizes ? tag_sizes : &tag->sizes;

      for(Tag::SizeMap::const_iterator tag_size_it =
            check_tag_sizes->begin();
          tag_size_it != check_tag_sizes->end(); ++tag_size_it)
      {
        if(creative_available_by_size(
             tag,
             tag_size_it->second,
             creative,
             up_expand_space,
             right_expand_space,
             down_expand_space,
             left_expand_space))
        {
          return true;
        }
      }

      return false;
    }

    bool
    CampaignIndex::creative_available_by_categories_(
      const Campaign* /*campaign*/,
      const Creative* creative,
      const Tag* tag,
      bool check_click_url_categories)
      const
      noexcept
    {
      const Creative::CategorySet& creative_categories =
        creative->categories;

      if (tag->site->rejected_creatives.find(creative->creative_id) !=
          tag->site->rejected_creatives.end())
      {
        return false;
      }

      const bool site_approve_creative = (
        tag->site->approved_creatives.find(creative->creative_id) !=
        tag->site->approved_creatives.end());

      const bool tag_exclusion = tag->site->tag_exclusion();
      const bool site_exclusion = tag->site->site_exclusion();

      if(!tag_exclusion && !site_exclusion)
      {
        return true;
      }

      if(!creative->defined_content_category && site_exclusion)
      {
        return false;
      }

      return check_categories_(
        tag,
        site_approve_creative,
        tag_exclusion,
        site_exclusion,
        creative_categories) &&
        (!check_click_url_categories ||
         check_categories_(
           tag,
           site_approve_creative,
           tag_exclusion,
           site_exclusion,
           creative->click_categories));
    }

    void
    CampaignIndex::index_for_tag_i_(
      UserStatus user_status,
      StatusType status_type,
      const Campaign* campaign,
      const Tag* tag,
      const ConstCreativePtrList& creatives,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      if(tag->is_test() && status_type != ST_TEST_REAL)
      {
        if(trace_params)
        {
          trace_params->trace_stream <<
            "<step name=\"test tag check\" passed=\"no\">" << std::endl <<
            "  request is test, but tag not" << std::endl <<
            "</step>" << std::endl;
        }

        return;
      }

      index_for_countries_(
        user_status,
        status_type,
        campaign,
        tag,
        creatives,
        trace_params);
    }
    
    void
    CampaignIndex::index_for_countries_(
      UserStatus user_status,
      StatusType status_type,
      const Campaign* campaign,
      const Tag* tag,
      const ConstCreativePtrList& creatives,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      if(trace_params)
      {
        if(trace_params->key.country_code != campaign->country)
        {
          trace_params->trace_stream <<
            "  <step name=\"campaign country check\" passed=\"no\">" << std::endl <<
            "    request country = " << trace_params->key.country_code << std::endl <<
            "    allowed countries = " << campaign->country << std::endl <<
            "  </step>" << std::endl;
        }
        else
        {
          trace_params->trace_stream <<
            "  <step name=\"campaign country check\" passed=\"yes\"/>" << std::endl;
        }
      }

      /* index only for campaign country */
      if(!tag->tag_pricings.empty())
      {
        const Tag::TagPricing* tag_pricing = tag->select_tag_pricing(
          campaign->country.c_str(),
          campaign->ccg_type,
          campaign->ccg_rate_type);

        if(campaign->mode == CM_RANDOM && campaign->ccg_type == CT_DISPLAY)
        {
          bool passed = (tag_pricing->cpm <= tag->max_random_cpm);

          if(trace_params)
          {
            if(passed)
            {
              trace_params->trace_stream <<
                "  <step name=\"max random cpm check\" passed=\"yes\"/>" << std::endl;
            }
            else
            {
              trace_params->trace_stream <<
                "  <step name=\"max random cpm check\" passed=\"no\">" << std::endl <<
                "    tag pricing cpm = " << tag_pricing->cpm << std::endl <<
                "    max random cpm = " << tag->max_random_cpm << std::endl <<
                "  </step>" << std::endl;
            }

            return;
          }
        }

        index_for_appformat_(
          user_status,
          status_type,
          campaign,
          tag,
          tag_pricing,
          campaign->country.c_str(),
          creatives,
          trace_params);
      }
    }

    bool
    CampaignIndex::margin_check_(
      const RevenueDecimal& adjusted_campaign_ecpm,
      const Tag::TagPricing* tag_pricing,
      const Campaign* campaign,
      bool walled_garden,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      // do real precheck only for non ctr campaigns
      // for str campaigns need to do check in runtime,
      // because result ctr depends on request params
      if(!campaign->use_ctr())
      {
        // use tag cpm = 0 if tag pricing isn't defined
        RevenueDecimal current_cpm = tag_pricing ?
          tag_pricing->cpm : RevenueDecimal::ZERO;

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"" <<
            (walled_garden ? "wg" : "foros") << " min margin check\" passed=\"" <<
            (adjusted_campaign_ecpm >= current_cpm ? "yes" : "no") <<
            "\">" << std::endl << "    tag cpm = " << current_cpm.str() << std::endl <<
            "    adjusted campaign ecpm = " << adjusted_campaign_ecpm << std::endl <<
            "    min ecpm = " << current_cpm << std::endl <<
            "  </step>" << std::endl;
        }

        return adjusted_campaign_ecpm >= current_cpm;
      }

      return true;
    }

    void
    CampaignIndex::index_for_appformat_(
      UserStatus user_status,
      StatusType status_type,
      const Campaign* campaign,
      const Tag* tag,
      const Tag::TagPricing* tag_pricing,
      const char* country_code,
      const ConstCreativePtrList& creatives,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      bool text_candidate = false;

      // display campaign traits
      bool lost_walled_garden = false;
      bool walled_garden = false; // target auction
      bool margin_passed = false;
      RevenueDecimal adjusted_campaign_ecpm = RevenueDecimal::ZERO;

      if(campaign->ccg_type == CT_DISPLAY)
      {
        if(campaign->mode == CM_NON_RANDOM)
        {
          // eval correct adjusted ecpm for CTR campaigns only
          // for correct order in max ecpm selection case
          if(campaign->use_ctr())
          {
            adjusted_campaign_ecpm = RevenueDecimal::mul(
              std::min(
                RevenueDecimal::mul(
                  RevenueDecimal::mul(campaign->ctr,
                    ECPM_FACTOR, Generics::DMR_FLOOR),
                  tag->adjustment,
                  Generics::DMR_FLOOR),
                ECPM_FACTOR),
              campaign->click_sys_revenue,
              Generics::DMR_FLOOR);
          }
          else
          {
            adjusted_campaign_ecpm = campaign->ecpm_;
          }
        }

        if(campaign->marketplace != 'O' && tag->marketplace != 'O' &&
           tag->site->account->walled_garden_accounts.find(
             campaign->account->account_id) !=
           tag->site->account->walled_garden_accounts.end())
        {
          if(campaign->mode == CM_NON_RANDOM)
          {
            margin_passed = true;

            margin_passed = margin_check_(
              adjusted_campaign_ecpm,
              tag_pricing,
              campaign,
              true, // WG
              trace_params);

            if(margin_passed)
            {
              walled_garden = true;
            }
            else
            {
              lost_walled_garden = true;
            }
          }
          else
          {
            margin_passed = true;
            walled_garden = true;
            lost_walled_garden = false;
          }
        }

        // push campaign into secondary (non WG) auction:
        // only if it isn't pushed into WG
        if(!walled_garden &&
           campaign->marketplace != 'W' &&
           tag->marketplace != 'W')
        {
          if(campaign->mode == CM_NON_RANDOM)
          {
            margin_passed = margin_check_(
              adjusted_campaign_ecpm,
              tag_pricing,
              campaign,
              false, // OIX marketplace
              trace_params);
          }
          else
          {
            margin_passed = true;
          }
        }
      }
      else
      {
        text_candidate = true;
      }

      // continue indexing for app format margin check indendently
      //   if it failed, campaign will be passed to lost instead selection list
      StringSet creative_formats;
      ConstCreativePtrList trace_creatives; // only for request app format

      for(ConstCreativePtrList::const_iterator cr_it = creatives.begin();
          cr_it != creatives.end(); ++cr_it)
      {
        for(Tag::SizeMap::const_iterator tag_size_it = tag->sizes.begin();
            tag_size_it != tag->sizes.end(); ++tag_size_it)
        {
          Creative::SizeMap::const_iterator crs_it =
            (*cr_it)->sizes.find(tag_size_it->first);

          if(crs_it != (*cr_it)->sizes.end())
          {
            const StringSet& check_app_formats = crs_it->second.available_appformats;

            if(trace_params &&
               check_app_formats.find(trace_params->key.format) !=
                 check_app_formats.end())
            {
              trace_creatives.push_back(*cr_it);
            }

            for(StringSet::const_iterator appf_it = check_app_formats.begin();
                appf_it != check_app_formats.end();
                ++appf_it)
            {
              creative_formats.insert(*appf_it);
            }
          }
        }
      }

      if(trace_params)
      {
        if(!trace_creatives.empty())
        {
          trace_params->trace_stream <<
            "  <step name=\"app format check\" passed=\"yes\">" << std::endl <<
            "    Next creatives available for requested format '" <<
            trace_params->key.format << "' (ccid): ";

          for(ConstCreativePtrList::const_iterator cr_it =
                trace_creatives.begin();
              cr_it != trace_creatives.end(); ++cr_it)
          {
            if(cr_it != trace_creatives.begin())
            {
              trace_params->trace_stream << ", ";
            }
            trace_params->trace_stream << (*cr_it)->ccid;
          }

          trace_params->trace_stream << std::endl << "  </step>" << std::endl;
        }
        else
        {
          trace_params->trace_stream <<
            "  <step name=\"app format check\" passed=\"no\"/>" << std::endl;
        }
      }

      // index by available app formats
      for(StringSet::const_iterator format_it = creative_formats.begin();
          format_it != creative_formats.end(); ++format_it)
      {
        KeyHashAdapter key_hash(
          static_cast<unsigned char>(user_status) | (
            static_cast<unsigned char>(status_type) << 4),
          tag->tag_id,
          country_code,
          format_it->c_str());
          
        IndexNode& index_node = ordered_campaigns_[key_hash];

        if(text_candidate)
        {
          CampaignCell_var cell(new CampaignCell(campaign));

          if(campaign->targeting_type == 'K')
          {
            if(campaign->mode == CM_RANDOM)
            {
              index_node.keyword_random_campaigns = campaign_cell_holder_->get(
                index_node.keyword_random_campaigns,
                cell);
            }
            else
            {
              index_node.keyword_campaigns = campaign_cell_holder_->get(
                index_node.keyword_campaigns,
                cell);
            }
          }
          else // channel targeted text ad
          {
            if(campaign->mode == CM_RANDOM)
            {
              index_node.text_random_campaigns = campaign_cell_holder_->get(
                index_node.text_random_campaigns,
                cell);
            }
            else
            {
              index_node.text_campaigns = campaign_cell_holder_->get(
                index_node.text_campaigns,
                cell);
            }
          }
        }
        else // display campaign
        {
          if(campaign->mode == CM_RANDOM)
          {
            if(tag->max_random_cpm >= tag_pricing->cpm)
            {
              CampaignSelectionCell_var cell(
                new CampaignSelectionCell(
                  campaign, adjusted_campaign_ecpm, tag_pricing));

              if(walled_garden)
              {
                index_node.wg_display_random_campaigns = cell_holder_->get(
                  index_node.wg_display_random_campaigns,
                  cell);
              }
              else
              {
                index_node.display_random_campaigns = cell_holder_->get(
                  index_node.display_random_campaigns,
                  cell);
              }
            }
          }
          else
          {
            if(lost_walled_garden)
            {
              // push campaign into WG lost indendent on margin_passed value.
              // possible specific case, when campaign don't allowed to WG auction by margins,
              //   but allowed to OIX auction,
              //   in this case we push it into wg lost campaigns (
              //     for collect lost if at WG auction campaign is selected)
              //   but we must remove it from lost if it selected in OIX auction after failed WG
              //
              index_node.lost_wg_campaigns = campaign_cell_holder_->get(
                index_node.lost_wg_campaigns,
                CampaignCell_var(new CampaignCell(campaign)));
            }

            if(!margin_passed)
            {
              // push to lost auctions (only display campaign can be pushed here)
              if(!lost_walled_garden)
              {
                index_node.lost_campaigns = campaign_cell_holder_->get(
                  index_node.lost_campaigns,
                  CampaignCell_var(new CampaignCell(campaign)));
              }
            }
            else
            {
              CampaignSelectionCell_var cell(
                new CampaignSelectionCell(
                  campaign, adjusted_campaign_ecpm, tag_pricing));

              if(walled_garden)
              {
                // push campaign into WG auction
                index_node.wg_display_campaigns =
                  cell_holder_->get(index_node.wg_display_campaigns, cell);
              }
              else
              {
                // push campaign into OIX auction
                index_node.display_campaigns =
                  cell_holder_->get(index_node.display_campaigns, cell);
              }
            }
          }
        }
      } // creative_formats loop
    }

    void
    CampaignIndex::preindex_for_tag_(
      const Tag* tag,
      const Campaign* campaign,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      const Site* site = tag->site;
      
      if(campaign->include_specific_sites() &&
         campaign->sites.find(site->site_id) ==
         campaign->sites.end())
      {
        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"site targeting\" passed=\"no\"/>" <<
            std::endl;
        }
        
        return;
      }
      else if(trace_params)
      {
        trace_params->trace_stream <<
          "  <step name=\"site targeting\" passed=\"yes\"/>" << std::endl;
      }

      TagDeliveryMap::const_iterator td_it = campaign->exclude_tags.find(tag->tag_id);

      if(td_it != campaign->exclude_tags.end() && td_it->second == 0)
      {
        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"tag delivery full exclusion\" passed=\"no\"/>" <<
            std::endl;
        }
        
        return;
      }
      else if(trace_params)
      {
        trace_params->trace_stream <<
          "  <step name=\"tag delivery full exclusion\" passed=\"yes\"/>" << std::endl;
      }

      if(campaign->exclude_pub_accounts.find(
           site->account->account_id) !=
         campaign->exclude_pub_accounts.end())
      {
        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"publisher accounts exclusion\" passed=\"no\"/>" <<
            std::endl;
        }
        
        return;
      }
      else if(trace_params)
      {
        trace_params->trace_stream <<
          "  <step name=\"publisher accounts exclusion\" passed=\"yes\"/>" <<
          std::endl;
      }

      if((campaign->ccg_type != CT_DISPLAY && tag->marketplace != 'W') ||
           // non display campaign can't be selected for tag with 'W' auction only
         (tag->marketplace != 'W' && campaign->marketplace != 'W') ||
         (tag->marketplace != 'O' && campaign->marketplace != 'O' &&
           tag->site->account->walled_garden_accounts.find(
             campaign->account->account_id) !=
           tag->site->account->walled_garden_accounts.end()))
      {
        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"walled garden approval\" passed=\"yes\"/>" <<
            std::endl;
        }

        ConstCreativePtrList creatives;

        /* fill available by categories creatives */
        const CreativeList& campaign_creatives =
          campaign->get_creatives();

        for(CreativeList::const_iterator cr_it = campaign_creatives.begin();
          cr_it != campaign_creatives.end();
          ++cr_it)
        {
          if((*cr_it)->status == 'A' &&
             creative_available_by_categories_(
               campaign,
               *cr_it,
               tag,
               !campaign->keyword_based()) &&
             creative_preavailable_by_sizes_(
               tag,
               *cr_it))
          {
            creatives.push_back(*cr_it);
          }
        }

        if(trace_params)
        {
          if(!creatives.empty())
          {
            trace_params->trace_stream <<
              "  <step name=\"creative categories and tag size\""
              " passed=\"yes\">" << std::endl <<
              "     Next creatives available by categories and tag size (ccid):";
              
            for(ConstCreativePtrList::const_iterator cr_it = creatives.begin();
              cr_it != creatives.end();
              ++cr_it)
            {
              trace_params->trace_stream << " " << (*cr_it)->ccid;
            }
              
            trace_params->trace_stream << std::endl << "  </step>" << std::endl;
          }
          else
          {
            trace_params->trace_stream <<
              "  <step name=\"creative categories and tag size\""
                 " passed=\"no\">" << std::endl <<
              "    No creatives available by categories and tag size" << std::endl <<
              "  </step>";
          }
        }

        if(!creatives.empty())
        {
          // check creatives expand direction
          ConstCreativePtrList res_creatives;
          for(ConstCreativePtrList::const_iterator cr_it = creatives.begin();
            cr_it != creatives.end();
            ++cr_it)
          {
            const Creative* creative = *cr_it;

            if(tag->allow_expandable)
            {
              res_creatives.push_back(*cr_it);
            }
            else
            {
              for(Creative::SizeMap::const_iterator crs_it =
                    creative->sizes.begin();
                  crs_it != creative->sizes.end(); ++crs_it)
              {
                if(crs_it->second.up_expand_space == 0 &&
                  crs_it->second.right_expand_space == 0 &&
                  crs_it->second.down_expand_space == 0 &&
                  crs_it->second.left_expand_space == 0)
                {
                  res_creatives.push_back(*cr_it);
                  break;
                }
              }
            }
          }

          res_creatives.swap(creatives);
        }

        if(trace_params)
        {
          if(!creatives.empty())
          {
            trace_params->trace_stream <<
              "  <step name=\"expandable creatives\""
              " passed=\"yes\">" << std::endl <<
              "     Next creatives available:";

            for(ConstCreativePtrList::const_iterator cr_it = creatives.begin();
                cr_it != creatives.end();
                ++cr_it)
            {
              trace_params->trace_stream << " " << (*cr_it)->ccid;
            }

            trace_params->trace_stream << std::endl << "  </step>" << std::endl;
          }
          else
          {
            trace_params->trace_stream <<
              "  <step name=\"expandable creatives\""
              " passed=\"no\">" << std::endl <<
              "    No creatives available" << std::endl <<
              "  </step>";
          }
        }

        if(!creatives.empty())
        {
          tag_campaign_approve_.insert(
            std::make_pair(
              TagCampaignApprove(campaign->campaign_id, tag),
              creatives));
        }
      } // walled garden check
      else if(trace_params)
      {
        trace_params->trace_stream << "  <step name=\"walled garden approval\" "
          "passed=\"no\">" << std::endl <<
          "    Tag marketplace = '" << tag->marketplace << "'" << std::endl <<
          "    Campaign marketplace = '" << campaign->marketplace << "'" << std::endl <<
          "    Publisher approved accounts: ";

        Algs::print(trace_params->trace_stream,
          site->account->walled_garden_accounts.begin(),
          site->account->walled_garden_accounts.end());

        trace_params->trace_stream << std::endl << "  </step>" << std::endl;
      }
    }

    void
    CampaignIndex::index_campaign(
      const Campaign* campaign,
      TraceParams* trace_params)
      /*throw(Exception, eh::Exception)*/
    {
      if(!trace_params)
      {
        /* pre index campaign for all tags */
        for(TagMap::const_iterator tag_it = campaign_config_->tags.begin();
            tag_it != campaign_config_->tags.end();
            ++tag_it)
        {
          preindex_for_tag_(tag_it->second, campaign, trace_params);
        }
      }
      else
      {
        /* pre index campaign for trace tag */
        preindex_for_tag_(trace_params->tag, campaign, trace_params);
      }

      UserStatus trace_match_type = US_OPTIN;

      bool only_for_opt_in =
        campaign->fc_id != 0 ||
        campaign->group_fc_id != 0 ||
        campaign->start_user_group_id != 0 ||
        campaign->end_user_group_id != MAX_TARGET_USERS_GROUPS ||
        campaign->seq_set_rotate_imps != 0 ||
        // only action payable
        (campaign->imp_revenue == RevenueDecimal::ZERO &&
         campaign->click_revenue == RevenueDecimal::ZERO &&
         !campaign->keyword_based() // keyword based campaign click cost defined inside keyword
         );

      if (trace_params)
      {
        trace_match_type = trace_params->key.user_status;

        if ((only_for_opt_in && trace_params->key.user_status == US_OPTIN) ||
           !only_for_opt_in)
        {
          trace_params->trace_stream <<
            "  <step name=\"opted-in check\" passed=\"yes\"/>" << std::endl;
        }
        else
        {
          trace_params->trace_stream <<
            "  <step name=\"opted-in check\" passed=\"no\"/>" << std::endl;
        }

        const bool passed = (
          (campaign->flags & CampaignFlags::US_NONE) ||
          match(campaign->flags, trace_params->key.user_status));

        trace_params->trace_stream <<
          "  <step name=\"ccg opt-in status targeting check\" passed=\"" <<
          (passed ? "yes" : "no") << "\">\n" <<
          "    campaign flags=\"" << campaign_flags_to_str(campaign->flags) << "\"\n" <<
          "    user status=\"" << to_str(trace_params->key.user_status) << "\"\n" <<
          "  </step>" <<
          std::endl;
      }

      if(only_for_opt_in)
      {
        // if campaign user status allow opt in - index it
        // here used ColocationAdServingType values specific property:
        // al values (excluding CS_NONE) contains optin targeting
        //
        if((campaign->flags & CampaignFlags::US_NONE) ||
           (campaign->flags & CampaignFlags::US_OPTIN))
        {
          index_for_status_(
            campaign,
            trace_params ? trace_params->key.user_status : US_OPTIN,
            trace_params);
        }
      }
      else if(campaign->flags & CampaignFlags::US_NONE)
      {
        // will be used colocation user status targeting
        index_for_status_(
          campaign,
          trace_params ? trace_params->key.user_status : US_NONE,
          trace_params // trace independent on user status
          );
      }
      else
      {
        // campaign user status targeting override colocation traits
        if(campaign->flags & CampaignFlags::US_UNDEFINED)
        {
          index_for_status_(
            campaign,
            US_UNDEFINED,
            trace_match_type == US_UNDEFINED ? trace_params : 0);
        }

        if(campaign->flags & CampaignFlags::US_OPTOUT)
        {
          index_for_status_(
            campaign,
            US_OPTOUT,
            trace_match_type == US_OPTOUT ? trace_params : 0);
        }

        if(campaign->flags & CampaignFlags::US_OPTIN)
        {
          index_for_status_(
            campaign,
            US_OPTIN,
            trace_match_type == US_OPTIN ? trace_params : 0);
        }
      }
    }

    bool
    CampaignIndex::check_campaign_time_(
      const Campaign* campaign,
      const Generics::Time& current_time_val)
      const
    {
      if(campaign->weekly_run_intervals.empty())
      {
        return true;
      }

      Generics::ExtendedTime ex_time(current_time_val.get_gm_time());
      return campaign->weekly_run_intervals.contains(
        (ex_time.tm_wday + 6) % 7 * 60 * 24 +
        ex_time.tm_hour * 60 +
        ex_time.tm_min);
    }

    bool
    CampaignIndex::match_domain(
      const std::string& domain,
      const String::SubString& referer_hostname)
      noexcept
    {
      if(domain.size() > 0)
      {
        if (referer_hostname.size() >= domain.size() &&
            referer_hostname.compare(
              referer_hostname.size() - domain.size(),
              domain.size(),
              domain) == 0)
        {
          return true;
        }
      }

      return false;
    }     

    bool
    CampaignIndex::check_tag_domain_exclusion(
      const String::SubString& url_val,
      const Tag* tag)
      noexcept
    {
      try
      {
        HTTP::BrowserAddress url(url_val);
        std::string check_domain = url.host().substr(
          url.host().compare(0, 4, "www.") == 0 ? 4 : 0).str();
        String::AsciiStringManip::to_lower(check_domain);
        return tag->exclude_creative_domains.find(check_domain) ==
          tag->exclude_creative_domains.end();
      }
      catch(...)
      {}

      return true;
    }

    bool
    CampaignIndex::check_tag_visibility(
      long tag_visibility,
      const Tag* tag,
      TraceParams* trace_params)
    {
      // full visible if visibility undefined (< 0)
      bool result = tag_visibility >= 0 &&
        static_cast<unsigned long>(tag_visibility) < tag->min_visibility;

      if (trace_params)
      {
        trace_params->trace_stream <<
          "  <step name=\"tag visibility checking\" passed=\"" <<
          (result ? "no" : "yes") << "\"/>" << std::endl;
      }

      return result;
    }

    bool
    CampaignIndex::check_site_freq_cap(
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps,
      const Tag* tag,
      TraceParams* trace_params)
    {
      bool result = tag->site->freq_cap_id &&
        (!profiling_available ||
        full_freq_caps.find(tag->site->freq_cap_id) != full_freq_caps.end());

      if (trace_params)
      {
        trace_params->trace_stream <<
          "  <step name=\"freq caps(site)\" passed=\"" <<
          (result ? "no" : "yes") << "\"/>" << std::endl;
      }

      return result;
    }

    bool
    CampaignIndex::check_campaign(
      const Key& key,
      const Campaign* campaign,
      const Generics::Time& current_time,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps,
      unsigned long colo_id,
      const Generics::Time& user_create_time,
      const AdServer::Commons::UserId& user_id,
      const TraceParams* trace_params)
      const
    {
      {
        /* freq caps filter */
        bool res = (campaign->fc_id || campaign->group_fc_id) && (
          full_freq_caps.find(campaign->fc_id) != full_freq_caps.end() ||
          full_freq_caps.find(campaign->group_fc_id) != full_freq_caps.end() ||
          !profiling_available);

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"freq caps(campaign, ccg)\" passed=\"" <<
            (res ? "no" : "yes") << "\"/>" <<
            std::endl;
        }

        if(res)
        {
          return false;
        }
      }
      
      {
        /* campaign filtering: by weekly run intervals */
        bool res = !check_campaign_time_(campaign, current_time);

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"time of day\" passed=\"" << (res ? "no" : "yes") << "\"/>" <<
            std::endl;
        }

        if(res)
        {
          return false;
        }
      }

      {
        bool res = key.ccg_delivery_factor >= campaign->delivery_coef;

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"ccg delivery factor exclusion\" passed=\"" <<
            (res ? "no" : "yes") << "\"/>" <<
            std::endl;
        }

        if(res)
        {
          return false;
        }
      }

      {
        TagDeliveryMap::const_iterator td_it = campaign->exclude_tags.find(key.tag->tag_id);
        if(td_it != campaign->exclude_tags.end() && td_it->second)
        {
          bool res = key.tag_delivery_factor >= td_it->second;

          if(trace_params)
          {
            trace_params->trace_stream <<
              "  <step name=\"tag delivery partly exclusion\" passed=\"" <<
              (res ? "no" : "yes") << "\"/>" <<
              std::endl;
          }

          if(res)
          {
            return false;
          }
        }
      }

      {
        /* user groups filtering */
        bool res = (user_id.hash() % MAX_TARGET_USERS_GROUPS <
            campaign->start_user_group_id ||
          user_id.hash() % MAX_TARGET_USERS_GROUPS >=
            campaign->end_user_group_id);

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"user group targeting\" passed=\"" <<
            (res ? "no" : "yes") << "\"/>" <<
            std::endl;
        }
        
        if(res)
        {
          return false;
        }
      }

      {
        /* colocation filtering */
        bool res = !campaign->colocations.empty() &&
          campaign->colocations.find(colo_id) == campaign->colocations.end();

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"colocation targeting\" passed=\"" <<
            (res ? "no" : "yes") << "\"/>" <<
            std::endl;
        }

        if(res)
        {
          return false;
        }
      }

      {
        /* uid age filtering */
        bool res = campaign->min_uid_age != Generics::Time::ZERO &&
          current_time - user_create_time < campaign->min_uid_age;

        if(trace_params)
        {
          trace_params->trace_stream <<
            "  <step name=\"uid age targeting\" passed=\"" <<
            (res ? "no" : "yes") << "\"/>" <<
            std::endl;
        }

        if(res)
        {
          return false;
        }
      }

      {
        // available checking
        bool res = !campaign->is_available() ||
          !campaign->account->is_available() ||
          !campaign->advertiser->is_available();

        if(trace_params)
        {
          if(res)
          {
            trace_params->trace_stream <<
              "  <step name=\"available\" passed=\"no\">" << std::endl <<
              "    account = " << (
                campaign->account->is_available() ? "available" : "not available") << std::endl <<
              "    advertiser = " << (
                campaign->advertiser->is_available() ? "available" : "not available") << std::endl <<
              "    campaign = " << (
                campaign->is_available() ? "available" : "not available") << std::endl <<
              "  </step>" << std::endl;
          }
          else
          {
            trace_params->trace_stream <<
              "  <step name=\"available\" passed=\"" <<
              (res ? "no" : "yes") << "\"/>" <<
              std::endl;
          }
        }

        if(res)
        {
          return false;
        }
      }

      return true;
    }

    bool
    CampaignIndex::check_campaign_channel(
      const Campaign* campaign,
      const ChannelIdHashSet& matched_channels)
      const
    {
      /*
      return !campaign->channel.in() ||
        campaign->channel->triggered(&matched_channels, 0);
      */
      return !campaign->fast_channel.in() ||
        campaign->fast_channel->triggered(&matched_channels, 0);
    }
  
    void
    CampaignIndex::filter_creatives(
      const Key& key,
      const Tag* tag,
      const Tag::SizeMap* tag_sizes,
      const Campaign* campaign,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps,
      const SeqOrderMap& seq_orders,
      ConstCreativePtrList& creatives,
      bool check_click_categories,
      unsigned long up_expand_space,
      unsigned long right_expand_space,
      unsigned long down_expand_space,
      unsigned long left_expand_space,
      unsigned long video_min_duration,
      const std::optional<unsigned long>& video_max_duration,
      const std::optional<unsigned long>& video_skippable_max_duration,
      bool video_allow_skippable,
      bool video_allow_unskippable,
      const AllowedDurationSet& allowed_durations,
      const CreativeCategoryIdSet& exclude_categories,
      const CreativeCategoryIdSet& required_categories,
      bool secure,
      bool filter_empty_destination,
      TraceParams* trace_params)
      const
    {
      const CreativeList& campaign_creatives = campaign->get_creatives();

      if(trace_params)
      {
        trace_params->trace_stream << "  <step name=\"creatives\">" << std::endl;
      }
      
      for(CreativeList::const_iterator cr_it = campaign_creatives.begin();
          cr_it != campaign_creatives.end();
          ++cr_it)
      {
        const Creative* creative = *cr_it;

        if((creative->fc_id == 0 || key.user_status == US_OPTIN) &&
           creative->status == 'A' &&
           (!secure || creative->https_safe_flag) &&
           (!filter_empty_destination || !creative->destination_url.url().empty()) &&
           creative_available_by_sizes_(
             tag,
             tag_sizes,
             creative,
             up_expand_space,
             right_expand_space,
             down_expand_space,
             left_expand_space) &&
           creative_available_by_categories_(
             campaign,
             creative,
             tag,
             check_click_categories) &&
           creative_available_by_templates_(
             campaign,
             creative,
             tag,
             key.format.c_str()) &&
           // check video duration
           (creative->video_duration >= video_min_duration &&
            (creative->video_skip_offset ? // skippable
              (!video_skippable_max_duration ||
                creative->video_duration <= *video_skippable_max_duration) :
              (!video_max_duration ||
                creative->video_duration <= *video_max_duration)
             )) &&
           (allowed_durations.empty() ||
             (allowed_durations.find(creative->video_duration) != allowed_durations.end())) &&
           // check video skippable
           ((video_allow_skippable && creative->video_skip_offset) ||
            (video_allow_unskippable && !creative->video_skip_offset)) &&
           // check frequency caps
           (creative->fc_id == 0 ||
            (profiling_available &&
             full_freq_caps.find(creative->fc_id) == full_freq_caps.end())) &&
           creative_available_by_exclude_categories_(
             creative,
             exclude_categories) &&
           creative_available_by_required_categories_(
             creative,
             required_categories)
           )
        {
          if(trace_params)
          {
            trace_params->trace_stream << "    <creative ccid=\"" << creative->ccid <<
              "\" passed=\"yes\"/>" << std::endl;
          }
          
          creatives.push_back(creative);
        }
        else if(trace_params)
        {
          trace_params->trace_stream << "    <creative ccid=\"" << creative->ccid <<
            "\" passed=\"no\">" << std::endl <<
            "      Not available ";

          if(!(creative->fc_id == 0 || key.user_status == US_OPTIN))
          {
            trace_params->trace_stream << "by freq caps and opt out; ";
          }

          if(!(creative->status == 'A'))
          {
            trace_params->trace_stream << "by status; ";
          }

          if (secure && !creative->https_safe_flag)
          {
            trace_params->trace_stream << "by https flag; ";
          }

          if(!(!filter_empty_destination || !creative->destination_url.url().empty()))
          {
            trace_params->trace_stream << "by empty destination; ";
          }

          if(!creative_available_by_sizes_(
               tag,
               tag_sizes,
               creative,
               up_expand_space,
               right_expand_space,
               down_expand_space,
               left_expand_space))
          {
            trace_params->trace_stream << "by size or expansion; ";
          }

          if(!creative_available_by_categories_(
               campaign,
               creative,
               tag,
               check_click_categories))
          {
            trace_params->trace_stream << "by categories; ";
          }

          if(!creative_available_by_templates_(
               campaign,
               creative,
               tag,
               key.format.c_str()))
          {
            trace_params->trace_stream << "by templates: format = '" <<
              key.format << "'; ";
          }

          if(!(creative->video_duration >= video_min_duration &&
            (!video_max_duration ||
             creative->video_duration <= *video_max_duration)))
          {
            trace_params->trace_stream << "by video duration; ";
          }

          if(!allowed_durations.empty() &&
            (allowed_durations.find(creative->video_duration) == allowed_durations.end()))
          {
            trace_params->trace_stream << "by video allowed_durations; ";
          }

          if(!((video_allow_skippable && creative->video_skip_offset) ||
            (video_allow_unskippable && !creative->video_skip_offset)))
          {
            trace_params->trace_stream << "by video skippable; ";
          }

          if(!(creative->fc_id == 0 ||
              (profiling_available &&
               full_freq_caps.find(creative->fc_id) == full_freq_caps.end())))
          {
            trace_params->trace_stream << "by freq caps; ";
          }

          if(!creative_available_by_exclude_categories_(
             creative,
             exclude_categories))
          {
            trace_params->trace_stream << "by external exclude categories; ";
          }

          if(!creative_available_by_required_categories_(
             creative,
             required_categories))
          {
            trace_params->trace_stream << "by external required categories; ";
          }

          trace_params->trace_stream << std::endl << "    </creative>" << std::endl;
        }
      }

      if(trace_params)
      {
        trace_params->trace_stream << "  </step>" << std::endl;
      }

      if(campaign->seq_set_rotate_imps != 0 && !creatives.empty())
      {
        if(campaign->opt_order_sets.empty())
        {
          creatives.clear();
        }
        else
        {
          SeqOrderMap::const_iterator seq_it = seq_orders.find(
            campaign->campaign_id);

          unsigned long search_order_set_id =
            *(campaign->opt_order_sets.begin());

          if(seq_it != seq_orders.end())
          {
            Campaign::OrderSetIdSet::const_iterator seq_order_it =
              campaign->opt_order_sets.lower_bound(
                seq_it->second.imps >= campaign->seq_set_rotate_imps ?
                  seq_it->second.set_id + 1 :
                  seq_it->second.set_id);
            if(seq_order_it == campaign->opt_order_sets.end())
            {
              seq_order_it = campaign->opt_order_sets.begin();
            }
            search_order_set_id = *seq_order_it;
          }

          // filter creatives by search_order_set_id
          ConstCreativePtrList new_creatives;

          for(ConstCreativePtrList::const_iterator cr_it =
                creatives.begin();
              cr_it != creatives.end(); ++cr_it)
          {
            if((*cr_it)->order_set_id == search_order_set_id)
            {
              new_creatives.push_back(*cr_it);
            }
          }

          creatives.swap(new_creatives);
        }
      }
    }

    template<
      typename ResultContainerType,
      typename ListFieldType,
      typename LessPredType>
    void
    CampaignIndex::merge_lists_(
      ResultContainerType& result,
      const IndexNodeList& nodes,
      ListFieldType IndexNode::* list_field,
      const LessPredType& less_pred)
    {
      std::vector<Algs::IteratorRange<
        typename ListFieldType::Type::const_iterator> > ranges;
      ranges.reserve(nodes.size());

      for(IndexNodeList::const_iterator ll_it = nodes.begin();
          ll_it != nodes.end(); ++ll_it)
      {
        if((*ll_it)->*list_field)
        {
          ranges.push_back(Algs::iterator_range(
            ((*ll_it)->*list_field)->begin(),
            ((*ll_it)->*list_field)->end()));
        }
      }

      Algs::custom_merge_n(
        ranges.begin(),
        ranges.end(),
        std::back_inserter(result),
        less_pred);
    }

    void
    CampaignIndex::get_index_nodes_(
      IndexNodeList& result_nodes,
      const Key& request_params) const
      noexcept
    {
      const UserStatus user_status_check[] = { request_params.user_status, US_NONE };
      const UserStatus* user_status_check_begin = user_status_check;
      const UserStatus* user_status_check_end = request_params.none_user_status ?
        user_status_check + 2 : user_status_check + 1;

      for(const UserStatus* user_status_check_it = user_status_check_begin;
          user_status_check_it != user_status_check_end;
          ++user_status_check_it)
      {
        unsigned char match_status_type =
          static_cast<unsigned char>(*user_status_check_it) |
            (static_cast<unsigned char>(
               request_params.test_request ? ST_TEST_REAL : ST_REAL) << 4);

        OrderedCampaignMap::const_iterator it =
          ordered_campaigns_.find(
            KeyHashAdapter(
              match_status_type,
              request_params.tag->tag_id,
              request_params.country_code.c_str(),
              request_params.format.c_str()));

        if(it != ordered_campaigns_.end())
        {
          result_nodes.push_back(&it->second);
        }
      }
    }

    void
    CampaignIndex::get_random_campaigns(
      const Key& request_params,
      CampaignSelectionCellPtrList& wg_display_campaign_cell_list,
      CampaignSelectionCellPtrList& display_campaign_cell_list,
      CampaignCellPtrList& text_campaign_cell_list,
      CampaignCellPtrList& keyword_campaign_cell_list)
      const
    {
      IndexNodeList index_nodes;

      get_index_nodes_(index_nodes, request_params);

      merge_lists_(
        wg_display_campaign_cell_list,
        index_nodes,
        &IndexNode::wg_display_random_campaigns,
        campaign_selection_cell_less_pred);

      merge_lists_(
        display_campaign_cell_list,
        index_nodes,
        &IndexNode::display_random_campaigns,
        campaign_selection_cell_less_pred);

      merge_lists_(
        text_campaign_cell_list,
        index_nodes,
        &IndexNode::text_random_campaigns,
        campaign_cell_less_pred);

      merge_lists_(
        keyword_campaign_cell_list,
        index_nodes,
        &IndexNode::keyword_random_campaigns,
        campaign_cell_less_pred);
    }

    void
    CampaignIndex::get_campaigns(
      const Key& request_params,
      CampaignSelectionCellPtrList& result_wg_campaign_cell_list,
      CampaignSelectionCellPtrList& result_campaign_cell_list,
      CampaignCellPtrList& result_text_campaign_cell_list,
      CampaignCellPtrList& result_keyword_campaign_cell_list,
      CampaignCellPtrList* result_lost_wg_campaign_cell_list,
      CampaignCellPtrList* result_lost_campaign_cell_list)
      const
    {
      IndexNodeList index_nodes;

      get_index_nodes_(index_nodes, request_params);

      merge_lists_(
        result_wg_campaign_cell_list,
        index_nodes,
        &IndexNode::wg_display_campaigns,
        campaign_selection_cell_less_pred);

      merge_lists_(
        result_campaign_cell_list,
        index_nodes,
        &IndexNode::display_campaigns,
        campaign_selection_cell_less_pred);

      merge_lists_(
        result_text_campaign_cell_list,
        index_nodes,
        &IndexNode::text_campaigns,
        campaign_cell_less_pred);

      merge_lists_(
        result_keyword_campaign_cell_list,
        index_nodes,
        &IndexNode::keyword_campaigns,
        campaign_cell_less_pred);
      
      if(result_lost_wg_campaign_cell_list)
      {
        merge_lists_(
          *result_lost_wg_campaign_cell_list,
          index_nodes,
          &IndexNode::lost_wg_campaigns,
          campaign_cell_less_pred);
      }

      if(result_lost_campaign_cell_list)
      {
        merge_lists_(
          *result_lost_campaign_cell_list,
          index_nodes,
          &IndexNode::lost_campaigns,
          campaign_cell_less_pred);
      }
    }

    void
    CampaignIndex::trace_indexing(
      const Key& key,
      const Generics::Time& current_time,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps,
      unsigned long colo_id,
      const Generics::Time& user_create_time,
      const ChannelIdHashSet& matched_channels,
      const AdServer::Commons::UserId& user_id,
      const Campaign* campaign,
      const RevenueDecimal& /*min_ecpm*/,
      unsigned long up_expand_space,
      unsigned long right_expand_space,
      unsigned long down_expand_space,
      unsigned long left_expand_space,
      long tag_visibility,
      unsigned long video_min_duration,
      const std::optional<unsigned long>& video_max_duration,
      const std::optional<unsigned long>& video_skippable_max_duration,
      bool video_allow_skippable,
      bool video_allow_unskippable,      
      const AllowedDurationSet& allowed_durations,
      const CreativeCategoryIdSet& exclude_categories,
      const CreativeCategoryIdSet& required_categories,
      AuctionType auction_type,
      bool secure,
      bool filter_empty_destination,
      std::ostream& ostr)
      /*throw(Exception, eh::Exception)*/
    {
      TraceParams trace_params(key, campaign, key.tag, auction_type, ostr);

      cell_holder_ = new CampaignSelectionCellListHolder();
      campaign_cell_holder_ = new CampaignCellListHolder();

      index_campaign(campaign, &trace_params);

      check_campaign(
        key,
        campaign,
        current_time,
        profiling_available,
        full_freq_caps,
        colo_id,
        user_create_time,
        user_id,
        &trace_params);

      if(check_campaign_channel(campaign, matched_channels))
      {
        trace_params.trace_stream <<
          "  <step name=\"channel targeting\" passed=\"yes\">" << std::endl <<
          "    matched channels = ";
        Algs::print(trace_params.trace_stream, matched_channels.begin(), matched_channels.end());
        trace_params.trace_stream << std::endl <<
          "  </step>" << std::endl;
      }
      else
      {
        trace_params.trace_stream <<
          "  <step name=\"channel targeting\" passed=\"no\">" << std::endl <<
          "    matched channels = ";
        Algs::print(trace_params.trace_stream, matched_channels.begin(), matched_channels.end());
        trace_params.trace_stream << std::endl <<
          "    fast expr = ";
        if(campaign->fast_channel)
        {
          campaign->fast_channel->print(trace_params.trace_stream);
          //campaign->fast_channel->print(trace_params.trace_stream);
          //print(trace_params.trace_stream, campaign->fast_channel, true);
        }
        else
        {
          trace_params.trace_stream << "NULL";
        }
        trace_params.trace_stream << std::endl <<
          "    expr = ";
        if(campaign->channel)
        {
          print(trace_params.trace_stream, campaign->channel, true);
        }
        else
        {
          trace_params.trace_stream << "NULL";
        }
        trace_params.trace_stream << std::endl <<
          "  </step>" << std::endl;
      }

      if(campaign->keyword_based())
      {
        trace_params.trace_stream <<
          "  <step name=\"negative keywords checking\" passed=\"unknown\"/>" <<
          std::endl;
      }

      // TODO: check campaign ecpm

      ConstCreativePtrList available_creatives;

      filter_creatives(
        key,
        key.tag,
        0,
        campaign,
        profiling_available,
        full_freq_caps,
        SeqOrderMap(),
        available_creatives,
        !campaign->keyword_based(),
        up_expand_space,
        right_expand_space,
        down_expand_space,
        left_expand_space,
        video_min_duration,
        video_max_duration,
        video_skippable_max_duration,
        video_allow_skippable,
        video_allow_unskippable,
        allowed_durations,
        exclude_categories,
        required_categories,
        secure,
        filter_empty_destination,
        &trace_params);

      /*
       * Note the same 'tag visibility' checking and 'frequency caps' checking are used
       * in the function CampaignManagerImpl::get_site_creative_().
       * This checks are used to prevent 'auction type check' step printing
       * in inappropriate case in debug.ccg mode.
       * Note if 'visibility checking' and 'frequency caps' checking are not passed
       * ad_slot_debug_info->auction_type/trace_params->auction_type will have default
       * value = AT_RANDOM.
       */
      if (!CampaignIndex::check_tag_visibility(tag_visibility, key.tag, &trace_params) &&
          !CampaignIndex::check_site_freq_cap(profiling_available, full_freq_caps, key.tag, &trace_params))
      {
        bool result =
          (campaign->mode == CM_RANDOM && trace_params.auction_type == AT_RANDOM) ||
          (campaign->mode != CM_RANDOM && trace_params.auction_type != AT_RANDOM);
        trace_params.trace_stream <<
          "  <step name=\"auction type check\" passed=\"" <<
          (result ? "yes" : "no") << "\">" << std::endl <<
          "    CCG mode = '" << to_str(campaign->mode) <<
          "'; auction mode = '" << to_str(trace_params.auction_type) << "'" << std::endl <<
          "  </step>" << std::endl;
      }

      cell_holder_.reset();
      campaign_cell_holder_.reset();
    }

    std::string
    CampaignIndex::decode_match_status_type_(
      unsigned char match_status_type) noexcept
    {
      char res[] = {
        (match_status_type & 0xF) == US_NONE ? '*' :
          ((match_status_type & 0xF) == US_OPTIN ? 'I' :
            ((match_status_type & 0xF) == US_OPTOUT ? 'O' : 'U')),
        'R',
        (match_status_type >> 4) & ST_TEST_REAL ? 'T' : 'N',
        0 };
      
      return res;
    }

    void print_campaign_cell_list_(
      std::ostream& ostr,
      const CampaignCellList* campaign_cell_list) noexcept
    {
      ostr << "(" << campaign_cell_list << "): ";
      if(campaign_cell_list)
      {
        for(CampaignCellList::const_iterator it = campaign_cell_list->begin();
            it != campaign_cell_list->end(); ++it)
        {
          if(it != campaign_cell_list->begin())
          {
            ostr << ", ";
          }
          ostr << (*it)->campaign->campaign_id;
        }
      }
    }

    void print_selection_campaign_cell_list_(
      std::ostream& ostr,
      const CampaignSelectionCellList* campaign_selection_cell_list)
      noexcept
    {
      ostr << "(" << campaign_selection_cell_list << "): ";
      if(campaign_selection_cell_list)
      {
        for(CampaignSelectionCellList::const_iterator cmp_it =
              campaign_selection_cell_list->begin();
            cmp_it != campaign_selection_cell_list->end();
            ++cmp_it)
        {
          if(cmp_it != campaign_selection_cell_list->begin())
          {
            ostr << ", ";
          }

          ostr << "(" <<
            ((*cmp_it).in() ? (*cmp_it)->campaign->campaign_id : 0) << ", " <<
            ((*cmp_it).in() ? (*cmp_it)->tag_pricing->site_rate_id : 0) << ", " <<
            ((*cmp_it).in() ? (*cmp_it)->ecpm : RevenueDecimal::ZERO).str() << ")";
        }
      }
    }

    void
    CampaignIndex::trace_tree(std::ostream& ostr) noexcept
    {
      const char OFFSET[] = "  ";

      ostr << "=== Campaign Tree ===" << std::endl <<
        "(match-type, colo-id, tid, country, app-format)" << std::endl;
      for(OrderedCampaignMap::const_iterator it = ordered_campaigns_.begin();
          it != ordered_campaigns_.end(); ++it)
      {
        ostr << "(" << decode_match_status_type_(it->first.match_status_type) <<
          ", " <<
          it->first.tag_id << ", " <<
          (it->first.country_code[0] ? it->first.country_code[0] : ' ')<< 
          (it->first.country_code[0] && it->first.country_code[1] ?
            it->first.country_code[1] : ' ') << ", " <<
          it->first.app_format << "): " << std::endl;

        ostr << OFFSET << "wg-display-campaigns ";
        print_selection_campaign_cell_list_(
          ostr, it->second.wg_display_campaigns.in());
        ostr << std::endl;
        ostr << OFFSET << "display-campaigns ";
        print_selection_campaign_cell_list_(
          ostr, it->second.display_campaigns.in());
        ostr << std::endl;
        ostr << OFFSET << "text-campaigns ";
        print_campaign_cell_list_(ostr, it->second.text_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "keyword-campaigns ";
        print_campaign_cell_list_(ostr, it->second.keyword_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "wg-display-random-campaigns ";
        print_selection_campaign_cell_list_(
          ostr, it->second.wg_display_random_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "display-random-campaigns ";
        print_selection_campaign_cell_list_(
          ostr, it->second.display_random_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "text-random-campaigns ";
        print_campaign_cell_list_(ostr, it->second.text_random_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "keyword-random-campaigns ";
        print_campaign_cell_list_(ostr, it->second.keyword_random_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "lost-wg-campaigns ";
        print_campaign_cell_list_(ostr, it->second.lost_wg_campaigns);
        ostr << std::endl;
        ostr << OFFSET << "lost-campaigns ";
        print_campaign_cell_list_(ostr, it->second.lost_campaigns);
        ostr << std::endl;
      }
    }
  } /* CampaignSvcs */
} /* AdServer */

