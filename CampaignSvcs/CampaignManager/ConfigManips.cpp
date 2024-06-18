
#include <vector>

#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>
#include <Commons/PathManip.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelGrpcAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/GrpcCampaignTypes.hpp>
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

      auto adopt_template_type_proto(
        CreativeTemplateFactory::Handler::Type type_val)
      {
        if(type_val == CreativeTemplateFactory::Handler::CTT_TEXT)
        {
          return Proto::CTT_TEXT;
        }
        else if(type_val == CreativeTemplateFactory::Handler::CTT_XSLT)
        {
          return Proto::CTT_XSLT;
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

      void
      pack_option_token_map(
        google::protobuf::RepeatedPtrField<Proto::OptionValueInfo>& res,
        const OptionTokenValueMap& option_values)
      {
        res.Reserve(option_values.size());
        for(const auto& [_, info]: option_values)
        {
          auto& element = *res.Add();
          element.set_option_id(info.option_id);
          element.set_value(info.value);
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

      // fill adv actions
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

      // fill creative options
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

      // fill accounts
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

      // fill campaigns
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
          creative_info.initial_contract_id = creative->initial_contract ?
            creative->initial_contract->contract_id : 0;

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
      }

      // fill expression channels
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
      
      // fill sites
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

        // fill creative category accepted & rejected for site
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

      // fill contracts
      {
        result->contracts.length(config->contracts.size());

        CORBA::ULong i = 0;
        for(CampaignConfig::ContractMap::const_iterator it = config->contracts.begin();
            it != config->contracts.end(); ++it, i++)
        {
          fill_contract_(result->contracts[i], *(it->second));
        }
      }

      return result._retn();
    }

    CampaignManagerImpl::GetConfigResponsePtr
    CampaignManagerImpl::get_config(GetConfigRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& get_config_props = request->get_config_props();

        CampaignConfig_var config = configuration();
        if(config.in() == nullptr)
        {
          config = new CampaignConfig();
          config->cost_limit = RevenueDecimal::ZERO;
        }

        auto response = create_grpc_response<Proto::GetConfigResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* campaign_config_response = info_response->mutable_campaign_config();

        campaign_config_response->set_master_stamp(
          GrpcAlgs::pack_time(config->master_stamp));
        campaign_config_response->set_first_load_stamp(
          GrpcAlgs::pack_time(config->first_load_stamp));
        campaign_config_response->set_finish_load_stamp(
          GrpcAlgs::pack_time(config->finish_load_stamp));
        campaign_config_response->set_global_params_timestamp(
          GrpcAlgs::pack_time(config->global_params_timestamp));

        {
          // fill sizes
          auto* sizes_response = campaign_config_response->mutable_sizes();
          sizes_response->Reserve(config->sizes.size());

          for(const auto& [key, info] : config->sizes)
          {
            auto& size_info_response = *sizes_response->Add();
            size_info_response.set_size_id(key);
            size_info_response.set_protocol_name(info->protocol_name);
            size_info_response.set_size_type_id(info->size_type_id);
            size_info_response.set_width(info->width);
            size_info_response.set_height(info->height);
            size_info_response.set_timestamp(
              GrpcAlgs::pack_time(info->timestamp));
          }
        }

        // fill app formats
        {
          auto* app_formats_response = campaign_config_response->mutable_app_formats();
          app_formats_response->Reserve(config->app_formats.size());

          for(const auto& [key, info] : config->app_formats)
          {
            auto& app_format_info_response = *app_formats_response->Add();
            app_format_info_response.set_app_format(key);
            app_format_info_response.set_mime_format(info.mime_format);
            app_format_info_response.set_timestamp(GrpcAlgs::pack_time(info.timestamp));
          }
        }

        // fill adv actions
        {
          auto* adv_actions_response = campaign_config_response->mutable_adv_actions();
          adv_actions_response->Reserve(config->adv_actions.size());

          for(const auto& [_, info] : config->adv_actions)
          {
            auto& adv_action_info = *adv_actions_response->Add();

            adv_action_info.set_action_id(info.action_id);
            adv_action_info.set_timestamp(GrpcAlgs::pack_time(info.timestamp));

            auto* ccg_ids_response = adv_action_info.mutable_ccg_ids();
            ccg_ids_response->Add(
              std::begin(info.ccg_ids),
              std::end(info.ccg_ids));
            GrpcAlgs::pack_decimal_into_repeated_field(
              *ccg_ids_response,
              info.cur_value);
          }
        }

        // fill creative options
        {
          auto* creative_options_response =
            campaign_config_response->mutable_creative_options();
          creative_options_response->Reserve(config->creative_options.size());

          for(const auto& [key, info] : config->creative_options)
          {
            auto& co_info_response = *creative_options_response->Add();
            co_info_response.set_option_id(key);
            co_info_response.set_token(info.token);
            co_info_response.set_type(info.type);
            co_info_response.set_timestamp(GrpcAlgs::pack_time(info.timestamp));

            co_info_response.mutable_token_relations()->Add(
              std::begin(info.token_relations),
              std::end(info.token_relations));
          }
        }

        // fill accounts
        {
          auto* accounts_response = campaign_config_response->mutable_accounts();
          accounts_response->Reserve(config->accounts.size());
          for(const auto& [key, info] : config->accounts)
          {
            auto& acc_info_response = *accounts_response->Add();
            acc_info_response.set_account_id(key);
            acc_info_response.set_agency_account_id(info->agency_account ?
              info->agency_account->account_id : 0);
            acc_info_response.set_internal_account_id(info->internal_account_id);
            acc_info_response.set_legal_name(info->legal_name);
            acc_info_response.set_flags(info->flags);
            acc_info_response.set_at_flags(info->at_flags);
            acc_info_response.set_text_adserving(info->text_adserving);
            acc_info_response.set_currency_id(info->currency->currency_id);
            acc_info_response.set_country(info->country);
            acc_info_response.set_commision(GrpcAlgs::pack_decimal(info->commision));
            acc_info_response.set_budget(GrpcAlgs::pack_decimal(info->budget));
            acc_info_response.set_paid_amount(GrpcAlgs::pack_decimal(info->paid_amount));
            acc_info_response.set_status(info->status);
            acc_info_response.set_eval_status(info->eval_status);
            acc_info_response.set_time_offset(GrpcAlgs::pack_time(info->time_offset));
            acc_info_response.mutable_walled_garden_accounts()->Add(
              std::begin(info->walled_garden_accounts),
              std::end(info->walled_garden_accounts));
            acc_info_response.set_auction_rate(static_cast<std::uint32_t>(info->auction_rate));
            acc_info_response.set_use_pub_pixels(info->use_pub_pixels);
            acc_info_response.set_pub_pixel_optin(info->pub_pixel_optin);
            acc_info_response.set_pub_pixel_optout(info->pub_pixel_optout);
            acc_info_response.set_self_service_commission(GrpcAlgs::pack_decimal(
              info->self_service_commission));
            acc_info_response.set_timestamp(GrpcAlgs::pack_time(info->timestamp));
          }
        }

        // fill campaigns
        auto* campaigns_response = campaign_config_response->mutable_campaigns();
        campaigns_response->Reserve(config->campaigns.size());
        for(const auto& [key, campaign]: config->campaigns)
        {
          auto& campaign_response = *campaigns_response->Add();
          campaign_response.set_ecpm(GrpcAlgs::pack_decimal(
            campaign->ecpm_));
          campaign_response.set_ctr(GrpcAlgs::pack_decimal(
            campaign->ctr));

          auto& campaign_info_response(*campaign_response.mutable_info());
          campaign_info_response.set_campaign_id(campaign->campaign_id);
          campaign_info_response.set_timestamp(GrpcAlgs::pack_time(campaign->timestamp));
          campaign_info_response.set_campaign_group_id(campaign->campaign_group_id);
          campaign_info_response.set_flags(campaign->flags);
          campaign_info_response.set_marketplace(campaign->marketplace);
          campaign_info_response.set_status(campaign->status);
          campaign_info_response.set_eval_status(campaign->eval_status);
          campaign_info_response.set_ccg_rate_id(campaign->ccg_rate_id);
          campaign_info_response.set_ccg_rate_type(campaign->ccg_rate_type);
          campaign_info_response.set_account_id(campaign->account->account_id);
          campaign_info_response.set_advertiser_id(campaign->advertiser->account_id);

          campaign_info_response.set_imp_revenue(GrpcAlgs::pack_decimal(
            campaign->imp_revenue));
          campaign_info_response.set_click_revenue(GrpcAlgs::pack_decimal(
            campaign->click_revenue));
          campaign_info_response.set_action_revenue(GrpcAlgs::pack_decimal(
            campaign->action_revenue));
          campaign_info_response.set_commision(GrpcAlgs::pack_decimal(campaign->commision));

          campaign_info_response.set_ccg_type(campaign->ccg_type);
          campaign_info_response.set_target_type(campaign->targeting_type);

          pack_delivery_limits(
            *campaign_info_response.mutable_campaign_delivery_limits(),
            campaign->campaign_delivery_limits);
          pack_delivery_limits(
            *campaign_info_response.mutable_ccg_delivery_limits(),
            campaign->ccg_delivery_limits);

          campaign_info_response.set_max_pub_share(GrpcAlgs::pack_decimal(campaign->max_pub_share));
          campaign_info_response.set_bid_strategy(campaign->bid_strategy);
          campaign_info_response.set_min_ctr_goal(GrpcAlgs::pack_decimal(campaign->min_ctr_goal()));
          campaign_info_response.set_start_user_group_id(campaign->start_user_group_id);
          campaign_info_response.set_end_user_group_id(campaign->end_user_group_id);
          campaign_info_response.set_ctr_reset_id(campaign->ctr_reset_id);
          campaign_info_response.set_mode(campaign->mode);
          campaign_info_response.set_min_uid_age(GrpcAlgs::pack_time(campaign->min_uid_age));
          campaign_info_response.set_seq_set_rotate_imps(campaign->seq_set_rotate_imps);

          campaign_info_response.set_fc_id(campaign->fc_id);
          campaign_info_response.set_group_fc_id(campaign->group_fc_id);

          campaign_info_response.set_flags(campaign->flags);
          campaign_info_response.set_country(campaign->country);
          campaign_info_response.set_delivery_coef(campaign->delivery_coef);

          fill_interval_sequence(
            *campaign_info_response.mutable_weekly_run_intervals(),
            campaign->weekly_run_intervals);

          campaign_info_response.mutable_sites()->Add(
            std::begin(campaign->sites),
            std::end(campaign->sites));

          pack_expression(
            *campaign_info_response.mutable_expression(),
            campaign->channel.in() ? campaign->channel->expression() :
            ExpressionChannel::Expression::EMPTY);

          pack_expression(
            *campaign_info_response.mutable_stat_expression(),
            campaign->stat_channel.in() ? campaign->stat_channel->expression() :
            ExpressionChannel::Expression::EMPTY);

          const CreativeList& cmp_creatives = campaign->get_creatives();

          auto* creatives_response = campaign_info_response.mutable_creatives();
          creatives_response->Reserve(cmp_creatives.size());
          for(const auto& creative : cmp_creatives)
          {
            auto& creative_info = *creatives_response->Add();
            creative_info.set_ccid(creative->ccid);
            creative_info.set_creative_id(creative->creative_id);
            creative_info.set_fc_id(creative->fc_id);
            creative_info.set_weight(creative->weight);
            creative_info.set_creative_format(creative->creative_format);
            creative_info.set_version_id(creative->version_id);
            creative_info.set_order_set_id(creative->order_set_id);

            auto& click_url_response = *creative_info.mutable_click_url();
            click_url_response.set_option_id(creative->click_url.option_id);
            click_url_response.set_value(creative->click_url.value);
            creative_info.set_status(creative->status);

            auto& sizes_response = *creative_info.mutable_sizes();
            sizes_response.Reserve(creative->sizes.size());
            for(const auto& [id, creative_info] : creative->sizes)
            {
              auto& size_info = *sizes_response.Add();
              size_info.set_size_id(id);
              size_info.set_up_expand_space(creative_info.up_expand_space);
              size_info.set_right_expand_space(creative_info.right_expand_space);
              size_info.set_down_expand_space(creative_info.down_expand_space);
              size_info.set_left_expand_space(creative_info.left_expand_space);
              pack_option_token_map(*size_info.mutable_tokens(), creative_info.tokens);
            }

            pack_option_token_map(*creative_info.mutable_tokens(), creative->tokens);

            creative_info.mutable_categories()->Add(
              std::begin(creative->categories),
              std::end(creative->categories));
          }

          campaign_info_response.mutable_colocations()->Add(
            std::begin(campaign->colocations),
            std::end(campaign->colocations));

          campaign_info_response.mutable_exclude_pub_accounts()->Add(
            std::begin(campaign->exclude_pub_accounts),
            std::end(campaign->exclude_pub_accounts));

          auto& exclude_tags_response = *campaign_info_response.mutable_exclude_tags();
          exclude_tags_response.Reserve(campaign->exclude_tags.size());
          for (const auto& [key, info] : campaign->exclude_tags)
          {
            auto& tag_info = *exclude_tags_response.Add();
            tag_info.set_tag_id(key);
            tag_info.set_delivery_value(info);
          }
        }

        // fill expression channels
        auto& expression_channels_response =
          *campaign_config_response->mutable_expression_channels();
        expression_channels_response.Reserve(config->expression_channels.size());
        for(const auto& [key, info] : config->expression_channels)
        {
          if(info->channel.in())
          {
            auto& expression_channel_info = *expression_channels_response.Add();
            pack_channel(expression_channel_info, info->channel);
          }
        }

        // fill sites
        auto& sites_response = *campaign_config_response->mutable_sites();
        sites_response.Reserve(config->sites.size());
        for (const auto& [key, info] : config->sites)
        {
          auto& site_info = *sites_response.Add();
          site_info.set_site_id(key);
          site_info.set_timestamp(GrpcAlgs::pack_time(info->timestamp));
          site_info.set_freq_cap_id(info->freq_cap_id);
          site_info.set_noads_timeout(info->noads_timeout);
          site_info.set_status(info->status);
          site_info.set_flags(info->flags);
          site_info.set_account_id(info->account->account_id);

          // fill creative category accepted & rejected for site
          site_info.mutable_approved_creative_categories()->Add(
            std::begin(info->approved_creative_categories),
            std::end(info->approved_creative_categories));

          site_info.mutable_rejected_creative_categories()->Add(
            std::begin(info->rejected_creative_categories),
            std::end(info->rejected_creative_categories));

          site_info.mutable_approved_creatives()->Add(
            std::begin(info->approved_creatives),
            std::end(info->approved_creatives));

          site_info.mutable_rejected_creatives()->Add(
            std::begin(info->rejected_creatives),
            std::end(info->rejected_creatives));
        }

        {
          auto& tags_response = *campaign_config_response->mutable_tags();
          tags_response.Reserve(config->tags.size());
          for(const auto& [key, tag] : config->tags)
          {
            auto& adapted_tag_info = *tags_response.Add();
            auto& tag_info = *adapted_tag_info.mutable_info();

            tag_info.set_tag_id(key);
            tag_info.set_site_id(tag->site->site_id);

            auto& sizes_response = *tag_info.mutable_sizes();
            sizes_response.Reserve(tag->sizes.size());
            for(const auto& [key_sizes, info_sizes] : tag->sizes)
            {
              auto& size_info = *sizes_response.Add();
              size_info.set_size_id(key_sizes);
              size_info.set_max_text_creatives(info_sizes->max_text_creatives);
              pack_option_token_map(*size_info.mutable_tokens(), info_sizes->tokens);
            }

            tag_info.set_allow_expandable(tag->allow_expandable);
            tag_info.set_imp_track_pixel(tag->imp_track_pixel);
            tag_info.set_passback(tag->passback);
            tag_info.set_passback_type(tag->passback_type);
            tag_info.set_flags(tag->flags);
            tag_info.set_marketplace(tag->marketplace);
            tag_info.set_adjustment(GrpcAlgs::pack_decimal(tag->adjustment));
            tag_info.set_auction_max_ecpm_share(GrpcAlgs::pack_decimal(
              tag->auction_max_ecpm_share));
            tag_info.set_auction_prop_probability_share(GrpcAlgs::pack_decimal(
              tag->auction_prop_probability_share));
            tag_info.set_auction_random_share(GrpcAlgs::pack_decimal(
              tag->auction_random_share));
            tag_info.set_cost_coef(GrpcAlgs::pack_decimal(tag->cost_coef));
            tag_info.set_tag_pricings_timestamp(GrpcAlgs::pack_time(
              tag->tag_pricings_timestamp));
            pack_option_token_map(*tag_info.mutable_tokens(), tag->tokens);
            pack_option_token_map(*tag_info.mutable_hidden_tokens(), tag->hidden_tokens);
            pack_option_token_map(*tag_info.mutable_passback_tokens(), tag->passback_tokens);

            auto& template_tokens = *tag_info.mutable_template_tokens();
            template_tokens.Reserve(tag->template_tokens.size());
            for(const auto& [key_template_token, info_template_token]: tag->template_tokens)
            {
              auto& tamplate_info = *template_tokens.Add();
              tamplate_info.set_template_name(key_template_token);
              pack_option_token_map(*tamplate_info.mutable_tokens(), info_template_token);
            }

            tag_info.set_timestamp(GrpcAlgs::pack_time(tag->timestamp));

            const Tag::TagPricings& tag_pricings_list = tag->tag_pricings;
            auto& tag_pricings_response = *tag_info.mutable_tag_pricings();
            tag_pricings_response.Reserve(tag_pricings_list.size());
            for (const auto& [key_tag_pricing, info_tag_pricing] : tag_pricings_list)
            {
              auto& tag_pricing = *tag_pricings_response.Add();
              tag_pricing.set_country_code(key_tag_pricing.country_code);
              tag_pricing.set_ccg_type(key_tag_pricing.ccg_type);
              tag_pricing.set_ccg_rate_type(key_tag_pricing.ccg_rate_type);
              tag_pricing.set_site_rate_id(info_tag_pricing.site_rate_id);
              tag_pricing.set_imp_revenue(GrpcAlgs::pack_decimal(
                info_tag_pricing.imp_revenue));
              tag_pricing.set_revenue_share(GrpcAlgs::pack_decimal(
                info_tag_pricing.revenue_share));
            }

            tag_info.mutable_accepted_categories()->Add(
              std::begin(tag->accepted_categories),
              std::end(tag->accepted_categories));

            tag_info.mutable_rejected_categories()->Add(
              std::begin(tag->rejected_categories),
              std::end(tag->rejected_categories));

            auto& cmps_response = *adapted_tag_info.mutable_cpms();
            cmps_response.Reserve(tag_pricings_list.size());
            for(const auto& [key_tag_pricings, info_tag_pricings] : tag_pricings_list)
            {
              cmps_response.Add(GrpcAlgs::pack_decimal(info_tag_pricings.cpm));
            }
          }
        }

        auto& frequency_caps_response = *campaign_config_response->mutable_frequency_caps();
        frequency_caps_response.Reserve(config->freq_caps.size());
        for (const auto& [key,freq_cap] : config->freq_caps)
        {
          auto& freq_cap_info = *frequency_caps_response.Add();
          freq_cap_info.set_fc_id(freq_cap.fc_id);
          freq_cap_info.set_timestamp(GrpcAlgs::pack_time(freq_cap.timestamp));
          freq_cap_info.set_lifelimit(freq_cap.lifelimit);
          freq_cap_info.set_period(freq_cap.period.tv_sec);
          freq_cap_info.set_window_limit(freq_cap.window_limit);
          freq_cap_info.set_window_time(freq_cap.window_time.tv_sec);
        }

        campaign_config_response->set_currency_exchange_id(
          config->currency_exchange_id);
        campaign_config_response->set_fraud_user_deactivate_period(
          GrpcAlgs::pack_time(config->fraud_user_deactivate_period));
        campaign_config_response->set_cost_limit(
          GrpcAlgs::pack_decimal(config->cost_limit));
        campaign_config_response->set_google_publisher_account_id(
          config->google_publisher_account_id);

        {
          // fill colocations
          auto& colocations_response = *campaign_config_response->mutable_colocations();
          colocations_response.Reserve(config->colocations.size());
          for (const auto& [key, info] : config->colocations)
          {
            auto& colo = *colocations_response.Add();
            colo.set_colo_id(key);
            colo.set_colo_name(info->colo_name);
            colo.set_colo_rate_id(info->colo_rate_id);
            colo.set_account_id(info->account->account_id);
            colo.set_at_flags(info->at_flags);
            colo.set_revenue_share(GrpcAlgs::pack_decimal(info->revenue_share));
            colo.set_ad_serving(info->ad_serving);
            colo.set_hid_profile(info->hid_profile);
            pack_option_token_map(*colo.mutable_tokens(), info->tokens);
            colo.set_timestamp(GrpcAlgs::pack_time(info->timestamp));
          }
        }

        {
          // fill countries
          auto& countries_response = *campaign_config_response->mutable_countries();
          countries_response.Reserve(config->countries.size());
          for (const auto& [key, info] : config->countries)
          {
            auto& country = *countries_response.Add();
            country.set_country_code(key);
            pack_option_token_map(*country.mutable_tokens(), info->tokens);
            country.set_timestamp(GrpcAlgs::pack_time(info->timestamp));
          }
        }

        auto& creative_template_files_response =
          *campaign_config_response->mutable_creative_template_files();
        creative_template_files_response.Reserve(config->creative_templates.size());
        for(const auto& [key, info] : config->creative_templates)
        {
          auto& ctf_info = *creative_template_files_response.Add();
          ctf_info.set_creative_format(key.creative_format);
          ctf_info.set_creative_size(key.creative_size);
          ctf_info.set_app_format(key.app_format);
          ctf_info.set_mime_format(info.mime_format);
          ctf_info.set_track_impr(info.track_impressions);
          ctf_info.set_template_file(info.file);
          ctf_info.set_type(adopt_template_type_proto(info.type));
          ctf_info.set_status(info.status);
          pack_option_token_map(*ctf_info.mutable_tokens(), *(info.tokens));
          pack_option_token_map(*ctf_info.mutable_hidden_tokens(), *(info.hidden_tokens));
          ctf_info.set_timestamp(GrpcAlgs::pack_time(info.timestamp));
        }

        auto& currencies_response = *campaign_config_response->mutable_currencies();
        currencies_response.Reserve(config->currencies.size());
        for (const auto& [key, info] : config->currencies)
        {
          auto& currency_info = *currencies_response.Add();
          currency_info.set_currency_id(key);
          currency_info.set_currency_exchange_id(info->currency_exchange_id);
          currency_info.set_effective_date(info->effective_date);
          currency_info.set_rate(GrpcAlgs::pack_decimal(info->rate));
          currency_info.set_fraction_digits(info->fraction);
          currency_info.set_currency_code(info->currency_code);
          currency_info.set_timestamp(GrpcAlgs::pack_time(info->timestamp));
        }

        {
          auto& creative_categories_response =
            *campaign_config_response->mutable_creative_categories();
          creative_categories_response.Reserve(
            config->creative_categories.size());
          for(const auto& [key, info] : config->creative_categories)
          {
            auto& ccat_info = *creative_categories_response.Add();
            ccat_info.set_creative_category_id(key);
            ccat_info.set_cct_id(info.cct_id);
            ccat_info.set_name(info.name);

            auto& external_categories_response = *ccat_info.mutable_external_categories();
            external_categories_response.Reserve(info.external_categories.size());
            for(const auto& [key_external_category, value_external_category] : info.external_categories)
            {
              auto& category_info = *external_categories_response.Add();
              category_info.set_ad_request_type(key_external_category);
              category_info.mutable_names()->Add(
                std::begin(value_external_category),
                std::end(value_external_category));
            }

            ccat_info.set_timestamp(GrpcAlgs::pack_time(
              info.timestamp));
          }
        }

        {
          auto& category_channels_response = *campaign_config_response->mutable_category_channels();
          category_channels_response.Reserve(config->category_channels.size());
          for(const auto& [key, info] : config->category_channels)
          {
            auto& category_channel_info = *category_channels_response.Add();
            category_channel_info.set_channel_id(key);
            category_channel_info.set_name(info->name);
            category_channel_info.set_newsgate_name(info->newsgate_name);
            category_channel_info.set_flags(info->flags);
            category_channel_info.set_parent_channel_id(info->parent_channel_id);
            category_channel_info.set_timestamp(GrpcAlgs::pack_time(info->timestamp));

            auto& localizations_response = *category_channel_info.mutable_localizations();
            localizations_response.Reserve(info->localizations.size());
            for(const auto& [key_localization, info_localization] : info->localizations)
            {
              auto& localization_info = *localizations_response.Add();
              localization_info.set_language(key_localization);
              localization_info.set_name(info_localization);
            }
          }
        }

        {
          // fill keywords
          auto& campaign_keywords_response =
            *campaign_config_response->mutable_campaign_keywords();
          campaign_keywords_response.Reserve(config->ccg_keyword_click_info_map.size());
          for(const auto& [key, info] : config->ccg_keyword_click_info_map)
          {
            auto& kw_info = *campaign_keywords_response.Add();
            kw_info.set_ccg_keyword_id(key);
            kw_info.set_original_keyword(info.original_keyword);
            kw_info.set_click_url(info.click_url);
            kw_info.set_timestamp(GrpcAlgs::pack_time(Generics::Time::ZERO));
          }
        }

        if(get_config_props.geo_channels())
        {
          const auto channels_count = config->geo_channels->channels().size();
          auto& geo_channels_response = *campaign_config_response->mutable_geo_channels();
          geo_channels_response.Reserve(channels_count);
          for(const auto& [key, info] : config->geo_channels->channels())
          {
            auto& geo_channel_info = *geo_channels_response.Add();
            geo_channel_info.set_channel_id(info);
            geo_channel_info.set_country(key.country());

            auto& geoip_targets_response = *geo_channel_info.mutable_geoip_targets();
            geoip_targets_response.Reserve(1);
            auto& geoip_target_info = *geoip_targets_response.Add();
            geoip_target_info.set_region(key.region());
            geoip_target_info.set_city(key.city());

            geo_channel_info.set_timestamp(GrpcAlgs::pack_time(Generics::Time::ZERO));
          }

          int geo_coord_channels_count = 0;
          for(const auto& [_, info] : config->geo_coord_channels->channels())
          {
            geo_coord_channels_count += info->channels.size();
          }

          auto& geo_coord_channels = *campaign_config_response->mutable_geo_coord_channels();
          geo_coord_channels.Reserve(geo_coord_channels_count);
          for(const auto& [key, info]: config->geo_coord_channels->channels())
          {
            auto& geo_coord_channel_info = *geo_coord_channels.Add();
            for(const auto& channel : info->channels)
            {
              geo_coord_channel_info.set_channel_id(channel);
              geo_coord_channel_info.set_longitude(GrpcAlgs::pack_decimal(
                key.longitude));
              geo_coord_channel_info.set_latitude(GrpcAlgs::pack_decimal(
                key.latitude));
              geo_coord_channel_info.set_radius(GrpcAlgs::pack_decimal(
                key.accuracy));
              geo_coord_channel_info.set_timestamp(GrpcAlgs::pack_time(
                Generics::Time::ZERO));
            }
          }
        }

        auto& web_operations_response = *campaign_config_response->mutable_web_operations();
        web_operations_response.Reserve(config->web_operations.size());
        for(const auto& [_, info] : config->web_operations)
        {
          auto& operation_info = *web_operations_response.Add();
          operation_info.set_id(info->id);
          operation_info.set_app(info->app);
          operation_info.set_source(info->source);
          operation_info.set_operation(info->operation);
          operation_info.set_flags(info->flags);
          operation_info.set_timestamp(
            GrpcAlgs::pack_time(Generics::Time::ZERO));
        }

        // fill contracts
        {
          auto& contracts = *campaign_config_response->mutable_contracts();
          contracts.Reserve(config->contracts.size());

          for(const auto& [_, contract] : config->contracts)
          {
            fill_contract_(*contracts.Add(), *contract);
          }
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetConfigResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetConfigResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetConfigResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
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
    CampaignManagerImpl::fill_contract_(
      ContractInfo& contract_info,
      const Contract& contract)
      noexcept
    {
      contract_info.contract_id = contract.contract_id;

      contract_info.number << contract.number;
      contract_info.date << contract.date;
      contract_info.type << contract.type;
      contract_info.vat_included = contract.vat_included;
      contract_info.parent_contract_id = contract.parent_contract ?
        contract.parent_contract->contract_id : 0;

      contract_info.ord_contract_id << contract.ord_contract_id;
      contract_info.ord_ado_id << contract.ord_ado_id;
      contract_info.subject_type << contract.subject_type;
      contract_info.action_type << contract.action_type;
      contract_info.agent_acting_for_publisher = contract.agent_acting_for_publisher;

      contract_info.client_id << contract.client_id;
      contract_info.client_name << contract.client_name;
      contract_info.client_legal_form << contract.client_legal_form;

      contract_info.contractor_id << contract.contractor_id;
      contract_info.contractor_name << contract.contractor_name;
      contract_info.contractor_legal_form << contract.contractor_legal_form;
      contract_info.timestamp = CorbaAlgs::pack_time(contract.timestamp);
    }

    void CampaignManagerImpl::fill_contract_(
      Proto::ContractInfo& contract_info,
      const Contract& contract)
    {
      contract_info.set_contract_id(contract.contract_id);
      contract_info.set_number(contract.number);
      contract_info.set_date(contract.date);
      contract_info.set_type(contract.type);
      contract_info.set_vat_included(contract.vat_included);
      contract_info.set_parent_contract_id(
        contract.parent_contract ? contract.parent_contract->contract_id : 0);
      contract_info.set_ord_contract_id(contract.ord_contract_id);
      contract_info.set_ord_ado_id(contract.ord_ado_id);
      contract_info.set_subject_type(contract.subject_type);
      contract_info.set_action_type(contract.action_type);
      contract_info.set_agent_acting_for_publisher(contract.agent_acting_for_publisher);
      contract_info.set_client_id(contract.client_id);
      contract_info.set_client_name(contract.client_name);
      contract_info.set_client_legal_form(contract.client_legal_form);
      contract_info.set_contractor_id(contract.contractor_id);
      contract_info.set_contractor_name(contract.contractor_name);
      contract_info.set_contractor_legal_form(contract.contractor_legal_form);
      contract_info.set_timestamp(GrpcAlgs::pack_time(contract.timestamp));
    }

    void
    CampaignManagerImpl::fill_campaign_contracts_(
      AdServer::CampaignSvcs::CampaignManager::ExtContractInfoSeq& contract_seq,
      const Contract* contract)
      noexcept
    {
      unsigned long MAX_CONTRACTS = 50;
      const Contract* cur_contract = contract;
      while(cur_contract && contract_seq.length() < MAX_CONTRACTS)
      {
        contract_seq.length(contract_seq.length() + 1);
        AdServer::CampaignSvcs::CampaignManager::ExtContractInfo& res_contract =
          contract_seq[contract_seq.length() - 1];
        fill_contract_(res_contract.contract_info, *cur_contract);
        if(cur_contract->parent_contract)
        {
          res_contract.parent_contract_id << cur_contract->parent_contract->ord_contract_id;
        }

        cur_contract = cur_contract->parent_contract;
      }
    }

    void CampaignManagerImpl::fill_campaign_contracts_(
      google::protobuf::RepeatedPtrField<Proto::ExtContractInfo>& contract_seq,
      const Contract* contract)
    {
      const int MAX_CONTRACTS = 50;
      contract_seq.Reserve(MAX_CONTRACTS);
      const Contract* cur_contract = contract;
      while(cur_contract && contract_seq.size() < MAX_CONTRACTS)
      {
        auto* ext_contract_info = contract_seq.Add();
        fill_contract_(*ext_contract_info->mutable_contract_info(), *cur_contract);
        if(cur_contract->parent_contract)
        {
          ext_contract_info->set_parent_contract_id(cur_contract->parent_contract->ord_contract_id);
        }

        cur_contract = cur_contract->parent_contract;
      }
    }
  }
}
