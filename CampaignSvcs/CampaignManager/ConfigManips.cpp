
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

#include "CampaignManagerCore.hpp"
#include "CampaignManagerLogger.hpp"

namespace AdServer::CampaignSvcs
{
  namespace
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  }

  void
  fill_contract_info(
    ::AdServer::CampaignSvcs::ContractInfo& contract_info,
    const Contract& contract)
    noexcept;

  void
  CampaignManagerCore::precalculate_pub_pixel_accounts_(
    CampaignConfig* campaign_config)
    /*throw(eh::Exception)*/
  {
    if (country_whitelist_.empty())
    {
      return;
    }

    for (const UserStatus user_status : {US_OPTIN, US_OPTOUT})
    {
      AccountSet accounts;

      for (const auto& country : country_whitelist_)
      {
        const auto country_it = campaign_config->pub_pixel_accounts.find(
          PubPixelAccountKey(country.c_str(), user_status));
        if (country_it != campaign_config->pub_pixel_accounts.end())
        {
          accounts.insert(country_it->second.begin(), country_it->second.end());
        }
      }

      if (!accounts.empty())
      {
        campaign_config->pub_pixel_accounts[
          PubPixelAccountKey("", user_status)].swap(accounts);
      }
    }
  }

  Generics::Time
  CampaignManagerCore::check_config() noexcept
  {
    static const char* FUN = "CampaignManagerCore::check_config()";

    CampaignIndexPtr configuration_index;
    CampaignConfigPtr new_config;

    try
    {
      ConstCampaignConfigPtr old_config = get_campaign_config();
      new_config = campaign_config_source_->update(old_config.get());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      callback_->critical(ostr.str(), "ADS-ICON-5000");
    }

    Generics::Time master_stamp;

    if (new_config)
    {
      master_stamp = new_config->master_stamp;
    }
    else
    {
      ConstCampaignConfigPtr old_config = get_campaign_config();
      if (old_config)
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

      if (config_expired)
      {
        logger_->stream(Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-5093") << FUN <<
          ": Config expired - disable ad showing: config timestamp = " <<
          master_stamp.get_gm_time();

        ConstCampaignIndexPtr old_configuration_index;

        {
          std::lock_guard<std::mutex> guard(lock_);
          configuration_index_.swap(old_configuration_index);
        }
      }
      else if (new_config)
      {
        if (logger_->log_level() >= Logging::Logger::TRACE + 1)
        {
          logger_->stream(Logging::Logger::TRACE + 1,
            Aspect::CAMPAIGN_MANAGER) << FUN <<
            ": To construct campaign index for " <<
            new_config->campaigns.size() << " campaigns, " <<
            new_config->tags.size() << " tags.";
        }

        configuration_index =
          std::make_shared<CampaignIndex>(new_config, logger_);

        if (configuration_index->index_campaigns(
             &indexing_progress_,
             this))
        {
          if (logger_->log_level() >= Logging::Logger::TRACE + 1)
          {
            logger_->stream(Logging::Logger::TRACE + 1,
              Aspect::CAMPAIGN_MANAGER) << FUN <<
              ": Campaign index constructed.";
          }

          precalculate_pub_pixel_accounts_(new_config.get());

          ConstCampaignIndexPtr new_configuration_index = configuration_index;
          ConstCampaignConfigPtr new_configuration = new_config;

          {
            std::lock_guard<std::mutex> guard(lock_);
            configuration_index_.swap(new_configuration_index);
            configuration_.swap(new_configuration);
          }

          campaign_manager_logger_->set_campaign_config(new_config);
        }
        else if (logger_->log_level() >= Logging::Logger::TRACE + 1)
        {
          logger_->stream(Logging::Logger::TRACE + 1,
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

    return Generics::Time::get_time_of_day() +
      campaign_manager_config_.config_update_period();
  }

  template<typename PredProviderType>
  ReferenceCounting::SmartPtr<const PredProviderType>
  CampaignManagerCore::update_rate_provider_(
    const PredProviderType* old_ctr_provider,
    const String::SubString& capture_root,
    const String::SubString& res_root,
    const Generics::Time& expire_timeout)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignManagerCore::update_rate_provider_()";

    const Generics::Time now = Generics::Time::get_time_of_day();

    // check that appeared new dir
    try
    {
      std::string new_ctr_config_root;
      std::string new_ctr_config_root_error;

      bool config_captured = true;
      Generics::Time new_config_timestamp;

      ReferenceCounting::SmartPtr<const PredProviderType> new_ctr_provider;

      if (!old_ctr_provider)
      {
        new_config_timestamp = PredProviderType::check_config_appearance(
          new_ctr_config_root,
          capture_root);
      }

      if (new_ctr_config_root.empty())
      {
        new_config_timestamp = PredProviderType::check_config_appearance(
          new_ctr_config_root,
          res_root);
        config_captured = false;
      }

      if (!new_ctr_config_root.empty())
      {
        bool config_not_expired =
          expire_timeout == Generics::Time::ZERO ||
          now <= new_config_timestamp + expire_timeout;

        if (config_captured || (
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
            if (::access(new_ctr_config_root_error.c_str(), F_OK) != 0)
            {
              if (!config_captured)
              {
                int rename_res = ::rename(new_ctr_config_root.c_str(),
                  captured_ctr_config_root.c_str());

                if (rename_res && errno != EEXIST)
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

          if (!new_ctr_config_root.empty())
          {
            try
            {
              new_ctr_provider = new PredProviderType(
                new_ctr_config_root,
                new_config_timestamp,
                task_runner_);

              if (old_ctr_provider)
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
    if (expire_timeout != Generics::Time::ZERO && old_ctr_provider)
    {
      Generics::Time ctr_config_timestamp = old_ctr_provider->config_timestamp();

      // check that ctr isn't expired
      if (now > ctr_config_timestamp + expire_timeout)
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

  CTR::ConstCTRProviderImpl_var
  CampaignManagerCore::update_ctr_rate_provider_(
    const CTR::CTRProviderImpl* old_ctr_provider,
    const String::SubString& capture_root,
    const String::SubString& res_root,
    const Generics::Time& expire_timeout)
  {
    static const char* FUN = "CampaignManagerCore::update_ctr_rate_provider_()";

    const Generics::Time now = Generics::Time::get_time_of_day();

    try
    {
      std::string new_ctr_config_root;
      std::string new_ctr_config_root_error;

      bool config_captured = true;
      Generics::Time new_config_timestamp;

      CTR::ConstCTRProviderImpl_var new_ctr_provider;

      if (!old_ctr_provider)
      {
        new_config_timestamp = CTR::CTRProviderImpl::check_config_appearance(
          new_ctr_config_root,
          capture_root);
      }

      if (new_ctr_config_root.empty())
      {
        new_config_timestamp = CTR::CTRProviderImpl::check_config_appearance(
          new_ctr_config_root,
          res_root);
        config_captured = false;
      }

      if (!new_ctr_config_root.empty())
      {
        const bool config_not_expired =
          expire_timeout == Generics::Time::ZERO ||
          now <= new_config_timestamp + expire_timeout;

        if (config_captured || (
             (!old_ctr_provider ||
              new_config_timestamp > old_ctr_provider->config_timestamp()) &&
             config_not_expired))
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

          if (::access(new_ctr_config_root_error.c_str(), F_OK) != 0)
          {
            if (!config_captured)
            {
              const int rename_res = ::rename(
                new_ctr_config_root.c_str(),
                captured_ctr_config_root.c_str());

              if (rename_res && errno != EEXIST)
              {
                eh::throw_errno_exception<CTR::CTRProviderImpl::InvalidConfig>(
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

          if (!new_ctr_config_root.empty())
          {
            try
            {
              new_ctr_provider = new CTR::CTRProviderImpl(
                new_ctr_config_root,
                new_config_timestamp,
                task_runner_);

              if (old_ctr_provider)
              {
                old_ctr_provider->remove_config_files_at_destroy(true);
              }

              return new_ctr_provider;
            }
            catch(const eh::Exception& ex)
            {
              ::rename(
                new_ctr_config_root.c_str(),
                new_ctr_config_root_error.c_str());

              Stream::Error ostr;
              ostr << FUN << ": can't load CTR config '" <<
                new_ctr_config_root << "': " << ex.what();
              throw Exception(ostr);
            }
          }
        }
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      callback_->critical(ostr.str(), "ADS-IMPL-5091");
    }

    if (expire_timeout != Generics::Time::ZERO && old_ctr_provider)
    {
      const Generics::Time ctr_config_timestamp =
        old_ctr_provider->config_timestamp();

      if (now > ctr_config_timestamp + expire_timeout)
      {
        old_ctr_provider->remove_config_files_at_destroy(true);

        Stream::Error ostr;
        ostr << FUN << ": CTR config expired and will be disabled, "
          "config timestamp = " << ctr_config_timestamp.gm_ft();
        callback_->warning(ostr.str(), "ADS-IMPL-5091");

        return CTR::ConstCTRProviderImpl_var();
      }
    }

    return ReferenceCounting::add_ref(old_ctr_provider);
  }

  Generics::Time
  CampaignManagerCore::update_ctr_provider() noexcept
  {
    static const char* FUN = "CampaignManagerCore::update_ctr_provider()";

    if (campaign_manager_config_.CTRConfig().present())
    {
      try
      {
        ctr_provider_ = update_ctr_rate_provider_(
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

      return Generics::Time::get_time_of_day() +
        campaign_manager_config_.CTRConfig()->check_period();
    }

    return Generics::Time::ZERO;
  }

  Generics::Time
  CampaignManagerCore::update_conv_rate_provider() noexcept
  {
    static const char* FUN = "CampaignManagerCore::update_conv_rate_provider()";

    if (campaign_manager_config_.ConvRateConfig().present())
    {
      try
      {
        conv_rate_provider_ = update_ctr_rate_provider_(
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

      return Generics::Time::get_time_of_day() +
        campaign_manager_config_.ConvRateConfig()->check_period();
    }

    return Generics::Time::ZERO;
  }

  Generics::Time
  CampaignManagerCore::update_bid_cost_provider() noexcept
  {
    static const char* FUN = "CampaignManagerCore::update_bid_cost_provider()";

    if (campaign_manager_config_.BidCostConfig().present())
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

      return Generics::Time::get_time_of_day() +
        campaign_manager_config_.BidCostConfig()->check_period();
    }

    return Generics::Time::ZERO;
  }

  ConstCampaignConfigPtr
  CampaignManagerCore::get_config(const ConfigRequestInfo& get_config_props)
    /*throw(Exception)*/
  {
    static_cast<void>(get_config_props);

    ConstCampaignConfigPtr config = get_campaign_config();

    if (!config)
    {
      CampaignConfigPtr new_config = std::make_shared<CampaignConfig>();
      new_config->cost_limit = RevenueDecimal::ZERO;
      config = new_config;
    }

    return config;
  }

  void
  fill_contract_info(
    ::AdServer::CampaignSvcs::ContractInfo& contract_info,
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

  void
  CampaignManagerCore::fill_contract_(
    ::AdServer::CampaignSvcs::ContractInfo& contract_info,
    const Contract& contract)
    noexcept
  {
    fill_contract_info(contract_info, contract);
  }
}
