
#include <vector>

#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>
#include <Commons/PathManip.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>

#include "CreativeTemplate.hpp"
#include "CampaignManagerImpl.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace
    {
      typedef Generics::LastPtr<CampaignConfig> LastCampaignConfig_var;
      typedef Generics::LastPtr<CampaignIndex> LastCampaignIndex_var;

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      
      AdServer::CampaignSvcs::CreativeTemplateType
      adopt_template_type(
        CreativeTemplateFactory::Handler::Type type_val)
      {
        if(type_val == CreativeTemplateFactory::Handler::CTT_TEXT)
        {
          return AdServer::CampaignSvcs::CTT_TEXT;
        }
        else if(type_val == CreativeTemplateFactory::Handler::CTT_XSLT)
        {
          return AdServer::CampaignSvcs::CTT_XSLT;
        }

        throw Exception("Unknown template type");
      }

      void
      pack_option_token_map(
        AdServer::CampaignSvcs::OptionValueSeq& res,
        const OptionTokenValueMap& option_values)
      {
        res.length(option_values.size());
        CORBA::ULong i = 0;
        for(OptionTokenValueMap::const_iterator it = option_values.begin();
            it != option_values.end(); ++it, ++i)
        {
          res[i].option_id = it->second.option_id;
          res[i].value << it->second.value;
        }
      }
    }

    void
    CampaignManagerImpl::check_config() noexcept
    {
      static const char* FUN = "CampaignManagerImpl::check_config()";

      CampaignIndex_var configuration_index;
      CampaignConfig_var new_config;

      try
      {
        CampaignConfig_var old_config = configuration();
        new_config = campaign_config_source_->update(old_config);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        callback_->critical(ostr.str(), "ADS-ICON-5000");
      }

      Generics::Time master_stamp;

      if(new_config.in())
      {
        master_stamp = new_config->master_stamp;
      }
      else
      {
        CampaignConfig_var old_config = configuration();
        if(old_config.in())
        {
          master_stamp = old_config->master_stamp;
        }
      }

      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        bool config_expired =
          campaign_manager_config_.campaigns_update_timeout() != 0 &&
          master_stamp != Generics::Time::ZERO && //there is old or new config
          now - master_stamp > Generics::Time(
            campaign_manager_config_.campaigns_update_timeout());

        if(config_expired)
        {
          logger_->stream(Logging::Logger::ERROR,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-5093") << FUN <<
            ": Config expired - disable ad showing: config timestamp = " <<
            master_stamp.get_gm_time();

          SyncPolicy::WriteGuard guard(lock_);
          configuration_index_.swap(configuration_index);
        }
        else if(new_config.in())
        {
          if (logger_->log_level() >= TraceLevel::MIDDLE)
          {
            logger_->stream(TraceLevel::MIDDLE,
              Aspect::CAMPAIGN_MANAGER) << FUN <<
              ": To construct campaign index for " <<
              new_config->campaigns.size() << " campaigns, " <<
              new_config->tags.size() << " tags.";
          }

          configuration_index = new CampaignIndex(new_config, logger_);

          if(configuration_index->index_campaigns(
               &indexing_progress_,
               this))
          {
            if (logger_->log_level() >= TraceLevel::MIDDLE)
            {
              logger_->stream(TraceLevel::MIDDLE,
                Aspect::CAMPAIGN_MANAGER) << FUN <<
                ": Campaign index constructed.";
            }

            precalculate_pub_pixel_accounts_(new_config);

            SyncPolicy::WriteGuard guard(lock_);
            configuration_index_.swap(configuration_index);
            configuration_.swap(new_config);
          }
          else if (logger_->log_level() >= TraceLevel::MIDDLE)
          {
            logger_->stream(TraceLevel::MIDDLE,
              Aspect::CAMPAIGN_MANAGER) << FUN <<
              ": Campaign indexing interrupted.";
          }
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        callback_->critical(ostr.str(), "ADS-IMPL-5091");
      }

      if (configuration_index.in())
      {
        LastCampaignIndex_var last_campaign_index(configuration_index.retn());
      }
      if (new_config.in())
      {
        LastCampaignConfig_var last_campaign_config(new_config.retn());
      }

      try
      {
        CampaignManagerTaskMessage_var msg =
          new CheckConfigTaskMessage(this, update_task_runner_);

        Generics::Time tm = Generics::Time::get_time_of_day() +
          campaign_manager_config_.config_update_period();

        scheduler_->schedule(msg, tm);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Exception caught while enqueueing CheckConfig task: " << e.what();

        callback_->critical(ostr.str(), "ADS-IMPL-5092");
      }
    }

    template<typename PredProviderType>
    ReferenceCounting::SmartPtr<const PredProviderType>
    CampaignManagerImpl::update_rate_provider_(
      const PredProviderType* old_ctr_provider,
      const String::SubString& capture_root,
      const String::SubString& res_root,
      const Generics::Time& expire_timeout)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::update_rate_provider_()";

      const Generics::Time now = Generics::Time::get_time_of_day();

      // check that appeared new dir
      try
      {
        std::string new_ctr_config_root;
        std::string new_ctr_config_root_error;

        bool config_captured = true;
        Generics::Time new_config_timestamp;

        ReferenceCounting::SmartPtr<const PredProviderType> new_ctr_provider;

        if(!old_ctr_provider)
        {
          new_config_timestamp = PredProviderType::check_config_appearance(
            new_ctr_config_root,
            capture_root);
        }

        if(new_ctr_config_root.empty())
        {
          new_config_timestamp = PredProviderType::check_config_appearance(
            new_ctr_config_root,
            res_root);
          config_captured = false;
        }

        if(!new_ctr_config_root.empty())
        {
          bool config_not_expired =
            expire_timeout == Generics::Time::ZERO ||
            now <= new_config_timestamp + expire_timeout;

          if(config_captured || (
               (!old_ctr_provider ||
                new_config_timestamp > old_ctr_provider->config_timestamp()) &&
               config_not_expired))
            // don't load config if it already expired, but load if it captured
            // for remove (will be reloaded or skipped at next interation)
          {
            // capture CTRConfig folder
            {
              std::string new_ctr_config_path;
              std::string new_ctr_config_name;

              AdServer::PathManip::split_path(
                new_ctr_config_root.c_str(),
                &new_ctr_config_path,
                &new_ctr_config_name,
                true);

              std::string captured_ctr_config_root = capture_root.str() +
                "/" + new_ctr_config_name;

              new_ctr_config_root_error = captured_ctr_config_root + ".error";

              // check that this config isn't invalidated in past
              // otherwise skip it and wait config with greatest timestamp
              if(::access(new_ctr_config_root_error.c_str(), F_OK) != 0)
              {
                if(!config_captured)
                {
                  int rename_res = ::rename(new_ctr_config_root.c_str(),
                    captured_ctr_config_root.c_str());

                  if(rename_res && errno != EEXIST)
                  {
                    eh::throw_errno_exception<typename PredProviderType::InvalidConfig>(
                      "Can't rename file '",
                      new_ctr_config_root,
                      "' to '",
                      captured_ctr_config_root.c_str(),
                      "'");
                  }

                  new_ctr_config_root = captured_ctr_config_root;
                }
              }
              else
              {
                new_ctr_config_root.clear();
              }
            }

            if(!new_ctr_config_root.empty())
            {
              try
              {
                new_ctr_provider = new PredProviderType(
                  new_ctr_config_root,
                  new_config_timestamp,
                  task_runner_);

                if(old_ctr_provider)
                {
                  old_ctr_provider->remove_config_files_at_destroy(true);
                }

                return new_ctr_provider;
              }
              catch(const eh::Exception& ex)
              {
                ::rename(new_ctr_config_root.c_str(),
                  new_ctr_config_root_error.c_str());

                Stream::Error ostr;
                ostr << FUN << ": can't load CTR config '" <<
                  new_ctr_config_root << "': " << ex.what();
                throw Exception(ostr);
              }
            }
          }
        } // !new_ctr_config_root.empty()
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        callback_->critical(ostr.str(), "ADS-IMPL-5091");
      }

      // check expiration for old provider
      if(expire_timeout != Generics::Time::ZERO && old_ctr_provider)
      {
        Generics::Time ctr_config_timestamp = old_ctr_provider->config_timestamp();

        // check that ctr isn't expired
        if(now > ctr_config_timestamp + expire_timeout)
        {
          old_ctr_provider->remove_config_files_at_destroy(true);

          Stream::Error ostr;
          ostr << FUN << ": CTR config expired and will be disabled, "
            "config timestamp = " << ctr_config_timestamp.gm_ft();
          callback_->warning(ostr.str(), "ADS-IMPL-5091");

          return ReferenceCounting::SmartPtr<const PredProviderType>();
        }
      }

      return ReferenceCounting::add_ref(old_ctr_provider);
    }

    void
    CampaignManagerImpl::update_ctr_provider() noexcept
    {
      static const char* FUN = "CampaignManagerImpl::update_ctr_provider()";

      if(campaign_manager_config_.CTRConfig().present())
      {
        try
        {
          ctr_provider_ = update_rate_provider_(
            ctr_provider_.get().in(),
            campaign_manager_config_.CTRConfig()->capture_root(),
            campaign_manager_config_.CTRConfig()->root(),
            Generics::Time(campaign_manager_config_.CTRConfig()->expire_timeout()));
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << e.what();
          callback_->critical(ostr.str(), "ADS-IMPL-5091");
        }

        try
        {
          CampaignManagerTaskMessage_var msg =
            new UpdateCTRProviderTask(this, task_runner_);

          Generics::Time tm = Generics::Time::get_time_of_day() +
            campaign_manager_config_.CTRConfig()->check_period();

          scheduler_->schedule(msg, tm);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": exception caught while enqueueing update CTR task: " << e.what();
          callback_->critical(ostr.str(), "ADS-IMPL-5092");
        }
      }
    }

    void
    CampaignManagerImpl::update_conv_rate_provider() noexcept
    {
      static const char* FUN = "CampaignManagerImpl::update_conv_rate_provider()";

      if(campaign_manager_config_.ConvRateConfig().present())
      {
        try
        {
          conv_rate_provider_ = update_rate_provider_(
            conv_rate_provider_.get().in(),
            campaign_manager_config_.ConvRateConfig()->capture_root(),
            campaign_manager_config_.ConvRateConfig()->root(),
            Generics::Time::ZERO);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << e.what();
          callback_->critical(ostr.str(), "ADS-IMPL-5091");
        }

        try
        {
          CampaignManagerTaskMessage_var msg =
            new UpdateConvRateProviderTask(this, task_runner_);

          Generics::Time tm = Generics::Time::get_time_of_day() +
            campaign_manager_config_.ConvRateConfig()->check_period();

          scheduler_->schedule(msg, tm);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": exception caught while enqueueing update ConvRate task: " << e.what();
          callback_->critical(ostr.str(), "ADS-IMPL-5092");
        }
      }
    }

    void
    CampaignManagerImpl::update_bid_cost_provider() noexcept
    {
      static const char* FUN = "CampaignManagerImpl::update_bid_cost_provider()";

      if(campaign_manager_config_.BidCostConfig().present())
      {
        try
        {
          bid_cost_provider_ = update_rate_provider_(
            bid_cost_provider_.get().in(),
            campaign_manager_config_.BidCostConfig()->capture_root(),
            campaign_manager_config_.BidCostConfig()->root(),
            Generics::Time(campaign_manager_config_.BidCostConfig()->expire_timeout()));
        }
        catch(const eh::Exception& e)
        {
          std::cerr << "From update bid cost provider: " << e.what() << std::endl;

          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << e.what();
          callback_->critical(ostr.str(), "ADS-IMPL-5091");
        }

        try
        {
          CampaignManagerTaskMessage_var msg =
            new UpdateBidCostProviderTask(this, task_runner_);

          Generics::Time tm = Generics::Time::get_time_of_day() +
            campaign_manager_config_.BidCostConfig()->check_period();

          scheduler_->schedule(msg, tm);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": exception caught while enqueueing update CTR task: " << e.what();
          callback_->critical(ostr.str(), "ADS-IMPL-5092");
        }
      }
    }

    AdServer::CampaignSvcs::CampaignManager::CampaignConfig*
    CampaignManagerImpl::get_config(
      const AdServer::CampaignSvcs::
        CampaignManager::GetConfigInfo& get_config_props)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      CampaignConfig_var config = configuration();

      if(config.in() == 0)
      {
        config = new CampaignConfig();
        config->cost_limit = RevenueDecimal::ZERO;
      }

      ::AdServer::CampaignSvcs::CampaignManager::CampaignConfig_var result =
          new ::AdServer::CampaignSvcs::CampaignManager::CampaignConfig();

      result->master_stamp =
        CorbaAlgs::pack_time(config->master_stamp);
      result->first_load_stamp =
        CorbaAlgs::pack_time(config->first_load_stamp);
      result->finish_load_stamp =
        CorbaAlgs::pack_time(config->finish_load_stamp);
      result->global_params_timestamp =
        CorbaAlgs::pack_time(config->global_params_timestamp);

      {
        // fill sizes
        result->sizes.length(config->sizes.size());

        CORBA::ULong i = 0;
        for(SizeMap::const_iterator size_it = config->sizes.begin();
            size_it != config->sizes.end(); ++size_it, ++i)
        {
          SizeInfo& size_info = result->sizes[i];
          size_info.size_id = size_it->first;
          size_info.protocol_name << size_it->second->protocol_name;
          size_info.size_type_id = size_it->second->size_type_id;
          size_info.width = size_it->second->width;
          size_info.height = size_it->second->height;
          size_info.timestamp = CorbaAlgs::pack_time(size_it->second->timestamp);          
        }
      }

      // fill app formats
      {
        result->app_formats.length(config->app_formats.size());

        CORBA::ULong i = 0;
        for(AppFormatMap::const_iterator it =
              config->app_formats.begin();
            it != config->app_formats.end(); ++it, ++i)
        {
          AppFormatInfo& app_format_info = result->app_formats[i];
          const AppFormatDef& app_format = it->second;

          app_format_info.app_format << it->first;
          app_format_info.mime_format << app_format.mime_format;
          app_format_info.timestamp = CorbaAlgs::pack_time(app_format.timestamp);
        }
      }

      /* fill adv actions */
      {
        result->adv_actions.length(config->adv_actions.size());

        CORBA::ULong i = 0;
        for(AdvActionMap::const_iterator it =
              config->adv_actions.begin();
            it != config->adv_actions.end(); ++it, i++)
        {
          AdvActionInfo& adv_action_info = result->adv_actions[i];
          const AdvActionDef& ai = it->second;

          adv_action_info.action_id = ai.action_id;
          adv_action_info.timestamp = CorbaAlgs::pack_time(ai.timestamp);

          CorbaAlgs::fill_sequence(
            ai.ccg_ids.begin(),
            ai.ccg_ids.end(),
            adv_action_info.ccg_ids);

          CorbaAlgs::pack_decimal_into_seq(
            adv_action_info.ccg_ids,
            ai.cur_value);
        }
      }

      /* fill creative options */
      {
        result->creative_options.length(config->creative_options.size());

        CORBA::ULong i = 0;
        for(CreativeOptionMap::const_iterator it =
              config->creative_options.begin();
            it != config->creative_options.end(); ++it, i++)
        {
          CreativeOptionInfo& co_info = result->creative_options[i];
          const CreativeOptionDef& co = it->second;

          co_info.option_id = it->first;
          co_info.token << co.token;
          co_info.type = co.type;
          co_info.timestamp = CorbaAlgs::pack_time(co.timestamp);

          CorbaAlgs::fill_sequence(
            co.token_relations.begin(),
            co.token_relations.end(),
            co_info.token_relations);
        }
      }

      /* fill accounts */
      {
        result->accounts.length(config->accounts.size());

        CORBA::ULong i = 0;
        for(AccountMap::const_iterator it = config->accounts.begin();
            it != config->accounts.end(); ++it, i++)
        {
          AccountInfo& acc_info = result->accounts[i];

          acc_info.account_id = it->first;
          acc_info.agency_account_id = it->second->agency_account ?
            it->second->agency_account->account_id : 0;
          acc_info.internal_account_id = it->second->internal_account_id;
          acc_info.legal_name << it->second->legal_name;
          acc_info.flags = it->second->flags;
          acc_info.at_flags = it->second->at_flags;
          acc_info.text_adserving = it->second->text_adserving;
          acc_info.currency_id = it->second->currency->currency_id;
          acc_info.country << it->second->country;
          acc_info.commision = CorbaAlgs::pack_decimal(it->second->commision);
          acc_info.budget = CorbaAlgs::pack_decimal(it->second->budget);
          acc_info.paid_amount = CorbaAlgs::pack_decimal(it->second->paid_amount);
          acc_info.status = it->second->status;
          acc_info.eval_status = it->second->eval_status;
          acc_info.time_offset = CorbaAlgs::pack_time(it->second->time_offset);
          CorbaAlgs::fill_sequence(
            it->second->walled_garden_accounts.begin(),
            it->second->walled_garden_accounts.end(),
            acc_info.walled_garden_accounts);
          acc_info.auction_rate = static_cast<CORBA::ULong>(it->second->auction_rate);
          acc_info.use_pub_pixels = it->second->use_pub_pixels;
          acc_info.pub_pixel_optin << it->second->pub_pixel_optin;
          acc_info.pub_pixel_optout << it->second->pub_pixel_optout;
          acc_info.self_service_commission = CorbaAlgs::pack_decimal(
            it->second->self_service_commission);
          acc_info.timestamp = CorbaAlgs::pack_time(it->second->timestamp);
        }
      }

      /* fill campaigns */
      result->campaigns.length(config->campaigns.size());

      CORBA::ULong i = 0;
      for(CampaignConfig::CampaignMap::const_iterator it =
            config->campaigns.begin();
          it != config->campaigns.end(); ++it, i++)
      {
        CampaignInfo& campaign_info = result->campaigns[i].info;
        const Campaign* campaign = it->second;

        result->campaigns[i].ecpm = CorbaAlgs::pack_decimal(
          campaign->ecpm_);
        result->campaigns[i].ctr = CorbaAlgs::pack_decimal(
          campaign->ctr);

        campaign_info.campaign_id = campaign->campaign_id;
        campaign_info.timestamp = CorbaAlgs::pack_time(campaign->timestamp);
        campaign_info.campaign_group_id = campaign->campaign_group_id;
        campaign_info.flags = campaign->flags;
        campaign_info.marketplace = campaign->marketplace;
        campaign_info.status = campaign->status;
        campaign_info.eval_status = campaign->eval_status;
        campaign_info.ccg_rate_id = campaign->ccg_rate_id;
        campaign_info.ccg_rate_type = campaign->ccg_rate_type;
        campaign_info.account_id = campaign->account->account_id;
        campaign_info.advertiser_id = campaign->advertiser->account_id;

        campaign_info.imp_revenue = CorbaAlgs::pack_decimal(
          campaign->imp_revenue);
        campaign_info.click_revenue = CorbaAlgs::pack_decimal(
          campaign->click_revenue);
        campaign_info.action_revenue = CorbaAlgs::pack_decimal(
          campaign->action_revenue);
        campaign_info.commision = CorbaAlgs::pack_decimal(campaign->commision);

        campaign_info.ccg_type = campaign->ccg_type;
        campaign_info.target_type = campaign->targeting_type;

        pack_delivery_limits(
          campaign_info.campaign_delivery_limits,
          campaign->campaign_delivery_limits);
        pack_delivery_limits(
          campaign_info.ccg_delivery_limits,
          campaign->ccg_delivery_limits);

        campaign_info.max_pub_share = CorbaAlgs::pack_decimal(campaign->max_pub_share);
        campaign_info.bid_strategy = campaign->bid_strategy;
        campaign_info.min_ctr_goal = CorbaAlgs::pack_decimal(campaign->min_ctr_goal());
        campaign_info.start_user_group_id = campaign->start_user_group_id;
        campaign_info.end_user_group_id = campaign->end_user_group_id;
        campaign_info.ctr_reset_id = campaign->ctr_reset_id;
        campaign_info.mode = campaign->mode;
        campaign_info.min_uid_age = CorbaAlgs::pack_time(campaign->min_uid_age);
        campaign_info.seq_set_rotate_imps = campaign->seq_set_rotate_imps;

        campaign_info.fc_id = campaign->fc_id;
        campaign_info.group_fc_id = campaign->group_fc_id;

        campaign_info.flags = campaign->flags;
        campaign_info.country << campaign->country;
        campaign_info.delivery_coef = campaign->delivery_coef;

        fill_interval_sequence(
          campaign_info.weekly_run_intervals,
          campaign->weekly_run_intervals);

        CorbaAlgs::fill_sequence(
          campaign->sites.begin(),
          campaign->sites.end(),
          campaign_info.sites);

        pack_expression(
          campaign_info.expression,
          campaign->channel.in() ? campaign->channel->expression() :
            ExpressionChannel::Expression::EMPTY);

        pack_expression(
          campaign_info.stat_expression,
          campaign->stat_channel.in() ? campaign->stat_channel->expression() :
            ExpressionChannel::Expression::EMPTY);

        unsigned int j = 0;
        const CreativeList& cmp_creatives = campaign->get_creatives();

        campaign_info.creatives.length(cmp_creatives.size());
        for(CreativeList::const_iterator it = cmp_creatives.begin();
            it != cmp_creatives.end(); ++it, j++)
        {
          CreativeInfo& creative_info = campaign_info.creatives[j];
          const Creative* creative = *it;
          creative_info.ccid = creative->ccid;
          creative_info.creative_id = creative->creative_id;
          creative_info.fc_id = creative->fc_id;
          creative_info.weight = creative->weight;
          creative_info.creative_format << creative->creative_format;
          creative_info.version_id << creative->version_id;
          creative_info.order_set_id = creative->order_set_id;

          creative_info.click_url.option_id = creative->click_url.option_id;
          creative_info.click_url.value << creative->click_url.value;
          creative_info.status = creative->status;

          creative_info.sizes.length(creative->sizes.size());
          CORBA::ULong size_i = 0;
          for(Creative::SizeMap::const_iterator size_it = creative->sizes.begin();
              size_it != creative->sizes.end(); ++size_it, ++size_i)
          {
            CreativeSizeInfo& size_info = creative_info.sizes[size_i];
            size_info.size_id = size_it->first;
            size_info.up_expand_space = size_it->second.up_expand_space;
            size_info.right_expand_space = size_it->second.right_expand_space;
            size_info.down_expand_space = size_it->second.down_expand_space;
            size_info.left_expand_space = size_it->second.left_expand_space;
            pack_option_token_map(creative_info.sizes[size_i].tokens, size_it->second.tokens);
          }

          pack_option_token_map(creative_info.tokens, creative->tokens);

          CorbaAlgs::fill_sequence(
            creative->categories.begin(),
            creative->categories.end(),
            creative_info.categories);
        }

        CorbaAlgs::fill_sequence(
          campaign->colocations.begin(),
          campaign->colocations.end(),
          campaign_info.colocations);

        CorbaAlgs::fill_sequence(
          campaign->exclude_pub_accounts.begin(),
          campaign->exclude_pub_accounts.end(),
          campaign_info.exclude_pub_accounts);

        CORBA::ULong res_i = 0;
        campaign_info.exclude_tags.length(campaign->exclude_tags.size());
        for(TagDeliveryMap::const_iterator dtag_it =
              campaign->exclude_tags.begin();
            dtag_it != campaign->exclude_tags.end(); ++dtag_it, ++res_i)
        {
          campaign_info.exclude_tags[res_i].tag_id = dtag_it->first;
          campaign_info.exclude_tags[res_i].delivery_value = dtag_it->second;          
        }

        fill_campaign_contracts_(
          campaign_info.contracts,
          campaign->contracts);
      }

      /* fill expression channels */
      result->expression_channels.length(config->expression_channels.size());
      CORBA::ULong ch_i = 0;
      for(CampaignConfig::ChannelMap::const_iterator ch_it =
            config->expression_channels.begin();
          ch_it != config->expression_channels.end(); ++ch_it)
      {
        if(ch_it->second->channel.in())
        {
          pack_channel(result->expression_channels[ch_i++], ch_it->second->channel);
        }
      }
      result->expression_channels.length(ch_i);
      
      /* fill sites */
      unsigned int len = config->sites.size();
      result->sites.length(len);
      SiteMap::const_iterator pt = config->sites.begin();

      for (CORBA::ULong k = 0; k < len; k++, pt++)
      {
        SiteInfo& site_info = result->sites[k];
        site_info.site_id = pt->first;
        site_info.timestamp = CorbaAlgs::pack_time(pt->second->timestamp);
        site_info.freq_cap_id = pt->second->freq_cap_id;
        site_info.noads_timeout = pt->second->noads_timeout;
        site_info.status = pt->second->status;
        site_info.flags = pt->second->flags;
        site_info.account_id = pt->second->account->account_id;

        /* fill creative category accepted & rejected for site */
        CorbaAlgs::fill_sequence(
          pt->second->approved_creative_categories.begin(),
          pt->second->approved_creative_categories.end(),
          site_info.approved_creative_categories);

        CorbaAlgs::fill_sequence(
          pt->second->rejected_creative_categories.begin(),
          pt->second->rejected_creative_categories.end(),
          site_info.rejected_creative_categories);

        CorbaAlgs::fill_sequence(
          pt->second->approved_creatives.begin(),
          pt->second->approved_creatives.end(),
          site_info.approved_creatives);

        CorbaAlgs::fill_sequence(
          pt->second->rejected_creatives.begin(),
          pt->second->rejected_creatives.end(),
          site_info.rejected_creatives);
      }

      {
        result->tags.length(config->tags.size());

        CORBA::ULong k = 0;

        for(TagMap::const_iterator pt = config->tags.begin();
            pt != config->tags.end();
            ++pt, ++k)
        {
          AdServer::CampaignSvcs::CampaignManager::AdaptedTagInfo&
            adapted_tag_info = result->tags[k];
          TagInfo& tag_info = adapted_tag_info.info;
          const Tag* tag = pt->second;

          tag_info.tag_id = pt->first;
          tag_info.site_id = tag->site->site_id;

          tag_info.sizes.length(pt->second->sizes.size());
          CORBA::ULong size_i = 0;
          for(Tag::SizeMap::const_iterator size_it = pt->second->sizes.begin();
              size_it != pt->second->sizes.end(); ++size_it, ++size_i)
          {
            tag_info.sizes[size_i].size_id = size_it->first;
            tag_info.sizes[size_i].max_text_creatives = size_it->second->max_text_creatives;
            pack_option_token_map(tag_info.sizes[size_i].tokens, size_it->second->tokens);
          }

          tag_info.allow_expandable = tag->allow_expandable;

          tag_info.imp_track_pixel << tag->imp_track_pixel;
          tag_info.passback << tag->passback;
          tag_info.passback_type << tag->passback_type;
          tag_info.flags = tag->flags;
          tag_info.marketplace = tag->marketplace;
          tag_info.adjustment = CorbaAlgs::pack_decimal(tag->adjustment);
          tag_info.auction_max_ecpm_share = CorbaAlgs::pack_decimal(
            tag->auction_max_ecpm_share);
          tag_info.auction_prop_probability_share = CorbaAlgs::pack_decimal(
            tag->auction_prop_probability_share);
          tag_info.auction_random_share = CorbaAlgs::pack_decimal(
            tag->auction_random_share);
          tag_info.cost_coef = CorbaAlgs::pack_decimal(tag->cost_coef);

          tag_info.tag_pricings_timestamp = CorbaAlgs::pack_time(
            tag->tag_pricings_timestamp);
          pack_option_token_map(tag_info.tokens, tag->tokens);
          pack_option_token_map(tag_info.hidden_tokens, tag->hidden_tokens);
          pack_option_token_map(tag_info.passback_tokens, tag->passback_tokens);

          tag_info.template_tokens.length(tag->template_tokens.size());
          CORBA::ULong templ_token_i = 0;
          for(Tag::TemplateOptionTokenValueMap::const_iterator templ_token_it =
                tag->template_tokens.begin();
              templ_token_it != tag->template_tokens.end();
              ++templ_token_it, ++templ_token_i)
          {
            tag_info.template_tokens[templ_token_i].template_name <<
              templ_token_it->first;

            pack_option_token_map(
              tag_info.template_tokens[templ_token_i].tokens,
              templ_token_it->second);
          }

          tag_info.timestamp = CorbaAlgs::pack_time(tag->timestamp);

          const Tag::TagPricings& tag_pricings_list = tag->tag_pricings;
          unsigned int tag_pricings_len = tag_pricings_list.size();
          tag_info.tag_pricings.length(tag_pricings_len);

          Tag::TagPricings::const_iterator ci = tag_pricings_list.begin();
          for (unsigned int l = 0; l < tag_pricings_len; l++, ci++)
          {
            TagPricingInfo& tag_pricing = tag_info.tag_pricings[l];
            tag_pricing.country_code << ci->first.country_code;
            tag_pricing.ccg_type = ci->first.ccg_type;
            tag_pricing.ccg_rate_type = ci->first.ccg_rate_type;
            tag_pricing.site_rate_id = ci->second.site_rate_id;
            tag_pricing.imp_revenue = CorbaAlgs::pack_decimal(
              ci->second.imp_revenue);
            tag_pricing.revenue_share = CorbaAlgs::pack_decimal(
              ci->second.revenue_share);
          }
          CorbaAlgs::fill_sequence(
            tag->accepted_categories.begin(),
            tag->accepted_categories.end(),
            tag_info.accepted_categories);

          CorbaAlgs::fill_sequence(
            tag->rejected_categories.begin(),
            tag->rejected_categories.end(),
            tag_info.rejected_categories);

          adapted_tag_info.cpms.length(tag_pricings_len);
          Tag::TagPricings::const_iterator cpm_it = tag_pricings_list.begin();
          for(CORBA::ULong cpm_i = 0; cpm_i < tag_pricings_len; ++cpm_i, ++cpm_it)
          {
            adapted_tag_info.cpms[cpm_i] = CorbaAlgs::pack_decimal(
              cpm_it->second.cpm);
          }
        }
      }

      i = 0;
      result->frequency_caps.length(config->freq_caps.size());
      for(FreqCapMap::const_iterator it = config->freq_caps.begin();
          it != config->freq_caps.end(); ++it, i++)
      {
        FreqCapInfo& freq_cap_info = result->frequency_caps[i];
        const FreqCap& freq_cap = it->second;

        freq_cap_info.fc_id = freq_cap.fc_id;
        freq_cap_info.timestamp = CorbaAlgs::pack_time(freq_cap.timestamp);
        freq_cap_info.lifelimit = freq_cap.lifelimit;
        freq_cap_info.period = freq_cap.period.tv_sec;
        freq_cap_info.window_limit = freq_cap.window_limit;
        freq_cap_info.window_time = freq_cap.window_time.tv_sec;
      }

      result->currency_exchange_id = config->currency_exchange_id;
      result->fraud_user_deactivate_period = CorbaAlgs::pack_time(
        config->fraud_user_deactivate_period);
      result->cost_limit = CorbaAlgs::pack_decimal(config->cost_limit);
      result->google_publisher_account_id = config->google_publisher_account_id;

      {
        // fill colocations
        i = 0;
        result->colocations.length(config->colocations.size());
        for (CampaignConfig::ColocationMap::const_iterator it =
               config->colocations.begin();
             it != config->colocations.end();
             ++it, i++)
        {
          ColocationInfo& colo = result->colocations[i];
          colo.colo_id = it->first;
          colo.colo_name << it->second->colo_name;
          colo.colo_rate_id = it->second->colo_rate_id;
          colo.account_id = it->second->account->account_id;
          colo.at_flags = it->second->at_flags;
          colo.revenue_share = CorbaAlgs::pack_decimal(it->second->revenue_share);
          colo.ad_serving = it->second->ad_serving;
          colo.hid_profile = it->second->hid_profile;
          pack_option_token_map(colo.tokens, it->second->tokens);
          colo.timestamp = CorbaAlgs::pack_time(it->second->timestamp);
        }
      }

      {
        // fill countries
        i = 0;
        result->countries.length(config->countries.size());
        for (CampaignConfig::CountryMap::iterator it =
               config->countries.begin();
             it != config->countries.end();
             ++it, i++)
        {
          CountryInfo& country = result->countries[i];
          country.country_code << it->first;
          pack_option_token_map(country.tokens, it->second->tokens);
          country.timestamp = CorbaAlgs::pack_time(it->second->timestamp);
        }
      }

      i = 0;
      result->creative_template_files.length(config->creative_templates.size());
      for(CreativeTemplateMap::const_iterator it =
            config->creative_templates.begin();
          it != config->creative_templates.end(); ++it, i++)
      {
        AdServer::CampaignSvcs::
        CampaignManager::CreativeTemplateFileInfo& ctf_info =
          result->creative_template_files[i];

        ctf_info.creative_format << it->first.creative_format;
        ctf_info.creative_size << it->first.creative_size;
        ctf_info.app_format << it->first.app_format;
        ctf_info.mime_format << it->second.mime_format;
        ctf_info.track_impr = it->second.track_impressions;
        ctf_info.template_file << it->second.file;
        ctf_info.type = adopt_template_type(it->second.type);
        ctf_info.status = it->second.status;
        pack_option_token_map(ctf_info.tokens, *(it->second.tokens));
        pack_option_token_map(ctf_info.hidden_tokens, *(it->second.hidden_tokens));
        ctf_info.timestamp = CorbaAlgs::pack_time(it->second.timestamp);
      }

      i = 0;
      result->currencies.length(config->currencies.size());
      for (CurrencyMap::const_iterator it = config->currencies.begin();
        it != config->currencies.end(); ++it, ++i)
      {
        CurrencyInfo& currency_info = result->currencies[i];
        currency_info.currency_id = it->first;
        currency_info.currency_exchange_id = it->second->currency_exchange_id;
        currency_info.effective_date = it->second->effective_date;
        currency_info.rate = CorbaAlgs::pack_decimal(it->second->rate);
        currency_info.fraction_digits = it->second->fraction;
        currency_info.currency_code << it->second->currency_code;
        currency_info.timestamp = CorbaAlgs::pack_time(it->second->timestamp);
      }

      {
        CORBA::ULong i = 0;
        result->creative_categories.length(config->creative_categories.size());

        for(CampaignConfig::CreativeCategoryMap::const_iterator ccat_it =
              config->creative_categories.begin();
            ccat_it != config->creative_categories.end();
            ++ccat_it, ++i)
        {
          CreativeCategoryInfo& ccat_info = result->creative_categories[i];
          ccat_info.creative_category_id = ccat_it->first;
          ccat_info.cct_id = ccat_it->second.cct_id;
          ccat_info.name << ccat_it->second.name;
          ccat_info.external_categories.length(
            ccat_it->second.external_categories.size());
          CORBA::ULong ec_i = 0;
          for(CreativeCategory::ExternalCategoryMap::const_iterator ec_it =
                ccat_it->second.external_categories.begin();
              ec_it != ccat_it->second.external_categories.end();
              ++ec_it, ++ec_i)
          {
            ccat_info.external_categories[ec_i].ad_request_type =
              ec_it->first;
            CorbaAlgs::fill_sequence(
              ec_it->second.begin(),
              ec_it->second.end(),
              ccat_info.external_categories[ec_i].names);
          }

          result->creative_categories[i].timestamp = CorbaAlgs::pack_time(
            ccat_it->second.timestamp);
        }
      }

      {
        CORBA::ULong i = 0;
        result->category_channels.length(config->category_channels.size());

        for(CampaignConfig::CategoryChannelMap::const_iterator cc_it =
              config->category_channels.begin();
            cc_it != config->category_channels.end();
            ++cc_it, ++i)
        {
          result->category_channels[i].channel_id = cc_it->first;
          result->category_channels[i].name << cc_it->second->name;
          result->category_channels[i].newsgate_name << cc_it->second->newsgate_name;
          result->category_channels[i].flags = cc_it->second->flags;
          result->category_channels[i].parent_channel_id =
            cc_it->second->parent_channel_id;
          result->category_channels[i].localizations.length(
            cc_it->second->localizations.size());
          CORBA::ULong loc_i = 0;
          for(CategoryChannel::LocalizationMap::const_iterator loc_it =
                cc_it->second->localizations.begin();
              loc_it != cc_it->second->localizations.end(); ++loc_it, ++loc_i)
          {
            result->category_channels[i].localizations[loc_i].language <<
              loc_it->first;
            result->category_channels[i].localizations[loc_i].name <<
              loc_it->second;
          }
          result->category_channels[i].timestamp =
            CorbaAlgs::pack_time(cc_it->second->timestamp);
        }
      }

      {
        // fill keywords
        result->campaign_keywords.length(config->ccg_keyword_click_info_map.size());

        CORBA::ULong i = 0;

        for(CCGKeywordPostClickInfoMap::const_iterator kit =
              config->ccg_keyword_click_info_map.begin();
            kit != config->ccg_keyword_click_info_map.end();
            ++kit, ++i)
        {
          AdServer::CampaignSvcs::CampaignKeywordInfo& kw_info =
            result->campaign_keywords[i];
          kw_info.ccg_keyword_id = kit->first;
          kw_info.original_keyword << kit->second.original_keyword;
          kw_info.click_url << kit->second.click_url;
          kw_info.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
        }
      }

      if(get_config_props.geo_channels)
      {
        CORBA::ULong channels_count = config->geo_channels->channels().size();
        result->geo_channels.length(channels_count);
        CORBA::ULong i = 0;

        for(GeoChannelIndex::GeoChannelMap::const_iterator ind_it =
              config->geo_channels->channels().begin();
            ind_it != config->geo_channels->channels().end();
            ++ind_it, ++i)
        {
          GeoChannelInfo& geo_channel_info = result->geo_channels[i];
          geo_channel_info.channel_id = ind_it->second;
          geo_channel_info.country << ind_it->first.country();
          geo_channel_info.geoip_targets.length(1);
          geo_channel_info.geoip_targets[0].region << ind_it->first.region();
          geo_channel_info.geoip_targets[0].city << ind_it->first.city();
          geo_channel_info.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
        }

        CORBA::ULong geo_coord_channels_count = 0;
        for(GeoCoordChannelIndex::ChannelMap::const_iterator ch_it =
              config->geo_coord_channels->channels().begin();
            ch_it != config->geo_coord_channels->channels().end();
            ++ch_it)
        {
          geo_coord_channels_count += ch_it->second->channels.size();
        }
        
        result->geo_coord_channels.length(geo_coord_channels_count);
        i = 0;

        for(GeoCoordChannelIndex::ChannelMap::const_iterator ch_it =
              config->geo_coord_channels->channels().begin();
            ch_it != config->geo_coord_channels->channels().end();
            ++ch_it)
        {
          for(ChannelIdList::const_iterator sub_ch_it =
                ch_it->second->channels.begin();
              sub_ch_it != ch_it->second->channels.end();
              ++sub_ch_it, ++i)
          {
            GeoCoordChannelInfo& geo_coord_channel_info =
              result->geo_coord_channels[i];
            geo_coord_channel_info.channel_id = *sub_ch_it;
            geo_coord_channel_info.longitude = CorbaAlgs::pack_decimal(
              ch_it->first.longitude);
            geo_coord_channel_info.latitude = CorbaAlgs::pack_decimal(
              ch_it->first.latitude);
            geo_coord_channel_info.radius = CorbaAlgs::pack_decimal(
              ch_it->first.accuracy);
            geo_coord_channel_info.timestamp = CorbaAlgs::pack_time(
              Generics::Time::ZERO);
          }
        }
      }

      result->web_operations.length(config->web_operations.size());
      i = 0;
      for(WebOperationHash::const_iterator it = config->web_operations.begin();
          it != config->web_operations.end(); ++it, i++)
      {
        WebOperationInfo& info = result->web_operations[i];
        info.id = it->second->id;
        info.app << it->second->app;
        info.source << it->second->source;
        info.operation << it->second->operation;
        info.flags = it->second->flags;
        info.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
      }

      return result._retn();
    }

    void
    CampaignManagerImpl::precalculate_pub_pixel_accounts_(
      CampaignConfig* campaign_config)
      /*throw(eh::Exception)*/
    {
      if (!country_whitelist_.empty())
      {
        PubPixelAccountMap pub_pixel_accounts;

        for (CountryList::const_iterator ci = country_whitelist_.begin();
             ci != country_whitelist_.end(); ++ci)
        {
          for (PubPixelAccountMap::const_iterator it =
                 campaign_config->pub_pixel_accounts.begin();
               it != campaign_config->pub_pixel_accounts.end(); ++it)
          {
            if (*ci == it->first.country)
            {
              AccountSet& accounts =
                pub_pixel_accounts[PubPixelAccountKey("", it->first.user_status)];
              accounts.insert(it->second.begin(), it->second.end());
            }
          }
        }

        campaign_config->pub_pixel_accounts.swap(pub_pixel_accounts);
      }
    }

    void
    CampaignManagerImpl::fill_campaign_contracts_(
      CampaignContractSeq& contract_seq,
      const Campaign::CampaignContractArray& contracts)
      noexcept
    {
      CORBA::ULong res_contract_i = contract_seq.length();
      contract_seq.length(contract_seq.length() + contracts.size());
      for(auto it = contracts.begin(); it != contracts.end(); ++it, ++res_contract_i)
      {
        CampaignContractInfo& res_contract = contract_seq[res_contract_i];
        const CampaignContract& contract = **it;
        res_contract.ord_contract_id << contract.ord_contract_id;
        res_contract.ord_ado_id << contract.ord_ado_id;
        res_contract.id << contract.id;
        res_contract.date << contract.date;
        res_contract.type << contract.type;
        res_contract.client_id << contract.client_id;
        res_contract.client_name << contract.client_name;
        res_contract.contractor_id << contract.contractor_id;
        res_contract.contractor_name << contract.contractor_name;
      }
    }
  }
}
