#include <Commons/CorbaAlgs.hpp>
#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>

#include "NonLinkedExpressionChannelCorbaAdapter.hpp"
#include "CampaignConfigServerSource.hpp"

namespace
{
  namespace Aspect
  {
    const char CAMPAIGN_CONFIG_SERVER_SOURCE[] = "CampaignConfigServerSource";
  }
}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    template<typename CollectionType, typename ResultCollectionType>
    Generics::Time deactivate_sequence(
      CollectionType& out,
      const ResultCollectionType& in)
    {
      Generics::Time res;

      for(CORBA::ULong i = 0; i < in.length(); ++i)
      {
        Generics::Time ts = CorbaAlgs::unpack_time(in[i].timestamp);
        out.deactivate(CorbaAlgs::convert_type_adapter(in[i].id), ts);
        res = std::max(res, ts);
      }

      return res;
    }

    SimpleChannelDef_var unpack_simple_channel(
      const AdServer::CampaignSvcs::SimpleChannelKey& ch_inf)
      noexcept
    {
      SimpleChannelDef_var result(new SimpleChannelDef);
      result->channel_id = ch_inf.channel_id;
      result->status = ch_inf.status;
      result->country = ch_inf.country_code;
      result->behav_param_list_id = ch_inf.behav_param_list_id;
      result->str_behav_param_list_id = ch_inf.str_behav_param_list_id;
      result->threshold = ch_inf.threshold;

      if(ch_inf.page_triggers.length() ||
         ch_inf.search_triggers.length() ||
         ch_inf.url_triggers.length() ||
         ch_inf.url_keyword_triggers.length())
      {
        SimpleChannelDef::MatchParams_var match_params(
          new SimpleChannelDef::MatchParams());
        CorbaAlgs::convert_sequence(
          ch_inf.page_triggers, match_params->page_triggers);
        CorbaAlgs::convert_sequence(
          ch_inf.search_triggers, match_params->search_triggers);
        CorbaAlgs::convert_sequence(
          ch_inf.url_triggers, match_params->url_triggers);
        CorbaAlgs::convert_sequence(
          ch_inf.url_keyword_triggers, match_params->url_keyword_triggers);
        result->match_params = match_params;
      }

      CorbaAlgs::convert_sequence(ch_inf.categories, result->categories);
      result->timestamp = CorbaAlgs::unpack_time(ch_inf.timestamp);
      return result;
    }
  }

  CampaignConfigServerSource::CampaignConfigServerSource(
    Logging::Logger* logger,
    unsigned long server_id,
    const CORBACommons::CorbaObjectRefList& campaign_server_refs,
    unsigned long colo_id,
    const char* version,
    const char* campaign_statuses,
    const String::SubString& channel_statuses,
    const char* country,
    bool only_tags)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id),
      version_(version),
      campaign_statuses_(campaign_statuses),
      channel_statuses_(channel_statuses.str()),
      country_(country),
      only_tags_(only_tags),
      server_id_(server_id)
  {
    static const char* FUN = "CampaignConfigServerSource::CampaignConfigServerSource()";

    try
    {
      /* init CORBA client adapter */
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

      campaign_servers_.reset(new CampaignServerPool(
        campaign_server_refs,
        corba_client_adapter_,
        CORBACommons::ChoosePolicyType::PT_PERSISTENT,
        Generics::Time(10) // timeout
        ));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  CampaignConfig_var
  CampaignConfigServerSource::update(bool* need_logging)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::update()";

    try
    {
      CampaignConfig_var config = campaign_config_;

      if (config.in() == 0)
      {
        config = new CampaignConfig();
      }

      CampaignGetConfigSettings get_config_settings;
      init_get_config_settings_(*config, get_config_settings);

      CampaignConfigUpdateInfo_var config_update;

      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<Exception>(
            logger_,
            Logging::Logger::ERROR,
            Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE,
            "ADS_ECON-6",
            server_id_,
            server_id_);

        try
        {
          config_update = campaign_server->get_config(get_config_settings);
          break;
        }
        catch(const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA exception on update config: " << e;

          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE,
            "ADS-ECON-6");
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& )
        {
          Stream::Error ostr;
          ostr << FUN << ": Proxy CampaignServer::NotReady";
          logger_->log(ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE,
            "ADS-IMPL-147");
          campaign_server.release_bad(ostr.str());
        }
        catch(const
          AdServer::CampaignSvcs::CampaignServer::ImplementationException&)
        {
          Stream::Error ostr;
          ostr << FUN << ": Proxy CampaignServer::ImplementationException";
          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE,
            "ADS-IMPL-147");
        }
      }

      if (config_update.ptr())
      {
        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->stream(Logging::Logger::TRACE,
            Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE) <<
            "Loading from proxy ('" << get_config_settings.campaign_statuses << "'): " <<
            config_update->campaigns.length() << " updated campaigns, " <<
            config_update->deleted_campaigns.length() << " deleted campaigns. "
            "Request stamp: " << CorbaAlgs::unpack_time(get_config_settings.timestamp) <<
            ", result stamp: " << CorbaAlgs::unpack_time(config_update->master_stamp);
        }

        if(CorbaAlgs::unpack_time(config_update->master_stamp) >
             config->master_stamp)
        {
          Generics::Time first_load_stamp =
            CorbaAlgs::unpack_time(config_update->first_load_stamp);

          CampaignConfig_var new_config =
            first_load_stamp > config->master_stamp ||
            config->server_id != config_update->server_id ?
            new CampaignConfig() :
            new CampaignConfig(*config);

          /* update only if master stamp great then local
           * can be less if used few CampaignServer references */
          apply_config_update_(config_update, *new_config);

          campaign_config_ = new_config;
        }

        if(need_logging)
        {
          *need_logging = false;
        }
      }
    }
    catch(const eh::Exception& e)
    {
      logger_->sstream(Logging::Logger::CRITICAL,
        Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE,
        "ADS-IMPL-146") <<
        FUN << ": caught eh::Exception: " << e.what();
    }

    return campaign_config_;
  }

  void
  CampaignConfigServerSource::init_get_config_settings_(
    const CampaignConfig& config,
    CampaignGetConfigSettings& get_config_settings)
    noexcept
  {
    get_config_settings.timestamp = CorbaAlgs::pack_time(config.master_stamp);
    get_config_settings.server_id = config.server_id;
    get_config_settings.portion = 0;
    get_config_settings.portions_number = 1;

    get_config_settings.colo_id = colo_id_;
    get_config_settings.version << version_;

    get_config_settings.campaign_statuses << campaign_statuses_;
    get_config_settings.channel_statuses << channel_statuses_;
    get_config_settings.country << country_;

    get_config_settings.provide_only_tags = only_tags_;
    get_config_settings.no_deleted = false;
    get_config_settings.provide_channel_triggers = true;

    get_config_settings.geo_channels_timestamp = CorbaAlgs::pack_time(
      Generics::Time::ZERO);
  }

  void
  CampaignConfigServerSource::apply_app_formats_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for(CORBA::ULong i = 0; i < update_info.app_formats.length(); ++i)
    {
      const AdServer::CampaignSvcs::AppFormatInfo& app_format_info =
        update_info.app_formats[i];
      AppFormatDef_var app_format = new AppFormatDef();
      app_format->mime_format = app_format_info.mime_format;
      app_format->timestamp = CorbaAlgs::unpack_time(app_format_info.timestamp);
      new_config.app_formats.activate(app_format_info.app_format.in(), app_format);
    }

    deactivate_sequence(new_config.app_formats, update_info.delete_app_formats);
  }

  void
  CampaignConfigServerSource::apply_countries_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for(CORBA::ULong i = 0; i < update_info.countries.length(); ++i)
    {
      const AdServer::CampaignSvcs::CountryInfo& country_info =
        update_info.countries[i];
      CountryDef_var country = new CountryDef();
      country->timestamp = CorbaAlgs::unpack_time(country_info.timestamp);
      unpack_option_value_map(country->tokens, country_info.tokens);
      new_config.countries.activate(country_info.country_code.in(), country);
    }

    deactivate_sequence(new_config.countries, update_info.deleted_countries);
  }


  void
  CampaignConfigServerSource::apply_sizes_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for(CORBA::ULong i = 0; i < update_info.sizes.length(); ++i)
    {
      const AdServer::CampaignSvcs::SizeInfo& size_info =
        update_info.sizes[i];
      SizeDef_var size = new SizeDef();
      size->protocol_name = size_info.protocol_name.in();
      size->size_type_id = size_info.size_type_id;
      size->width = size_info.width;
      size->height = size_info.height;
      size->timestamp = CorbaAlgs::unpack_time(size_info.timestamp);
      new_config.sizes.activate(size_info.size_id, size);
    }

    deactivate_sequence(new_config.sizes, update_info.delete_sizes);
  }

  void CampaignConfigServerSource::apply_adv_actions_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    CORBA::ULong adv_actions_count = update_info.adv_actions.length();

    for(CORBA::ULong i = 0; i < adv_actions_count; ++i)
    {
      const AdServer::CampaignSvcs::AdvActionInfo& adv_action_info =
        update_info.adv_actions[i];

      AdvActionDef_var action = new AdvActionDef(
        adv_action_info.action_id,
        CorbaAlgs::unpack_time(adv_action_info.timestamp));

      CORBA::ULong j = 0;
      for(; j < adv_action_info.ccg_ids.length() &&
        adv_action_info.ccg_ids[j] != 0; ++j)
      {
        action->ccg_ids.insert(adv_action_info.ccg_ids[j]);
      }

      if(j < adv_action_info.ccg_ids.length())
      {
        CorbaAlgs::unpack_decimal_from_seq(
          action->cur_value, adv_action_info.ccg_ids, j + 1);
      }

      new_config.adv_actions.activate(adv_action_info.action_id, action);
    }

    deactivate_sequence(new_config.adv_actions, update_info.deleted_adv_actions);
  }

  void CampaignConfigServerSource::apply_creative_category_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong i = 0;
         i < update_info.creative_categories.length();
         ++i)
    {
      CreativeCategoryDef_var cat_def(new CreativeCategoryDef());
      cat_def->cct_id = update_info.creative_categories[i].cct_id;
      cat_def->name = update_info.creative_categories[i].name;
      for(CORBA::ULong ec_i = 0;
        ec_i < update_info.creative_categories[i].external_categories.length();
        ++ec_i)
      {
        const AdServer::CampaignSvcs::ExternalCategoryInfo& ext_cat =
          update_info.creative_categories[i].external_categories[ec_i];

        CreativeCategoryDef::ExternalCategoryNameSet& names =
          cat_def->external_categories[
            static_cast<AdRequestType>(ext_cat.ad_request_type)];
        for(CORBA::ULong name_i = 0;
          name_i < ext_cat.names.length(); ++name_i)
        {
          names.insert(ext_cat.names[name_i].in());
        }
      }

      cat_def->timestamp = CorbaAlgs::unpack_time(
        update_info.creative_categories[i].timestamp);

      new_config.creative_categories.activate(
        update_info.creative_categories[i].creative_category_id,
        cat_def);
    }

    deactivate_sequence(new_config.creative_categories,
      update_info.deleted_creative_categories);
  }

  void CampaignConfigServerSource::apply_category_channel_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    CORBA::ULong category_channels_count = update_info.category_channels.length();

    for(CORBA::ULong i = 0; i < category_channels_count; ++i)
    {
      const AdServer::CampaignSvcs::CategoryChannelInfo& category_channel_info =
        update_info.category_channels[i];

      CategoryChannelDef::LocalizationMap localizations;
      for(CORBA::ULong loc_i = 0; loc_i < category_channel_info.localizations.length();
          ++loc_i)
      {
        localizations.insert(std::make_pair(
          category_channel_info.localizations[loc_i].language.in(),
          category_channel_info.localizations[loc_i].name.in()));
      }

      CategoryChannelDef_var category_channel_def(
        new CategoryChannelDef(
          category_channel_info.channel_id,
          category_channel_info.name.in(),
          category_channel_info.newsgate_name.in(),
          category_channel_info.parent_channel_id,
            category_channel_info.flags,
            localizations,
            CorbaAlgs::unpack_time(category_channel_info.timestamp)));
      new_config.category_channels.activate(
        category_channel_info.channel_id, category_channel_def);
    }

    deactivate_sequence(new_config.category_channels,
      update_info.deleted_category_channels);
  }

  void
  CampaignConfigServerSource::apply_bp_param_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_bp_param_update_";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.behav_params.length(); ++i)
      {
        const AdServer::CampaignSvcs::BehavParamInfo& bp_info =
          update_info.behav_params[i];
        BehavioralParameterListDef_var new_bp =
          new BehavioralParameterListDef;
        new_bp->threshold = bp_info.threshold;
        new_bp->timestamp = CorbaAlgs::unpack_time(bp_info.timestamp);

        for(CORBA::ULong j = 0; j < bp_info.bp_seq.length(); j++)
        {
          BehavioralParameterDef bp_def;
          bp_def.min_visits = bp_info.bp_seq[j].min_visits;
          bp_def.time_from = bp_info.bp_seq[j].time_from;
          bp_def.time_to = bp_info.bp_seq[j].time_to;
          bp_def.weight = bp_info.bp_seq[j].weight;
          bp_def.trigger_type = bp_info.bp_seq[j].trigger_type;
          new_bp->behave_params.push_back(bp_def);
        }

        new_config.behav_param_lists.activate(bp_info.id, new_bp);
      }

      for(CORBA::ULong i = 0; i < update_info.key_behav_params.length(); ++i)
      {
        const AdServer::CampaignSvcs::KeyBehavParamInfo& bp_info =
          update_info.key_behav_params[i];
        BehavioralParameterListDef_var new_bp =
          new BehavioralParameterListDef;
        new_bp->threshold = bp_info.threshold;
        new_bp->timestamp = CorbaAlgs::unpack_time(bp_info.timestamp);

        for(CORBA::ULong j = 0; j < bp_info.bp_seq.length(); j++)
        {
          BehavioralParameterDef bp_def;
          bp_def.min_visits = bp_info.bp_seq[j].min_visits;
          bp_def.time_from = bp_info.bp_seq[j].time_from;
          bp_def.time_to = bp_info.bp_seq[j].time_to;
          bp_def.weight = bp_info.bp_seq[j].weight;
          bp_def.trigger_type = bp_info.bp_seq[j].trigger_type;
          new_bp->behave_params.push_back(bp_def);
        }
        new_config.str_behav_param_lists.activate(bp_info.id.in(), new_bp);
      }

      deactivate_sequence(new_config.behav_param_lists,
        update_info.deleted_behav_params);

      for(CORBA::ULong i = 0;
          i < update_info.deleted_key_behav_params.length(); ++i)
      {
        new_config.str_behav_param_lists.deactivate(
          update_info.deleted_key_behav_params[i].id.in(),
          CorbaAlgs::unpack_time(
            update_info.deleted_key_behav_params[i].timestamp));
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigServerSource::apply_expression_channel_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    /* update expression channels */
    const ExpressionChannelSeq& updated_expression_channels =
      update_info.expression_channels;
    unsigned int expression_channels_len = updated_expression_channels.length();

    for (CORBA::ULong i = 0; i < expression_channels_len; ++i)
    {
      NonLinkedExpressionChannel_var channel =
        unpack_non_linked_channel(
          updated_expression_channels[i]);

      new_config.expression_channels.activate(
        updated_expression_channels[i].channel_id,
        channel);
    }

    deactivate_sequence(new_config.expression_channels,
      update_info.deleted_expression_channels);
  }

  void CampaignConfigServerSource::apply_fraud_conditions_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    const char* FUN = "CampaignConfigServerSource::apply_fraud_conditions_update_()";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.fraud_conditions.length(); i++)
      {
        const AdServer::CampaignSvcs::FraudConditionInfo& fraud_cond_info =
          update_info.fraud_conditions[i];

        FraudConditionDef_var new_fraud_cond = new FraudConditionDef();
        new_fraud_cond->id = fraud_cond_info.id;
        new_fraud_cond->type = fraud_cond_info.type;
        new_fraud_cond->limit = fraud_cond_info.limit;
        new_fraud_cond->period = CorbaAlgs::unpack_time(fraud_cond_info.period);
        new_fraud_cond->timestamp = CorbaAlgs::unpack_time(fraud_cond_info.timestamp);

        new_config.fraud_conditions.activate(fraud_cond_info.id, new_fraud_cond);
      }

      deactivate_sequence(new_config.fraud_conditions,
        update_info.deleted_fraud_conditions);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigServerSource::apply_search_engines_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_search_engines_update_()";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.search_engines.length(); ++i)
      {
        const AdServer::CampaignSvcs::SearchEngineInfo& in =
          update_info.search_engines[i];
        SearchEngine_var value = new SearchEngine;
        value->timestamp = CorbaAlgs::unpack_time(in.timestamp);

        for(CORBA::ULong j = 0; j < in.regexps.length(); ++j)
        {
          SearchEngineRegExp reg_exp;
          reg_exp.host_postfix = in.regexps[j].host_postfix;
          reg_exp.regexp = in.regexps[j].regexp;
          reg_exp.encoding = in.regexps[j].encoding;
          reg_exp.post_encoding = in.regexps[j].post_encoding;
          reg_exp.decoding_depth = in.regexps[j].decoding_depth;
          value->regexps.push_back(reg_exp);
        }
        new_config.search_engines.activate(in.id, value);
        new_config.detectors_timestamp = std::max(
          new_config.detectors_timestamp, value->timestamp);
      }
      Generics::Time del_timestamp = deactivate_sequence(
        new_config.search_engines,
        update_info.deleted_search_engines);
      new_config.detectors_timestamp = std::max(
        new_config.detectors_timestamp, del_timestamp);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigServerSource::apply_web_browsers_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_web_browsers_update_()";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.web_browsers.length(); ++i)
      {
        const AdServer::CampaignSvcs::WebBrowserInfo& in =
          update_info.web_browsers[i];
        WebBrowser_var value = new WebBrowser;
        value->timestamp = CorbaAlgs::unpack_time(in.timestamp);

        for(CORBA::ULong j = 0; j < in.detectors.length(); ++j)
        {
          WebBrowser::Detector detector;
          detector.marker = in.detectors[j].marker;
          detector.regexp = in.detectors[j].regexp;
          detector.regexp_required = in.detectors[j].regexp_required;
          detector.priority = in.detectors[j].priority;
          value->detectors.push_back(detector);
        }

        new_config.web_browsers.activate(in.name.in(), value);
        new_config.detectors_timestamp = std::max(
          new_config.detectors_timestamp, value->timestamp);
      }
      Generics::Time del_timestamp = deactivate_sequence(
        new_config.web_browsers,
        update_info.deleted_web_browsers);
      new_config.detectors_timestamp = std::max(
        new_config.detectors_timestamp, del_timestamp);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigServerSource::apply_platforms_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_platforms_update_()";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.platforms.length(); ++i)
      {
        const AdServer::CampaignSvcs::PlatformInfo& in =
          update_info.platforms[i];
        Platform_var value = new Platform;
        value->name = in.name.in();
        value->type = in.type.in();
        value->timestamp = CorbaAlgs::unpack_time(in.timestamp);

        for(CORBA::ULong j = 0; j < in.detectors.length(); ++j)
        {
          Platform::Detector detector;
          detector.marker = in.detectors[j].marker;
          detector.match_regexp = in.detectors[j].match_regexp;
          detector.output_regexp = in.detectors[j].output_regexp;
          detector.priority = in.detectors[j].priority;
          value->detectors.push_back(detector);
        }

        new_config.platforms.activate(in.platform_id, value);
        new_config.detectors_timestamp = std::max(
          new_config.detectors_timestamp, value->timestamp);
      }
      Generics::Time del_timestamp = deactivate_sequence(
        new_config.platforms,
        update_info.deleted_platforms);
      new_config.detectors_timestamp = std::max(
        new_config.detectors_timestamp, del_timestamp);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigServerSource::apply_creative_options_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_creative_options_update_()";

    try
    {
      CORBA::ULong creative_options_count = update_info.activate_creative_options.length();

      for(CORBA::ULong i = 0; i < creative_options_count; ++i)
      {
        const AdServer::CampaignSvcs::CreativeOptionInfo& creative_option_info =
          update_info.activate_creative_options[i];

        StringSet token_relations;

        CorbaAlgs::convert_sequence(
          creative_option_info.token_relations,
          token_relations);

        CreativeOptionDef_var creative_option_def(
          new CreativeOptionDef(
            creative_option_info.token,
            creative_option_info.type,
            token_relations,
            CorbaAlgs::unpack_time(creative_option_info.timestamp)));
        new_config.creative_options.activate(
          creative_option_info.option_id, creative_option_def);
      }

      deactivate_sequence(
        new_config.creative_options,
        update_info.delete_creative_options);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigServerSource::apply_web_operations_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_web_operations_update_()";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.web_operations.length(); ++i)
      {
        const AdServer::CampaignSvcs::WebOperationInfo& in =
          update_info.web_operations[i];
        WebOperationDef_var value = new WebOperationDef;
        value->app = in.app;
        value->source = in.source;
        value->operation = in.operation;
        value->flags = in.flags;
        value->timestamp = CorbaAlgs::unpack_time(in.timestamp);
        new_config.web_operations.activate(in.id, value);
      }

      deactivate_sequence(
        new_config.web_operations,
        update_info.delete_web_operations);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignConfigServerSource::apply_block_channels_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_block_channels_update_()";

    try
    {
      for(CORBA::ULong i = 0; i < update_info.activate_block_channels.length(); ++i)
      {
        const AdServer::CampaignSvcs::BlockChannelInfo& block_channel_info =
          update_info.activate_block_channels[i];
        BlockChannelDef_var block_channel = new BlockChannelDef();
        block_channel->size_id = block_channel_info.size_id;
        block_channel->timestamp = CorbaAlgs::unpack_time(
          block_channel_info.timestamp);
        new_config.block_channels.activate(
          block_channel_info.channel_id, block_channel);
      }

      deactivate_sequence(
        new_config.block_channels,
        update_info.delete_block_channels);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignConfigServerSource::apply_config_update_(
    const CampaignConfigUpdateInfo& update_info,
    CampaignConfig& new_config)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigServerSource::apply_config_update_()";

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_SERVER_SOURCE) << FUN <<
        ": " << std::endl <<
         "  Number of updated countries : " <<
        update_info.countries.length() << std::endl <<
        "  Number of deleted countries : " <<
        update_info.deleted_countries.length() << std::endl <<
        "  Number of updated campaigns : " <<
        update_info.campaigns.length() << std::endl <<
        "  Number of deleted campaigns : " <<
        update_info.deleted_campaigns.length() << std::endl <<
        "  Number of updated ecpms : " <<
        update_info.ecpms.length() << std::endl <<
        "  Number of deleted ecpms : " <<
        update_info.deleted_ecpms.length() << std::endl <<
        "  Number of updated expression channels : " <<
        update_info.expression_channels.length() << std::endl <<
        "  Number of deleted expression channels : " <<
        update_info.deleted_expression_channels.length() << std::endl <<
        "  Number of updated simple channels : " <<
        update_info.simple_channels.length() << std::endl <<
        "  Number of deleted simple channels : " <<
        update_info.deleted_simple_channels.length() << std::endl <<
        "  Number of updated sites : " <<
        update_info.sites.length() << std::endl <<
        "  Number of deleted sites : " <<
        update_info.deleted_sites.length() << std::endl <<
        "  Number of updated tags : " <<
        update_info.tags.length() << std::endl <<
        "  Number of deleted tags : " <<
        update_info.deleted_tags.length() << std::endl <<
        "  Number of updated frequency caps : " <<
        update_info.frequency_caps.length() << std::endl <<
        "  Number of deleted frequency caps : " <<
        update_info.deleted_freq_caps.length() << std::endl <<
        "  Number of updated colocations : " <<
        update_info.colocations.length() << std::endl <<
        "  Number of deleted colocations : " <<
        update_info.deleted_colocations.length() << std::endl <<
        "  Number of updated creative templates : " <<
        update_info.creative_templates.length() << std::endl <<
        "  Number of deleted creative templates : " <<
        update_info.deleted_templates.length() << std::endl <<
        "  Number of updated campaign keywords : " <<
        update_info.campaign_keywords.length() << std::endl <<
        "  Number of deleted campaign keywords : " <<
        update_info.deleted_keywords.length() << std::endl <<
        "  Number of updated keyword ecpms : " <<
        update_info.accounts.length() << std::endl <<
        "  Number of deleted accounts : " <<
        update_info.deleted_accounts.length() << std::endl <<
        "  Number of updated currencies : " <<
        update_info.currencies.length() << std::endl <<
        "  Number of updated behav params: " <<
        update_info.behav_params.length() << std::endl <<
        "  Number of deleted behav params : " <<
        update_info.deleted_behav_params.length() << std::endl <<
        "  Number of updated key behav params: " <<
        update_info.key_behav_params.length() << std::endl <<
        "  Number of deleted key behav params : " <<
        update_info.deleted_key_behav_params.length() << std::endl <<
        "  Number of updated search engines: " <<
        update_info.search_engines.length() << std::endl <<
        "  Number of deleted search engines: " <<
        update_info.deleted_search_engines.length() << std::endl;
    }

    /* fill global params */
    new_config.server_id = update_info.server_id;
    new_config.master_stamp = CorbaAlgs::unpack_time(update_info.master_stamp);
    new_config.first_load_stamp = CorbaAlgs::unpack_time(update_info.first_load_stamp);
    new_config.finish_load_stamp = CorbaAlgs::unpack_time(update_info.finish_load_stamp);
    new_config.global_params.fraud_user_deactivate_period = CorbaAlgs::unpack_time(
      update_info.fraud_user_deactivate_period);
    new_config.global_params.currency_exchange_id = update_info.currency_exchange_id;
    new_config.global_params.cost_limit =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(update_info.cost_limit);
    new_config.global_params.google_publisher_account_id =
      update_info.google_publisher_account_id;
    new_config.global_params.timestamp =
      CorbaAlgs::unpack_time(update_info.global_params_timestamp);

    apply_app_formats_update_(update_info, new_config);
    apply_sizes_update_(update_info, new_config);
    apply_countries_update_(update_info, new_config);
    apply_adv_actions_update_(update_info, new_config);
    apply_creative_category_update_(update_info, new_config);
    apply_category_channel_update_(update_info, new_config);
    apply_bp_param_update_(update_info, new_config);
    apply_fraud_conditions_update_(update_info, new_config);
    apply_search_engines_update_(update_info, new_config);
    apply_web_browsers_update_(update_info, new_config);
    apply_platforms_update_(update_info, new_config);
    apply_expression_channel_update_(update_info, new_config);
    apply_creative_options_update_(update_info, new_config);
    apply_web_operations_update_(update_info, new_config);
    apply_block_channels_update_(update_info, new_config);

    CORBA::ULong currency_count = update_info.currencies.length();

    for(CORBA::ULong i = 0; i < currency_count; ++i)
    {
      const AdServer::CampaignSvcs::CurrencyInfo& currency_info =
        update_info.currencies[i];
      CurrencyDef_var new_currency(new CurrencyDef);
      new_currency->rate = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        currency_info.rate);
      new_currency->currency_id = currency_info.currency_id;
      new_currency->currency_exchange_id = currency_info.currency_exchange_id;
      new_currency->effective_date = currency_info.effective_date;
      new_currency->fraction_digits = currency_info.fraction_digits;
      new_currency->currency_code = currency_info.currency_code;
      new_currency->timestamp = CorbaAlgs::unpack_time(currency_info.timestamp);

      new_config.currencies.activate(currency_info.currency_id, new_currency);
    }

    /* update accounts */
    deactivate_sequence(new_config.accounts,
      update_info.deleted_accounts);

    for (CORBA::ULong i = 0; i < update_info.accounts.length(); i++)
    {
      const AccountInfo& acc_info = update_info.accounts[i];
      AccountDef_var acc(new AccountDef());

      acc->account_id = acc_info.account_id;
      acc->agency_account_id = acc_info.agency_account_id;
      acc->internal_account_id = acc_info.internal_account_id;
      acc->role_id = acc_info.role_id;
      acc->legal_name = acc_info.legal_name;
      acc->flags = acc_info.flags;
      acc->at_flags = acc_info.at_flags;
      acc->text_adserving = acc_info.text_adserving;
      acc->currency_id = acc_info.currency_id;
      acc->country = acc_info.country;
      acc->commision = CorbaAlgs::unpack_decimal<RevenueDecimal>(acc_info.commision);
      acc->budget = CorbaAlgs::unpack_decimal<RevenueDecimal>(acc_info.budget);
      acc->paid_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(acc_info.paid_amount);
      acc->time_offset = CorbaAlgs::unpack_time(acc_info.time_offset);
      CorbaAlgs::convert_sequence(
        acc_info.walled_garden_accounts, acc->walled_garden_accounts);
      acc->auction_rate = static_cast<AuctionRateType>(acc_info.auction_rate);
      acc->use_pub_pixels = acc_info.use_pub_pixels;
      acc->pub_pixel_optin = acc_info.pub_pixel_optin.in();
      acc->pub_pixel_optout = acc_info.pub_pixel_optout.in();
      acc->self_service_commission = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        acc_info.self_service_commission);
      acc->status = acc_info.status;
      acc->eval_status = acc_info.eval_status;

      acc->timestamp = CorbaAlgs::unpack_time(acc_info.timestamp);

      new_config.accounts.activate(acc_info.account_id, acc);
    }

    /* update ecpms */
    deactivate_sequence(new_config.ecpms, update_info.deleted_ecpms);

    for (CORBA::ULong i = 0; i < update_info.ecpms.length(); i++)
    {
      Ecpm_var ecpm(new EcpmDef(
        CorbaAlgs::unpack_decimal<RevenueDecimal>(
          update_info.ecpms[i].ecpm),
        CorbaAlgs::unpack_decimal<RevenueDecimal>(
          update_info.ecpms[i].ctr),
        CorbaAlgs::unpack_time(update_info.ecpms[i].timestamp)));
      new_config.ecpms.activate(
        update_info.ecpms[i].ccg_id, ecpm);
    }

    /* fill campaigns */
    deactivate_sequence(new_config.campaigns, update_info.deleted_campaigns);

    for (CORBA::ULong i = 0; i < update_info.campaigns.length(); i++)
    {
      const CampaignInfo& campaign_info = update_info.campaigns[i];
      Campaign_var campaign(new CampaignDef);

      campaign->campaign_group_id = campaign_info.campaign_group_id;
      campaign->fc_id = campaign_info.fc_id;
      campaign->group_fc_id = campaign_info.group_fc_id;
      campaign->status = campaign_info.status;
      campaign->eval_status = campaign_info.eval_status;
      campaign->country = campaign_info.country;
      campaign->ccg_rate_id = campaign_info.ccg_rate_id;
      campaign->ccg_rate_type = campaign_info.ccg_rate_type;
      campaign->flags = campaign_info.flags;
      campaign->marketplace = campaign_info.marketplace;
      campaign->account_id = campaign_info.account_id;
      campaign->advertiser_id = campaign_info.advertiser_id;

      campaign->imp_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.imp_revenue);
      campaign->click_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.click_revenue);
      campaign->action_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.action_revenue);
      campaign->commision = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.commision);
      campaign->ccg_type = static_cast<CCGType>(campaign_info.ccg_type);
      campaign->target_type = campaign_info.target_type;
      campaign->start_user_group_id = campaign_info.start_user_group_id;
      campaign->end_user_group_id = campaign_info.end_user_group_id;
      campaign->ctr_reset_id = campaign_info.ctr_reset_id;
      campaign->mode = static_cast<CampaignMode>(campaign_info.mode);
      campaign->seq_set_rotate_imps = campaign_info.seq_set_rotate_imps;
      campaign->min_uid_age = CorbaAlgs::unpack_time(campaign_info.min_uid_age);

      unpack_delivery_limits(
        campaign->campaign_delivery_limits,
        campaign_info.campaign_delivery_limits);
      unpack_delivery_limits(
        campaign->ccg_delivery_limits,
        campaign_info.ccg_delivery_limits);

      campaign->max_pub_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.max_pub_share);
      campaign->bid_strategy = static_cast<BidStrategy>(
        campaign_info.bid_strategy);
      campaign->min_ctr_goal = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_info.min_ctr_goal);

      campaign->timestamp =
        CorbaAlgs::unpack_time(campaign_info.timestamp);

      convert_interval_sequence(
        campaign->weekly_run_intervals,
        campaign_info.weekly_run_intervals);

      CorbaAlgs::convert_sequence(campaign_info.sites, campaign->sites);

      for(CORBA::ULong j = 0; j < campaign_info.creatives.length(); j++)
      {
        const CreativeInfo& creative_info = campaign_info.creatives[j];

        CreativeDef_var new_creative(
          new CreativeDef(
            creative_info.ccid,
            creative_info.creative_id,
            creative_info.fc_id,
            creative_info.weight,
            creative_info.creative_format.in(),
            creative_info.version_id.in(),
            creative_info.status,
            CreativeDef::SystemOptions(
              creative_info.click_url.option_id,
              creative_info.click_url.value,
              creative_info.html_url.option_id,
              creative_info.html_url.value),
            creative_info.tokens,
            creative_info.categories));

        new_creative->order_set_id = creative_info.order_set_id;
        for(CORBA::ULong k = 0; k < creative_info.sizes.length(); ++k)
        {
          const CreativeSizeInfo& size_info = creative_info.sizes[k];
          CreativeDef::Size& size = new_creative->sizes[size_info.size_id];
          size.up_expand_space = size_info.up_expand_space;
          size.right_expand_space = size_info.right_expand_space;
          size.down_expand_space = size_info.down_expand_space;
          size.left_expand_space = size_info.left_expand_space;
          unpack_option_value_map(size.tokens, size_info.tokens);
        }

        campaign->creatives.push_back(new_creative);
      }

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

      campaign->delivery_coef = campaign_info.delivery_coef;

      unpack_non_linked_expression(campaign->expression, campaign_info.expression);
      unpack_non_linked_expression(campaign->stat_expression, campaign_info.stat_expression);

      new_config.campaigns.activate(campaign_info.campaign_id, campaign);
    }

    /* update simple channels */
    const SimpleChannelKeySeq& updated_simple_channels =
      update_info.simple_channels;

    for (CORBA::ULong i = 0; i < updated_simple_channels.length(); ++i)
    {
      SimpleChannelDef_var new_ch = unpack_simple_channel(
        updated_simple_channels[i]);

      new_config.simple_channels.activate(new_ch->channel_id, new_ch);
    }

    deactivate_sequence(new_config.simple_channels, update_info.deleted_simple_channels);

    /* update geo channels */
    const GeoChannelSeq& activate_geo_channels =
      update_info.activate_geo_channels;

    if(activate_geo_channels.length() || update_info.delete_geo_channels.length())
    {
      new_config.geo_channels = new GeoChannelMap(*new_config.geo_channels);

      for (CORBA::ULong i = 0; i < activate_geo_channels.length(); ++i)
      {
        const GeoChannelInfo& geo_channel_info = activate_geo_channels[i];
        GeoChannelDef_var new_ch = new GeoChannelDef();
        new_ch->country = geo_channel_info.country;
        new_ch->timestamp = CorbaAlgs::unpack_time(geo_channel_info.timestamp);

        for(CORBA::ULong ti = 0; ti < geo_channel_info.geoip_targets.length(); ++ti)
        {
          GeoChannelDef::GeoIPTarget tgt;
          tgt.region = geo_channel_info.geoip_targets[ti].region;
          tgt.city = geo_channel_info.geoip_targets[ti].city;
          new_ch->geoip_targets.push_back(tgt);
        }

        new_config.geo_channels->activate(
          geo_channel_info.channel_id,
          new_ch);
      }

      deactivate_sequence(
        *new_config.geo_channels,
        update_info.delete_geo_channels);
    }

    /* update geo coord channels */
    const GeoCoordChannelSeq& activate_geo_coord_channels =
      update_info.activate_geo_coord_channels;

    for (CORBA::ULong i = 0; i < activate_geo_coord_channels.length(); ++i)
    {
      const GeoCoordChannelInfo& geo_coord_channel_info =
        update_info.activate_geo_coord_channels[i];
      GeoCoordChannelDef_var new_ch = new GeoCoordChannelDef();
      new_ch->longitude = CorbaAlgs::unpack_decimal<CoordDecimal>(
        geo_coord_channel_info.longitude);
      new_ch->latitude = CorbaAlgs::unpack_decimal<CoordDecimal>(
        geo_coord_channel_info.latitude);
      new_ch->radius = CorbaAlgs::unpack_decimal<CoordDecimal>(
        geo_coord_channel_info.radius);
      new_ch->timestamp = CorbaAlgs::unpack_time(geo_coord_channel_info.timestamp);
      new_config.geo_coord_channels.activate(
        geo_coord_channel_info.channel_id,
        new_ch);
    }

    deactivate_sequence(
      new_config.geo_coord_channels,
      update_info.delete_geo_coord_channels);

    /* update freq caps */
    deactivate_sequence(new_config.freq_caps, update_info.deleted_freq_caps);

    for (CORBA::ULong i = 0; i < update_info.frequency_caps.length(); i++)
    {
      const FreqCapInfo& freq_cap_info = update_info.frequency_caps[i];
      FreqCapDef_var freq_cap = new FreqCapDef(
        freq_cap_info.fc_id,
        freq_cap_info.lifelimit,
        freq_cap_info.period,
        freq_cap_info.window_limit,
        freq_cap_info.window_time,
        CorbaAlgs::unpack_time(freq_cap_info.timestamp));

      new_config.freq_caps.activate(freq_cap->fc_id, freq_cap);
    }

    /* update creative templates */
    deactivate_sequence(new_config.creative_templates, update_info.deleted_templates);

    for (CORBA::ULong i = 0; i < update_info.creative_templates.length(); i++)
    {
      const CreativeTemplateInfo& cr_templ_info =
        update_info.creative_templates[i];

      CreativeTemplateDef_var res_templ(new CreativeTemplateDef());

      res_templ->timestamp = CorbaAlgs::unpack_time(cr_templ_info.timestamp);

      for (CORBA::ULong ctf_i = 0;
           ctf_i < cr_templ_info.files.length(); ++ctf_i)
      {
        const CreativeTemplateFileInfo& ctf_info =
          cr_templ_info.files[ctf_i];
        res_templ->files.push_back(
          CreativeTemplateFileDef(
            ctf_info.creative_format.in(),
            ctf_info.creative_size.in(),
            ctf_info.app_format.in(),
            ctf_info.mime_format.in(),
            ctf_info.track_impr,
            ctf_info.type,
            ctf_info.template_file.in()));
      }

      unpack_option_value_map(res_templ->tokens, cr_templ_info.tokens);
      unpack_option_value_map(res_templ->hidden_tokens, cr_templ_info.hidden_tokens);

      new_config.creative_templates.activate(cr_templ_info.id, res_templ);
    }

    /* update sites */
    deactivate_sequence(new_config.sites, update_info.deleted_sites);

    for (unsigned int i = 0; i < update_info.sites.length(); i++)
    {
      const SiteInfo& site_info = update_info.sites[i];
      SiteDef_var p_site(new SiteDef());

      p_site->freq_cap_id = site_info.freq_cap_id;
      p_site->noads_timeout = site_info.noads_timeout;
      p_site->status = site_info.status;
      p_site->flags = site_info.flags;
      p_site->account_id = site_info.account_id;
      p_site->timestamp = CorbaAlgs::unpack_time(site_info.timestamp);

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

      new_config.sites.activate(site_info.site_id, p_site);
    }

    /* fill tags */
    deactivate_sequence(new_config.tags, update_info.deleted_tags);

    for (CORBA::ULong i = 0; i < update_info.tags.length(); i++)
    {
      const TagInfo& tag_info = update_info.tags[i];
      TagDef_var p_tag(new TagDef());

      p_tag->tag_id = tag_info.tag_id;
      p_tag->site_id = tag_info.site_id;
      for(CORBA::ULong size_i = 0; size_i < tag_info.sizes.length(); ++size_i)
      {
        TagDef::Size& res_size = p_tag->sizes[tag_info.sizes[size_i].size_id];
        res_size.max_text_creatives = tag_info.sizes[size_i].max_text_creatives;
        unpack_option_value_map(res_size.tokens, tag_info.sizes[size_i].tokens);
        unpack_option_value_map(res_size.hidden_tokens, tag_info.sizes[size_i].hidden_tokens);
      }

      p_tag->imp_track_pixel = tag_info.imp_track_pixel;
      p_tag->passback = tag_info.passback;
      p_tag->passback_type = tag_info.passback_type;
      p_tag->allow_expandable = tag_info.allow_expandable;

      p_tag->flags = tag_info.flags;
      p_tag->marketplace = tag_info.marketplace;
      p_tag->adjustment = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.adjustment);
      p_tag->auction_max_ecpm_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.auction_max_ecpm_share);
      p_tag->auction_prop_probability_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.auction_prop_probability_share);
      p_tag->auction_random_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        tag_info.auction_random_share);
      p_tag->cost_coef = CorbaAlgs::unpack_decimal<RevenueDecimal>(tag_info.cost_coef);

      p_tag->tag_pricings_timestamp = CorbaAlgs::unpack_time(
        tag_info.tag_pricings_timestamp);
      p_tag->timestamp = CorbaAlgs::unpack_time(tag_info.timestamp);

      /* fill tag pricings */
      const TagPricingInfoSeq& tag_pricing_list =
        tag_info.tag_pricings;
      for (CORBA::ULong j = 0; j < tag_pricing_list.length(); j++)
      {
        const TagPricingInfo& tag_pricing_ref = tag_pricing_list[j];
        TagPricingDef tag_pricing;
        tag_pricing.country_code = tag_pricing_ref.country_code;
        tag_pricing.site_rate_id = tag_pricing_ref.site_rate_id;
        tag_pricing.ccg_type = static_cast<CCGType>(tag_pricing_ref.ccg_type);
        tag_pricing.ccg_rate_type = static_cast<CCGRateType>(tag_pricing_ref.ccg_rate_type);
        tag_pricing.imp_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          tag_pricing_ref.imp_revenue);
        tag_pricing.revenue_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          tag_pricing_ref.revenue_share);
        p_tag->tag_pricings.push_back(tag_pricing);
      }

      CorbaAlgs::convert_sequence(
        tag_info.accepted_categories,
        p_tag->accepted_categories);

      CorbaAlgs::convert_sequence(
        tag_info.rejected_categories,
        p_tag->rejected_categories);

      unpack_option_value_map(p_tag->tokens, tag_info.tokens);
      unpack_option_value_map(p_tag->hidden_tokens, tag_info.hidden_tokens);
      unpack_option_value_map(p_tag->passback_tokens, tag_info.passback_tokens);

      for(CORBA::ULong template_i = 0;
          template_i < tag_info.template_tokens.length();
          ++template_i)
      {
        OptionValueMap tokens;
        unpack_option_value_map(
          tokens,
          tag_info.template_tokens[template_i].tokens);
        p_tag->template_tokens[
          tag_info.template_tokens[template_i].template_name.in()].swap(
            tokens);
      }

      new_config.tags.activate(p_tag->tag_id, p_tag);
    }

    /* update colocations */
    {
      deactivate_sequence(new_config.colocations,
        update_info.deleted_colocations);

      for(CORBA::ULong i = 0; i < update_info.colocations.length(); ++i)
      {
        const AdServer::CampaignSvcs::ColocationInfo& colo_info =
          update_info.colocations[i];

        ColocationDef_var colo = new ColocationDef();
        colo->colo_id = colo_info.colo_id;
        colo->colo_name = colo_info.colo_name;
        colo->colo_rate_id = colo_info.colo_rate_id;
        colo->at_flags = colo_info.at_flags;
        colo->account_id = colo_info.account_id;
        colo->revenue_share = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          colo_info.revenue_share);
        colo->ad_serving = static_cast<ColocationAdServingType>(colo_info.ad_serving);
        colo->hid_profile = colo_info.hid_profile;
        unpack_option_value_map(colo->tokens, colo_info.tokens);
        colo->timestamp = CorbaAlgs::unpack_time(colo_info.timestamp);

        new_config.colocations.activate(colo_info.colo_id, colo);
      }
    }

    /* update campaign keywords */
    deactivate_sequence(new_config.campaign_keywords,
      update_info.deleted_keywords);

    for(CORBA::ULong i = 0; i < update_info.campaign_keywords.length(); ++i)
    {
      const AdServer::CampaignSvcs::CampaignKeywordInfo& kw_info =
        update_info.campaign_keywords[i];

      CampaignKeyword_var campaign_keyword(
        new CampaignKeyword(
          kw_info.ccg_keyword_id,
          kw_info.original_keyword,
          kw_info.click_url,
          CorbaAlgs::unpack_time(kw_info.timestamp)));
      new_config.campaign_keywords.activate(
        kw_info.ccg_keyword_id, campaign_keyword);
    }
  }
}
}
