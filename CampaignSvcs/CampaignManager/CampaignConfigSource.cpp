#include <Generics/Time.hpp>
#include <Generics/Proc.hpp>
#include <String/StringManip.hpp>
#include <String/UTF8Case.hpp>
#include <Stream/MemoryStream.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>

#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>

#include "CampaignManagerDeclarations.hpp"
#include "CreativeTextGenerator.hpp"
#include "CampaignConfigSource.hpp"

namespace Aspect
{
  const char CAMPAIGN_CONFIG_SOURCE[] = "CampaignConfigSource";
}

namespace
{
  const char* CHECK_CREATIVE_TOKENS[] =
  {
    "HEADLINE",
    "DISPLAY_URL",
    "DESCRIPTION1",
    "DESCRIPTION2"
  };

  const char* CHECK_CREATIVE_URL_TOKENS[] =
  {
    "DISPLAY_URL"
  };
}

namespace
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  void
  normalize(const String::SubString& src, std::string& res)
    /*throw(eh::Exception)*/
  {
    std::string t;
    String::case_change<String::Uniform>(src, t);
    String::StringManip::flatten(res, String::StringManip::trim_ret(t));
  }

  bool
  plain_match(const String::SubString& pattern,
    const String::SubString& phrase) /*throw(eh::Exception)*/
  {
    std::string pattern_s, phrase_s;
    normalize(pattern, pattern_s);
    normalize(phrase, phrase_s);

    std::string::size_type pos = -1;

    while ((pos = phrase_s.find(pattern_s, pos + 1)) != std::string::npos)
    {
      if ((!pos || phrase_s[pos - 1] == ' ') &&
        (pos + pattern_s.size() == phrase_s.size() ||
          phrase_s[pos + pattern_s.size()] == ' '))
      {
        return true;
      }
    }

    return false;
  }
}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const ExtRevenueDecimal EXT_CPM_FACTOR = ExtRevenueDecimal::mul(
      EXT_REVENUE_DECIMAL_THS,
      ExtRevenueDecimal(false, 100, 0),
      Generics::DMR_FLOOR);

    CreativeTemplateFactory::Handler::Type
    adopt_template_type(AdServer::CampaignSvcs::CreativeTemplateType type_val)
    {
      if(type_val == AdServer::CampaignSvcs::CTT_TEXT)
      {
        return CreativeTemplateFactory::Handler::CTT_TEXT;
      }
      else if(type_val == AdServer::CampaignSvcs::CTT_XSLT)
      {
        return CreativeTemplateFactory::Handler::CTT_XSLT;
      }

      throw Exception("Unknown template type");
    }
  }

  inline unsigned
  hash_calc(const std::string& s) noexcept
  {
    std::size_t value = 0;
    {
      Generics::Murmur64Hash hasher(value);
      hasher.add(s.data(), s.size());
    }
    return value;
  }

  CampaignConfigSource::CampaignConfigSource(
    Logging::Logger* logger,
    DomainParser* domain_parser,
    const CORBACommons::CorbaObjectRefList& campaign_server_refs,
    const char* campaigns_types,
    const char* creative_file_dir,
    const char* template_file_dir,
    const std::string& service_index,
    const CreativeInstantiateRuleMap& creative_rules,
    bool drop_https_safe)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      domain_parser_(ReferenceCounting::add_ref(domain_parser)),
      campaigns_types_(campaigns_types),
      creative_file_dir_(creative_file_dir),
      template_file_dir_(template_file_dir),
      SERVICE_INDEX_(hash_calc(service_index)),
      creative_rules_(creative_rules),
      drop_https_safe_(drop_https_safe),
      file_access_manager_(Generics::Time(120))
  {
    static const char* FUN = "CampaignConfigSource::CampaignConfigSource()";

    try
    {
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

      campaign_servers_.reset(new CampaignServerPool(
        campaign_server_refs,
        corba_client_adapter_,
        CORBACommons::ChoosePolicyType::PT_PERSISTENT,
        Generics::Time(10) // timeout
        ));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Caught eh::Exception on campaign server pool initialization: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  const char*
  CampaignConfigSource::adapt_creative_file_path(
    const char* file_path,
    std::string& result_file_path)
    noexcept
  {
    result_file_path = creative_file_dir_;
    result_file_path += "/";
    result_file_path += file_path;
    return result_file_path.c_str();
  }

  const char*
  CampaignConfigSource::adopt_template_path(
    const char* path,
    std::string& result_template_path)
    noexcept
  {
    result_template_path = template_file_dir_;
    result_template_path += "/";
    result_template_path += path;
    return result_template_path.c_str();
  }

  Creative*
  CampaignConfigSource::adapt_creative_info_(
    const Campaign* campaign,
    const AdServer::CampaignSvcs::CreativeInfo& creative_info)
    /*throw(Exception, eh::Exception)*/
  {
    Creative::CategorySet categories;

    CorbaAlgs::convert_sequence(creative_info.categories, categories);

    std::string short_click_url;
    std::string click_url_domain;
    if(creative_info.click_url.value[0])
    {
      try
      {
        HTTP::BrowserAddress http_url(
          String::SubString(creative_info.click_url.value));
        http_url.get_view(
          HTTP::HTTPAddress::VW_PROTOCOL |
          HTTP::HTTPAddress::VW_HOSTNAME |
          HTTP::HTTPAddress::VW_NDEF_PORT,
          short_click_url);
        short_click_url += "/";
        domain_parser_->specific_domain(http_url.host(), click_url_domain);
      }
      catch (const HTTP::URLAddress::InvalidURL& e)
      {}
    }

    Creative* result = new Creative(
      campaign,
      creative_info.ccid,
      creative_info.creative_id,
      creative_info.fc_id,
      creative_info.weight,
      creative_info.creative_format.in(),
      creative_info.version_id.in(),
      OptionValue(creative_info.click_url.option_id, creative_info.click_url.value),
      click_url_domain.c_str(),
      short_click_url.c_str(),
      categories);

    result->status = creative_info.status;

    for(CORBA::ULong size_i = 0; size_i < creative_info.sizes.length(); ++size_i)
    {
      Creative::Size& res_size = result->sizes[creative_info.sizes[size_i].size_id];
      res_size.up_expand_space = creative_info.sizes[size_i].up_expand_space;
      res_size.right_expand_space = creative_info.sizes[size_i].right_expand_space;
      res_size.down_expand_space = creative_info.sizes[size_i].down_expand_space;
      res_size.left_expand_space = creative_info.sizes[size_i].left_expand_space;
      res_size.expandable = (
        res_size.up_expand_space > 0 ||
        res_size.right_expand_space > 0 ||
        res_size.down_expand_space > 0 ||
        res_size.left_expand_space > 0);
    }
    result->order_set_id = creative_info.order_set_id;

    return result;
  }

  CampaignConfig_var
  CampaignConfigSource::update(const CampaignConfig* old_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigSource::update()";
    const unsigned long PORTIONS_NUMBER = 20;

    try
    {
      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<Exception>(
            logger_,
            Logging::Logger::CRITICAL,
            Aspect::CAMPAIGN_CONFIG_SOURCE,
            "ADS_ICON-5000",
            SERVICE_INDEX_,
            SERVICE_INDEX_);

        try
        {
          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": Reloading config ...";
          }

          Generics::Time now = Generics::Time::get_time_of_day();
          CampaignConfig_var new_config = new CampaignConfig();
          ConfigUpdateLinks config_update_links;

          for(unsigned long portion = 0; portion < PORTIONS_NUMBER; ++portion)
          {
            if(!active())
            {
              throw Interrupted("");
            }

            CampaignGetConfigSettings get_config_settings;
            get_config_settings.timestamp =
              CorbaAlgs::pack_time(Generics::Time::ZERO);
            get_config_settings.server_id = 0;
            get_config_settings.portion = portion;
            get_config_settings.portions_number = PORTIONS_NUMBER;

            get_config_settings.colo_id = 0;

            get_config_settings.campaign_statuses << campaigns_types_;
            get_config_settings.channel_statuses = "A";

            get_config_settings.provide_only_tags = false;
            get_config_settings.no_deleted = true;
            get_config_settings.provide_channel_triggers = false;

            get_config_settings.geo_channels_timestamp = CorbaAlgs::pack_time(
              old_config ?
              old_config->geo_channels_timestamp : Generics::Time::ZERO);

            CampaignConfigUpdateInfo_var update_info;

            update_info = campaign_server->get_config(get_config_settings);

            if (logger_->log_level() >= Logging::Logger::TRACE)
            {
              logger_->stream(Logging::Logger::TRACE,
                Aspect::CAMPAIGN_CONFIG_SOURCE) << FUN <<
                ": config update contains (portion = " << portion << "): " <<
                update_info->countries.length() << " updated countries," <<
                update_info->colocations.length() << " updated colocations," <<
                update_info->deleted_colocations.length() << " deleted colocations," <<
                update_info->campaigns.length() << " updated campaigns, " <<
                update_info->deleted_campaigns.length() << " deleted campaigns; " <<
                update_info->campaign_keywords.length() << " updated keywords, " <<
                update_info->deleted_keywords.length() << " deleted keywords; " <<
                update_info->creative_categories.length() << " updated creative categories, " <<
                update_info->tags.length() << " updated tags, " <<
                update_info->deleted_tags.length() << " deleted tags; " <<
                update_info->expression_channels.length() << " updated expression channels, " <<
                update_info->deleted_expression_channels.length() << " deleted expression channels, " <<
                update_info->category_channels.length() << " updated category channels, " <<
                update_info->deleted_category_channels.length() << " deleted category channels, " <<
                update_info->activate_geo_channels.length() << " updated geo channels(" <<
                  CorbaAlgs::unpack_time(update_info->geo_channels_timestamp).get_gm_time() << ")";
            }

            apply_config_update_(
              *new_config,
              config_update_links,
              *update_info,
              old_config);
          }

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": link config entities.";
          }

          link_config_update_(config_update_links, *new_config);

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": apply campaign limitations.";
          }

          apply_campaign_limitations_(*new_config, now);

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": check creative file references.";
          }

          check_creative_files_option_(*new_config);

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": check creative template files.";
          }

          check_creative_template_files_(*new_config);

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": preinstantiate creative tokens.";
          }

          preinstantiate_creative_tokens_(*new_config);

          if(!old_config ||
             old_config->geo_channels.in() != new_config->geo_channels.in())
          {
            new_config->geo_channels->close();
          }

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::CAMPAIGN_CONFIG_SOURCE) <<
              FUN << ": To construct campaign index for " <<
              new_config->campaigns.size() << " campaigns, " <<
              new_config->tags.size() << " tags.";
          }

          return new_config;
        }
        catch(const CampaignServer::NotReady& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": CampaignServer::NotReady caught: " << ex.description;
          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(), Logging::Logger::NOTICE,
            Aspect::CAMPAIGN_CONFIG_SOURCE, "ADS-ICON-5000");
        }
        catch(const CampaignServer::ImplementationException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": CampaignServer::ImplementationException caught: "
            << e.description;
          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(), Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_CONFIG_SOURCE, "ADS-IMPL-5091");
        }
        catch(const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": CORBA::SystemException caught: " << e;
          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(), Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_CONFIG_SOURCE, "ADS-ICON-5000");
        }
      } // for (;;)
    }
    catch (const Interrupted&)
    {}
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      throw Exception(ostr, "ADS-ICON-5000");
    }

    return CampaignConfig_var();
  }

  unsigned long
  CampaignConfigSource::filter_not_exist_fc_(
    unsigned long fc_id, const FreqCapMap& freq_caps_map)
    noexcept
  {
    return freq_caps_map.find(fc_id) == freq_caps_map.end() ? 0 : fc_id;
  }

  void
  CampaignConfigSource::apply_config_update_(
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links,
    const CampaignConfigUpdateInfo& update_info,
    const CampaignConfig* old_config)
    /*throw(Exception)*/
  {
    Generics::Time now(Generics::Time::get_time_of_day());

    if(new_config.master_stamp == Generics::Time::ZERO)
    {
      new_config.master_stamp = CorbaAlgs::unpack_time(update_info.master_stamp);
    }

    new_config.first_load_stamp = CorbaAlgs::unpack_time(
      update_info.first_load_stamp);
    new_config.finish_load_stamp = CorbaAlgs::unpack_time(
      update_info.finish_load_stamp);
    new_config.global_params_timestamp =
      CorbaAlgs::unpack_time(update_info.global_params_timestamp);

    new_config.currency_exchange_id = update_info.currency_exchange_id;
    new_config.fraud_user_deactivate_period = CorbaAlgs::unpack_time(
      update_info.fraud_user_deactivate_period);
    new_config.cost_limit = CorbaAlgs::unpack_decimal<RevenueDecimal>(
      update_info.cost_limit);

    apply_sizes_update_(update_info, new_config);
    apply_app_formats_update_(update_info, new_config);
    apply_creative_options_update_(update_info, new_config);
    apply_account_update_(update_info, new_config, config_update_links);

    for(CORBA::ULong i = 0; i < update_info.ecpms.length(); ++i)
    {
      ConfigUpdateLinks::EcpmHolder ecpm;
      ecpm.ecpm = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        update_info.ecpms[i].ecpm);
      ecpm.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        update_info.ecpms[i].ctr);

      config_update_links.campaign_ecpms.insert(
        std::make_pair(
          update_info.ecpms[i].ccg_id,
          ecpm));
    }

    for (CORBA::ULong i = 0; i < update_info.adv_actions.length(); i++)
    {
      const AdvActionInfo& ai_info = update_info.adv_actions[i];
      AdvActionDef& ai = new_config.adv_actions[ai_info.action_id];
      ai.action_id = ai_info.action_id;
      ai.timestamp = CorbaAlgs::unpack_time(ai_info.timestamp);

      CORBA::ULong j = 0;
      for(; j < ai_info.ccg_ids.length() && ai_info.ccg_ids[j] != 0; ++j)
      {
        ai.ccg_ids.push_back(ai_info.ccg_ids[j]);
      }

      if(j < ai_info.ccg_ids.length())
      {
        CorbaAlgs::unpack_decimal_from_seq(
          ai.cur_value, ai_info.ccg_ids, j + 1);
      }
    }

    apply_creative_categories_update_(update_info, new_config, config_update_links);
    apply_currency_update_(update_info, new_config);
    apply_frequency_caps_update_(update_info, new_config);
    apply_creative_templates_update_(update_info, new_config, config_update_links);
    apply_campaigns_update_(update_info, new_config, config_update_links);
    apply_expression_channels_update_(update_info, new_config, config_update_links);
    apply_sites_update_(update_info, new_config, config_update_links);
    apply_tags_update_(update_info, new_config, config_update_links);
    apply_colocations_update_(update_info, new_config, config_update_links);
    apply_countries_update_(update_info, new_config, config_update_links);
    apply_category_channels_update_(update_info, new_config);
    apply_simple_channels_update_(update_info, new_config);
    apply_ccg_keyword_update_(update_info, new_config, config_update_links);
    apply_geo_channel_update_(new_config, update_info, old_config);
    apply_platform_update_(new_config, update_info);
    apply_web_app_update_(new_config, update_info);
    apply_block_channel_update_(update_info, config_update_links);
    apply_contract_update_(new_config, update_info, config_update_links);
  }

  void CampaignConfigSource::link_account_update_(
    CampaignConfig& new_config,
    const ConfigUpdateLinks& config_update_links)
    noexcept
  {
    for(AccountMap::iterator account_it = new_config.accounts.begin();
        account_it != new_config.accounts.end(); )
    {
      ConfigUpdateLinks::IdMap::const_iterator currency_link_it =
        config_update_links.account_currencies.find(account_it->first);

      assert(currency_link_it != config_update_links.account_currencies.end());

      CurrencyMap::const_iterator currency_it = new_config.currencies.find(
        currency_link_it->second);

      if(currency_it != new_config.currencies.end())
      {
        account_it->second->currency = currency_it->second;

        ConfigUpdateLinks::IdMap::const_iterator account_agency_it =
          config_update_links.account_agencies.find(account_it->first);

        if(account_agency_it != config_update_links.account_agencies.end())
        {
          AccountMap::const_iterator agency_it = new_config.accounts.find(
            account_agency_it->second);

          if(agency_it != new_config.accounts.end())
          {
            account_it->second->agency_account = agency_it->second;
            ++account_it;
          }
          else
          {
            new_config.accounts.erase(account_it++);
          }
        }
        else
        {
          ++account_it;
        }
      }
      else
      {
        new_config.accounts.erase(account_it++);
      }
    }
  }

  void CampaignConfigSource::link_url_categories_(
    Creative::CategorySet& target_creative_categories,
    const String::SubString& url_val,
    const ConfigUpdateLinks::DomainExcludeCategoryMap&
      domain_category_exclusions)
    noexcept
  {
    try
    {
      HTTP::BrowserAddress url(url_val);
      std::string check_domain = url.host().substr(
        url.host().compare(0, 4, "www.") == 0 ? 4 : 0).str();
      String::AsciiStringManip::to_lower(check_domain);

      ConfigUpdateLinks::DomainExcludeCategoryMap::const_iterator
        domain_categories_it = domain_category_exclusions.find(check_domain);

      if(domain_categories_it != domain_category_exclusions.end())
      {
        std::copy(domain_categories_it->second.begin(),
         domain_categories_it->second.end(),
         std::inserter(target_creative_categories,
           target_creative_categories.begin()));
      }
    }
    catch(...)
    {}
  }

  void
  CampaignConfigSource::link_template_update_(
    CampaignConfig& new_config,
    const ConfigUpdateLinks& config_update_links)
    noexcept
  {
    for(ConfigUpdateLinks::TemplateOptionsLinkMap::const_iterator it =
          config_update_links.template_option_values.begin();
        it != config_update_links.template_option_values.end(); ++it)
    {
      link_option_values_(
        *(it->second.tokens_ref),
        new_config,
        it->second.unlinked_tokens);

      link_option_values_(
        *(it->second.hidden_tokens_ref),
        new_config,
        it->second.unlinked_hidden_tokens);
    }
  }

  void CampaignConfigSource::link_contract_update_(
    CampaignConfig& new_config,
    const ConfigUpdateLinks& config_update_links)
    noexcept
  {
    //static const char* FUN = "CampaignConfigSource::link_contract_update_()";

    for (CampaignConfig::ContractMap::iterator contract_it = new_config.contracts.begin();
       contract_it != new_config.contracts.end(); ++contract_it)
    {
      // link parent contract to contract
      auto parent_contract_link_it = config_update_links.contract_parent_contracts.find(contract_it->first);
      if(parent_contract_link_it != config_update_links.contract_parent_contracts.end())
      {
        auto parent_contract_it = new_config.contracts.find(parent_contract_link_it->second);
        if(parent_contract_it != new_config.contracts.end())
        {
          contract_it->second->parent_contract = parent_contract_it->second;
        }
      }
    }
  }

  void CampaignConfigSource::link_campaign_update_(
    CampaignConfig& new_config,
    const ConfigUpdateLinks& config_update_links,
    const CreativeFormatTemplateMap_& /*creative_format_template_map*/)
    noexcept
  {
    static const char* FUN = "CampaignConfigSource::link_campaign_update_()";

    for (CampaignConfig::CampaignMap::iterator cmp_it =
           new_config.campaigns.begin();
         cmp_it != new_config.campaigns.end(); )
    {
      cmp_it->second->fc_id =
        filter_not_exist_fc_(cmp_it->second->fc_id, new_config.freq_caps);
      cmp_it->second->group_fc_id =
        filter_not_exist_fc_(cmp_it->second->group_fc_id, new_config.freq_caps);

      ConfigUpdateLinks::IdMap::const_iterator account_link_it =
        config_update_links.campaign_accounts.find(cmp_it->first);

      ConfigUpdateLinks::IdMap::const_iterator advertiser_link_it =
        config_update_links.campaign_advertisers.find(cmp_it->first);

      ConfigUpdateLinks::EcpmMap::const_iterator ecpm_it =
        config_update_links.campaign_ecpms.find(cmp_it->first);

      assert(account_link_it != config_update_links.campaign_accounts.end());
      assert(advertiser_link_it != config_update_links.campaign_advertisers.end());

      AccountMap::const_iterator account_it =
        new_config.accounts.find(account_link_it->second);

      AccountMap::const_iterator advertiser_it =
        new_config.accounts.find(advertiser_link_it->second);

      if(account_it != new_config.accounts.end() &&
         advertiser_it != new_config.accounts.end())
      {
        assert(account_it->second.in() && advertiser_it->second.in());

        cmp_it->second->account = account_it->second;
        cmp_it->second->advertiser = advertiser_it->second;

        cmp_it->second->click_sys_revenue =
          advertiser_it->second->currency->to_system_currency(
            advertiser_it->second->adapt_cost(
              cmp_it->second->click_revenue,
              cmp_it->second->commision));

        if(ecpm_it != config_update_links.campaign_ecpms.end())
        {
          cmp_it->second->ecpm_ = advertiser_it->second->adapt_cost(
            ecpm_it->second.ecpm,
            cmp_it->second->commision);
          cmp_it->second->ctr = ecpm_it->second.ctr;
        }
        else
        {
          cmp_it->second->ecpm_ = RevenueDecimal::ZERO;
          cmp_it->second->ctr = RevenueDecimal::ZERO;
        }

        for(CreativeList::iterator cr_it = cmp_it->second->creatives.begin();
            cr_it != cmp_it->second->creatives.end();
            ++cr_it)
        {
          (*cr_it)->fc_id = filter_not_exist_fc_((*cr_it)->fc_id, new_config.freq_caps);
          (*cr_it)->defined_content_category = cmp_it->second->is_text();

          for(Creative::SizeMap::iterator size_it = (*cr_it)->sizes.begin();
              size_it != (*cr_it)->sizes.end();)
          {
            SizeMap::const_iterator size_def_it = new_config.sizes.find(size_it->first);
            if(size_def_it != new_config.sizes.end())
            {
              size_it->second.size = size_def_it->second;
              ++size_it;
            }
            else
            {
              (*cr_it)->sizes.erase(size_it++);
            }
          }

          for(Creative::CategorySet::const_iterator cr_cat_it =
                (*cr_it)->categories.begin();
              cr_cat_it != (*cr_it)->categories.end(); ++cr_cat_it)
          {
            CampaignConfig::CreativeCategoryMap::const_iterator cat_it =
              new_config.creative_categories.find(*cr_cat_it);

            if(cat_it != new_config.creative_categories.end())
            {
              if(cat_it->second.cct_id == CCT_CONTENT)
              {
                (*cr_it)->defined_content_category = true;
                (*cr_it)->content_categories.push_back(*cr_cat_it);
              }
              else if(cat_it->second.cct_id == CCT_VISUAL)
              {
                (*cr_it)->visual_categories.push_back(*cr_cat_it);
              }
            }
          }

          if(!link_option_values_(
              (*cr_it)->tokens,
              new_config,
              config_update_links.creative_option_values,
              (*cr_it)->ccid))
          {
            (*cr_it)->status = 'W';
          }

          {
            ConfigUpdateLinks::IdCreativeSizeOptionValueMap::
              const_iterator link_tokens_it =
                config_update_links.creative_size_option_values.find((*cr_it)->ccid);

            if(link_tokens_it == config_update_links.creative_size_option_values.end())
            {
              (*cr_it)->status = 'W';
            }
            else
            {
              for(Creative::SizeMap::iterator cr_size_it =
                    (*cr_it)->sizes.begin();
                  cr_size_it != (*cr_it)->sizes.end(); ++cr_size_it)
              {
                ConfigUpdateLinks::CreativeSizeOptionValueMap::
                  const_iterator link_size_tokens_it =
                    link_tokens_it->second.find(cr_size_it->first);

                if(link_size_tokens_it == link_tokens_it->second.end() ||
                   !link_option_values_(
                     cr_size_it->second.tokens,
                     new_config,
                     link_size_tokens_it->second))
                {
                  (*cr_it)->status = 'W';
                }
              }
            }
          }

          if((*cr_it)->status == 'A' && cmp_it->second->is_active())
          {
            // link to creative match categories (CCT_TAG)
            for(unsigned long j = 0;
                j < sizeof(CHECK_CREATIVE_TOKENS) / sizeof(CHECK_CREATIVE_TOKENS[0]);
                ++j)
            {
              OptionTokenValueMap::const_iterator tok_it = (*cr_it)->tokens.find(
                CHECK_CREATIVE_TOKENS[j]);

              if(tok_it != (*cr_it)->tokens.end())
              {
                for(CampaignConfig::CreativeCategoryMap::const_iterator it =
                      new_config.creative_categories.begin();
                    it != new_config.creative_categories.end();
                    ++it)
                {
                  if(it->second.cct_id == CCT_TAG &&
                     plain_match(it->second.name, tok_it->second.value))
                  {
                    (*cr_it)->categories.insert(it->first);
                  }
                }
              }
            }

            // link url categories where this possible,
            // impossible for keyword based creative click url's
            for(unsigned long j = 0;
                j < sizeof(CHECK_CREATIVE_URL_TOKENS) / sizeof(CHECK_CREATIVE_URL_TOKENS[0]);
                ++j)
            {
              OptionTokenValueMap::const_iterator tok_it = (*cr_it)->tokens.find(
                CHECK_CREATIVE_URL_TOKENS[j]);

              if(tok_it != (*cr_it)->tokens.end())
              {
                link_url_categories_(
                  (*cr_it)->categories,
                  tok_it->second.value,
                  config_update_links.domain_category_exclusions);
              }
            }

            link_url_categories_(
              (*cr_it)->click_categories,
              (*cr_it)->click_url.value,
              config_update_links.domain_category_exclusions);

            for(Creative::SizeMap::iterator size_it =
                  (*cr_it)->sizes.begin();
                size_it != (*cr_it)->sizes.end(); ++size_it)
            {
              for(StringSet::const_iterator app_format_it =
                    new_config.all_template_appformats.begin();
                  app_format_it != new_config.all_template_appformats.end();
                  ++app_format_it)
              {
                if(new_config.creative_templates.exist(
                     CreativeTemplateKey(
                       (*cr_it)->creative_format.c_str(),
                       size_it->second.size->protocol_name.c_str(),
                       app_format_it->c_str())))
                {
                  size_it->second.available_appformats.insert(*app_format_it);
                }
              }
            }

            {
              // fill destination_url
              OptionTokenValueMap::const_iterator tok_it =
                (*cr_it)->tokens.find(CreativeTokens::DESTURL);
              std::string destination_url;

              if (tok_it != (*cr_it)->tokens.end() &&
                  !tok_it->second.value.empty())
              {
                destination_url = tok_it->second.value;
              }
              else
              {
                tok_it = (*cr_it)->tokens.find(CreativeTokens::DISPLAY_URL);

                if (tok_it != (*cr_it)->tokens.end() &&
                    !tok_it->second.value.empty())
                {
                  destination_url = tok_it->second.value;
                }
                else
                {
                  destination_url = (*cr_it)->short_click_url;
                }
              }

              try
              {
                if(!destination_url.empty())
                {
                  (*cr_it)->destination_url.url(destination_url);
                }
              }
              catch (HTTP::URLAddress::InvalidURL& ex)
              {
                Stream::Error ostr;
                ostr << FUN << ": ccid = " << (*cr_it)->ccid <<
                  ", invalid destination url '" <<
                  destination_url << "', " << ex.what();

                logger_->log(
                  ostr.str(),
                  Logging::Logger::ERROR,
                  Aspect::TRAFFICKING_PROBLEM,
                  "ADS-TF-1002");
              }
            }

            {
              // fill erid
              OptionTokenValueMap::const_iterator tok_it =
                (*cr_it)->tokens.find(CreativeTokens::ERID);

              if (tok_it != (*cr_it)->tokens.end() &&
                  !tok_it->second.value.empty())
              {
                (*cr_it)->erid = tok_it->second.value;
              }
            }

            {
              // fill video creative traits
              // fill video_duration
              OptionTokenValueMap::const_iterator video_duration_tok_it =
                (*cr_it)->tokens.find(CreativeTokens::VIDEO_DURATION);
              if(video_duration_tok_it != (*cr_it)->tokens.end())
              {
                unsigned long video_duration;
                if(String::StringManip::str_to_int(
                  video_duration_tok_it->second.value,
                  video_duration))
                {
                  (*cr_it)->video_duration = video_duration;
                }
              }

              // fill video_skip_offset
              OptionTokenValueMap::const_iterator video_skip_offset_tok_it =
                (*cr_it)->tokens.find(CreativeTokens::VIDEO_SKIP_OFFSET);
              if(video_skip_offset_tok_it != (*cr_it)->tokens.end())
              {
                unsigned long video_skip_offset;
                if(String::StringManip::str_to_int(
                  video_skip_offset_tok_it->second.value,
                  video_skip_offset))
                {
                  (*cr_it)->video_skip_offset = video_skip_offset;
                }
              }
            }

            Creative& creative = **cr_it;

            if(!drop_https_safe_)
            {
              // fill https_safe_flag
              OptionTokenValueMap::const_iterator https_safe_tok_it =
                creative.tokens.find(CreativeTokens::HTTPS_SAFE);

              if (https_safe_tok_it != creative.tokens.end())
              {
                creative.https_safe_flag =
                  (https_safe_tok_it->second.value == "HTTPS-Safe");
              }
            }
            else
            {
              creative.https_safe_flag = false;
            }
          }
        }

        // link campaign initial contract
        auto campaign_contract_it = config_update_links.campaign_contracts.find(cmp_it->first);
        if(campaign_contract_it != config_update_links.campaign_contracts.end())
        {
          auto contract_it = new_config.contracts.find(campaign_contract_it->second);
          if(contract_it != new_config.contracts.end())
          {
            cmp_it->second->initial_contract = contract_it->second;
          }
        }

        ++cmp_it;
      }
      else
      {
        new_config.inconsistent_campaigns.push_front(cmp_it->second);
        new_config.campaigns.erase(cmp_it++);
      }
    }
  }

  void
  CampaignConfigSource::fill_platform_channel_priorities_(
    CampaignConfig& new_config,
    const ConfigUpdateLinks& config_update_links) noexcept
  {
    typedef std::map<unsigned long, ChannelIdList> ChannelChildMap;

    ChannelChildMap channel_childs;

    for(ExpressionChannelHolderMap::const_iterator ch_it =
          config_update_links.platform_channels.begin();
        ch_it != config_update_links.platform_channels.end();
        ++ch_it)
    {
      unsigned long parent_channel_id = 0;
      if(ch_it->second->params().descriptive_params.in() &&
         ch_it->second->params().descriptive_params->parent_channel_id &&
         config_update_links.platform_channels.find(
           ch_it->second->params().descriptive_params->parent_channel_id) !=
         config_update_links.platform_channels.end())
      {
        parent_channel_id = ch_it->second->params().descriptive_params->parent_channel_id;
      }

      channel_childs[parent_channel_id].push_back(ch_it->first);
    }

    ChannelIdList current_root_channels;
    current_root_channels.push_back(0);
    unsigned long current_priority = 0;
    while(!current_root_channels.empty())
    {
      ChannelIdList new_root_channels;

      for(ChannelIdList::const_iterator root_ch_it = current_root_channels.begin();
          root_ch_it != current_root_channels.end();
          ++root_ch_it)
      {
        ChannelChildMap::iterator ch_it = channel_childs.find(*root_ch_it);
        if(ch_it != channel_childs.end())
        {
          for(ChannelIdList::const_iterator cch_it = ch_it->second.begin();
              cch_it != ch_it->second.end(); ++cch_it)
          {
            ExpressionChannelHolderMap::const_iterator ch_it =
              config_update_links.platform_channels.find(*cch_it);

            assert(ch_it != config_update_links.platform_channels.end());

            std::string norm_name;

            if(ch_it->second->params().descriptive_params.in())
            {
              norm_name = ch_it->second->params().descriptive_params->name;
            }
            
            new_config.platform_channel_priorities.insert(
              std::make_pair(
                *cch_it,
                CampaignConfig::PlatformChannelHolder(current_priority, norm_name)));
          }
          new_root_channels.splice(new_root_channels.end(), ch_it->second);
        }
        channel_childs.erase(*root_ch_it);
      }

      current_root_channels.swap(new_root_channels);
      ++current_priority;
    }

    assert(channel_childs.empty());
  }

  void
  CampaignConfigSource::link_block_channel_update_(
    CampaignConfig& new_config,
    const ConfigUpdateLinks& config_update_links)
    noexcept
  {
    for(ConfigUpdateLinks::BlockChannelMap::const_iterator bch_it =
          config_update_links.block_channels.begin();
        bch_it != config_update_links.block_channels.end(); ++bch_it)
    {
      for(ConfigUpdateLinks::IdList::const_iterator ch_id_it =
            bch_it->second.begin();
          ch_id_it != bch_it->second.end(); ++ch_id_it)
      {
        CampaignConfig::ChannelMap::const_iterator ch_it =
          new_config.expression_channels.find(*ch_id_it);
        if(ch_it != new_config.expression_channels.end() &&
           ch_it->second->has_params())
        {
          new_config.block_channels[bch_it->first].push_back(
            ch_it->second);
        }
      }
    }
  }

  void
  CampaignConfigSource::check_category_channels_(CampaignConfig& new_config)
    noexcept
  {
    static const char* FUN = "CampaignConfigSource::check_category_channels_()";

    // check category channels recursion
    for (CampaignConfig::CategoryChannelMap::iterator cat_ch_it =
           new_config.category_channels.begin();
         cat_ch_it != new_config.category_channels.end(); )
    {
      bool erase_channel = false;

      if(cat_ch_it->second->parent_channel_id)
      {
        CampaignConfig::CategoryChannelMap::iterator parent_cat_ch_it =
          new_config.category_channels.find(
            cat_ch_it->second->parent_channel_id);

        if(parent_cat_ch_it != new_config.category_channels.end())
        {
          // check recursion
          std::set<unsigned long> used_channels;
          used_channels.insert(cat_ch_it->first);

          const CategoryChannel* cur_cat_ch = parent_cat_ch_it->second;

          while(cur_cat_ch &&
            used_channels.find(cur_cat_ch->channel_id) == used_channels.end())
          {
            if(cur_cat_ch->parent_channel_id)
            {
              CampaignConfig::CategoryChannelMap::iterator lp_ch_it =
                new_config.category_channels.find(
                  cur_cat_ch->parent_channel_id);
              if(lp_ch_it != new_config.category_channels.end())
              {
                used_channels.insert(cur_cat_ch->channel_id);
                cur_cat_ch = lp_ch_it->second.in();
              }
              else
              {
                erase_channel = true;
                cur_cat_ch = 0;
              }
            }
            else
            {
              cur_cat_ch = 0;
            }
          }

          if(cur_cat_ch)
          {
            erase_channel = true;

            logger_->sstream(Logging::Logger::ERROR,
              Aspect::CAMPAIGN_CONFIG_SOURCE,
              "ADS-IMPL-60") << FUN <<
              ": Found recursion in category channel: '" <<
              cat_ch_it->second->name <<
              "' - it will be ignored";
          }
        }
        else
        {
          erase_channel = true;
        }
      }

      if(erase_channel)
      {
        new_config.category_channels.erase(cat_ch_it++);
      }
      else
      {
        ++cat_ch_it;
      }
    }
  }

  void
  CampaignConfigSource::fill_campaign_action_markers_(CampaignConfig& new_config)
    noexcept
  {
    for(AdvActionMap::const_iterator act_it = new_config.adv_actions.begin();
        act_it != new_config.adv_actions.end(); ++act_it)
    {
      for(AdvActionDef::CCGIdList::const_iterator act_ccg_it =
            act_it->second.ccg_ids.begin();
          act_ccg_it != act_it->second.ccg_ids.end(); ++act_ccg_it)
      {
        CampaignConfig::CampaignMap::iterator ccg_it =
          new_config.campaigns.find(*act_ccg_it);
        if(ccg_it != new_config.campaigns.end())
        {
          ccg_it->second->has_custom_actions = true;
        }
      }
    }
  }

  void CampaignConfigSource::enrich_channels_by_geo_channels_(
    CampaignConfig& new_config) noexcept
  {
    for(CampaignConfig::ChannelMap::iterator ch_it =
          new_config.expression_channels.begin();
        ch_it != new_config.expression_channels.end(); ++ch_it)
    {
      if(!ch_it->second->channel.in() &&
         new_config.geo_channels->channel_ids().find(ch_it->first) !=
           new_config.geo_channels->channel_ids().end())
      {
        ChannelParams channel_params;
        channel_params.channel_id = ch_it->first;
        channel_params.type = 'G';
        channel_params.status = 'A';
        ExpressionChannelBase_var new_channel(new SimpleChannel(channel_params));
        ch_it->second->channel = new_channel;
      }
    }
  }

  void
  CampaignConfigSource::enrich_channels_by_geo_coord_channels_(
    CampaignConfig& new_config) noexcept
  {
    const ChannelIdSet& geo_coord_channel_ids =
      new_config.geo_coord_channels->channel_ids();

    for(CampaignConfig::ChannelMap::iterator ch_it =
          new_config.expression_channels.begin();
        ch_it != new_config.expression_channels.end(); ++ch_it)
    {
      if(!ch_it->second->channel.in() &&
         geo_coord_channel_ids.find(ch_it->first) !=
           geo_coord_channel_ids.end())
      {
        ChannelParams channel_params;
        channel_params.channel_id = ch_it->first;
        channel_params.type = 'G';
        channel_params.status = 'A';
        ExpressionChannelBase_var new_channel(new SimpleChannel(channel_params));
        ch_it->second->channel = new_channel;
      }
    }
  }

  void CampaignConfigSource::enrich_channels_by_platform_channels_(
    CampaignConfig& new_config) noexcept
  {
    for(CampaignConfig::ChannelMap::iterator ch_it =
          new_config.expression_channels.begin();
        ch_it != new_config.expression_channels.end(); ++ch_it)
    {
      if(!ch_it->second->channel.in() &&
         new_config.platforms.find(ch_it->first) !=
           new_config.platforms.end())
      {
        ChannelParams channel_params;
        channel_params.channel_id = ch_it->first;
        channel_params.type = 'V';
        channel_params.status = 'A';
        ExpressionChannelBase_var new_channel(new SimpleChannel(channel_params));
        ch_it->second->channel = new_channel;
      }
    }
  }

  void
  CampaignConfigSource::fill_tag_pricings_(Tag* tag)
    noexcept
  {
    const CCGRateType ALL_CCG_RATE_TYPES[] = { CR_CPM, CR_CPC, CR_CPA };
    const CCGType ALL_CCG_TYPES[] = { CT_DISPLAY, CT_TEXT };

    const std::string* prev_country_code = 0;

    for(Tag::TagPricings::const_iterator tp_it = tag->tag_pricings.begin();
        tp_it != tag->tag_pricings.end(); ++tp_it)
    {
      if(prev_country_code == 0 ||
         tp_it->first.country_code != *prev_country_code)
      {
        const std::string& cur_country_code = tp_it->first.country_code;
        prev_country_code = &cur_country_code;

        Tag::TagPricings::const_iterator next_tp_it = tp_it;
        ++next_tp_it;

        if((next_tp_it == tag->tag_pricings.end() ||
            next_tp_it->first.country_code != cur_country_code) &&
           tp_it->first.ccg_rate_type == CR_ALL &&
           tp_it->first.ccg_type == CT_ALL)
        {
          tag->no_imp_tag_pricings.insert(std::make_pair(
            cur_country_code,
            &tp_it->second));
        }
        else
        {
          // insert null stub
          tag->no_imp_tag_pricings.insert(std::make_pair(
            cur_country_code,
            (const AdServer::CampaignSvcs::AdInstances::Tag::TagPricing*)0));
        }

        const Tag::TagPricing* max_tp = 0;

        for(unsigned long ccg_rate_type_i = 0;
            ccg_rate_type_i < sizeof(ALL_CCG_RATE_TYPES) /
              sizeof(ALL_CCG_RATE_TYPES[0]);
            ++ccg_rate_type_i)
        {
          for(unsigned long ccg_type_i = 0;
              ccg_type_i < sizeof(ALL_CCG_TYPES) / sizeof(ALL_CCG_TYPES[0]);
              ++ccg_type_i)
          {
            const Tag::TagPricing* cur_tp = tag->select_tag_pricing(
              cur_country_code.c_str(),
              ALL_CCG_TYPES[ccg_type_i],
              ALL_CCG_RATE_TYPES[ccg_rate_type_i]);

            if(max_tp == 0 || max_tp->cpm < cur_tp->cpm)
            {
              max_tp = cur_tp;
            }
          }
        }

        if(max_tp)
        {
          tag->country_tag_pricings.insert(std::make_pair(cur_country_code, *max_tp));
        }
      }
    }
  }

  void
  CampaignConfigSource::link_tag_size_option_values_(
    Tag& tag,
    OptionTokenValueMap Tag::Size::* tokens_field,
    const CampaignConfig& new_config,
    unsigned long tag_id,
    const ConfigUpdateLinks::IdTagSizeOptionValueMap& tag_size_option_values)
    noexcept
  {
    ConfigUpdateLinks::IdTagSizeOptionValueMap::const_iterator it =
      tag_size_option_values.find(tag_id);

    if(it != tag_size_option_values.end())
    {
      for(ConfigUpdateLinks::TagSizeOptionValueMap::const_iterator
            topt_it = it->second.begin();
          topt_it != it->second.end(); ++topt_it)
      {
        auto tag_size_it = tag.sizes.find(topt_it->first);

        if (tag_size_it != tag.sizes.end())
        {
          Tag::Size_var new_tag_size = new Tag::Size(*tag_size_it->second);
          link_option_values_(
            (*new_tag_size).*tokens_field,
            new_config,
            topt_it->second);
          tag_size_it->second = new_tag_size;
        }
      }
    }
  }

  void CampaignConfigSource::link_config_update_(
    const ConfigUpdateLinks& config_update_links,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigSource::link_config_update_()";

    CreativeFormatTemplateMap_ creative_format_template_map;

    for(CreativeTemplateMap::KeyMap::const_iterator ct_it =
          new_config.creative_templates.begin();
        ct_it != new_config.creative_templates.end(); ++ct_it)
    {
      creative_format_template_map[ct_it->first.creative_format][
        ct_it->first.creative_size].insert(ct_it->first.app_format);
    }

    link_template_update_(
      new_config,
      config_update_links);

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_SOURCE) <<
        FUN << ": link accounts ...";
    }

    link_account_update_(
      new_config,
      config_update_links);

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_SOURCE) <<
        FUN << ": link campaigns (number of creative categories = " <<
        new_config.creative_categories.size() << ") ...";
    }

    link_campaign_update_(
      new_config,
      config_update_links,
      creative_format_template_map);

    for (SiteMap::iterator site_it = new_config.sites.begin();
         site_it != new_config.sites.end(); )
    {
      site_it->second->freq_cap_id =
        filter_not_exist_fc_(site_it->second->freq_cap_id, new_config.freq_caps);

      ConfigUpdateLinks::IdMap::const_iterator account_link_it =
        config_update_links.site_accounts.find(site_it->first);

      assert(account_link_it != config_update_links.site_accounts.end());

      AccountMap::const_iterator account_it =
        new_config.accounts.find(account_link_it->second);

      if(account_it != new_config.accounts.end())
      {
        site_it->second->account = account_it->second;
        ++site_it;
      }
      else
      {
        new_config.sites.erase(site_it++);
      }
    }

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_SOURCE) <<
        FUN << ": link tags ...";
    }

    for (TagMap::iterator tag_it = new_config.tags.begin();
         tag_it != new_config.tags.end(); )
    {
      for(Tag::SizeMap::iterator size_it = tag_it->second->sizes.begin();
          size_it != tag_it->second->sizes.end(); )
      {
        SizeMap::const_iterator size_def_it = new_config.sizes.find(size_it->first);
        if(size_def_it != new_config.sizes.end())
        {
          Tag::Size_var tag_size = new Tag::Size(*size_it->second);
          tag_size->size = size_def_it->second;
          size_it->second = tag_size;
          ++size_it;
        }
        else
        {
          tag_it->second->sizes.erase(size_it++);
        }
      }

      link_option_values_(
        tag_it->second->tokens,
        new_config,
        config_update_links.tag_option_values,
        tag_it->first);

      link_option_values_(
        tag_it->second->hidden_tokens,
        new_config,
        config_update_links.tag_hidden_option_values,
        tag_it->first);

      link_option_values_(
        tag_it->second->passback_tokens,
        new_config,
        config_update_links.tag_passback_option_values,
        tag_it->first);

      {
        ConfigUpdateLinks::IdTemplateOptionValueMap::const_iterator it =
          config_update_links.tag_template_option_values.find(tag_it->first);
        if(it != config_update_links.tag_template_option_values.end())
        {
          for(ConfigUpdateLinks::TemplateOptionValueMap::const_iterator
                topt_it = it->second.begin();
              topt_it != it->second.end(); ++topt_it)
          {
            link_option_values_(
              tag_it->second->template_tokens[topt_it->first],
              new_config,
              topt_it->second);
          }
        }
      }

      link_tag_size_option_values_(
        *(tag_it->second),
        &Tag::Size::tokens,
        new_config,
        tag_it->first,
        config_update_links.tag_size_option_values);

      link_tag_size_option_values_(
        *(tag_it->second),
        &Tag::Size::hidden_tokens,
        new_config,
        tag_it->first,
        config_update_links.tag_size_option_hidden_values);

      ConfigUpdateLinks::IdMap::const_iterator site_link_it =
        config_update_links.tag_sites.find(tag_it->first);

      assert(site_link_it != config_update_links.tag_sites.end());

      SiteMap::const_iterator site_it =
        new_config.sites.find(site_link_it->second);

      if(site_it != new_config.sites.end())
      {
        assert(site_it->second->account.in());

        tag_it->second->site = site_it->second;
        tag_it->second->max_random_cpm =
          site_it->second->account->currency->to_system_currency(
            tag_it->second->pub_max_random_cpm);

        for (Tag::TagPricings::iterator tp_it =
               tag_it->second->tag_pricings.begin();
             tp_it != tag_it->second->tag_pricings.end(); ++tp_it)
        {
          ExtRevenueDecimal ext_imp_revenue;
          narrow_decimal(ext_imp_revenue, tp_it->second.imp_revenue);

          ExtRevenueDecimal ext_tag_sys_imp_revenue =
            site_it->second->account->currency->to_system_currency(
              ext_imp_revenue);

          ExtRevenueDecimal ext_commision;
          narrow_decimal(ext_commision, site_it->second->account->commision);

          ExtRevenueDecimal ext_cpm = ExtRevenueDecimal::mul(
            ExtRevenueDecimal::mul(
              ext_tag_sys_imp_revenue,
              EXT_REVENUE_ONE - ext_commision,
              Generics::DMR_FLOOR),
            EXT_CPM_FACTOR,
            Generics::DMR_FLOOR);

          narrow_decimal(tp_it->second.cpm, ext_cpm);
        }

        fill_tag_pricings_(tag_it->second);

        // fill excluded domains by categories
        // tag level
        for(CreativeCategoryIdSet::const_iterator rej_cat_it =
              tag_it->second->rejected_categories.begin();
            rej_cat_it != tag_it->second->rejected_categories.end();
            ++rej_cat_it)
        {
          CampaignConfig::CreativeCategoryMap::const_iterator cat_it =
            new_config.creative_categories.find(*rej_cat_it);

          if(cat_it != new_config.creative_categories.end() &&
             !cat_it->second.exclude_domain.empty())
          {
            tag_it->second->exclude_creative_domains.insert(
              cat_it->second.exclude_domain);
          }
        }

        for(CreativeCategoryIdSet::const_iterator rej_cat_it =
              tag_it->second->site->rejected_creative_categories.begin();
            rej_cat_it != tag_it->second->site->rejected_creative_categories.end();
            ++rej_cat_it)
        {
          if(tag_it->second->accepted_categories.find(*rej_cat_it) ==
             tag_it->second->accepted_categories.end())
          {
            // tag don't override category status
            CampaignConfig::CreativeCategoryMap::const_iterator cat_it =
              new_config.creative_categories.find(*rej_cat_it);

            if(cat_it != new_config.creative_categories.end() &&
               !cat_it->second.exclude_domain.empty())
            {
              tag_it->second->exclude_creative_domains.insert(
                cat_it->second.exclude_domain);
            }
          }
        }

        for(Tag::SizeMap::const_iterator tag_size_it =
              tag_it->second->sizes.begin();
            tag_size_it != tag_it->second->sizes.end();
            ++tag_size_it)
        {
          if(tag_size_it->second->size->protocol_name.compare(0, 2, "rm") == 0)
          {
            const char* protocol_name = nullptr;
            if (tag_size_it->second->size->protocol_name.compare(2, 3, "dto") == 0)
            {
              protocol_name = "rmdto";
            }
            else if (tag_size_it->second->size->protocol_name.compare(2, 4, "rich") == 0)
            {
              protocol_name = "rmrich";
            }

            if (protocol_name)
            {
              new_config.site_tags[
                IdTagKey(
                  tag_it->second->site->site_id,
                  protocol_name)].push_back(
                tag_it->second);
              new_config.account_tags[
                IdTagKey(
                  tag_it->second->site->account->account_id,
                  protocol_name)].push_back(
                  tag_it->second);
            }
            new_config.site_tags[
              IdTagKey(
                tag_it->second->site->site_id,
                "rm")].push_back(tag_it->second);
            new_config.account_tags[
              IdTagKey(
                tag_it->second->site->account->account_id,
                "rm")].push_back(tag_it->second);
          }
          else
          {
            new_config.site_tags[
              IdTagKey(
                tag_it->second->site->site_id,
                tag_size_it->second->size->protocol_name.c_str())].push_back(
               tag_it->second);
            new_config.account_tags[
              IdTagKey(
                tag_it->second->site->account->account_id,
                tag_size_it->second->size->protocol_name.c_str())].push_back(
              tag_it->second);
          }
        }

        ++tag_it;
      }
      else
      {
        new_config.tags.erase(tag_it++);
      }
    }

    std::vector<unsigned long> del_colocations;

    for (CampaignConfig::ColocationMap::iterator colo_it =
           new_config.colocations.begin();
         colo_it != new_config.colocations.end(); ++colo_it)
    {
      link_option_values_(
        colo_it->second->tokens,
        new_config,
        config_update_links.colocation_option_values,
        colo_it->first);

      ConfigUpdateLinks::IdMap::const_iterator account_link_it =
        config_update_links.colocation_accounts.find(colo_it->first);

      assert(account_link_it !=
        config_update_links.colocation_accounts.end());

      AccountMap::const_iterator account_it =
        new_config.accounts.find(account_link_it->second);

      if (account_it != new_config.accounts.end())
      {
        colo_it->second->account = account_it->second;
      }
      else
      {
        del_colocations.push_back(colo_it->first);
      }
    }

    for(auto colo_it = del_colocations.begin(); colo_it != del_colocations.end();
      ++colo_it)
    {
      new_config.colocations.erase(*colo_it);
    }

    for (CampaignConfig::CountryMap::iterator country_it =
            new_config.countries.begin();
         country_it != new_config.countries.end();
         ++country_it)
    {
      ConfigUpdateLinks::CountryOptionValueMap::
        const_iterator options_it =
        config_update_links.country_option_values.find(
          country_it->first);

      if(options_it != config_update_links.country_option_values.end())
      {
        link_option_values_(
          country_it->second->tokens,
          new_config,
          options_it->second);
      }
    }


    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_SOURCE) <<
        FUN << ": link category channels ...";
    }

    check_category_channels_(new_config);

    // fill category channels tree
    typedef std::map<unsigned long, CategoryChannelNode_var> CategoryIdChannelNodeMap;
    CategoryIdChannelNodeMap all_category_channel_nodes;
    for(CampaignConfig::CategoryChannelMap::const_iterator ch_it =
          new_config.category_channels.begin();
        ch_it != new_config.category_channels.end(); ++ch_it)
    {
      CategoryChannelNode_var node(new CategoryChannelNode());
      node->channel_id = ch_it->second->channel_id;
      node->name = ch_it->second->name;
      node->localizations = ch_it->second->localizations;
      node->flags = ch_it->second->flags;
      all_category_channel_nodes.insert(
        std::make_pair(ch_it->second->channel_id, node));
    }

    for(CampaignConfig::CategoryChannelMap::const_iterator ch_it =
          new_config.category_channels.begin();
        ch_it != new_config.category_channels.end(); ++ch_it)
    {
      CategoryIdChannelNodeMap::iterator ch_node_it =
        all_category_channel_nodes.find(
          ch_it->second->channel_id);

      assert(ch_node_it != all_category_channel_nodes.end());

      if(ch_it->second->parent_channel_id)
      {
        CategoryIdChannelNodeMap::iterator parent_ch_node_it =
          all_category_channel_nodes.find(
            ch_it->second->parent_channel_id);

        if(ch_node_it != all_category_channel_nodes.end())
        {
          parent_ch_node_it->second->child_category_channels.insert(
            std::make_pair(
              ch_node_it->second->name,
              ch_node_it->second));
        }
      }
      else
      {
        new_config.category_channel_nodes.insert(
          std::make_pair(ch_node_it->second->name, ch_node_it->second));
      }
    }

    fill_campaign_action_markers_(new_config);

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_SOURCE) <<
        FUN << ": enrich channels by geo channels ...";
    }

    enrich_channels_by_geo_channels_(new_config);

    enrich_channels_by_geo_coord_channels_(new_config);

    enrich_channels_by_platform_channels_(new_config);

    // init platform channels index
    new_config.platform_channels = new ExpressionChannelIndex();
    new_config.platform_channels->index(config_update_links.platform_channels);

    fill_platform_channel_priorities_(new_config, config_update_links);

    link_block_channel_update_(new_config, config_update_links);

    //
    for(auto it = new_config.campaigns.begin(); it != new_config.campaigns.end(); ++it)
    {
      if(it->second->channel.in())
      {
        it->second->fast_channel = new FastExpressionChannel(it->second->channel);
      }
    }

    link_contract_update_(new_config, config_update_links);
  }

  void
  CampaignConfigSource::apply_creative_categories_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.creative_categories.length(); i++)
    {
      const CreativeCategoryInfo& ccat_info = update_info.creative_categories[i];
      CreativeCategory& ccat =
        new_config.creative_categories[ccat_info.creative_category_id];
      ccat.cct_id = static_cast<
        AdServer::CampaignSvcs::AdInstances::CreativeCategoryType>(ccat_info.cct_id);
      ccat.name = ccat_info.name;
      for(CORBA::ULong ec_i = 0; ec_i < ccat_info.external_categories.length(); ++ec_i)
      {
        CorbaAlgs::convert_sequence(
          ccat_info.external_categories[ec_i].names,
          ccat.external_categories[
            static_cast<AdRequestType>(
              ccat_info.external_categories[ec_i].ad_request_type)]);
      }

      // ADSC-9551
      // For AppNexus we create artificial mapping by category name for visual categories
      // This trick can be removed when will be filled mapping on DB side.
      if(ccat.cct_id == CCT_VISUAL)
      {
        ccat.external_categories[AR_APPNEXUS].insert(ccat.name);
      }

      ccat.timestamp = CorbaAlgs::unpack_time(ccat_info.timestamp);

      // fill external categories mapping to internal categories
      for(CreativeCategory::ExternalCategoryMap::const_iterator ext_ccat_it =
            ccat.external_categories.begin();
          ext_ccat_it != ccat.external_categories.end(); ++ext_ccat_it)
      {
        for(StringSet::const_iterator ext_ccat_name_it =
              ext_ccat_it->second.begin();
            ext_ccat_name_it != ext_ccat_it->second.end();
            ++ext_ccat_name_it)
        {
          new_config.external_creative_categories[
            ext_ccat_it->first][*ext_ccat_name_it].insert(
              ccat_info.creative_category_id);
        }
      }


      if(ccat.cct_id == CCT_TAG)
      {
        try
        {
          HTTP::BrowserAddress url(ccat.name);
          ccat.exclude_domain = url.host().substr(
            url.host().compare(0, 4, "www.") == 0 ? 4 : 0).str();
          String::AsciiStringManip::to_lower(ccat.exclude_domain);

          if(!ccat.exclude_domain.empty())
          {
            config_update_links.domain_category_exclusions[
              ccat.exclude_domain].insert(ccat_info.creative_category_id);
          }
        }
        catch(...)
        {}
      }
    }
  }

  void
  CampaignConfigSource::apply_currency_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.currencies.length(); ++i)
    {
      const AdServer::CampaignSvcs::CurrencyInfo& currency_info =
        update_info.currencies[i];

      Currency_var p_currency = new Currency();

      p_currency->currency_id = currency_info.currency_id;
      p_currency->currency_exchange_id = currency_info.currency_exchange_id;
      p_currency->effective_date = currency_info.effective_date;
      p_currency->rate = CorbaAlgs::unpack_decimal<RevenueDecimal>(currency_info.rate);
      p_currency->timestamp = CorbaAlgs::unpack_time(currency_info.timestamp);
      p_currency->fraction = currency_info.fraction_digits;
      p_currency->currency_code = currency_info.currency_code;

      new_config.currencies[p_currency->currency_id] = p_currency;
      new_config.currency_codes[p_currency->currency_code] = p_currency;
    }
  }

  void
  CampaignConfigSource::apply_frequency_caps_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.frequency_caps.length(); i++)
    {
      const AdServer::CampaignSvcs::FreqCapInfo& freq_cap_info =
        update_info.frequency_caps[i];

      /* filter non active freq caps */
      if(freq_cap_info.lifelimit ||
         freq_cap_info.period ||
         freq_cap_info.window_limit)
      {
        new_config.freq_caps.insert(
          std::make_pair(
            freq_cap_info.fc_id,
            FreqCap(
              freq_cap_info.fc_id,
              CorbaAlgs::unpack_time(freq_cap_info.timestamp),
              freq_cap_info.lifelimit,
              Generics::Time(freq_cap_info.period),
              freq_cap_info.window_limit,
              Generics::Time(freq_cap_info.window_time))));
      }
    }
  }

  void
  CampaignConfigSource::apply_creative_templates_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for(CORBA::ULong i = 0; i < update_info.creative_templates.length(); ++i)
    {
      const CreativeTemplateInfo& cr_templ_info =
        update_info.creative_templates[i];

      ConfigUpdateLinks::TemplateOptionsLink& template_options_link =
        config_update_links.template_option_values[cr_templ_info.id];

      ConfigUpdateLinks::OptionTokenValueMap_var tokens =
        new RCOptionTokenValueMap();
      unpack_option_value_map(
        template_options_link.unlinked_tokens, cr_templ_info.tokens);
      template_options_link.tokens_ref = tokens;

      ConfigUpdateLinks::OptionTokenValueMap_var hidden_tokens =
        new RCOptionTokenValueMap();
      unpack_option_value_map(
        template_options_link.unlinked_hidden_tokens, cr_templ_info.hidden_tokens);
      template_options_link.hidden_tokens_ref = hidden_tokens;

      for(CORBA::ULong ctf_i = 0; ctf_i < cr_templ_info.files.length(); ++ctf_i)
      {
        const CreativeTemplateFileInfo& ctf_info =
          cr_templ_info.files[ctf_i];

        std::string templ_path;

        new_config.creative_templates.insert(
          CreativeTemplateKey(
            ctf_info.creative_format.in(),
            ctf_info.creative_size.in(),
            ctf_info.app_format.in()),
          CreativeTemplate(
            adopt_template_path(ctf_info.template_file.in(), templ_path),
            adopt_template_type(ctf_info.type),
            ctf_info.mime_format.in(),
            ctf_info.track_impr,
            tokens,
            hidden_tokens,
            CorbaAlgs::unpack_time(cr_templ_info.timestamp)));

        new_config.all_template_appformats.insert(
          ctf_info.app_format.in());
      }
    }
  }

  void
  CampaignConfigSource::apply_sizes_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.sizes.length(); i++)
    {
      Size_var new_size = new Size();
      new_size->size_id = update_info.sizes[i].size_id;
      new_size->protocol_name = update_info.sizes[i].protocol_name;
      new_size->size_type_id = update_info.sizes[i].size_type_id;
      new_size->width = update_info.sizes[i].width;
      new_size->height = update_info.sizes[i].height;
      new_size->timestamp = CorbaAlgs::unpack_time(update_info.sizes[i].timestamp);

      new_config.sizes.insert(std::make_pair(
        update_info.sizes[i].size_id,
        new_size));
    }
  }

  void
  CampaignConfigSource::apply_app_formats_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.app_formats.length(); i++)
    {
      new_config.app_formats.insert(std::make_pair(
        update_info.app_formats[i].app_format.in(),
        AppFormatDef(
          update_info.app_formats[i].mime_format,
          CorbaAlgs::unpack_time(update_info.app_formats[i].timestamp))));
    }
  }

  void CampaignConfigSource::apply_creative_options_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.activate_creative_options.length(); i++)
    {
      const CreativeOptionInfo& co_info = update_info.activate_creative_options[i];
      CreativeOptionDef& co =
        new_config.creative_options[co_info.option_id];

      co.token = co_info.token.in();
      co.timestamp = CorbaAlgs::unpack_time(co_info.timestamp);
      co.type = co_info.type;

      CorbaAlgs::convert_sequence(
        co_info.token_relations, co.token_relations);

      if(co.type == 'L')
      {
        new_config.token_processors[co_info.option_id] =
          new LinkTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
      else if(co.type == 'U')
      {
        new_config.token_processors[co_info.option_id] =
          new UrlTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
      else if(co.type == 'F')
      {
        new_config.token_processors[co_info.option_id] =
          new FileTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
      else if(co.type == 'D')
      {
        new_config.token_processors[co_info.option_id] =
          new DynamicContentUrlTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
      else if(co.type == 'u')
      {
        new_config.token_processors[co_info.option_id] =
          new PublisherUrlTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
      else if(co.type == 'f')
      {
        new_config.token_processors[co_info.option_id] =
          new PublisherFileTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
      else
      {
        new_config.token_processors[co_info.option_id] =
          new BaseTokenProcessor(
            co.token.c_str(), co.token_relations);
      }
    }

    TokenSet default_click_token_relations;
    default_click_token_relations.insert(CreativeTokens::KEYWORD);
    default_click_token_relations.insert(CreativeTokens::RANDOM);
    default_click_token_relations.insert(CreativeTokens::CGID);
    default_click_token_relations.insert(CreativeTokens::CID);

    new_config.default_click_token_processor =
      new BaseTokenProcessor(
        CreativeTokens::ADV_CLICK_URL.c_str(),
        default_click_token_relations);

    // add rule tokens processors
    for(auto inst_rule_it = creative_rules_.begin();
      inst_rule_it != creative_rules_.end(); ++inst_rule_it)
    {
      for(auto token_it = inst_rule_it->second.tokens.begin();
        token_it != inst_rule_it->second.tokens.end(); ++token_it)
      {
        String::TextTemplate::Keys keys;
        Template::get_keys(keys, token_it->second.value);

        TokenSet token_relations;
        for(auto key_it = keys.begin(); key_it != keys.end(); ++key_it)
        {
          token_relations.insert(*key_it);
        }

        new_config.token_processors[token_it->second.option_id] =
          new BaseTokenProcessor(
            token_it->first.c_str(),
            token_relations);
      }
    }
  }

  void
  CampaignConfigSource::apply_account_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.accounts.length(); i++)
    {
      const AccountInfo& acc_info = update_info.accounts[i];

      Account_var p_acc = new AccountDef();
      p_acc->account_id = acc_info.account_id;
      p_acc->internal_account_id = acc_info.internal_account_id;
      p_acc->legal_name = acc_info.legal_name;
      p_acc->flags = acc_info.flags;
      p_acc->at_flags = acc_info.at_flags;
      p_acc->text_adserving = acc_info.text_adserving;
      p_acc->country = acc_info.country.in();
      p_acc->time_offset = CorbaAlgs::unpack_time(acc_info.time_offset);
      p_acc->commision = CorbaAlgs::unpack_decimal<RevenueDecimal>(acc_info.commision);
      p_acc->media_handling_fee = RevenueDecimal::ZERO;
      for(CORBA::ULong wa_i = 0; wa_i < acc_info.walled_garden_accounts.length(); ++wa_i)
      {
        if(acc_info.walled_garden_accounts[wa_i] > 4000000000)
        {
          p_acc->media_handling_fee = RevenueDecimal::div(
            RevenueDecimal(false, acc_info.walled_garden_accounts[wa_i] - 4000000000, 0),
            RevenueDecimal(false, 100000000, 0));
          break;
        }
      }
      p_acc->budget = CorbaAlgs::unpack_decimal<RevenueDecimal>(acc_info.budget);
      p_acc->paid_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(acc_info.paid_amount);
      p_acc->status = acc_info.status;
      p_acc->eval_status = acc_info.eval_status;
      p_acc->timestamp = CorbaAlgs::unpack_time(acc_info.timestamp);
      CorbaAlgs::convert_sequence(acc_info.walled_garden_accounts,
        p_acc->walled_garden_accounts);
      p_acc->auction_rate = static_cast<AuctionRateType>(acc_info.auction_rate);
      p_acc->use_pub_pixels = acc_info.use_pub_pixels;
      p_acc->pub_pixel_optin = acc_info.pub_pixel_optin.in();
      p_acc->pub_pixel_optout = acc_info.pub_pixel_optout.in();
      p_acc->self_service_commission = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        acc_info.self_service_commission);

      // agency and currency will be linked after

      new_config.accounts[acc_info.account_id] = p_acc;

      if(p_acc->is_active())
      {
        // Don't check result inserting accounts into the set
        if(!p_acc->pub_pixel_optin.empty())
        {
          new_config.pub_pixel_accounts[
            PubPixelAccountKey(p_acc->country.c_str(), US_OPTIN)].insert(
              p_acc);
        }

        if(!p_acc->pub_pixel_optout.empty())
        {
          new_config.pub_pixel_accounts[
            PubPixelAccountKey(p_acc->country.c_str(), US_OPTOUT)].insert(
              p_acc);
        }
      }

      config_update_links.account_currencies.insert(
        std::make_pair(acc_info.account_id, acc_info.currency_id));

      if(acc_info.agency_account_id)
      {
        config_update_links.account_agencies.insert(
          std::make_pair(acc_info.account_id, acc_info.agency_account_id));
      }
    }
  }

  void CampaignConfigSource::apply_campaigns_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.campaigns.length(); i++)
    {
      const CampaignInfo& campaign_info = update_info.campaigns[i];

      config_update_links.campaign_accounts.insert(
        std::make_pair(
          campaign_info.campaign_id, campaign_info.account_id));

      config_update_links.campaign_advertisers.insert(
        std::make_pair(
          campaign_info.campaign_id, campaign_info.advertiser_id));

      if(campaign_info.initial_contract_id)
      {
        config_update_links.campaign_contracts.emplace(
          campaign_info.campaign_id, campaign_info.initial_contract_id);
      }

      Campaign_var campaign = new Campaign();

      campaign->campaign_id = campaign_info.campaign_id;
      campaign->timestamp = CorbaAlgs::unpack_time(campaign_info.timestamp);
      campaign->campaign_group_id = campaign_info.campaign_group_id;
      campaign->fc_id = campaign_info.fc_id;
      campaign->group_fc_id = campaign_info.group_fc_id;
      campaign->flags = campaign_info.flags;
      campaign->marketplace = campaign_info.marketplace;
      campaign->imp_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.imp_revenue);
      campaign->click_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.click_revenue);
      campaign->action_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.action_revenue);
      campaign->commision = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.commision);

      ExpressionChannel::Expression expr;
      unpack_expression(expr, campaign_info.expression, new_config.expression_channels);

      if(!(expr == ExpressionChannel::Expression::EMPTY))
      {
        campaign->channel = new ExpressionChannel(expr);
        //campaign->fast_channel = new FastExpressionChannel(campaign->channel);
      }

      ExpressionChannel::Expression stat_expr;
      unpack_expression(stat_expr,
        campaign_info.stat_expression,
        new_config.expression_channels);

      if(!(stat_expr == ExpressionChannel::Expression::EMPTY))
      {
        campaign->stat_channel = new ExpressionChannel(stat_expr);
      }

      campaign->ccg_rate_id = campaign_info.ccg_rate_id;
      campaign->ccg_rate_type = static_cast<CCGRateType>(
        campaign_info.ccg_rate_type);

      campaign->status = campaign_info.status;
      campaign->eval_status = campaign_info.eval_status;
      campaign->ccg_type = static_cast<CCGType>(campaign_info.ccg_type);
      campaign->targeting_type = campaign_info.target_type;
      campaign->start_user_group_id = campaign_info.start_user_group_id;
      campaign->end_user_group_id = campaign_info.end_user_group_id;
      campaign->ctr_reset_id = campaign_info.ctr_reset_id;
      campaign->mode = static_cast<CampaignMode>(campaign_info.mode);
      campaign->min_uid_age = CorbaAlgs::unpack_time(campaign_info.min_uid_age);
      campaign->seq_set_rotate_imps = campaign_info.seq_set_rotate_imps;
      campaign->delivery_coef = campaign_info.delivery_coef;

      unpack_delivery_limits(
        campaign->campaign_delivery_limits,
        campaign_info.campaign_delivery_limits);
      unpack_delivery_limits(
        campaign->ccg_delivery_limits,
        campaign_info.ccg_delivery_limits);
      campaign->max_pub_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.max_pub_share);
      campaign->has_custom_actions = false;

      campaign->country = campaign_info.country.in();
      campaign->bid_strategy = static_cast<BidStrategy>(campaign_info.bid_strategy);

      {
        // configure base ctr goal
        const RevenueDecimal min_ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          campaign_info.min_ctr_goal);
        campaign->ctr_modifiable = true; // TODO: flag control
        campaign->base_min_ctr_goal = min_ctr;
        campaign->set_min_ctr_goal(min_ctr);
      }

      convert_interval_sequence(
        campaign->weekly_run_intervals,
        campaign_info.weekly_run_intervals);

      CorbaAlgs::convert_sequence(campaign_info.sites, campaign->sites);

      CorbaAlgs::convert_sequence(
        campaign_info.colocations,
        campaign->colocations);

      CorbaAlgs::convert_sequence(
        campaign_info.exclude_pub_accounts,
        campaign->exclude_pub_accounts);

      for(CORBA::ULong tag_i = 0; tag_i < campaign_info.exclude_tags.length();
          ++tag_i)
      {
        campaign->exclude_tags.insert(std::make_pair(
          campaign_info.exclude_tags[tag_i].tag_id,
          campaign_info.exclude_tags[tag_i].delivery_value));
      }

      // fill creatives
      for (CORBA::ULong j = 0; j < campaign_info.creatives.length(); j++)
      {
        const CreativeInfo& creative_info = campaign_info.creatives[j];

        try
        {
          Creative_var cr(adapt_creative_info_(campaign, creative_info));
          campaign->add_creative(cr.in());

          OptionValueMap option_values;
          unpack_option_value_map(option_values, creative_info.tokens);
          // TODO: remove from protocol as separate field when will be changed CampaignServer interface
          if(creative_info.html_url.option_id)
          {
            option_values.insert(
              std::make_pair(creative_info.html_url.option_id, creative_info.html_url.value.in()));
          }
          config_update_links.creative_option_values[
            creative_info.ccid].swap(option_values);

          for(CORBA::ULong size_i = 0; size_i < creative_info.sizes.length(); ++size_i)
          {
            OptionValueMap size_option_values;
            unpack_option_value_map(size_option_values, creative_info.sizes[size_i].tokens);
            config_update_links.creative_size_option_values[
              creative_info.ccid][
                creative_info.sizes[size_i].size_id].swap(size_option_values);
          }

          new_config.campaign_creatives.insert(
            std::make_pair(cr->ccid, cr));

          Creative_var& tcr = new_config.creatives[creative_info.creative_id];
          if(!tcr.in() || cr->ccid < tcr->ccid)
          {
            tcr = cr;
          }
        }
        catch(const Exception& ex)
        {
          logger_->log(
            String::SubString(ex.what()),
            Logging::Logger::NOTICE,
            Aspect::TRAFFICKING_PROBLEM);

          continue;
        }
      }

      /*
      // fill contracts
      for(CORBA::ULong contract_i = 0; contract_i < campaign_info.contracts.length(); ++contract_i)
      {
        const CampaignContractInfo& contract = campaign_info.contracts[contract_i];
        CampaignContract_var new_contract(new CampaignContract());
        new_contract->ord_contract_id = contract.ord_contract_id;
        new_contract->ord_ado_id = contract.ord_ado_id;
        new_contract->id = contract.id;
        new_contract->date = contract.date;
        new_contract->type = contract.type;
        new_contract->client_id = contract.client_id;
        new_contract->client_name = contract.client_name;
        new_contract->contractor_id = contract.contractor_id;
        new_contract->contractor_name = contract.contractor_name;
        campaign->contracts.emplace_back(new_contract);
      }
      */

      new_config.campaigns.insert(
        std::make_pair(campaign_info.campaign_id, campaign));
    }
  }

  void CampaignConfigSource::apply_contract_update_(
    CampaignConfig& new_config,
    const CampaignConfigUpdateInfo& update_info,
    ConfigUpdateLinks& config_update_links)
    noexcept
  {
    for (CORBA::ULong i = 0; i < update_info.contracts.length(); i++)
    {
      const ContractInfo& contract_info = update_info.contracts[i];
      Contract_var contract(new Contract());
      contract->contract_id = contract_info.contract_id;

      contract->number = contract_info.number;
      contract->date = contract_info.date;
      contract->type = contract_info.type;
      contract->vat_included = contract_info.vat_included;

      contract->ord_contract_id = contract_info.ord_contract_id;
      contract->ord_ado_id = contract_info.ord_ado_id;
      contract->subject_type = contract_info.subject_type;
      contract->action_type = contract_info.action_type;
      contract->agent_acting_for_publisher = contract_info.agent_acting_for_publisher;

      contract->client_id = contract_info.client_id;
      contract->client_name = contract_info.client_name;
      contract->client_legal_form = contract_info.client_legal_form;

      contract->contractor_id = contract_info.contractor_id;
      contract->contractor_name = contract_info.contractor_name;
      contract->contractor_legal_form = contract_info.contractor_legal_form;

      new_config.contracts.emplace(contract_info.contract_id, contract);

      if(contract_info.parent_contract_id)
      {
        config_update_links.contract_parent_contracts.emplace(
          contract_info.contract_id, contract_info.parent_contract_id);
      }
    }
  }

  void CampaignConfigSource::apply_expression_channels_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.expression_channels.length(); ++i)
    {
      ExpressionChannelBase_var new_channel = unpack_channel(
        update_info.expression_channels[i],
        new_config.expression_channels);

      CampaignConfig::ChannelMap::iterator ch_it = new_config.expression_channels.find(
        update_info.expression_channels[i].channel_id);

      ExpressionChannelHolder_var channel_holder;

      if(ch_it == new_config.expression_channels.end())
      {
        channel_holder = new ExpressionChannelHolder(new_channel);

        new_config.expression_channels.insert(
          std::make_pair(update_info.expression_channels[i].channel_id,
            channel_holder));
      }
      else
      {
        ch_it->second->channel = new_channel;
        channel_holder = ch_it->second;
      }

      if(new_channel->params().type == 'V')
      {
        config_update_links.platform_channels.insert(
          std::make_pair(update_info.expression_channels[i].channel_id,
            channel_holder));
      }

      if(new_channel->params().discover_params.in())
      {
        new_config.discover_channels.insert(
          std::make_pair(update_info.expression_channels[i].channel_id,
            channel_holder));
      }
    }
  }

  void CampaignConfigSource::apply_sites_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.sites.length(); i++)
    {
      const SiteInfo& site_info = update_info.sites[i];

      config_update_links.site_accounts.insert(
        std::make_pair(site_info.site_id, site_info.account_id));

      Site_var p_site = new Site();

      p_site->site_id = site_info.site_id;
      p_site->timestamp = CorbaAlgs::unpack_time(site_info.timestamp);
      p_site->freq_cap_id = site_info.freq_cap_id;
      p_site->noads_timeout = site_info.noads_timeout;
      p_site->status = site_info.status;
      p_site->flags = site_info.flags;

      /* fill accepted & rejected creative category  */
      CorbaAlgs::convert_sequence(
        site_info.approved_creative_categories,
        p_site->approved_creative_categories);

      CorbaAlgs::convert_sequence(
        site_info.rejected_creative_categories,
        p_site->rejected_creative_categories);

      CorbaAlgs::convert_sequence(
        site_info.approved_creatives,
        p_site->approved_creatives);

      CorbaAlgs::convert_sequence(
        site_info.rejected_creatives,
        p_site->rejected_creatives);

      new_config.sites[p_site->site_id] = p_site;
    }
  }

  void CampaignConfigSource::apply_tags_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.tags.length(); i++)
    {
      const TagInfo& tag_info = update_info.tags[i];

      config_update_links.tag_sites.insert(
        std::make_pair(tag_info.tag_id, tag_info.site_id));

      Tag_var p_tag = new Tag();

      p_tag->tag_id = tag_info.tag_id;
      p_tag->tag_pricings_timestamp = CorbaAlgs::unpack_time(
        tag_info.tag_pricings_timestamp);
      p_tag->timestamp = CorbaAlgs::unpack_time(tag_info.timestamp);

      for(CORBA::ULong size_i = 0; size_i < tag_info.sizes.length(); ++size_i)
      {
        Tag::Size_var res_size = new Tag::Size();
        res_size->max_text_creatives = tag_info.sizes[size_i].max_text_creatives;
        unpack_option_value_map(
          config_update_links.tag_size_option_values[
            p_tag->tag_id][tag_info.sizes[size_i].size_id],
          tag_info.sizes[size_i].tokens);
        unpack_option_value_map(
          config_update_links.tag_size_option_hidden_values[
            p_tag->tag_id][tag_info.sizes[size_i].size_id],
          tag_info.sizes[size_i].hidden_tokens);
        p_tag->sizes.insert(std::make_pair(
          tag_info.sizes[size_i].size_id, res_size));
      }

      p_tag->flags = tag_info.flags;
      p_tag->marketplace = tag_info.marketplace;
      p_tag->imp_track_pixel = tag_info.imp_track_pixel;

      p_tag->passback = tag_info.passback;
      p_tag->passback_type = tag_info.passback_type;

      p_tag->adjustment = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.adjustment);

      p_tag->cost_coef = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.cost_coef);
      p_tag->skip_min_ecpm = false;

      // fill tag pricings
      bool default_tag_pricing_found = false;
      const TagPricingInfoSeq& tag_pricing_list = tag_info.tag_pricings;

      for (unsigned int j = 0; j < tag_pricing_list.length(); j++)
      {
        const TagPricingInfo& tag_pricing_ref = tag_pricing_list[j];
        Tag::TagPricing tag_pricing;
        tag_pricing.site_rate_id = tag_pricing_ref.site_rate_id;
        tag_pricing.revenue_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          tag_pricing_ref.revenue_share);
        tag_pricing.imp_revenue = RevenueDecimal::div(
          CorbaAlgs::unpack_decimal<RevenueDecimal>(
            tag_pricing_ref.imp_revenue),
          REVENUE_ONE + p_tag->cost_coef,
          Generics::DDR_CEIL);

        if(tag_pricing_ref.country_code[0] == 0)
        {
          default_tag_pricing_found = true;
        }
        
        p_tag->tag_pricings.insert(
          std::make_pair(
            Tag::TagPricingKey(
              tag_pricing_ref.country_code,
              static_cast<CCGType>(tag_pricing_ref.ccg_type),
              static_cast<CCGRateType>(tag_pricing_ref.ccg_rate_type)),
            tag_pricing));
      }
      
      if(!p_tag->tag_pricings.empty() && !default_tag_pricing_found)
      {
        p_tag->tag_pricings.insert(
          std::make_pair(
            Tag::TagPricingKey("", CT_ALL, CR_ALL), Tag::TagPricing()));
      }

      p_tag->accepted_categories.insert(
        tag_info.accepted_categories.get_buffer(),
        tag_info.accepted_categories.get_buffer() +
        tag_info.accepted_categories.length());

      p_tag->rejected_categories.insert(
        tag_info.rejected_categories.get_buffer(),
        tag_info.rejected_categories.get_buffer() +
        tag_info.rejected_categories.length());

      p_tag->allow_expandable = tag_info.allow_expandable;

      p_tag->min_visibility = 0;

      p_tag->auction_max_ecpm_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.auction_max_ecpm_share);
      p_tag->auction_prop_probability_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.auction_prop_probability_share);
      p_tag->auction_random_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.auction_random_share);

      OptionValueMap option_values;
      unpack_option_value_map(option_values, tag_info.tokens);

      OptionValueMap hidden_option_values;
      unpack_option_value_map(hidden_option_values, tag_info.hidden_tokens);

      //std::cerr << tag_info.tag_id << " = " <<
      //  tag_info.passback_tokens.length() << std::endl;

      OptionValueMap passback_option_values;
      unpack_option_value_map(passback_option_values, tag_info.passback_tokens);

      // fill min_visibility by specific option (2.5)
      OptionValueMap::const_iterator min_visibility_option_it =
        option_values.find(TAG_MIN_VISIBILITY_OPTION_ID);
      if(min_visibility_option_it != option_values.end())
      {
        String::StringManip::str_to_int(
          min_visibility_option_it->second,
          p_tag->min_visibility);
      }

      // fill max_random_cpm by specific option (3.1)
      OptionValueMap::const_iterator max_random_cpm_option_it =
        option_values.find(MAX_RANDOM_CPM_OPTION_ID);
      if(max_random_cpm_option_it != option_values.end())
      {
        p_tag->pub_max_random_cpm = RevenueDecimal(
          max_random_cpm_option_it->second);
      }
      else
      {
        p_tag->pub_max_random_cpm = RevenueDecimal::ZERO;
      }

      // fill skip_min_ecpm by specific option (3.1)
      OptionValueMap::const_iterator skip_min_ecpm_option_it =
        option_values.find(SKIP_MIN_ECPM_OPTION_ID);
      if(skip_min_ecpm_option_it != option_values.end())
      {
        int v;
        String::StringManip::str_to_int(skip_min_ecpm_option_it->second, v);
        p_tag->skip_min_ecpm = (v != 0);
      }
      else
      {
        p_tag->skip_min_ecpm = false;
      }

      p_tag->max_random_cpm = RevenueDecimal::ZERO;

      ConfigUpdateLinks::TemplateOptionValueMap template_option_values;
      for(CORBA::ULong templ_i = 0;
          templ_i < tag_info.template_tokens.length(); ++templ_i)
      {
        unpack_option_value_map(
          template_option_values[tag_info.template_tokens[templ_i].template_name.in()],
          tag_info.template_tokens[templ_i].tokens);
      }

      new_config.tags[p_tag->tag_id] = p_tag;

      config_update_links.tag_option_values[
        p_tag->tag_id].swap(option_values);
      config_update_links.tag_hidden_option_values[
        p_tag->tag_id].swap(hidden_option_values);
      config_update_links.tag_passback_option_values[
        p_tag->tag_id].swap(passback_option_values);
      config_update_links.tag_template_option_values[
        p_tag->tag_id].swap(template_option_values);
    }
  }

  void
  CampaignConfigSource::apply_ccg_keyword_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& /*config_update_links*/)
    /*throw(Exception, eh::Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.campaign_keywords.length(); ++i)
    {
      CampaignKeywordBase ccg_kw;
      ccg_kw.ccg_keyword_id = update_info.campaign_keywords[i].ccg_keyword_id;
      ccg_kw.click_url = update_info.campaign_keywords[i].click_url;
      ccg_kw.original_keyword = update_info.campaign_keywords[i].original_keyword;

      new_config.ccg_keyword_click_info_map.insert(
        std::make_pair(update_info.campaign_keywords[i].ccg_keyword_id, ccg_kw));
    }
  }

  void CampaignConfigSource::apply_colocations_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.colocations.length(); ++i)
    {
      const AdServer::CampaignSvcs::ColocationInfo& colo_info =
        update_info.colocations[i];

      config_update_links.colocation_accounts.insert(
        std::make_pair(colo_info.colo_id, colo_info.account_id));

      Colocation_var colo = new Colocation();
      colo->colo_id = colo_info.colo_id;
      colo->colo_rate_id = colo_info.colo_rate_id;
      colo->colo_name = colo_info.colo_name;
      colo->at_flags = colo_info.at_flags;
      colo->revenue_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(colo_info.revenue_share);
      colo->ad_serving = static_cast<ColocationAdServingType>(colo_info.ad_serving);
      colo->hid_profile = colo_info.hid_profile;
      colo->timestamp = CorbaAlgs::unpack_time(colo_info.timestamp);

      new_config.colocations.insert(std::make_pair(colo_info.colo_id, colo));

      OptionValueMap option_values;
      unpack_option_value_map(option_values, colo_info.tokens);
      config_update_links.colocation_option_values[
        colo_info.colo_id].swap(option_values);
    }
  }

  void CampaignConfigSource::apply_countries_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.countries.length(); ++i)
    {
      const AdServer::CampaignSvcs::CountryInfo& country_info =
        update_info.countries[i];

      Country_var country = new Country();
      country->timestamp = CorbaAlgs::unpack_time(country_info.timestamp);

      new_config.countries.insert(std::make_pair(country_info.country_code.in(), country));

      OptionValueMap option_values;
      unpack_option_value_map(option_values, country_info.tokens);
      config_update_links.country_option_values[
        country_info.country_code.in()].swap(option_values);
    }
  }


  void CampaignConfigSource::apply_category_channels_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception, eh::Exception)*/
  {
    for (CORBA::ULong i = 0; i < update_info.category_channels.length(); i++)
    {
      const CategoryChannelInfo& cc_info = update_info.category_channels[i];
      CategoryChannel_var category_channel(new CategoryChannel());
      category_channel->channel_id = cc_info.channel_id;
      category_channel->name = cc_info.name.in();
      category_channel->newsgate_name = cc_info.newsgate_name.in();
      category_channel->flags = cc_info.flags;
      category_channel->parent_channel_id = cc_info.parent_channel_id;

      for(CORBA::ULong loc_i = 0; loc_i < cc_info.localizations.length(); ++loc_i)
      {
        category_channel->localizations.insert(
          std::make_pair(
            cc_info.localizations[loc_i].language.in(),
            cc_info.localizations[loc_i].name.in()));
      }
      category_channel->timestamp = CorbaAlgs::unpack_time(cc_info.timestamp);
      new_config.category_channels.insert(
        std::make_pair(cc_info.channel_id, category_channel));
    }
  }

  void CampaignConfigSource::apply_simple_channels_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception, eh::Exception)*/
  {
    const SimpleChannelKeySeq& updated_simple_channels =
      update_info.simple_channels;

    for (CORBA::ULong i = 0; i < updated_simple_channels.length(); i++)
    {
      SimpleChannelCategories_var simple_channel(new SimpleChannelCategories);
      CorbaAlgs::convert_sequence(
        updated_simple_channels[i].categories,
        simple_channel->categories);

      new_config.simple_channels[
        updated_simple_channels[i].channel_id] = simple_channel;
    }
  }

  void
  CampaignConfigSource::apply_geo_channel_update_(
    CampaignConfig& new_config,
    const CampaignConfigUpdateInfo& update_info,
    const CampaignConfig* old_config)
    noexcept
  {
    Generics::Time geo_channels_timestamp = CorbaAlgs::unpack_time(
      update_info.geo_channels_timestamp);

    if(!old_config ||
       geo_channels_timestamp > old_config->geo_channels_timestamp)
    {
      if(!new_config.geo_channels.in())
      {
        // first portion
        new_config.geo_channels = new GeoChannelIndex();
        new_config.geo_channels_timestamp = geo_channels_timestamp;
      }
      else
      {
        if(old_config && new_config.geo_channels == old_config->geo_channels)
        {
          // few apply iterations can return equal timestamp
          new_config.geo_channels = new GeoChannelIndex(*old_config->geo_channels);
          new_config.geo_channels_timestamp = std::min(
            old_config->geo_channels_timestamp, geo_channels_timestamp);
        }
      }

      for (CORBA::ULong i = 0; i < update_info.activate_geo_channels.length(); i++)
      {
        const GeoChannelInfo& geo_channel_info = update_info.activate_geo_channels[i];

        if(geo_channel_info.geoip_targets.length())
        {
          for(CORBA::ULong geoip_target_i = 0;
              geoip_target_i < geo_channel_info.geoip_targets.length();
              ++geoip_target_i)
          {
            new_config.geo_channels->add(
              String::SubString(geo_channel_info.country),
              String::SubString(geo_channel_info.geoip_targets[geoip_target_i].region),
              String::SubString(geo_channel_info.geoip_targets[geoip_target_i].city),
              geo_channel_info.channel_id);
          }
        }
        else
        {
          new_config.geo_channels->add(
            String::SubString(geo_channel_info.country),
            String::SubString(),
            String::SubString(),
            geo_channel_info.channel_id);
        }
      }
    }
    else
    {
      new_config.geo_channels_timestamp =
        old_config->geo_channels_timestamp;
      new_config.geo_channels = old_config->geo_channels;
    }

    if(!new_config.geo_coord_channels)
    {
      new_config.geo_coord_channels = new GeoCoordChannelIndex();
    }

    for(CORBA::ULong geo_channel_i = 0;
        geo_channel_i < update_info.activate_geo_coord_channels.length();
        ++geo_channel_i)
    {
      const AdServer::CampaignSvcs::GeoCoordChannelInfo& coord_channel_info =
        update_info.activate_geo_coord_channels[geo_channel_i];

      new_config.geo_coord_channels->add(
        GeoCoordChannelIndex::Key(
          CorbaAlgs::unpack_decimal<CoordDecimal>(
            coord_channel_info.longitude),
          CorbaAlgs::unpack_decimal<CoordDecimal>(
            coord_channel_info.latitude),
          CorbaAlgs::unpack_decimal<AccuracyDecimal>(
            coord_channel_info.radius)),
        coord_channel_info.channel_id);
    }
  }

  void
  CampaignConfigSource::apply_platform_update_(
    CampaignConfig& new_config,
    const CampaignConfigUpdateInfo& update_info)
    noexcept
  {
    for(CORBA::ULong i = 0; i < update_info.platforms.length(); ++i)
    {
      std::string platform_name = update_info.platforms[i].name.in();
      String::AsciiStringManip::to_lower(platform_name);
      new_config.platforms.insert(
        std::make_pair(
          update_info.platforms[i].platform_id,
          platform_name));
    }
  }

  void
  CampaignConfigSource::apply_web_app_update_(
    CampaignConfig& new_config,
    const CampaignConfigUpdateInfo& update_info)
    noexcept
  {
    for(CORBA::ULong i = 0; i < update_info.web_operations.length(); ++i)
    {
      const WebOperationInfo& op = update_info.web_operations[i];
      WebOperation_var opval = new WebOperation;
      opval->id = op.id;
      opval->app = op.app;
      opval->source = op.source;
      opval->operation = op.operation;
      opval->flags = op.flags;
      new_config.web_operations[WebOperationKey(
        opval->app.c_str(), opval->source.c_str(), opval->operation.c_str())] =
        opval;
    }
  }

  void
  CampaignConfigSource::apply_block_channel_update_(
    const CampaignConfigUpdateInfo& update_info,
    ConfigUpdateLinks& config_update_links)
    /*throw(Exception, eh::Exception)*/
  {
    for(CORBA::ULong block_channel_i = 0;
        block_channel_i < update_info.activate_block_channels.length();
        ++block_channel_i)
    {
      config_update_links.block_channels[
        update_info.activate_block_channels[block_channel_i].size_id].push_back(
          update_info.activate_block_channels[block_channel_i].channel_id);
    }
  }

  bool
  CampaignConfigSource::link_option_values_(
    OptionTokenValueMap& option_token_values,
    const CampaignConfig& campaign_config,
    const OptionValueMap& option_values)
  {
    for(OptionValueMap::const_iterator opt_val_it =
          option_values.begin();
        opt_val_it != option_values.end(); ++opt_val_it)
    {
      CreativeOptionMap::const_iterator opt_it =
        campaign_config.creative_options.find(opt_val_it->first);

      if(opt_it != campaign_config.creative_options.end())
      {
        option_token_values.insert(std::make_pair(
          opt_it->second.token,
          OptionValue(
            opt_val_it->first,
            opt_val_it->second.c_str())));
      }
      else
      {
        // option traits isn't loaded or has inactive state
        return false;
      }
    }

    return true;
  }

  bool
  CampaignConfigSource::link_option_values_(
    OptionTokenValueMap& option_token_values,
    const CampaignConfig& campaign_config,
    const ConfigUpdateLinks::IdOptionValueMap& option_values,
    unsigned long id)
  {
    ConfigUpdateLinks::IdOptionValueMap::
      const_iterator options_it = option_values.find(id);

    if(options_it != option_values.end())
    {
      return link_option_values_(
        option_token_values,
        campaign_config,
        options_it->second);
    }

    return true;
  }

  void CampaignConfigSource::apply_campaign_limitations_(
    CampaignConfig& config,
    const Generics::Time& now)
    noexcept
  {
    for(CampaignConfig::CampaignMap::iterator camp_it =
          config.campaigns.begin();
        camp_it != config.campaigns.end();
        ++camp_it)
    {
      Campaign* cmp = camp_it->second;

      if((cmp->campaign_delivery_limits.date_start != Generics::Time::ZERO &&
           now < cmp->campaign_delivery_limits.date_start) ||
         (cmp->campaign_delivery_limits.date_end != Generics::Time::ZERO &&
           now > cmp->campaign_delivery_limits.date_end) ||
         (cmp->ccg_delivery_limits.date_start != Generics::Time::ZERO &&
           now < cmp->ccg_delivery_limits.date_start) ||
         (cmp->ccg_delivery_limits.date_end != Generics::Time::ZERO &&
           now > cmp->ccg_delivery_limits.date_end))
      {
        cmp->eval_status = 'I';
      }
    }
  }

  void CampaignConfigSource::check_creative_files_option_(
    CampaignConfig& new_config) noexcept
  {
    /*
    std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
      ": check_creative_files_option_ enter" <<
      std::endl;
    */

    for(CampaignConfig::CampaignMap::const_iterator camp_it =
          new_config.campaigns.begin();
        camp_it != new_config.campaigns.end();
        ++camp_it)
    {
      std::string path_to_file;
      const CreativeList& creatives = camp_it->second->creatives;
      for(CreativeList::const_iterator creat_it = creatives.begin();
          creat_it != creatives.end();
          ++creat_it)
      {
        Creative* cur_creative = *creat_it;
        bool deactivate = false;

        for(OptionTokenValueMap::const_iterator token_it =
              cur_creative->tokens.begin();
            token_it != cur_creative->tokens.end();
            ++token_it)
        {
          CreativeOptionMap::const_iterator creat_opt_it =
            new_config.creative_options.find(token_it->second.option_id);

          assert(creat_opt_it != new_config.creative_options.end());

          if(!token_it->second.value.empty() &&
              (creat_opt_it->second.type == 'F' ||
               ((creat_opt_it->second.type == 'D' ||
                 creat_opt_it->second.type == 'U') &&
                token_it->second.value.compare(0, 7, "http://") &&
                token_it->second.value.compare(0, 8, "https://") &&
                token_it->second.value.compare(0, 2, "//"))))
          {
            adapt_creative_file_path(token_it->second.value.c_str(), path_to_file);

            if(!file_access_manager_.get(path_to_file.c_str()))
            {
              /*
              std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
                ": file '" << path_to_file << "' not found" <<
                std::endl;
              */
              deactivate = true;
              break;
            }
          }
        }

        if(deactivate)
        {
          cur_creative->status = 'W';
        }
      }
    }

    /*
    std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
      ": check_creative_files_option_ exit" <<
      std::endl;
    */
  }

  void CampaignConfigSource::preinstantiate_creative_tokens_(
    CampaignConfig& new_config) noexcept
  {
    static const char* FUN =
      "CampaignConfigSource::preinstantiate_creative_tokens_()";

    try
    {
      CreativeInstantiateRule rule(false);
      for(CampaignConfig::CampaignMap::iterator camp_it =
            new_config.campaigns.begin();
          camp_it != new_config.campaigns.end();
          ++camp_it)
      {
        if(camp_it->second->is_active())
        {
          TokenValueMap request_args_clickurl;
          CreativeList& creatives = camp_it->second->creatives;
          fill_clickurl_tokens_(*camp_it->second, request_args_clickurl);

          for(CreativeList::iterator creat_it = creatives.begin();
              creat_it != creatives.end();
              ++creat_it)
          {
            Creative* cur_creative = *creat_it;
            TokenValueMap request_args;
            fill_default_tokens_(*camp_it->second, cur_creative, request_args);

            if(cur_creative->status == 'A')
            {
              /*
               Try to init system tokens CRCLICK.
               Note only limited list of tokens are used for CRCLICK instantiation.
               This list of tokens are defined in method CampaignConfigSource::fill_clickurl_tokens_().
               Don't shuffle this CRCLICK instantiation with other instantiations. */
              try
              {
                OptionTokenValueMap args = {{
                  CreativeTokens::ADV_CLICK_URL, cur_creative->click_url }};
                TokenValueMap result_args;
                CreativeTextGenerator::init_creative_tokens(
                  rule,
                  CreativeInstantiateArgs(),
                  new_config.token_processors,
                  request_args_clickurl,
                  args,
                  result_args);
              }
              catch (const eh::Exception& e)
              {
                cur_creative->status = 'C';

                logger_->sstream(Logging::Logger::ERROR,
                  Aspect::TRAFFICKING_PROBLEM,
                  "ADS-TF-1004") << __func__ <<
                  ": Can't preinstantiate click-url for creative ccid = " <<
                  cur_creative->ccid << "(ccg_id = " <<
                  camp_it->first << ")" <<
                  " : " << e.what();

                continue;
              }

              Creative::SizeMap::iterator size_it = cur_creative->sizes.begin();

              while (size_it != cur_creative->sizes.end())
              {
                OptionTokenValueMap args = cur_creative->tokens;
                const Creative::Size& size = size_it->second;

                args.insert(size.tokens.begin(), size.tokens.end());
                args[CreativeTokens::ADV_CLICK_URL] = cur_creative->click_url;

                /*
                {
                  Stream::Error ostr;
                  ostr << "TOKENS:" << std::endl;
                  for(auto it = args.begin(); it != args.end(); ++it)
                  {
                    ostr << "  '" << it->first << "' : '" << it->second.value << "'" << std::endl;
                  }

                  logger_->log(ostr.str(), Logging::Logger::ERROR,
                    Aspect::CAMPAIGN_CONFIG_SOURCE, "ADS-?");
                }
                */
                
                try
                {
                  TokenValueMap result_args;
                  CreativeTextGenerator::init_creative_tokens(
                    rule,
                    CreativeInstantiateArgs(),
                    new_config.token_processors,
                    request_args,
                    args,
                    result_args);

                  ++size_it;
                }
                catch(const eh::Exception& e)
                {
                  unsigned long size_id = size_it->first;
                  cur_creative->sizes.erase(size_it++);
                  if (cur_creative->sizes.empty())
                  {
                    cur_creative->status = 'C';
                  }

                  logger_->sstream(Logging::Logger::ERROR,
                    Aspect::TRAFFICKING_PROBLEM,
                    "ADS-TF-1004") << __func__ <<
                    ": Can't preinstantiate creative ccid = " <<
                    cur_creative->ccid << "(ccg_id = " <<
                    camp_it->first << ") for size_id = " << size_id <<
                    " : " << e.what();
                }
              } // while
            }
          }
        }
      }
    }
    catch(const eh::Exception& e)
    {
      logger_->sstream(Logging::Logger::ERROR,
        Aspect::CAMPAIGN_CONFIG_SOURCE,
        "ADS-IMPL-188") << FUN <<
        ": caught eh::Exception on preinstantiate creatives: " << e.what();
    }
  }

  void CampaignConfigSource::check_creative_template_files_(
    CampaignConfig& new_config) noexcept
  {
    /*
    std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
      ": check_creative_template_files_ enter" <<
      std::endl;
    */

    for(CreativeTemplateMap::iterator it =
          new_config.creative_templates.begin();
        it != new_config.creative_templates.end(); ++it)
    {
      if(file_access_manager_.get(it->second.file.c_str()))
      {
        it->second.status = 'A';
      }
      else
      {
        /*
        std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
          ": file '" << it->second.file << "' not found" <<
          std::endl;
        */

        it->second.status = 'W';
      }
    }

    /*
    std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
      ": check_creative_template_files_ exit" <<
      std::endl;
    */
  }

  void
  CampaignConfigSource::fill_default_tokens_(
    const Campaign& campaign,
    const Creative* creative,
    TokenValueMap& system_tokens)
    /*throw(eh::Exception)*/
  {
    // generic
    system_tokens[CreativeTokens::RANDOM] = "1";
    system_tokens[CreativeTokens::PP] = "";
    system_tokens[CreativeTokens::EXTDATA] = "";
    system_tokens[CreativeTokens::VIDEOW] = "";
    system_tokens[CreativeTokens::VIDEOH] = "";
    system_tokens[CreativeTokens::WIDTH] = "0";
    system_tokens[CreativeTokens::HEIGHT] = "0";
    system_tokens[CreativeTokens::TAGWIDTH] = "0";
    system_tokens[CreativeTokens::TAGHEIGHT] = "0";
    system_tokens[CreativeTokens::AD_SERVER] = "";
    system_tokens[CreativeTokens::ADIMAGE_SERVER] = "";
    system_tokens[CreativeTokens::APP_FORMAT] = "";
    system_tokens[CreativeTokens::CREATIVE_SIZE] = "";
    system_tokens[CreativeTokens::TAGSIZE] = "";
    system_tokens[CreativeTokens::TEST_REQUEST] = "0";
    system_tokens[CreativeTokens::CRVSERVER] = "";

    // advertiser
    system_tokens[CreativeTokens::ADVERTISER_ID] = "0";
    system_tokens[CreativeTokens::CID] = "0";
    system_tokens[CreativeTokens::CCID] = "0";
    system_tokens[CreativeTokens::CGID] = "0";
    system_tokens[CreativeTokens::ADIMAGE_PATH] = "";
    system_tokens[CreativeTokens::TEMPLATE_FORMAT] = "";
    system_tokens[CreativeTokens::CRVBASE] = "";

    if (!creative->click_url.value.empty())
    {
      system_tokens[CreativeTokens::CLICKURL] = "";
      system_tokens[CreativeTokens::CLICK0] = "";
    }

    system_tokens[CreativeTokens::PRECLICKURL] = "";
    system_tokens[CreativeTokens::PRECLICK0] = "";

    if(campaign.keyword_based())
    {
      // define KEYWORD for keyword based campaign
      system_tokens[CreativeTokens::KEYWORD] = "";
    }

    // publisher
    system_tokens[CreativeTokens::PUBLISHER_ID] = "0";
    system_tokens[CreativeTokens::SITE_ID] = "0";
    system_tokens[CreativeTokens::TAGID] = "0";
    system_tokens[CreativeTokens::PASSBACK_TYPE] = "";
    system_tokens[CreativeTokens::PASSBACK_PIXEL] = "";
    system_tokens[CreativeTokens::PASSBACK_CODE] = "";
    system_tokens[CreativeTokens::PASSBACK_URL] = "";
    system_tokens[CreativeTokens::EXT_TRACK_PARAMS] = "";
    system_tokens[CreativeTokens::APPLICATION_ID] = "";
    system_tokens[CreativeTokens::ADVERTISING_ID] = "";
    system_tokens[CreativeTokens::IDFA] = "";
    system_tokens[CreativeTokens::TNS_COUNTER_DEVICE_TYPE] = "";
    system_tokens["BS_ID"] = "";
    system_tokens["BR_ID"] = "";
    system_tokens["BP_ID"] = "";

    // internal
    system_tokens[CreativeTokens::USER_BIND] = "";
    system_tokens[CreativeTokens::REFERER] = "";
    system_tokens[CreativeTokens::ETID] = "";
    system_tokens[CreativeTokens::UID] = "";
    system_tokens[CreativeTokens::ORIGLINK] = "";
    system_tokens[CreativeTokens::PUBPIXELS] = "";
    system_tokens[CreativeTokens::PUB_PIXELS_OPTIN] = "";
    system_tokens[CreativeTokens::PUB_PIXELS_OPTOUT] = "";
    system_tokens[CreativeTokens::USER_STATUS] = "";
    system_tokens[CreativeTokens::REQUEST_TOKEN] = "";
    system_tokens[CreativeTokens::COLOCATION] = "";
    system_tokens[CreativeTokens::COHORT] = "";
    system_tokens[CreativeTokens::TRACKPIXEL] = "";
    system_tokens[CreativeTokens::REQUESTID] = "";
    system_tokens[CreativeTokens::GREQUESTID] = "";

    // other
    system_tokens[CreativeTokens::REFERER_KW_MATCH] = "";
    system_tokens[CreativeTokens::CONTEXT_KW_MATCH] = "";
    system_tokens[CreativeTokens::SEARCH_TR_MATCH] = "";
    system_tokens[CreativeTokens::REFERER_KW] = "";
    system_tokens[CreativeTokens::CREATIVE_SIZE] = "";
    system_tokens[CreativeTokens::PUBPRECLICK] = "";
  }

  void
  CampaignConfigSource::fill_clickurl_tokens_(
    const Campaign& campaign,
    TokenValueMap& system_tokens)
    /*throw(eh::Exception)*/
  {
    // generic
    system_tokens[CreativeTokens::RANDOM] = "1";
    
    // advertiser
    system_tokens[CreativeTokens::ADVERTISER_ID] = "0";
    system_tokens[CreativeTokens::CID] = "0";
    system_tokens[CreativeTokens::CCID] = "0";
    system_tokens[CreativeTokens::CGID] = "0";

    if(campaign.keyword_based())
    {
      // define KEYWORD for keyword based campaign
      system_tokens[CreativeTokens::KEYWORD] = "";
    }

    // publisher
    system_tokens[CreativeTokens::PUBLISHER_ID] = "0";
    system_tokens[CreativeTokens::TAGID] = "0";
    system_tokens[CreativeTokens::SITE_ID] = "0";
    system_tokens[CreativeTokens::EXT_TRACK_PARAMS] = "";
    system_tokens[CreativeTokens::APPLICATION_ID] = "";
    system_tokens[CreativeTokens::ADVERTISING_ID] = "";
    system_tokens[CreativeTokens::IDFA] = "";

    // internal
    system_tokens[CreativeTokens::COLOCATION] = "";

    // other
    system_tokens[CreativeTokens::CREATIVE_SIZE] = "";
  }
}
}
