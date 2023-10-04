#include <algorithm>
#include <map>
#include <Commons/Algs.hpp>
#include <Commons/Postgres/ResultSet.hpp>

#include "ExpressionChannelParser.hpp"
#include "ExecutionTimeTracer.hpp"
#include "StatDBSource.hpp"
#include "StatLogGeneralizerSource.hpp"
#include "CampaignConfigDBSource.hpp"


using String::StringManip::IntToStr;

namespace AdServer
{
namespace CampaignSvcs
{
  namespace CreativeTokens
  {
    const char CLICKURL[] = "CLICK";
    const char PRECLICKURL[] = "PRECLICK";
    const char CLICKF[] = "CLICKF";
    const char PRECLICKF[] = "PRECLICKF";
    const char CLICK0URL[] = "CLICK0";
    const char PRECLICK0URL[] = "PRECLICK0";
    const char CLICK0F[] = "CLICK0F";
    const char PRECLICK0F[] = "PRECLICK0F";
  }

  namespace
  {
    // this id must be great then all db creative option ids,
    // if any colo have <= 2.1 version
    const unsigned long OPTION_EXTENSION_START_ID = 0xFFFFFFFA;

    const char AUDIENCE_CHANNEL_TYPE = 'A';

    namespace Aspect
    {
      const char CAMPAIGN_SERVER[] = "CampaignServer";
      const char TRAFFICKING_PROBLEM[] = "TraffickingProblem";
    }

    namespace SpecialTokens
    {
      const char ST_HTML_URL[] = "CRHTML";
      const char ST_CLICK_URL[] = "CRCLICK";
    }

    const char TEXT_CREATIVE_FORMAT[] = "Text";
    const char TEXT_CCG_TYPE = 'T';

    namespace RtbNames
    {
      const String::SubString IAB_NAME("IAB");
      const String::SubString OPENX_NAME("OPENX");
      const String::SubString APPNEXUS_NAME("APPNEXUS");
      const String::SubString GOOGLE_NAME("GOOGLE");
    }

    struct OptionsTypeConverter
    {
      OptionsTypeConverter()
      {
        names_[String::SubString("string")] = 'S';
        names_[String::SubString("text")] = 'T';
        names_[String::SubString("url")] = 'L';
      }

      char operator()(const String::SubString& s_type) const
      {
        NameTable::const_iterator it = names_.find(s_type);
        return it != names_.end() ? it->second : 'O';
      }

    protected:
      typedef Generics::GnuHashTable<
        Generics::SubStringHashAdapter, unsigned long> NameTable;
      NameTable names_;
    };

    struct AdvertiserOptionsTypeConverter: public OptionsTypeConverter
    {
      AdvertiserOptionsTypeConverter()
      {
        names_[String::SubString("file")] = 'F';
        names_[String::SubString("file/url")] = 'U';
        names_[String::SubString("dynamic file")] = 'D';
      }
    };

    struct PublisherOptionsTypeConverter: public OptionsTypeConverter
    {
      PublisherOptionsTypeConverter()
      {
        names_[String::SubString("file")] = 'f';
        names_[String::SubString("file/url")] = 'u';
        names_[String::SubString("dynamic file")] = 'D';
      }
    };

    static const AdvertiserOptionsTypeConverter
      NAME_ADV_OPTION_TYPE_CONVERTER;
    static const PublisherOptionsTypeConverter
      NAME_PUB_OPTION_TYPE_CONVERTER;

    // versions <= 2.0 requirement: cpm can be represent as uint32_t
    const RevenueDecimal MAXIMUM_COST_LIMIT = RevenueDecimal::div(
      RevenueDecimal::div(
        RevenueDecimal(false, std::numeric_limits<uint32_t>::max() - 1, 0),
        REVENUE_DECIMAL_THS),
      RevenueDecimal(false, 100, 0)); // ecpm precision (cents)

    const AccuracyDecimal YARD_IN_METERS("0.9144");
    const AccuracyDecimal MILE_IN_METERS("1609.344");

    struct CurrSize : public CreativeDef::Size
    {
      unsigned long cur_options_width;
      unsigned long cur_options_height;
      unsigned long cur_options_max_width;
      unsigned long cur_options_max_height;

      CurrSize()
        : cur_options_width(0), cur_options_height(0),
          cur_options_max_width(0), cur_options_max_height(0)
      {}
    };

    typedef std::map<unsigned long, CurrSize> CurrSizeMap;

    struct CurrSizeEquals
    {
      bool
      operator()(
        const std::pair<unsigned long, CurrSize>& left,
        const std::pair<unsigned long, CreativeDef::Size>& right) const
        noexcept
      {
        return (left.first == right.first && left.second == right.second);
      }
    };

    const unsigned long OPTION_GENERIC_GROUP_ID = 1;
    const unsigned long OPTION_ADVERTISER_GROUP_ID = 1;
    const unsigned long OPTION_PUBLISHER_GROUP_ID = 1;
    const unsigned long OPTION_INTERNAL_GROUP_ID = 1;

    OptionValueMap::const_iterator
    find_token(
      const CreativeDef& creative,
      const CreativeOptionMap& creative_options,
      const std::string& name)
      noexcept
    {
      auto ti = creative.tokens.begin();

      for (; ti != creative.tokens.end(); ++ti)
      {
        const auto coi = creative_options.active().find(ti->first);

        if (coi != creative_options.active().end() &&
            name == coi->second->token)
        {
          break;
        }
      }

      return ti;
    }
  }

  bool
  CampaignConfigDBSource::check_statuses_(
    const AccountMap::ActiveMap& account_map,
    const CreativeDef* creative,
    const CampaignDef* campaign) noexcept
  {
    if (!creative || !campaign || account_map.empty())
    {
      return false;
    }

    if ((creative->status == 'A' || creative->status == 'P') &&
        (campaign->status == 'A' || campaign->status == 'P'))
    {
      AccountMap::ActiveMap::const_iterator acc_it =
        account_map.find(campaign->account_id);

      if (acc_it != account_map.end())
      {
        if (acc_it->second->status == 'A' || acc_it->second->status == 'P')
        {
          if (campaign->account_id == campaign->advertiser_id)
          {
            return true;
          }
          else
          {
            AccountMap::ActiveMap::const_iterator acc_adv_it =
              account_map.find(campaign->advertiser_id);

            if (acc_adv_it != account_map.end())
            {
              return (acc_adv_it->second->status == 'A' || acc_adv_it->second->status == 'P');
            }
            else
            {
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  CampaignConfigDBSource::CampaignConfigDBSource(
    Logging::Logger* logger,
    unsigned long server_id,
    const String::SubString& campaign_statuses,
    const String::SubString& channel_statuses,
    Commons::Postgres::Environment* pg_env,
    const Generics::Time& stat_stamp_sync_period,
    const CORBACommons::CorbaObjectRefList& stat_providers,
    const Generics::Time& audience_expiration_time,
    const Generics::Time& pending_expire_time,
    bool enable_delivery_thresholds)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      server_id_(server_id),
      campaign_statuses_(campaign_statuses.str()),
      channel_statuses_(channel_statuses.str()),
      pending_expire_time_(pending_expire_time),
      pg_env_(ReferenceCounting::add_ref(pg_env))
  {
    static const char* FUN = "CampaignConfigDBSource::CampaignConfigDBSource()";

    try
    {
      pg_pool_ = pg_env_->create_connection_pool();

      db_stat_source_ = new StatDBSource(
        logger, pg_pool_, server_id, stat_providers);

      campaign_config_modifier_ = new CampaignConfigModifier(
        logger,
        db_stat_source_,
        stat_stamp_sync_period,
        StatSource_var(new StatLogGeneralizerSource(
          logger, server_id, stat_providers)),
        ModifyConfigSource_var(new ModifyConfigDBSource(
          logger, pg_pool_)),
          enable_delivery_thresholds);

      {
        CreativeOptionDef_var adimage_path_suffix_option = new CreativeOptionDef(
          AD_IMAGE_PATH_SUFFIX_TOKEN,
          'I',
          StringSet());
        adimage_path_suffix_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          AD_IMAGE_PATH_SUFFIX_OPTION_ID,
          adimage_path_suffix_option));
      }

      {
        StringSet adimage_path_option_relations;
        adimage_path_option_relations.insert("ADIMAGE-SERVER");
        adimage_path_option_relations.insert("CRVSERVER");
        adimage_path_option_relations.insert(AD_IMAGE_PATH_SUFFIX_TOKEN);
        CreativeOptionDef_var adimage_path_option = new CreativeOptionDef(
          AD_IMAGE_PATH_TOKEN,
          'I',
          adimage_path_option_relations);
        adimage_path_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          AD_IMAGE_PATH_OPTION_ID,
          adimage_path_option));

        CreativeOptionDef_var crvbase_option = new CreativeOptionDef(
          CRVBASE_TOKEN,
          'I',
          adimage_path_option_relations);
        crvbase_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          CRVBASE_OPTION_ID,
          crvbase_option));
      }

      {
        CreativeOptionDef_var adfooter_enabled_option = new CreativeOptionDef(
          FOOTER_ENABLED_TOKEN,
          'I',
          StringSet());
        adfooter_enabled_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          FOOTER_ENABLED_OPTION_ID,
          adfooter_enabled_option));
      }

      {
        CreativeOptionDef_var adfooter_url_option = new CreativeOptionDef(
          FOOTER_URL_TOKEN,
          'I',
          StringSet());
        adfooter_url_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          FOOTER_URL_OPTION_ID,
          adfooter_url_option));
      }

      {
        CreativeOptionDef_var max_random_cpm_option = new CreativeOptionDef(
          MAX_RANDOM_CPM_TOKEN,
          'I',
          StringSet());
        max_random_cpm_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          MAX_RANDOM_CPM_OPTION_ID,
          max_random_cpm_option));
      }

      {
        CreativeOptionDef_var creative_width_option = new CreativeOptionDef(
          DEFAULT_CREATIVE_WIDTH_TOKEN,
          'I',
          StringSet());
        creative_width_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          DEFAULT_CREATIVE_WIDTH_OPTION_ID,
          creative_width_option));
      }

      {
        CreativeOptionDef_var creative_height_option = new CreativeOptionDef(
          DEFAULT_CREATIVE_HEIGHT_TOKEN,
          'I',
          StringSet());
        creative_height_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          DEFAULT_CREATIVE_HEIGHT_OPTION_ID,
          creative_height_option));
      }

      {
        CreativeOptionDef_var creative_max_width_option = new CreativeOptionDef(
          CREATIVE_MAX_WIDTH_TOKEN,
          'I',
          StringSet());
        creative_max_width_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          CREATIVE_MAX_WIDTH_OPTION_ID,
          creative_max_width_option));
      }

      {
        CreativeOptionDef_var creative_max_height_option = new CreativeOptionDef(
          CREATIVE_MAX_HEIGHT_TOKEN,
          'I',
          StringSet());
        creative_max_height_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          CREATIVE_MAX_HEIGHT_OPTION_ID,
          creative_max_height_option));
      }

      {
        CreativeOptionDef_var creative_expand_direction_option =
          new CreativeOptionDef(
            CREATIVE_EXPAND_DIRECTION_TOKEN,
            'I',
            StringSet());
        creative_expand_direction_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          CREATIVE_EXPAND_DIRECTION_OPTION_ID,
          creative_expand_direction_option));
      }

      {
        CreativeOptionDef_var tag_min_visibility_option =
          new CreativeOptionDef(
            TAG_MIN_VISIBILITY_TOKEN,
            'I',
            StringSet());
        tag_min_visibility_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          TAG_MIN_VISIBILITY_OPTION_ID,
          tag_min_visibility_option));
      }

      {
        CreativeOptionDef_var passback_url_option = new CreativeOptionDef(
          TAG_PASSBACK_URL_TOKEN,
          'I',
          StringSet());
        passback_url_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          TAG_PASSBACK_URL_OPTION_ID,
          passback_url_option));
      }

      {
        CreativeOptionDef_var passback_type_option = new CreativeOptionDef(
          TAG_PASSBACK_TYPE_TOKEN,
          'I',
          StringSet());
        passback_type_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          TAG_PASSBACK_TYPE_OPTION_ID,
          passback_type_option));
      }

      {
        CreativeOptionDef_var passback_code_option = new CreativeOptionDef(
          TAG_PASSBACK_CODE_TOKEN,
          'I',
          StringSet());
        passback_code_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          TAG_PASSBACK_CODE_OPTION_ID,
          passback_code_option));
      }

      {
        CreativeOptionDef_var mp4_duration_hformat_option = new CreativeOptionDef(
          MP4_DURATION_HFORMAT_TOKEN,
          'I',
          StringSet());
        mp4_duration_hformat_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          MP4_DURATION_HFORMAT_OPTION_ID,
          mp4_duration_hformat_option));
      }

      {
        CreativeOptionDef_var skip_min_ecpm_option = new CreativeOptionDef(
          SKIP_MIN_ECPM_TOKEN,
          'I',
          StringSet());
        skip_min_ecpm_option->timestamp = Generics::Time::ONE_SECOND;
        predefined_options_.insert(std::make_pair(
          SKIP_MIN_ECPM_OPTION_ID,
          skip_min_ecpm_option));
      }

      {
        audience_channel_behav_params_ = new BehavioralParameterListDef();
        audience_channel_behav_params_->threshold = 1;
        audience_channel_behav_params_->timestamp = Generics::Time::ONE_SECOND;

        BehavioralParameterDef bp_def;
        bp_def.min_visits = 1;
        bp_def.time_from = 0;
        bp_def.time_to = audience_expiration_time.tv_sec;
        bp_def.weight = 1;
        bp_def.trigger_type = 'A';
        audience_channel_behav_params_->behave_params.push_back(bp_def);

        audience_channel_behav_params_key_ = generate_key_(
          audience_channel_behav_params_->behave_params);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  CampaignConfigDBSource::~CampaignConfigDBSource() noexcept
  {}

  CampaignConfig_var
  CampaignConfigDBSource::update(bool* need_logging) /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::update_config()";

    CampaignConfig_var new_config;
    UpdatingState_var new_updating_state;

    try
    {
      CampaignConfig_var db_config = new CampaignConfig();
      UpdatingState_var db_updating_state = new UpdatingState();

      if(update_ora_config_(db_config, db_updating_state))
      {
        new_config = db_config;
        new_updating_state = db_updating_state;
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::CRITICAL,
        Aspect::CAMPAIGN_SERVER,
        "ADS-IMPL-149") << FUN <<
        ": Can't receive campaigns config from DB: " << ex.what();
    }

    try
    {
      if(!new_config.in() && campaign_config_.get().in())
      {
        new_config = new CampaignConfig(*campaign_config_.get());
      }

      if(new_config.in())
      {
        campaign_config_modifier_->update(
          new_config, Generics::Time::get_time_of_day());
        campaign_config_ = new_config;
        updating_state_ = new_updating_state;
      }

      if(need_logging)
      {
        *need_logging = true;
      }

      return campaign_config_.get();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  StatSource::CStat_var
  CampaignConfigDBSource::stat() const /*throw(NotReady)*/
  {
    try
    {
      return campaign_config_modifier_->stat();
    }
    catch(const CampaignConfigModifier::NotReady&)
    {
      throw NotReady("");
    }
  }

  CampaignConfigModifier::CState_var
  CampaignConfigDBSource::modify_state() const /*throw(NotReady)*/
  {
    try
    {
      return campaign_config_modifier_->state();
    }
    catch(const CampaignConfigModifier::NotReady&)
    {
      throw NotReady("");
    }
  }

  void
  CampaignConfigDBSource::update_stat() /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::update_stat()";

    try
    {
      campaign_config_modifier_->update_stat(
        campaign_config_.get(),
        Generics::Time::get_time_of_day());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  CampaignConfigDBSource::check_cost_(
    const RevenueDecimal& val,
    const CampaignConfig* config,
    const CurrencyDef* currency)
    noexcept
  {
    if(!currency)
    {
      return val < config->global_params.cost_limit;
    }

    try
    {
      return RevenueDecimal::div(val, currency->rate) <
        config->global_params.cost_limit;
    }
    catch(const RevenueDecimal::Overflow&)
    {
      return false;
    }
  }

  bool
  CampaignConfigDBSource::update_ora_config_(
    CampaignConfig* new_config,
    UpdatingState* new_updating_state)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::update_ora_config_()";

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_SERVER) <<
        FUN << ": to update config from DB.";
    }

    Commons::Postgres::Connection_var conn;
    try
    {
      conn = pg_pool_->get_connection();
    }
    catch(const Commons::Postgres::NotActive& e)
    {
      return false;
    }
    catch(const Commons::Postgres::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": Commons::Postgres::Exception : " << ex.what();
      throw Exception(ostr);
    }

    const CampaignConfig_var old_config = campaign_config_.get();
    const UpdatingState_var old_updating_state = updating_state_;

    TimestampValue sysdate = Generics::Time::get_time_of_day();

    new_config->server_id = server_id_;
    new_config->first_load_stamp = old_config.in() ?
      old_config->first_load_stamp : sysdate;

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      new_config->db_stamp = query_db_stamp_(conn);
      query_global_params_(new_config, conn, old_config.in(), sysdate);
      query_app_formats_(new_config, conn, old_config.in(), sysdate);
      query_sizes_(new_config, conn, old_config.in(), sysdate);
      query_countries_(conn, new_config, old_config.in(), sysdate);
      query_accounts_(conn, new_config, old_config.in(), sysdate);
      query_recursive_tokens_(conn);
      query_creative_options_(
        conn, new_config, old_config.in(), sysdate);
      query_sites_(conn, new_config, old_config.in(), sysdate);
      query_currency_exchange_(conn, new_config);
      query_currencies_(conn, new_config, old_config.in(), sysdate);
      query_tags_(conn, new_config, old_config.in(), sysdate);
      query_colocations_(conn, new_config, old_config.in(), sysdate);
      query_campaigns_(conn, new_config, old_config.in(), sysdate);
      query_geo_channels_(
        conn,
        new_config, new_updating_state,
        old_config.in(), old_updating_state.in(),
        sysdate);
      query_geo_coord_channels_(conn, new_config, old_config, sysdate);
      // use result of query_geo_channels_ and query_campaigns_
      query_channels_(conn, new_config, old_config, sysdate);
      query_freq_caps_(conn, new_config, old_config.in(), sysdate);
      query_creative_templates_(conn, new_config, old_config.in(), sysdate);
      query_creative_categories_(
        conn, old_config.in(), new_config, sysdate);
      query_category_channels_(
        conn, new_config, old_config.in(), sysdate);
      query_adv_actions_(conn, new_config, old_config.in(), sysdate);
      query_simple_channels_(conn, new_config, old_config, sysdate);
      query_simple_channel_triggers_(conn, new_config, old_config, sysdate);
      query_behavioral_parameters_(
        conn, new_config, old_config.in(), sysdate);
      query_fraud_conditions_(
        conn, new_config, old_config.in(), sysdate);
      query_search_engines_(
        conn, new_config, old_config.in(), sysdate);
      query_web_browsers_(
        conn, new_config, old_config.in(), sysdate);
      query_platforms_(
        conn, new_config, old_config.in(), sysdate);
      query_web_operations_(conn, new_config, old_config.in(), sysdate);
    }
    catch(const Exception&)
    {
      pg_pool_->bad_connection(conn);
      throw;
    }

    new_config->detectors_timestamp = new_config->search_engines.max_stamp();
    new_config->detectors_timestamp = std::max(
      new_config->detectors_timestamp,
      new_config->web_browsers.max_stamp());
    new_config->detectors_timestamp = std::max(
      new_config->detectors_timestamp,
      new_config->platforms.max_stamp());

    const Generics::Time now = Generics::Time::get_time_of_day();
    new_config->master_stamp = now;
    new_config->finish_load_stamp = now;

    if (old_config.in() != 0 && new_config->master_stamp < old_config->master_stamp)
    {
      const char TIME_FORMAT[] = "%F %T";

      logger_->sstream(Logging::Logger::WARNING,
        Aspect::CAMPAIGN_SERVER,
        "ADS-IMPL-153") << FUN << ": Old master stamp = " <<
        old_config->master_stamp.get_gm_time().format(TIME_FORMAT) <<
        ", new master stamp = " <<
        new_config->master_stamp.get_gm_time().format(TIME_FORMAT) << ".";
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::CAMPAIGN_SERVER) << FUN <<
        ": from update config from DB: geo channels " <<
        (!old_config || new_config->geo_channels.in() != old_config->geo_channels.in() ?
         "reloaded" : "reused");
    }

    return true;
  }

  Generics::Time CampaignConfigDBSource::query_db_stamp_(
    Commons::Postgres::Connection* conn)
    /*throw(Exception)*/
  {
    try
    {
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement("SELECT transaction_timestamp()::timestamp");
      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      if(rs->next())
      {
        return rs->get_timestamp(1);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw Exception(ostr);
    }
    throw Exception("Cant get timestamp of db");
  }

  void CampaignConfigDBSource::query_global_params_(
    CampaignConfig* new_config,
    Commons::Postgres::Connection* conn,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_global_params_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      static const char* FRAUD_USER_DEACTIVATE_PERIOD = "USER_INACTIVITY_TIMEOUT";
      static const char* COST_LIMIT = "OIX_COST_LIMIT";
      static const char* GOOGLE_PUBLISHER_ACCOUNT_ID = "GOOGLE_PUBLISHER_ACCOUNT_ID";

      typedef std::map<std::string, std::string> GlobalParamMap;
      GlobalParamMap global_params;
      global_params.insert(std::make_pair(FRAUD_USER_DEACTIVATE_PERIOD, ""));
      global_params.insert(std::make_pair(COST_LIMIT, ""));

      std::ostringstream sql_request;
      sql_request <<
        "SELECT param_name, param_value FROM AdsConfig where param_name in (";
      for(GlobalParamMap::const_iterator param_it = global_params.begin();
          param_it != global_params.end(); ++param_it)
      {
        if(param_it != global_params.begin())
        {
          sql_request << ", ";
        }
        sql_request << "'" << param_it->first << "'";
      }
      sql_request << ")";

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(sql_request.str().c_str());
      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        global_params[rs->get_string(1)] = rs->get_string(2);
      }

      {
        const std::string& fraud_user_deactivate_period_s =
          global_params[FRAUD_USER_DEACTIVATE_PERIOD];

        if(!fraud_user_deactivate_period_s.empty())
        {
          unsigned long fraud_user_deactivate_period;
          if(String::StringManip::str_to_int(
               fraud_user_deactivate_period_s,
               fraud_user_deactivate_period))
          {
            new_config->global_params.fraud_user_deactivate_period =
              Generics::Time(fraud_user_deactivate_period);
          }
          else
          {
            Stream::Error ostr;
            ostr << FUN << "Non correct '" <<
              FRAUD_USER_DEACTIVATE_PERIOD << "' value in AdsConfig : '" <<
              fraud_user_deactivate_period_s << "'.";
            throw Exception(ostr);
          }
        }
      }

      {
        new_config->global_params.cost_limit = MAXIMUM_COST_LIMIT;
        const std::string& cost_limit_s = global_params[COST_LIMIT];

        if(!cost_limit_s.empty())
        {
          try
          {
            new_config->global_params.cost_limit =
              RevenueDecimal(cost_limit_s.c_str());
          }
          catch(const RevenueDecimal::Exception&)
          {
            Stream::Error ostr;
            ostr << FUN << "Non correct '" <<
              COST_LIMIT << "' value in AdsConfig : '" <<
              cost_limit_s << "'.";
            throw Exception(ostr);
          }
        }
      }

      {
        const std::string& google_publisher_account_id_s =
          global_params[GOOGLE_PUBLISHER_ACCOUNT_ID];

        if(!google_publisher_account_id_s.empty())
        {
          unsigned long google_publisher_account_id;
          if(String::StringManip::str_to_int(
               google_publisher_account_id_s,
               google_publisher_account_id))
          {
            new_config->global_params.google_publisher_account_id =
              google_publisher_account_id;
          }
          else
          {
            Stream::Error ostr;
            ostr << FUN << "Non correct '" <<
              GOOGLE_PUBLISHER_ACCOUNT_ID << "' value in AdsConfig : '" <<
              google_publisher_account_id_s << "'.";
            throw Exception(ostr);
          }
        }
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }

    if (old_config != 0)
    {
      const GlobalParamsDef& old_gp = old_config->global_params;
      GlobalParamsDef& new_gp = new_config->global_params;

      if (old_gp.currency_exchange_id != new_gp.currency_exchange_id ||
          old_gp.fraud_user_deactivate_period != new_gp.fraud_user_deactivate_period ||
          old_gp.cost_limit != new_gp.cost_limit ||
          old_gp.max_random_cpm != new_gp.max_random_cpm)
      {
        new_gp.timestamp = sysdate;
      }
      else
      {
        new_gp.timestamp = old_gp.timestamp;
      }
    }
    else
    {
      new_config->global_params.timestamp = sysdate;
    }
  }

  void
  CampaignConfigDBSource::query_app_formats_(
    CampaignConfig* new_config,
    Commons::Postgres::Connection* conn,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_app_formats_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      const AppFormatMap* old_app_formats = old_config ? &(old_config->app_formats) : 0;

      enum
      {
        POS_APP_FORMAT_NAME = 1,
        POS_MIME_FORMAT
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT name, mime_type "
          "FROM AppFormat");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        std::string app_format_name = rs->get_string(POS_APP_FORMAT_NAME);
        AppFormatDef_var app_format = new AppFormatDef();
        app_format->mime_format = rs->get_string(POS_MIME_FORMAT);
        new_config->app_formats.activate(app_format_name, app_format, sysdate, old_app_formats);
      }

      if(old_app_formats)
      {
        new_config->app_formats.deactivate_nonactive(*old_app_formats, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignConfigDBSource::query_sizes_(
    CampaignConfig* new_config,
    Commons::Postgres::Connection* conn,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_sizes_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      const SizeMap* old_sizes =
        old_config ? &(old_config->sizes) : 0;

      enum
      {
        POS_SIZE_ID = 1,
        POS_PROTOCOL_NAME,
        POS_SIZE_TYPE_ID,
        POS_WIDTH,
        POS_HEIGHT,
        POS_MAX_WIDTH,
        POS_MAX_HEIGHT
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "size_id,"
            "protocol_name,"
            "size_type_id,"
            "case when coalesce(width, 0) < 0 then 0 "
              "else coalesce(width, 0) end as width,"
            "case when coalesce(height, 0) < 0 then 0 "
              "else coalesce(height, 0) end as height, "
            "case when coalesce(max_width, 0) < 0 then 0 "
              "else coalesce(max_width, 0) end as max_width,"
            "case when coalesce(max_height, 0) < 0 then 0 "
              "else coalesce(max_height, 0) end as max_height "
          "FROM CreativeSize");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        unsigned long size_id = rs->get_number<unsigned long>(POS_SIZE_ID);
        SizeDef_var size = new SizeDef();
        size->protocol_name = rs->get_string(POS_PROTOCOL_NAME);
        size->size_type_id = rs->get_number<unsigned long>(POS_SIZE_TYPE_ID);
        size->width = rs->get_number<unsigned long>(POS_WIDTH);
        size->height = rs->get_number<unsigned long>(POS_HEIGHT);
        size->max_width = rs->get_number<unsigned long>(POS_MAX_WIDTH);
        size->max_height = rs->get_number<unsigned long>(POS_MAX_HEIGHT);

        new_config->sizes.activate(size_id, size, sysdate, old_sizes);
      }

      if(old_sizes)
      {
        new_config->sizes.deactivate_nonactive(*old_sizes, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_accounts_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_accounts_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      const AccountMap* old_accounts = old_config ? &(old_config->accounts) : 0;

      enum
      {
        POS_ACCOUNT_ID = 1,
        POS_AGENCY_ACCOUNT_ID,
        POS_INTERNAL_ACCOUNT_ID,
        POS_ROLE_ID,
        POS_NAME,
        POS_LEGAL_NAME,
        POS_FLAGS,
        POS_AT_FLAGS,
        POS_TEXT_ADSERVING,
        POS_COUNTRY,
        POS_CURRENCY_ID,
        POS_COMMISION,
        POS_MEDIA_HANDLING_FEE,
        POS_BUDGET,
        POS_PAID_AMOUNT,
        POS_TIME_OFFSET,
        POS_AUCTION_RATE,
        POS_USE_PUB_PIXEL,
        POS_PUB_PIXEL_OPTIN,
        POS_PUB_PIXEL_OPTOUT,
        POS_STATUS,
        POS_SELF_SERVICE_COMMISSION
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "ga.account_id,"
            "ga.agency_account_id,"
            "ga.internal_account_id,"
            "ga.role_id,"
            "ga.name,"
            "ga.legal_name,"
            "ga.flags,"
            "ga.at_flags,"
            "ga.text_adserving,"
            "ga.country_code,"
            "ga.currency_id,"
            "ga.commission,"
            "ga.media_handling_fee,"
            "ga.budget,"
            "ga.paid_amount,"
            "ga.time_offset,"
            "ga.auction_rate,"
            "ga.use_pub_pixel,"
            "ga.pub_pixel_optin,"
            "ga.pub_pixel_optout,"
            "ga.status, "
            "jac.self_service_commission "
          "FROM adserver.get_accounts($1) ga JOIN account jac using(account_id)");

      stmt->set_timestamp(1, sysdate - pending_expire_time_);

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        const unsigned long account_id =
          rs->get_number<unsigned long>(POS_ACCOUNT_ID);
        const char cross_status = rs->get_char(POS_STATUS);

        if(campaign_statuses_.empty() ||
          campaign_statuses_.find(cross_status) != std::string::npos)
        {
          AccountDef_var account;

          if(old_accounts)
          {
            AccountMap::ActiveMap::const_iterator acc_it =
              old_accounts->active().find(account_id);

            if(acc_it != old_accounts->active().end())
            {
              account = new AccountDef(*(acc_it->second));
            }
          }

          if(!account.in())
          {
            account = new AccountDef();
          }

          try
          {
            account->account_id = account_id;
            account->agency_account_id =
              rs->get_number<unsigned long>(POS_AGENCY_ACCOUNT_ID);
            account->internal_account_id = 
              rs->get_number<unsigned long>(POS_INTERNAL_ACCOUNT_ID);
            account->role_id = rs->get_number<unsigned long>(POS_ROLE_ID);
            account->legal_name = rs->get_string(POS_LEGAL_NAME);
            account->flags = rs->get_number<unsigned long>(POS_FLAGS);
            account->at_flags = rs->get_number<unsigned long>(POS_AT_FLAGS);
            account->text_adserving = rs->get_char(POS_TEXT_ADSERVING);
            account->country = rs->get_string(POS_COUNTRY);
            account->currency_id = rs->get_number<unsigned long>(POS_CURRENCY_ID);
            account->time_offset =
              Generics::Time(rs->get_number<int>(POS_TIME_OFFSET) * 60);
            try
            {
              if(rs->is_null(POS_BUDGET))
              {
                account->budget = RevenueDecimal::ZERO;
              }
              else
              {
                account->budget = rs->get_decimal<RevenueDecimal>(POS_BUDGET);
              }
            }
            catch(const RevenueDecimal::Overflow&)
            {
              account->budget = RevenueDecimal(false, 1000000000, 0); // To REDO
            }
            account->paid_amount =
              rs->get_decimal<RevenueDecimal>(POS_PAID_AMOUNT);
            account->commision = rs->get_decimal<RevenueDecimal>(POS_COMMISION);
            account->media_handling_fee = rs->get_decimal<RevenueDecimal>(
              POS_MEDIA_HANDLING_FEE);
            if(account->media_handling_fee < RevenueDecimal::ZERO ||
               account->media_handling_fee >= REVENUE_ONE)
            {
              Stream::Error ostr;
              ostr << "account have invalid media_handling_fee = " <<
                account->media_handling_fee;
              throw InvalidObject(ostr);
            }
            account->auction_rate = rs->get_char(POS_AUCTION_RATE) == 'G' ?
              AR_GROSS : AR_NET;
            account->use_pub_pixels = (rs->get_char(POS_USE_PUB_PIXEL) == 'Y');
            account->pub_pixel_optin = rs->get_string(POS_PUB_PIXEL_OPTIN);
            account->pub_pixel_optout = rs->get_string(POS_PUB_PIXEL_OPTOUT);
            if(rs->is_null(POS_SELF_SERVICE_COMMISSION))
            {
              account->self_service_commission = RevenueDecimal::ZERO;
            }
            else
            {
              account->self_service_commission = rs->get_decimal<RevenueDecimal>(POS_SELF_SERVICE_COMMISSION);
            }
            account->status = cross_status;

            new_config->accounts.activate(
              account_id, account, sysdate, old_accounts);
          }
          catch(const eh::Exception& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": ignory account_id = " << account_id <<
              ", at filling caught eh::Exception: " << ex.what();
            logger_->log(
              ostr.str(),
              Logging::Logger::WARNING,
              Aspect::TRAFFICKING_PROBLEM,
              "ADS-TF-4");
          }
        } // status check
      }

      if(old_accounts)
      {
        new_config->accounts.deactivate_nonactive(*old_accounts, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // collect walled garden account links
    try
    {
      enum
      {
        POS_PUB_ACCOUNT_ID = 1,
        POS_ACCOUNT_ID
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT pub_account_id, agency_account_id "
        "FROM WalledGarden "
        "ORDER BY pub_account_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      AccountIdSet cur_walled_garden_accounts;
      bool rs_next = rs->next();
      unsigned long cur_account_id =
        rs_next ? rs->get_number<unsigned long>(POS_PUB_ACCOUNT_ID) : 0;

      for(AccountMap::ActiveMap::iterator ait =
            new_config->accounts.active().begin();
          ait != new_config->accounts.active().end(); )
      {
        if(!rs_next || ait->first < cur_account_id)
        {
          if(ait->second->media_handling_fee != RevenueDecimal::ZERO)
          {
            // push media handling fee as walled_garden_accounts
            // by compatibility reasons (3.5.5)
            uint32_t media_handling_fee_mediator = 4000000000 + RevenueDecimal::mul(
              ait->second->media_handling_fee,
              RevenueDecimal(false, 100000000, 0),
              Generics::DMR_CEIL).integer<uint32_t>();
            cur_walled_garden_accounts.insert(media_handling_fee_mediator);
          }

          if(ait->second->walled_garden_accounts.size() !=
               cur_walled_garden_accounts.size() ||
             !std::equal(cur_walled_garden_accounts.begin(),
               cur_walled_garden_accounts.end(),
               ait->second->walled_garden_accounts.begin()))
          {
            AccountDef_var account(new AccountDef(*(ait->second)));
            account->walled_garden_accounts.swap(cur_walled_garden_accounts);
            account->timestamp = sysdate;
            ait->second = account;
          }

          cur_walled_garden_accounts.clear();
          ++ait;
        }
        else
        {
          // rs_next is true
          if(ait->first == cur_account_id)
          {
            cur_walled_garden_accounts.insert(
              rs->get_number<unsigned long>(POS_ACCOUNT_ID));
          }

          rs_next = rs->next();
          cur_account_id = rs_next ?
            rs->get_number<unsigned long>(POS_PUB_ACCOUNT_ID) : 0;
        }
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at WalledGarden query: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_recursive_tokens_(
    Commons::Postgres::Connection* conn)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_recursive_tokens_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);
    try
    {
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
          "name, "
          "tokens "
          "FROM recursivetokens");
      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      enum
      {
        POS_NAME = 1,
        POS_TOKENS
      };

      while(rs->next())
      {
        std::string name = rs->get_string(POS_NAME);
        StringSet* tokens;
        if(name == "GENERIC")
        {
          tokens = &generic_tokens_;
        }
        else if(name == "ADVERTISER")
        {
          tokens = &advertiser_tokens_;
        }
        else if(name == "PUBLISHER")
        {
          tokens = &publisher_tokens_;
        }
        else if(name == "INTERNAL")
        {
          tokens = &internal_tokens_;
        }
        else
        {
          logger_->stream(
            Logging::Logger::WARNING,
            Aspect::TRAFFICKING_PROBLEM,
            "ADS-TF-4") <<
            "Unknown group of tokens '" << name << "', ignory it";
          continue;
        }
        std::string token_names = rs->get_string(POS_TOKENS);
        String::StringManip::SplitComma tokenizer(token_names);
        String::SubString value;
        while(tokenizer.get_token(value))
        {
          String::StringManip::trim(value);
          std::string tok = value.str();
          String::AsciiStringManip::to_upper(tok);
          tokens->insert(tok);
        }
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_creative_options_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_creative_options_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      CreativeOptionMap& creative_options = new_config->creative_options;

      enum
      {
        POS_OPTION_ID = 1,
        POS_TOKEN,
        POS_TYPE,
        POS_GROUP_TYPE,
        POS_TOKEN_RELATION_FLAGS
      };

      struct TokenMask
      {
        unsigned long mask;
        const StringSet* tokens;
      };

      const TokenMask TOKEN_RELATION_MASKS[] =
      {
        { 0x01, &generic_tokens_ },
        { 0x02, &advertiser_tokens_ },
        { 0x04, &publisher_tokens_ },
        { 0x08, &internal_tokens_ },
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "o.option_id, "
            "o.token, "
            "o.type, "
            "og.type, "
            "coalesce(o.recursive_tokens, 0) as recursive_tokens "
          "FROM Options o "
            "INNER JOIN OptionGroup og ON("
              "o.option_group_id = og.option_group_id)");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        StringSet token_relations;
        std::string token = rs->get_string(POS_TOKEN);
        unsigned long token_relation_flags =
          rs->get_number<unsigned long>(POS_TOKEN_RELATION_FLAGS);
        for(size_t i = 0; i < sizeof(TOKEN_RELATION_MASKS) /
              sizeof(TOKEN_RELATION_MASKS[0]); ++i)
        {
          if(token_relation_flags & TOKEN_RELATION_MASKS[i].mask)
          {
            std::copy(
              TOKEN_RELATION_MASKS[i].tokens->begin(),
              TOKEN_RELATION_MASKS[i].tokens->end(),
              std::inserter(token_relations, token_relations.end()));
          }
        }

        std::string rs_type = rs->get_string(POS_TYPE);
        String::AsciiStringManip::to_lower(rs_type);
        char type = rs->get_string(POS_GROUP_TYPE)[0] == 'A' ?
           (token == "CRHTML" ? 'D' : NAME_ADV_OPTION_TYPE_CONVERTER(rs_type)) :
           NAME_PUB_OPTION_TYPE_CONVERTER(rs_type);

        CreativeOptionDef_var creative_option = new CreativeOptionDef(
          token.c_str(),
          type,
          token_relations);

        creative_options.activate(
          rs->get_number<unsigned long>(POS_OPTION_ID),
          creative_option,
          sysdate,
          old_config ? &old_config->creative_options : 0);
      }

      for(CreativeOptionMap::ActiveMap::const_iterator opt_it =
            predefined_options_.begin();
          opt_it != predefined_options_.end(); ++opt_it)
      {
        creative_options.activate(opt_it->first, opt_it->second);
      }

      if(old_config)
      {
        creative_options.deactivate_nonactive(
          old_config->creative_options, sysdate);
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_sites_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_sites_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    const SiteMap* old_sites = old_config ? &(old_config->sites) : 0;
    SiteMap& sites = config->sites;

    try
    {
      /* query sites */
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "site_id, "
            "freq_cap_id, "
            "no_ads_timeout, "
            "flags, "
            "account_id "
          "FROM adserver.query_sites() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      enum
      {
        POS_SITE_ID = 1,
        POS_FREQ_CAP,
        POS_NOADS_TIMEOUT,
        POS_FLAGS,
        POS_ACCOUNT_ID
      };

      while(rs->next())
      {
        unsigned long site_id = rs->get_number<unsigned long>(POS_SITE_ID);
        SiteDef_var site;

        if(old_sites)
        {
          SiteMap::ActiveMap::const_iterator old_sit =
            old_sites->active().find(site_id);
          if(old_sit != old_sites->active().end())
          {
            site = new SiteDef(*(old_sit->second));
          }
        }

        if(!site.in())
        {
          site = new SiteDef();
        }

        site->status = 'A';
        site->freq_cap_id = rs->get_number<unsigned long>(POS_FREQ_CAP);
        site->noads_timeout = rs->get_number<unsigned long>(POS_NOADS_TIMEOUT);
        site->flags = rs->get_number<unsigned long>(POS_FLAGS);
        site->account_id = rs->get_number<unsigned long>(POS_ACCOUNT_ID);
        sites.activate(site_id, site, sysdate, old_sites);
      }

      if(old_sites)
      {
        sites.deactivate_nonactive(*old_sites, sysdate);
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at sites query: " << e.what();
      throw Exception(ostr);
    }

    try
    {
      enum
      {
        POS_SITE_ID = 1,
        POS_CREATIVE_CATEGORY_ID,
        POS_APPROVAL
      };

      /* query site approved & rejected creative categories */
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "scce.site_id, "
          "scce.creative_category_id, "
          "scce.approval "
        "FROM SiteCreativeCategoryExclusion scce "
        "ORDER BY scce.site_id, scce.creative_category_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      CreativeCategoryIdList cur_approved_creative_categories;
      CreativeCategoryIdList cur_rejected_creative_categories;
      bool rs_next = rs->next();
      unsigned long cur_site_id =
        rs_next ? rs->get_number<unsigned long>(POS_SITE_ID) : 0;

      for(SiteMap::ActiveMap::iterator sit =
            config->sites.active().begin();
          sit != config->sites.active().end(); )
      {
        if(!rs_next || sit->first < cur_site_id)
        {
          // check approved/rejected creative category sets equality
          if(sit->second->approved_creative_categories.size() !=
               cur_approved_creative_categories.size() ||
             sit->second->rejected_creative_categories.size() !=
               cur_rejected_creative_categories.size() ||
             !std::equal(cur_approved_creative_categories.begin(),
               cur_approved_creative_categories.end(),
               sit->second->approved_creative_categories.begin()) ||
             !std::equal(cur_rejected_creative_categories.begin(),
               cur_rejected_creative_categories.end(),
               sit->second->rejected_creative_categories.begin()))
          {
            SiteDef_var site = new SiteDef(*(sit->second));
            site->approved_creative_categories.swap(cur_approved_creative_categories);
            site->rejected_creative_categories.swap(cur_rejected_creative_categories);
            site->timestamp = sysdate;
            sit->second = site;
          }

          cur_approved_creative_categories.clear();
          cur_rejected_creative_categories.clear();
          ++sit;
        }
        else
        {
          // rs_next is true
          if(sit->first == cur_site_id)
          {
            unsigned long creative_category_id =
              rs->get_number<unsigned long>(POS_CREATIVE_CATEGORY_ID);
            char approval = rs->get_char(POS_APPROVAL);

            if(approval == 'P')
            {
              cur_approved_creative_categories.push_back(
                creative_category_id);
            }
            else if(approval == 'R')
            {
              cur_rejected_creative_categories.push_back(
                creative_category_id);
            }
            else if(approval != 'A')
            {
              Stream::Error ostr;
              ostr << FUN <<
                ": Creative category exclusion record (site_id = " <<
                cur_site_id << ", cr_cat_id = " << creative_category_id <<
                ") has unknown approval value '" << approval << "'.";
              throw Exception(ostr);
            }
          }

          rs_next = rs->next();
          cur_site_id =
            rs_next ? rs->get_number<unsigned long>(POS_SITE_ID) : 0;
        }
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at "
        "SiteCreativeCategoryExclusion query: " << e.what();
      throw Exception(ostr);
    }

    try
    {
      enum
      {
        POS_SITE_ID = 1,
        POS_CREATIVE_ID,
        POS_APPROVAL_ID
      };

      /* query site approved campaigns */
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT site_id, creative_id, approval "
        "FROM SiteCreativeApproval sca "
        "ORDER BY sca.site_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      CreativeIdSet cur_approved_creatives;
      CreativeIdSet cur_rejected_creatives;
      bool rs_next = rs->next();
      unsigned long cur_site_id =
        rs_next ? rs->get_number<unsigned long>(POS_SITE_ID) : 0;

      for(SiteMap::ActiveMap::iterator sit =
            config->sites.active().begin();
          sit != config->sites.active().end(); )
      {
        if(!rs_next || sit->first < cur_site_id)
        {
          // check approved campaign sets equality
          if(sit->second->approved_creatives.size() !=
               cur_approved_creatives.size() ||
             !std::equal(cur_approved_creatives.begin(),
               cur_approved_creatives.end(),
               sit->second->approved_creatives.begin()))
          {
            SiteDef_var site = new SiteDef(*(sit->second));
            site->approved_creatives.swap(cur_approved_creatives);
            site->timestamp = sysdate;
            sit->second = site;
          }

          if(sit->second->rejected_creatives.size() !=
              cur_rejected_creatives.size() ||
             !std::equal(cur_rejected_creatives.begin(),
               cur_rejected_creatives.end(),
               sit->second->rejected_creatives.begin()))
          {
            SiteDef_var site = new SiteDef(*(sit->second));
            site->rejected_creatives.swap(cur_rejected_creatives);
            site->timestamp = sysdate;
            sit->second = site;
          }

          cur_approved_creatives.clear();
          cur_rejected_creatives.clear();
          ++sit;
        }
        else
        {
          // rs_next is true
          if(sit->first == cur_site_id)
          {
            const unsigned long creative_id =
              rs->get_number<unsigned long>(POS_CREATIVE_ID);
            const char approval = rs->get_char(POS_APPROVAL_ID);

            if (approval == 'A')
            {
              cur_approved_creatives.insert(creative_id);
            }
            else if (approval == 'R' || approval == 'P')
            {
              cur_rejected_creatives.insert(creative_id);
            }
          }

          rs_next = rs->next();
          cur_site_id =
            rs_next ? rs->get_number<unsigned long>(POS_SITE_ID) : 0;
        }
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at SiteCreativeApproval query: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_currencies_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_currencies_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      /* query currencies */
      {
        enum
        {
          POS_CURRENCY_ID = 1,
          POS_CURRENCY_EXCHANGE_ID,
          POS_RATE,
          POS_EFFECTIVE_DATE,
          POS_FRACTION_DIGITS,
          POS_CURRENCY_CODE
        };

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
            "SELECT "
              "currency_id, currency_exchange_id, rate, "
              "effective_date, fraction_digits, currency_code "
            "FROM adserver.query_currencies($1)");

        stmt->set_value(1, new_config->global_params.currency_exchange_id);

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

        while (rs->next())
        {
          unsigned long currency_id =
            rs->get_number<unsigned long>(POS_CURRENCY_ID);

          CurrencyDef_var currency(new CurrencyDef());
          currency->currency_id = currency_id;
          currency->currency_exchange_id =
            rs->get_number<unsigned long>(POS_CURRENCY_EXCHANGE_ID);
          currency->rate = rs->get_decimal<RevenueDecimal>(POS_RATE);
          currency->effective_date = rs->get_date(POS_EFFECTIVE_DATE).tv_sec;
          currency->fraction_digits =
            rs->get_number<unsigned long>(POS_FRACTION_DIGITS);
          currency->currency_code = rs->get_string(POS_CURRENCY_CODE);

          String::AsciiStringManip::to_lower(currency->currency_code);

          new_config->currencies.activate(
            currency_id,
            currency,
            sysdate,
            old_config ? &(old_config->currencies) : 0);
        }
      }

      if(old_config)
      {
        new_config->currencies.deactivate_nonactive(
          old_config->currencies,
          sysdate);
      }
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_tags_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_tags_()";

    typedef std::map<unsigned long, std::string> BlockTagMap;

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      const TagMap* old_tags = old_config ? &(old_config->tags) : 0;

      // query tags
      {
        enum
        {
          POS_SITE_ID = 1,
          POS_TAG_ID,
          POS_PASSBACK,
          POS_PASSBACK_TYPE,
          POS_FLAGS,
          POS_MARKETPLACE,
//          POS_ADJUSTMENT,
          POS_ALLOW_EXPANDABLE,
          POS_AUCTION_MAX_ECPM_SHARE,
          POS_AUCTION_PROP_PROBABILITY_SHARE,
          POS_AUCTION_RANDOM_SHARE,
          POS_CROSS_STATUS,
          POS_COST_COEF
        };

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
            "SELECT "
              "site_id,"
              "tag_id,"
              "passback,"
              "passback_type,"
              "flags,"
              "marketplace,"
//              "adjustment,"
              "allow_expandable,"
              "max_ecpm_share,"
              "prop_probability_share,"
              "random_share,"
              "cross_status, "
              "cost_coef "
            "FROM adserver.get_tag_ctr_adjustment()");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

        while (rs->next())
        {
          char cross_status = rs->get_char(POS_CROSS_STATUS);
          unsigned long tag_id = rs->get_number<unsigned long>(POS_TAG_ID);

          if (cross_status != 'D' && cross_status != 'I')
          {
            TagDef_var tag;

            if(old_config)
            {
              TagMap::ActiveMap::const_iterator old_tag_it =
                old_config->tags.active().find(tag_id);
              if(old_tag_it != old_config->tags.active().end())
              {
                tag = new TagDef(*(old_tag_it->second));
              }
            }

            if(!tag.in())
            {
              tag = new TagDef();
            }

            tag->tag_id = tag_id;
            tag->site_id = rs->get_number<unsigned long>(POS_SITE_ID);

            if(!rs->is_null(POS_PASSBACK))
            {
              std::string passback_url_str = rs->get_string(POS_PASSBACK);
              if (HTTP::HTTP_BEGIN.start(passback_url_str) ||
                HTTP::HTTPS_BEGIN.start(passback_url_str))
              {
                try
                {
                  HTTP::BrowserAddress passback_url(passback_url_str);
                  passback_url.get_view(
                    HTTP::HTTPAddress::VW_FULL,
                    tag->passback);
                }
                catch(const eh::Exception& ex)
                {
                  Stream::Error ostr;
                  ostr << "Tag #" << tag_id << " have invalid passback url '" <<
                    passback_url_str << "', url will be ignored : " <<
                    ex.what();

                  logger_->log(
                    ostr.str(),
                    Logging::Logger::WARNING,
                    Aspect::TRAFFICKING_PROBLEM,
                    "ADS-TF-4");
                }
              }
              else
              {
                tag->passback = passback_url_str;
              }
            }
            else
            {
              tag->passback.clear();
            }

            std::string passback_type = rs->get_string(POS_PASSBACK_TYPE);
            String::AsciiStringManip::to_lower(passback_type);
            if(passback_type.compare(0, 4, "html") == 0)
            {
              tag->passback_type = "html";
            }
            else if(passback_type.compare(0, 2, "js") == 0)
            {
              tag->passback_type = "js";
            }
            else
            {
              tag->passback_type = passback_type;
            }
            tag->flags =!rs->is_null(POS_FLAGS) ?
              rs->get_number<unsigned long>(POS_FLAGS) : 0;
            tag->marketplace = rs->get_string(POS_MARKETPLACE)[0];
            tag->adjustment = REVENUE_ONE;
            if(tag->adjustment < RevenueDecimal::ZERO)
            {
              logger_->sstream(Logging::Logger::ERROR,
                Aspect::CAMPAIGN_SERVER,
               "ADS-IMPL-160") <<
               "tag with id = " << tag_id <<
               " have negative adjustment value = " << tag->adjustment;

              tag->adjustment = RevenueDecimal::ZERO;
            }

            tag->allow_expandable = (rs->get_char(POS_ALLOW_EXPANDABLE) == 'Y');
            tag->auction_max_ecpm_share = rs->get_decimal<RevenueDecimal>(
              POS_AUCTION_MAX_ECPM_SHARE);
            tag->auction_prop_probability_share = rs->get_decimal<RevenueDecimal>(
              POS_AUCTION_PROP_PROBABILITY_SHARE);
            tag->auction_random_share = rs->get_decimal<RevenueDecimal>(
              POS_AUCTION_RANDOM_SHARE);
            tag->cost_coef = rs->get_decimal<RevenueDecimal>(POS_COST_COEF);

            new_config->tags.activate(tag_id, tag, sysdate, old_tags);
          }
          else
          {
            new_config->tags.deactivate(tag_id, sysdate, old_tags);
          }
        }
      }

      // query tag sizes and options
      {
        enum
        {
          POS_TAG_ID = 1,
          POS_SIZE_ID,
          POS_OPTION_ID,
          POS_MAX_TEXT_CREATIVES_MARKER,
          POS_OPTION_VALUE,
          POS_IS_HIDDEN
        };

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
            "SELECT "
              "tag_id,"
              "size_id,"
              "option_id,"
              "max_text_creatives_marker, "
              "option_value, "
              "0 is_hidden " // TO FIX after PGDB-1896
            "FROM adserver.query_tagsizes_options()");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

        bool rs_next = rs->next();
        unsigned long cur_tag_id =
          rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;

        TagDef::SizeMap cur_sizes;

        for(TagMap::ActiveMap::iterator tag_it =
              new_config->tags.active().begin();
            tag_it != new_config->tags.active().end(); )
        {
          if(!rs_next || tag_it->first < cur_tag_id)
          {
            if(cur_sizes.size() != tag_it->second->sizes.size() ||
               !std::equal(cur_sizes.begin(),
                 cur_sizes.end(),
                 tag_it->second->sizes.begin(),
                 Algs::PairEqual()))
            {
              TagDef_var tag(new TagDef(*(tag_it->second)));
              tag->timestamp = sysdate;
              tag->sizes.swap(cur_sizes);
              tag_it->second = tag;
            }

            cur_sizes.clear();
            ++tag_it;
          }
          else
          {
            if(tag_it->first == cur_tag_id)
            {
              unsigned long size_id =
                rs->get_number<unsigned long>(POS_SIZE_ID);
              unsigned long is_max_text_creatives =
                rs->get_number<unsigned long>(POS_MAX_TEXT_CREATIVES_MARKER);
              std::string option_value = rs->get_string(POS_OPTION_VALUE);

              TagDef::Size& res_size = cur_sizes[size_id];

              if(!rs->is_null(POS_OPTION_ID))
              {
                if(rs->get_number<unsigned long>(POS_IS_HIDDEN) > 0)
                {
                  res_size.hidden_tokens.insert(std::make_pair(
                    rs->get_number<int>(POS_OPTION_ID), option_value));
                }
                else
                {
                  res_size.tokens.insert(std::make_pair(
                    rs->get_number<int>(POS_OPTION_ID), option_value));
                }
              }

              if(is_max_text_creatives)
              {
                String::StringManip::str_to_int(
                  option_value, res_size.max_text_creatives);
              }
            }

            rs_next = rs->next();
            cur_tag_id =
              rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
          }
        }
      }

      // query tag creative category exclusion
      {
        enum
        {
          POS_CREATIVE_CATEGORY_ID = 1,
          POS_TAG_ID,
          POS_APPROVAL
        };

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
            "SELECT DISTINCT creative_category_id, tag_id, approval "
            "FROM TagsCreativeCategoryExclusion "
            "ORDER BY tag_id, creative_category_id");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
        bool rs_next = rs->next();
        unsigned long cur_tag_id =
          rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
        CreativeCategorySet cur_accepted_categories;
        CreativeCategorySet cur_rejected_categories;

        for(TagMap::ActiveMap::iterator tag_it =
              new_config->tags.active().begin();
            tag_it != new_config->tags.active().end(); )
        {
          if(!rs_next || tag_it->first < cur_tag_id)
          {
            if(cur_accepted_categories.size() !=
                 tag_it->second->accepted_categories.size() ||
               cur_rejected_categories.size() !=
                 tag_it->second->rejected_categories.size() ||
               !std::equal(cur_accepted_categories.begin(),
                 cur_accepted_categories.end(),
                 tag_it->second->accepted_categories.begin()) ||
               !std::equal(cur_rejected_categories.begin(),
                 cur_rejected_categories.end(),
                 tag_it->second->rejected_categories.begin()))
            {
              TagDef_var tag(new TagDef(*(tag_it->second)));
              tag->timestamp = sysdate;
              tag->accepted_categories.swap(cur_accepted_categories);
              tag->rejected_categories.swap(cur_rejected_categories);
              tag_it->second = tag;
            }

            cur_accepted_categories.clear();
            cur_rejected_categories.clear();
            ++tag_it;
          }
          else
          {
            if(tag_it->first == cur_tag_id)
            {
              char status = rs->get_char(POS_APPROVAL);
              unsigned long category_id =
                rs->get_number<unsigned long>(POS_CREATIVE_CATEGORY_ID);
              if(status == 'A')
              {
                cur_accepted_categories.insert(category_id);
              }
              else
              {
                cur_rejected_categories.insert(category_id);
              }
            }

            rs_next = rs->next();
            cur_tag_id =
              rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
          }
        }
      }

      BlockTagMap blocked_tags;

      // query tag pricings
      {
        enum
        {
          POS_TAG_ID = 1,
          POS_SITE_RATE_ID,
          POS_COUNTRY_CODE,
          POS_CCG_TYPE,
          POS_CCG_RATE_TYPE,
          POS_TAG_RATE_TYPE,
          POS_IMP_REVENUE,
          POS_CURRENCY_ID
        };

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
            "SELECT "
              "tag_id, "
              "site_rate_id, "
              "country_code, "
              "ccg_type, "
              "ccg_rate_type, "
              "rate_type, "
              "imp_revenue, "
              "currency_id "
            "FROM adserver.query_tag_pricings() ");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
        bool rs_next = rs->next();
        unsigned long cur_tag_id =
          rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
        TagPricings cur_tag_pricings;
        CurrencyDef_var cur_currency;

        for(TagMap::ActiveMap::iterator tag_it =
              new_config->tags.active().begin();
            tag_it != new_config->tags.active().end(); )
        {
          if(!rs_next || tag_it->first < cur_tag_id)
          {
            if(cur_tag_pricings.size() !=
                 tag_it->second->tag_pricings.size() ||
               !std::equal(cur_tag_pricings.begin(),
                 cur_tag_pricings.end(),
                 tag_it->second->tag_pricings.begin()))
            {
              bool default_tp_found = false;

              for(TagPricings::const_iterator tp_it = cur_tag_pricings.begin();
                  tp_it != cur_tag_pricings.end(); ++tp_it)
              {
                if(tp_it->country_code.empty())
                {
                  default_tp_found = true;
                  break;
                }
              }

              if(default_tp_found)
              {
                TagDef_var tag(new TagDef(*(tag_it->second)));
                tag->tag_pricings_timestamp = sysdate;
                tag->timestamp = sysdate;
                tag->tag_pricings.swap(cur_tag_pricings);
                tag_it->second = tag;
              }
              else
              {
                blocked_tags[cur_tag_id] += "default pricing isn't defined; ";
              }
            }

            cur_tag_pricings.clear();
            cur_currency.reset();
            ++tag_it;
          }
          else
          {
            if(tag_it->first == cur_tag_id)
            {
              if(!cur_currency.in())
              {
                CurrencyMap::ActiveMap::const_iterator currency_it =
                  new_config->currencies.active().find(
                    rs->get_number<unsigned long>(POS_CURRENCY_ID));
                if(currency_it != new_config->currencies.active().end())
                {
                  cur_currency = currency_it->second;
                }
              }

              if(cur_currency.in())
              {
                TagPricingDef tag_pricing;

                tag_pricing.site_rate_id =
                  rs->get_number<unsigned long>(POS_SITE_RATE_ID);
                tag_pricing.country_code = rs->get_string(POS_COUNTRY_CODE);

                if(!rs->is_null(POS_CCG_TYPE))
                {
                  tag_pricing.ccg_type = static_cast<CCGType>(rs->get_char(POS_CCG_TYPE));
                }
                else
                {
                  tag_pricing.ccg_type = CT_ALL;
                }

                tag_pricing.ccg_rate_type = CR_ALL;


                if(!rs->is_null(POS_CCG_RATE_TYPE))
                {
                  std::string str = rs->get_string(POS_CCG_RATE_TYPE);
                  if(str.size() == 3 && str[0] == 'C' && str[1] == 'P')
                  {
                    tag_pricing.ccg_rate_type = static_cast<CCGRateType>(str[2]);
                  }
                }

                if(rs->get_string(POS_TAG_RATE_TYPE) == "CPM")
                {
                  tag_pricing.revenue_share = RevenueDecimal::ZERO;
                  try
                  {
                    tag_pricing.imp_revenue = rs->get_decimal<RevenueDecimal>(
                      POS_IMP_REVENUE);

                    if(!check_cost_(tag_pricing.imp_revenue, new_config, cur_currency))
                    {
                      std::ostringstream block_reason_ostr;
                      block_reason_ostr << "big imp revenue = " <<
                        tag_pricing.imp_revenue.str() << "; ";

                      blocked_tags[cur_tag_id] += block_reason_ostr.str();
                    }
                  }
                  catch(const RevenueDecimal::Exception& ex)
                  {
                    std::ostringstream block_reason_ostr;
                    block_reason_ostr << "very big imp revenue = " <<
                      rs->get_string(POS_IMP_REVENUE) << "; ";

                    blocked_tags[cur_tag_id] += block_reason_ostr.str();
                  }
                }
                else //revenue share
                {
                  tag_pricing.revenue_share = rs->get_decimal<RevenueDecimal>(
                    POS_IMP_REVENUE);
                  tag_pricing.imp_revenue = RevenueDecimal::ZERO;
                }
                cur_tag_pricings.push_back(tag_pricing);
              }
            }

            rs_next = rs->next();
            cur_tag_id =
              rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
          }
        }
      }

      if(!blocked_tags.empty())
      {
        Stream::Error ostr;

        ostr << "Next tags will be ignored: ";
        for(BlockTagMap::const_iterator blocked_tag_it =
              blocked_tags.begin();
            blocked_tag_it != blocked_tags.end(); ++blocked_tag_it)
        {
          TagMap::ActiveMap::const_iterator dtag_it =
            new_config->tags.active().find(blocked_tag_it->first);
          if(dtag_it != new_config->tags.active().end())
          {
            TagDef_var cleared_tag = new TagDef(*(dtag_it->second));
            cleared_tag->tag_pricings.clear();
            new_config->tags.activate(
              dtag_it->first,
              cleared_tag,
              sysdate,
              old_config ? &old_config->tags : 0);
          }
          ostr << "(#" << blocked_tag_it->first << ": " << blocked_tag_it->second << ")";
        }

        logger_->log(
          ostr.str(),
          Logging::Logger::WARNING,
          Aspect::TRAFFICKING_PROBLEM,
          "ADS-TF-4");
      }

      query_tag_option_values_(conn, new_config, sysdate);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_tag_option_values_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "CampaignConfigDBSource::query_tag_option_values_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_TAG_ID = 1,
        POS_OPTION_ID,
        POS_TOKEN_VALUE
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT tag_id, option_id, value_ "
          "FROM "
          "adserver.query_tag_option_values() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_tag_id =
        rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
      OptionValueMap cur_tokens;

      for (TagMap::ActiveMap::iterator tag_it =
             config->tags.active().begin();
           tag_it != config->tags.active().end(); )
      {
        if(!rs_next || tag_it->first < cur_tag_id)
        {
          if(tag_it->second->tokens.size() != cur_tokens.size() ||
             !std::equal(cur_tokens.begin(),
               cur_tokens.end(),
               tag_it->second->tokens.begin(),
               Algs::PairEqual()))
          {
            TagDef_var new_tag(new TagDef(*(tag_it->second)));
            new_tag->tokens.swap(cur_tokens);
            new_tag->timestamp = sysdate;
            tag_it->second = new_tag;
          }

          cur_tokens.clear();
          ++tag_it;
        }
        else
        {
          if(tag_it->first == cur_tag_id)
          {
            cur_tokens.insert(std::make_pair(
              rs->get_number<int>(POS_OPTION_ID),
              rs->get_string(POS_TOKEN_VALUE)));
          }

          rs_next = rs->next();
          cur_tag_id = rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // query passback tokens
    try
    {
      enum
      {
        POS_TAG_ID = 1,
        POS_PASSBACK_CODE,
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT tag_id, passback_code "
          "FROM Tags "
          "ORDER BY tag_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_tag_id =
        rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
      OptionValueMap cur_tokens;

      for (TagMap::ActiveMap::iterator tag_it =
             config->tags.active().begin();
           tag_it != config->tags.active().end(); )
      {
        if(!rs_next || tag_it->first < cur_tag_id)
        {
          if(tag_it->second->tokens.size() != cur_tokens.size() ||
             !std::equal(cur_tokens.begin(),
               cur_tokens.end(),
               tag_it->second->passback_tokens.begin(),
               Algs::PairEqual()))
          {
            TagDef_var new_tag(new TagDef(*(tag_it->second)));
            new_tag->passback_tokens.swap(cur_tokens);
            new_tag->timestamp = sysdate;
            tag_it->second = new_tag;
          }

          cur_tokens.clear();
          ++tag_it;
        }
        else
        {
          if(tag_it->first == cur_tag_id)
          {
            if(!rs->is_null(POS_PASSBACK_CODE))
            {
              cur_tokens.insert(std::make_pair(
                TAG_PASSBACK_CODE_OPTION_ID,
                rs->get_string(POS_PASSBACK_CODE)));
            }
          }

          rs_next = rs->next();
          cur_tag_id = rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // query template specific options
    try
    {
      enum
      {
        POS_TAG_ID = 1,
        POS_TEMPLATE_NAME,
        POS_OPTION_ID,
        POS_TOKEN_VALUE
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "tov.tag_id,"
            "templ.name,"
            "o.option_id,"
            "tov.value "
          "FROM TagOptionValue tov "
            "INNER JOIN Options o ON(tov.option_id = o.option_id) "
            "INNER JOIN OptionGroup og ON("
              "og.option_group_id = o.option_group_id) "
            "INNER JOIN Template templ ON(templ.template_id = og.template_id) "
          "WHERE og.type = 'Publisher' "
          "ORDER BY tov.tag_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_tag_id =
        rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
      TemplateOptionValueMap cur_template_tokens;

      for (TagMap::ActiveMap::iterator tag_it =
             config->tags.active().begin();
           tag_it != config->tags.active().end(); )
      {
        if(!rs_next || tag_it->first < cur_tag_id)
        {
          if(tag_it->second->template_tokens.size() != cur_template_tokens.size() ||
             !std::equal(cur_template_tokens.begin(),
               cur_template_tokens.end(),
               tag_it->second->template_tokens.begin()))
          {
            TagDef_var new_tag(new TagDef(*(tag_it->second)));
            new_tag->template_tokens.swap(cur_template_tokens);
            new_tag->timestamp = sysdate;
            tag_it->second = new_tag;
          }

          cur_template_tokens.clear();
          ++tag_it;
        }
        else
        {
          if(tag_it->first == cur_tag_id)
          {
            cur_template_tokens[rs->get_string(POS_TEMPLATE_NAME)].insert(
              std::make_pair(
                rs->get_number<int>(POS_OPTION_ID),
                rs->get_string(POS_TOKEN_VALUE)));
          }

          rs_next = rs->next();
          cur_tag_id = rs_next ? rs->get_number<unsigned long>(POS_TAG_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception on templates options query: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_countries_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_countries_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      const CountryMap* old_countries =
        old_config ? &(old_config->countries) : 0;
      
      enum
      {
        POS_COUNTRY_CODE = 1,
        POS_FOOTER_URL
      };
      
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "lower(country_code), "
            "ad_footer_url "
          "FROM Country");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        std::string country_code = rs->get_string(POS_COUNTRY_CODE);
        CountryDef_var country(new CountryDef());

        if(!rs->is_null(POS_FOOTER_URL))
        {
          country->tokens.insert(
            std::make_pair(
              FOOTER_URL_OPTION_ID,
              rs->get_string(POS_FOOTER_URL)));
        }

        new_config->countries.activate(
          country_code,
          country,
          sysdate,
          old_countries);
      }
      
      if(old_countries)
      {
        new_config->countries.deactivate_nonactive(
          *old_countries, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
    
  }

  void CampaignConfigDBSource::query_colocations_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_colocations_()";

    static const char* CS_ONLY_OPTIN_STR = "OPTIN_ONLY";
    static const char* CS_NON_OPTOUT_STR = "NON_OPTOUT";
    static const char* CS_NONE_STR = "NONE";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      const ColocationMap* old_colocations =
        old_config ? &(old_config->colocations) : 0;

      enum
      {
        POS_COLO_ID = 1,
        POS_COLO_RATE_ID,
        POS_ACCOUNT_ID,
        POS_ACCOUNT_FLAGS,
        POS_REVENUE_SHARE,
        POS_AD_SERVING,
        POS_COLO_NAME,
        POS_HID_PROFILE,
        POS_FOOTER_URL
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "colo_id, "
            "colo_rate_id, "
            "account_id, "
            "flags, "
            "revenue_share, "
            "optout_serving, "
            "name, "
            "hid_profile, "
            "ad_footer_url "
          "FROM adserver.query_colocations() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        unsigned long id = rs->get_number<unsigned long>(POS_COLO_ID);

        ColocationAdServingType ad_serving = CS_ALL;
        std::string ad_serving_str = rs->get_string(POS_AD_SERVING);
        if(ad_serving_str == CS_ONLY_OPTIN_STR)
        {
          ad_serving = CS_ONLY_OPTIN;
        }
        else if(ad_serving_str == CS_NON_OPTOUT_STR)
        {
          ad_serving = CS_NON_OPTOUT;
        }
        else if(ad_serving_str == CS_NONE_STR)
        {
          ad_serving = CS_NONE;
        }

        ColocationDef_var colocation(new ColocationDef());
        colocation->colo_id = id;
        colocation->colo_name = rs->get_string(POS_COLO_NAME);
        colocation->colo_rate_id = rs->is_null(POS_COLO_RATE_ID) ? 0 :
          rs->get_number<unsigned long>(POS_COLO_RATE_ID);
        colocation->at_flags = rs->get_number<unsigned long>(POS_ACCOUNT_FLAGS);
        colocation->account_id = rs->get_number<unsigned long>(POS_ACCOUNT_ID);
        colocation->revenue_share = rs->is_null(POS_REVENUE_SHARE) ?
          RevenueDecimal::ZERO : rs->get_decimal<RevenueDecimal>(POS_REVENUE_SHARE);
        colocation->ad_serving = ad_serving;
        colocation->hid_profile = (rs->get_char(POS_HID_PROFILE) == 'Y');

        if(!rs->is_null(POS_FOOTER_URL))
        {
          colocation->tokens.insert(
            std::make_pair(
              FOOTER_URL_OPTION_ID,
              rs->get_string(POS_FOOTER_URL)));
        }

        new_config->colocations.activate(
          id,
          colocation,
          sysdate,
          old_colocations);
      }

      if(old_colocations)
      {
        new_config->colocations.deactivate_nonactive(
          *old_colocations, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_currency_exchange_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_currency_exchange_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      /* query currency exchange */
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT currency_exchange_id "
          "FROM adserver.query_max_currency_exchange_id()");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      if (rs->next())
      {
        new_config->global_params.currency_exchange_id =
          rs->get_number<unsigned long>(1);
      }
      else
      {
        new_config->global_params.currency_exchange_id = 0;
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_campaigns_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_campaigns_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      ExecutionTimeTracer exec_tracer(FUN,
        Aspect::CAMPAIGN_SERVER, logger_, "only campaigns query");

      enum /* QueryCampaigns */
      {
        QC_CG_ID = 1,
        QC_CMP_ID,
        QC_CCG_RATE_ID,
        QC_CCG_RATE_TYPE,
        QC_FCG_ID,
        QC_FC_ID,
        QC_COUNTRY,
        QC_FLAGS,
        QC_MARKETPLACE,
        QC_IMP_REVENUE,
        QC_CLICK_REVENUE,
        QC_ACTION_REVENUE,
        QC_COMMISION,
        QC_ACCOUNT_ID,
        QC_ADVERTISER_ID,
        QC_CAMPAIGN_TYPE,
        QC_TARGET_TYPE,
        QC_USER_STATUS_TARGETING,
        QC_CTR_RESET_ID,
        QC_MODE,
        QC_MIN_UID_AGE,
        QC_SEQ_SET_ROTATE_IMPS,
        QC_STAT_CHANNEL_ID,

        QC_CMP_DATE_START,
        QC_CMP_DATE_END,
        QC_CMP_BUDGET,
        QC_CMP_DAILY_BUDGET,
        QC_CMP_DELIVERY_PACING,

        QC_DATE_START,
        QC_DATE_END,
        QC_BUDGET,
        QC_DAILY_BUDGET,
        QC_DELIVERY_PACING,

        QC_MAX_PUB_SHARE,
        QC_STATUS,

        QC_CURRENCY_ID,
        QC_START_USER_GROUP_ID,
        QC_END_USER_GROUP_ID,
        QC_BID_STRATEGY,
        QC_MIN_CTR_GOAL,
        QC_ECPM,
        QC_ECPM_FOR_MAXBID,
        QC_FLIGHT_RATE_TYPE,

        QC_CCG_IMP_TOTAL_LIMIT,
        QC_CCG_IMP_DAILY_LIMIT,
        QC_CCG_CLICK_TOTAL_LIMIT,
        QC_CCG_CLICK_DAILY_LIMIT,

        QC_CMP_IMP_TOTAL_LIMIT,
        QC_CMP_IMP_DAILY_LIMIT,
        QC_CMP_CLICK_TOTAL_LIMIT,
        QC_CMP_CLICK_DAILY_LIMIT,

        QC_INITIAL_CONTRACT_ID
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "cmp.campaign_id, "
            "cmp.ccg_id, "
            "cmp.ccg_rate_id, "
            "cmp.rate_type, "
            "cmp.campaign_fc_id, "
            "cmp.ccg_fc_id, "
            "cmp.country_code, "
            "cmp.flags, "
            "cmp.marketplace, "
            "cmp.cpm, "
            "cmp.cpc, "
            "cmp.cpa, "
            "cmp.commission, "
            "cmp.agency_account_id, " // account_id
            "cmp.account_id, " // advertiser_id
            "cmp.ccg_type, "
            "cmp.tgt_type, "
            "cmp.optin_status_targeting, "
            "cmp.ctr_reset_id, "
            "cmp.random_imps, "
            "cmp.min_uid_age,"
            "cmp.rotation_criteria,"
            "cmp.channel_id,"
            // CCG delivery limitations
            "cmp.campaign_date_start, "
            "cmp.campaign_date_end, "
            "cmp.campaign_budget, "
            "cmp.campaign_daily_budget, "
            "cmp.campaign_delivery_pacing, "
            // CCG delivery limitations
            "cmp.ccg_date_start, "
            "cmp.ccg_date_end, "
            "cmp.ccg_budget, "
            "cmp.ccg_daily_budget, "
            "cmp.ccg_delivery_pacing, "
            "cmp.max_pub_share, "
            "cmp.cross_status,"
            "cmp.currency_id, "
            "cmp.user_sample_group_start, "
            "cmp.user_sample_group_end, "
            "cmp.bid_strategy, "
            "GREATEST(COALESCE(cmp.min_ctr_goal, 0), COALESCE(ccg.min_ctr_goal, 0)) / 100, "
            "cmp.ecpm, "
            "round(ccgrate.cpa * 100 /  cer.rate, 8), "
            "flight.rate_type, "
            "flight.impressions_total_limit, "
            "flight.impressions_daily_limit, "
            "flight.clicks_total_limit, "
            "flight.clicks_daily_limit, "
            "flightcmp.impressions_total_limit, "
            "flightcmp.impressions_daily_limit, "
            "flightcmp.clicks_total_limit, "
            "flightcmp.clicks_daily_limit, "
            "contract.id "
          "FROM "
            "adserver.get_campaign_ctr($1) cmp "
            "join Campaign campaign using(campaign_id) "
            "join CampaignCreativeGroup ccg on(ccg.ccg_id = cmp.ccg_id) "
            "join Account acc on(acc.account_id = campaign.account_id) "
            "join CCGRate on(CCGRate.ccg_rate_id = cmp.ccg_rate_id) "
            "join CurrencyExchangeRate cer ON(cer.currency_id = acc.currency_id AND cer.currency_exchange_id = "
              "(select max(currency_exchange_id) currency_exchange_id from CurrencyExchange where effective_date = ("
                "select max(effective_date) from CurrencyExchange where effective_date <= now()))) "
            "left join flightccg on(cmp.ccg_id = flightccg.ccg_id) "
            "left join flight using(flight_id) "
            "left join flight flightcmp on(flightcmp.flight_id = flight.parent_id) "
            "LEFT JOIN contract ON (contract.account_id = acc.account_id AND "
              "contract.id NOT IN (SELECT parent_contract_id FROM contract WHERE parent_contract_id IS NOT NULL))"
          );

      stmt->set_timestamp(1, sysdate - pending_expire_time_);

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        char cross_status = rs->get_char(QC_STATUS);

        if(campaign_statuses_.empty() ||
          campaign_statuses_.find(cross_status) != std::string::npos)
        {
          const unsigned long campaign_id =
            rs->get_number<unsigned long>(QC_CMP_ID); // <-ccg_id

          try
          {
            CurrencyMap::ActiveMap::const_iterator currency_it =
              new_config->currencies.active().find(
                rs->get_number<unsigned long>(QC_CURRENCY_ID));
            if(currency_it != new_config->currencies.active().end())
            {
              CurrencyDef_var cur_currency = currency_it->second;
              Campaign_var campaign;

              if(old_config)
              {
                CampaignMap::ActiveMap::const_iterator cmp_it =
                  old_config->campaigns.active().find(campaign_id);
                if(cmp_it != old_config->campaigns.active().end())
                {
                  campaign = new CampaignDef(*(cmp_it->second));
                }
              }

              if(!campaign.in())
              {
                campaign = new CampaignDef();
                campaign->delivery_coef = TAG_DELIVERY_MAX;
              }

              campaign->campaign_group_id =
                rs->get_number<unsigned long>(QC_CG_ID); // <-campaign_id
              campaign->ccg_rate_id =
                rs->get_number<unsigned long>(QC_CCG_RATE_ID);

              std::string flight_rate_type = !rs->is_null(QC_FLIGHT_RATE_TYPE) ?
                rs->get_string(QC_FLIGHT_RATE_TYPE) :
                std::string();

              if(flight_rate_type == "MCPM")
              {
                campaign->ccg_rate_type = 'B'; // CR_MAXBID
              }
              else
              {
                campaign->ccg_rate_type = '-';
                std::string rate_type = rs->get_string(QC_CCG_RATE_TYPE);
                if(rate_type.size() == 3)
                {
                  campaign->ccg_rate_type = rate_type[2];
                }
              }

              campaign->fc_id = rs->is_null(QC_FC_ID) ?
                0 : rs->get_number<unsigned long>(QC_FC_ID);
              campaign->group_fc_id = rs->is_null(QC_FCG_ID) ?
                0 : rs->get_number<unsigned long>(QC_FCG_ID);
              campaign->flags = rs->get_number<unsigned long>(QC_FLAGS);
              campaign->marketplace = rs->get_string(QC_MARKETPLACE)[0];

              campaign->country = rs->get_string(QC_COUNTRY);
              String::AsciiStringManip::to_lower(campaign->country);

              campaign->status = cross_status;

              try
              {
                campaign->imp_revenue = rs->get_decimal<RevenueDecimal>(QC_IMP_REVENUE);
                campaign->click_revenue = rs->get_decimal<RevenueDecimal>(QC_CLICK_REVENUE);
                campaign->action_revenue = rs->get_decimal<RevenueDecimal>(QC_ACTION_REVENUE);
                if(campaign->imp_revenue == RevenueDecimal::ZERO && campaign->action_revenue != RevenueDecimal::ZERO)
                {
                  campaign->imp_revenue = RevenueDecimal::div(campaign->action_revenue, RevenueDecimal(false, 1000, 0));
                  campaign->action_revenue = RevenueDecimal::ZERO;
                }
              }
              catch(const RevenueDecimal::Overflow& ex)
              {
                Stream::Error ostr;
                ostr << "very big imp/click/action cost: " <<
                  rs->get_string(QC_IMP_REVENUE) << "/" <<
                  rs->get_string(QC_CLICK_REVENUE) << "/" <<
                  rs->get_string(QC_ACTION_REVENUE);
                throw InvalidObject(ostr);
              }

              if(!check_cost_(campaign->imp_revenue, new_config, cur_currency) ||
                 !check_cost_(campaign->click_revenue, new_config, cur_currency) ||
                 !check_cost_(campaign->action_revenue, new_config, cur_currency))
              {
                Stream::Error ostr;
                ostr << "big imp/click/action cost: " <<
                  campaign->imp_revenue.str() << "/" <<
                  campaign->click_revenue.str() << "/" <<
                  campaign->action_revenue.str();
                throw InvalidObject(ostr);
              }

              /* adopt flags */
              if(campaign->action_revenue > RevenueDecimal::ZERO)
              {
                campaign->flags |= AdServer::CampaignSvcs::CampaignFlags::TRACK_ACTIONS;
              }

              campaign->commision = rs->get_decimal<RevenueDecimal>(QC_COMMISION);
              campaign->account_id = rs->get_number<unsigned long>(QC_ACCOUNT_ID);
              campaign->advertiser_id =
                rs->get_number<unsigned long>(QC_ADVERTISER_ID);

              campaign->ccg_type = rs->get_char(QC_CAMPAIGN_TYPE);
              campaign->target_type = rs->get_char(QC_TARGET_TYPE);
              std::string user_status_targeting = rs->get_string(QC_USER_STATUS_TARGETING);
              if(user_status_targeting.size() != 3)
              {
                campaign->flags |= AdServer::CampaignSvcs::CampaignFlags::US_NONE;
              }
              else
              {
                // 0: opt-in, 1: opt-out, 2: undefined
                if(user_status_targeting[0] == 'Y')
                {
                  campaign->flags |= AdServer::CampaignSvcs::CampaignFlags::US_OPTIN;
                }

                if(user_status_targeting[1] == 'Y')
                {
                  campaign->flags |= AdServer::CampaignSvcs::CampaignFlags::US_OPTOUT;
                }

                if(user_status_targeting[2] == 'Y')
                {
                  campaign->flags |= AdServer::CampaignSvcs::CampaignFlags::US_UNDEFINED;
                }
              }

              campaign->ctr_reset_id = rs->get_number<unsigned long>(QC_CTR_RESET_ID);
              campaign->mode = (rs->get_number<unsigned long>(QC_MODE) == 0 ?
                                CM_RANDOM : CM_NON_RANDOM);
              campaign->min_uid_age = Generics::Time::ONE_HOUR *
                rs->get_number<unsigned long>(QC_MIN_UID_AGE);
              campaign->campaign_delivery_limits.date_start = !rs->is_null(QC_CMP_DATE_START) ?
                rs->get_timestamp(QC_CMP_DATE_START) : Generics::Time::ZERO;
              campaign->campaign_delivery_limits.date_end = !rs->is_null(QC_CMP_DATE_END) ?
                rs->get_timestamp(QC_CMP_DATE_END) : Generics::Time::ZERO;
              campaign->campaign_delivery_limits.budget =
                !rs->is_null(QC_CMP_BUDGET) ?
                OptionalRevenueDecimal(rs->get_decimal<RevenueDecimal>(QC_CMP_BUDGET)) :
                OptionalRevenueDecimal();
              campaign->campaign_delivery_limits.daily_budget =
                !rs->is_null(QC_CMP_DAILY_BUDGET) ?
                OptionalRevenueDecimal(rs->get_decimal<RevenueDecimal>(QC_CMP_DAILY_BUDGET)) :
                OptionalRevenueDecimal();
              campaign->campaign_delivery_limits.delivery_pacing =
                rs->get_char(QC_CMP_DELIVERY_PACING);
              campaign->seq_set_rotate_imps =
                rs->get_number<unsigned long>(QC_SEQ_SET_ROTATE_IMPS);

              campaign->ccg_delivery_limits.imps =
                !rs->is_null(QC_CCG_IMP_TOTAL_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CCG_IMP_TOTAL_LIMIT)) :
                Commons::Optional<unsigned long>();
              campaign->ccg_delivery_limits.daily_imps =
                !rs->is_null(QC_CCG_IMP_DAILY_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CCG_IMP_DAILY_LIMIT)) :
                Commons::Optional<unsigned long>();
              campaign->ccg_delivery_limits.clicks =
                !rs->is_null(QC_CCG_CLICK_TOTAL_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CCG_CLICK_TOTAL_LIMIT)) :
                Commons::Optional<unsigned long>();
              campaign->ccg_delivery_limits.daily_clicks =
                !rs->is_null(QC_CCG_CLICK_DAILY_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CCG_CLICK_DAILY_LIMIT)) :
                Commons::Optional<unsigned long>();

              campaign->campaign_delivery_limits.imps =
                !rs->is_null(QC_CMP_IMP_TOTAL_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CMP_IMP_TOTAL_LIMIT)) :
                Commons::Optional<unsigned long>();
              campaign->campaign_delivery_limits.daily_imps =
                !rs->is_null(QC_CMP_IMP_DAILY_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CMP_IMP_DAILY_LIMIT)) :
                Commons::Optional<unsigned long>();
              campaign->campaign_delivery_limits.clicks =
                !rs->is_null(QC_CMP_CLICK_TOTAL_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CMP_CLICK_TOTAL_LIMIT)) :
                Commons::Optional<unsigned long>();
              campaign->campaign_delivery_limits.daily_clicks =
                !rs->is_null(QC_CMP_CLICK_DAILY_LIMIT) ?
                Commons::Optional<unsigned long>(rs->get_number<unsigned long>(QC_CMP_CLICK_DAILY_LIMIT)) :
                Commons::Optional<unsigned long>();

              if(!rs->is_null(QC_STAT_CHANNEL_ID))
              {
                campaign->stat_expression.op = NonLinkedExpressionChannel::NOP;
                campaign->stat_expression.channel_id =
                  rs->get_number<unsigned long>(QC_STAT_CHANNEL_ID);
              }
              else
              {
                campaign->stat_expression = NonLinkedExpressionChannel::Expression();
              }

              campaign->ccg_delivery_limits.date_start = !rs->is_null(QC_DATE_START) ?
                rs->get_timestamp(QC_DATE_START) : Generics::Time::ZERO;
              campaign->ccg_delivery_limits.date_end = !rs->is_null(QC_DATE_END) ?
                rs->get_timestamp(QC_DATE_END) : Generics::Time::ZERO;
              campaign->ccg_delivery_limits.budget =
                !rs->is_null(QC_BUDGET) ?
                OptionalRevenueDecimal(rs->get_decimal<RevenueDecimal>(QC_BUDGET)) :
                OptionalRevenueDecimal();
              campaign->ccg_delivery_limits.daily_budget =
                !rs->is_null(QC_DAILY_BUDGET) ?
                OptionalRevenueDecimal(rs->get_decimal<RevenueDecimal>(QC_DAILY_BUDGET)) :
                OptionalRevenueDecimal();
              campaign->ccg_delivery_limits.delivery_pacing =
                rs->get_char(QC_DELIVERY_PACING);

              campaign->max_pub_share = rs->get_decimal<RevenueDecimal>(QC_MAX_PUB_SHARE);

              char bid_strategy_sym = rs->get_char(QC_BID_STRATEGY);
              if(bid_strategy_sym == 'R')
              {
                campaign->bid_strategy = BS_MAX_REACH;
                campaign->min_ctr_goal = RevenueDecimal::ZERO;
              }
              else if(bid_strategy_sym == 'C')
              {
                campaign->bid_strategy = BS_MIN_CTR_GOAL;
                campaign->min_ctr_goal = rs->get_decimal<RevenueDecimal>(QC_MIN_CTR_GOAL);
              }
              else
              {
                Stream::Error ostr;
                ostr << "unexpected bid strategy value '" << bid_strategy_sym << "'";
                throw InvalidObject(ostr);
              }

              campaign->start_user_group_id =
                rs->get_number<unsigned long>(QC_START_USER_GROUP_ID);
              campaign->end_user_group_id =
                rs->get_number<unsigned long>(QC_END_USER_GROUP_ID);

              campaign->initial_contract_id = !rs->is_null(QC_INITIAL_CONTRACT_ID) ?
                rs->get_number<unsigned long>(QC_INITIAL_CONTRACT_ID) :
                0;

              RevenueDecimal orig_ecpm = rs->get_decimal<RevenueDecimal>(QC_ECPM);
              RevenueDecimal ecpm_for_maxbid = rs->get_decimal<RevenueDecimal>(QC_ECPM_FOR_MAXBID);
              RevenueDecimal ecpm = campaign->ccg_rate_type == 'A' ? ecpm_for_maxbid : orig_ecpm;

              new_config->campaigns.activate(
                campaign_id,
                campaign,
                sysdate,
                old_config ? &old_config->campaigns : 0);

              new_config->ecpms.activate(
                campaign_id,
                Ecpm_var(new EcpmDef(ecpm, RevenueDecimal::ZERO)),
                sysdate,
                old_config ? &old_config->ecpms : 0);
            }
          }
          catch(const Commons::Postgres::Exception& e)
          {
            logger_->sstream(Logging::Logger::ERROR,
                             Aspect::CAMPAIGN_SERVER,
                             "ADS-IMPL-160") <<
              "Can't load campaign with id = " << campaign_id <<
              ". Postgres::Exception: " << e.what();
          }
          catch(const InvalidObject& ex)
          {
            logger_->sstream(Logging::Logger::ERROR,
                             Aspect::CAMPAIGN_SERVER,
                             "ADS-IMPL-160") <<
              "Can't load campaign with id = " << campaign_id <<
              ". Reason: " << ex.what();
          }
          catch(const RevenueDecimal::Overflow& ex)
          {
            logger_->sstream(Logging::Logger::ERROR,
                             Aspect::CAMPAIGN_SERVER,
                             "ADS-IMPL-160") <<
              "Can't load campaign with id = " << campaign_id <<
              ". Caught RevenueDecimal::Overflow: " << ex.what();
          }
          catch(const eh::Exception& ex)
          {
            Stream::Error ostr;
            ostr << "At processing campaign with id = " << campaign_id <<
              ", caught eh::Exception: " << ex.what();
            throw Exception(ostr);
          }
        } // status check
      }

      if(old_config)
      {
        new_config->campaigns.deactivate_nonactive(
          old_config->campaigns, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": on campaigns selection, caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      ExecutionTimeTracer exec_tracer(FUN,
        Aspect::CAMPAIGN_SERVER, logger_, "campaign colocations query");

      enum
      {
        POS_CCG_ID = 1,
        POS_COLO_ID
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT ccg_id, colo_id "
          "FROM CCGColocation "
          "ORDER BY ccg_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_ccg_id =
        rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
      ColoIdSet cur_colocations;

      for(CampaignMap::ActiveMap::iterator cmp_it =
            new_config->campaigns.active().begin();
          cmp_it != new_config->campaigns.active().end(); )
      {
        if(!rs_next || cmp_it->first < cur_ccg_id)
        {
          if(!(cur_colocations.size() ==
                 cmp_it->second->colocations.size() &&
               std::equal(cur_colocations.begin(),
                 cur_colocations.end(),
                 cmp_it->second->colocations.begin())))
          {
            Campaign_var new_campaign(new CampaignDef(*(cmp_it->second)));
            new_campaign->colocations.swap(cur_colocations);
            new_campaign->timestamp = sysdate;
            cmp_it->second = new_campaign;
          }

          cur_colocations.clear();
          ++cmp_it;
        }
        else
        {
          if(cmp_it->first == cur_ccg_id)
          {
            cur_colocations.insert(rs->get_number<unsigned long>(POS_COLO_ID));
          }

          rs_next = rs->next();
          cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": on colocations for campaigns selection, "
        "caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      ExecutionTimeTracer exec_tracer(FUN,
        Aspect::CAMPAIGN_SERVER, logger_, "campaign weekly run query");

      /* query campaign weeklyrun intervals */
      enum
      {
        POS_CCG_ID = 1,
        POS_SCH_TYPE,
        POS_TIME_FROM,
        POS_TIME_TO
      };

      enum
      {
        CCG_SCH_TYPE = 0,
        CMP_SCH_TYPE
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT ccg_id, sch_type, time_from, time_to "
            "FROM adserver.query_campaign_weekly_runs() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_ccg_id =
        rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
      WeeklyRunIntervalSet cur_cmp_weekly_run_intervals;
      WeeklyRunIntervalSet cur_ccg_weekly_run_intervals;

      for(CampaignMap::ActiveMap::iterator cmp_it =
            new_config->campaigns.active().begin();
          cmp_it != new_config->campaigns.active().end(); )
      {
        if(!rs_next || cmp_it->first < cur_ccg_id)
        {
          if(cur_ccg_weekly_run_intervals.empty())
          {
            cur_ccg_weekly_run_intervals.insert(
              WeeklyRunIntervalDef(
                0, Generics::Time::ONE_WEEK.tv_sec / 60));
          }

          cur_ccg_weekly_run_intervals.normalize(
            0, Generics::Time::ONE_WEEK.tv_sec / 60);
          cur_cmp_weekly_run_intervals.normalize(
            0, Generics::Time::ONE_WEEK.tv_sec / 60);

          if(!cur_cmp_weekly_run_intervals.empty())
          {
            cur_ccg_weekly_run_intervals.cross(cur_cmp_weekly_run_intervals);
          }

          if(cur_ccg_weekly_run_intervals.size() == 1 &&
               cur_ccg_weekly_run_intervals.begin()->min == 0 &&
               cur_ccg_weekly_run_intervals.begin()->max ==
               static_cast<unsigned long>(
                 Generics::Time::ONE_WEEK.tv_sec / 60))
          {
            // full week interval
            cur_ccg_weekly_run_intervals.clear();
          }

          if(!(cur_ccg_weekly_run_intervals.size() ==
                 cmp_it->second->weekly_run_intervals.size() &&
               std::equal(cur_ccg_weekly_run_intervals.begin(),
                 cur_ccg_weekly_run_intervals.end(),
                 cmp_it->second->weekly_run_intervals.begin())))
          {
            Campaign_var new_campaign(new CampaignDef(*(cmp_it->second)));
            new_campaign->weekly_run_intervals.swap(cur_ccg_weekly_run_intervals);
            new_campaign->timestamp = sysdate;
            cmp_it->second = new_campaign;
          }

          cur_cmp_weekly_run_intervals.clear();
          cur_ccg_weekly_run_intervals.clear();
          ++cmp_it;
        }
        else
        {
          if(cmp_it->first == cur_ccg_id)
          {
            Generics::Time tz_offset;

            {
              AccountMap::ActiveMap::const_iterator acc_it =
                new_config->accounts.active().find(cmp_it->second->account_id);

              if (acc_it != new_config->accounts.active().end())
              {
                tz_offset =
                  acc_it->second->time_offset >= Generics::Time::ZERO ?
                  acc_it->second->time_offset :
                  Generics::Time::ONE_WEEK + acc_it->second->time_offset;
              }
            }

            Generics::Time time_from = Generics::Time(
              rs->is_null(POS_TIME_FROM) ? 0 :
              rs->get_number<unsigned long>(POS_TIME_FROM) * 60);
            // time_to at DB side have including (]) semantic
            // IntervalSet use [x, y) intervals
            Generics::Time time_to =
              rs->is_null(POS_TIME_TO) ?
              Generics::Time::ONE_WEEK :
              Generics::Time((
                  rs->get_number<unsigned long>(POS_TIME_TO) + 1) * 60);

            if (!(time_from == Generics::Time::ZERO &&
                  time_to == Generics::Time::ONE_WEEK))
            {
              time_from = time_from < tz_offset ?
                Generics::Time::ONE_WEEK + time_from - tz_offset:
                time_from - tz_offset;

              time_to = time_to <= tz_offset ?
                Generics::Time::ONE_WEEK + time_to - tz_offset:
                time_to - tz_offset;
            }

            WeeklyRunIntervalSet& target_intervals =
              rs->get_number<unsigned long>(POS_SCH_TYPE) == CCG_SCH_TYPE ?
              cur_ccg_weekly_run_intervals :
              cur_cmp_weekly_run_intervals;

            if (time_from > time_to)
            {
              target_intervals.insert(
                WeeklyRunIntervalDef(
                  time_from.tv_sec / 60, Generics::Time::ONE_WEEK.tv_sec / 60));

              target_intervals.insert(
                WeeklyRunIntervalDef(0, time_to.tv_sec / 60));
            }
            else
            {
              target_intervals.insert(
                WeeklyRunIntervalDef(
                  time_from.tv_sec / 60, time_to.tv_sec / 60));
            }
          }

          rs_next = rs->next();
          cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": on weekly run for campaigns selection, "
        "caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      ExecutionTimeTracer exec_tracer(FUN,
        Aspect::CAMPAIGN_SERVER, logger_, "campaign sites query");

      /* query campaign sites */
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT ccg_id, site_id FROM CCGSite ORDER BY ccg_id, site_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_ccg_id = rs_next ? rs->get_number<unsigned long>(1) : 0;
      SiteIdSet cur_sites;

      for(CampaignMap::ActiveMap::iterator cmp_it =
            new_config->campaigns.active().begin();
          cmp_it != new_config->campaigns.active().end(); )
      {
        if(!rs_next || cmp_it->first < cur_ccg_id)
        {
          if(!(cur_sites.size() == cmp_it->second->sites.size() &&
               std::equal(cur_sites.begin(),
                 cur_sites.end(),
                 cmp_it->second->sites.begin())))
          {
            Campaign_var new_campaign(new CampaignDef(*(cmp_it->second)));
            new_campaign->sites.swap(cur_sites);
            new_campaign->timestamp = sysdate;
            cmp_it->second = new_campaign;
          }

          cur_sites.clear();
          ++cmp_it;
        }
        else
        {
          if(cmp_it->first == cur_ccg_id)
          {
            cur_sites.insert(rs->get_number<unsigned long>(2));
          }

          rs_next = rs->next();
          cur_ccg_id = rs_next ? rs->get_number<unsigned long>(1) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": on sites for campaigns selection, "
        "caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      query_contracts_(conn, new_config, old_config, sysdate);

      query_campaign_expressions_(conn, new_config, sysdate);

      // Fetching creatives
      query_campaign_creatives_(conn, new_config, sysdate);

      query_campaign_keywords_(conn, new_config, old_config, sysdate);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_campaign_expressions_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_campaign_expressions_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_CCG_ID = 1,
        POS_CHANNEL_ID,
        POS_CHANNEL_TYPE
      };

      enum
      {
        CT_TARGETING_CHANNEL = 0,
        CT_GEO_CHANNEL,
        CT_DEVICE_CHANNEL,
      };

      // use audience channel if it is defined
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT ccg_id, channel_id, channel_type "
          "FROM adserver.query_campaign_expressions() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_ccg_id =
        rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
      unsigned long cur_ccg_channel_id = 0;
      ChannelIdSet cur_ccg_geo_channels;
      ChannelIdSet cur_ccg_platform_channels;

      for(CampaignMap::ActiveMap::iterator cmp_it =
            new_config->campaigns.active().begin();
          cmp_it != new_config->campaigns.active().end(); )
      {
        if(!rs_next || cmp_it->first < cur_ccg_id)
        {
          bool geo_sub_expression_defined = false;
          NonLinkedExpressionChannel::Expression geo_sub_expression;
          bool platform_sub_expression_defined = false;
          NonLinkedExpressionChannel::Expression platform_sub_expression;

          // fill geo sub expression
          if(!cur_ccg_geo_channels.empty())
          {
            if(cur_ccg_geo_channels.size() == 1)
            {
              geo_sub_expression.op = NonLinkedExpressionChannel::NOP;
              geo_sub_expression.channel_id = *cur_ccg_geo_channels.begin();
            }
            else
            {
              geo_sub_expression.op = NonLinkedExpressionChannel::OR;
              geo_sub_expression.channel_id = 0;
              geo_sub_expression.sub_channels.reserve(
                cur_ccg_geo_channels.size());
              for(ChannelIdSet::const_iterator ch_it =
                  cur_ccg_geo_channels.begin();
                  ch_it != cur_ccg_geo_channels.end(); ++ch_it)
              {
                geo_sub_expression.sub_channels.push_back(
                  NonLinkedExpressionChannel::Expression(*ch_it));
              }
            }

            geo_sub_expression_defined = true;
          }

          // fill platform sub expression
          if(!cur_ccg_platform_channels.empty())
          {
            if(cur_ccg_platform_channels.size() == 1)
            {
              platform_sub_expression.op = NonLinkedExpressionChannel::NOP;
              platform_sub_expression.channel_id =
                *cur_ccg_platform_channels.begin();
            }
            else
            {
              platform_sub_expression.op = NonLinkedExpressionChannel::OR;
              platform_sub_expression.channel_id = 0;
              platform_sub_expression.sub_channels.reserve(
                cur_ccg_platform_channels.size());
              for(ChannelIdSet::const_iterator ch_it =
                  cur_ccg_platform_channels.begin();
                  ch_it != cur_ccg_platform_channels.end(); ++ch_it)
              {
                platform_sub_expression.sub_channels.push_back(
                  NonLinkedExpressionChannel::Expression(*ch_it));
              }
            }

            platform_sub_expression_defined = true;
          }

          NonLinkedExpressionChannel::Expression expression;

          if((cur_ccg_channel_id ? 1 : 0) +
             (geo_sub_expression_defined ? 1 : 0) +
             (platform_sub_expression_defined ? 1 : 0) >= 2)
          {
            // two compenents defined: .. AND .. composition required
            expression.op = NonLinkedExpressionChannel::AND;
            expression.channel_id = 0;

            if(cur_ccg_channel_id)
            {
              expression.sub_channels.push_back(
                NonLinkedExpressionChannel::Expression(cur_ccg_channel_id));
            }

            if(geo_sub_expression_defined)
            {
              expression.sub_channels.push_back(geo_sub_expression);
            }

            if(platform_sub_expression_defined)
            {
              expression.sub_channels.push_back(platform_sub_expression);
            }
          }
          // cases when only one component defined
          else if(cur_ccg_channel_id)
          {
            expression.op = NonLinkedExpressionChannel::NOP;
            expression.channel_id = cur_ccg_channel_id;
          }
          else if(geo_sub_expression_defined)
          {
            expression = geo_sub_expression;
          }
          else if(platform_sub_expression_defined)
          {
            expression = platform_sub_expression;
          }

          if(!(expression == cmp_it->second->expression))
          {
            Campaign_var new_campaign(new CampaignDef(*(cmp_it->second)));

            new_campaign->expression.swap(expression);
            new_campaign->timestamp = sysdate;
            cmp_it->second = new_campaign;
          }

          cur_ccg_channel_id = 0;
          cur_ccg_geo_channels.clear();
          cur_ccg_platform_channels.clear();

          ++cmp_it;
        }
        else
        {
          if(cmp_it->first == cur_ccg_id)
          {
            unsigned long channel_type =
              rs->get_number<unsigned long>(POS_CHANNEL_TYPE);

            if(channel_type == CT_TARGETING_CHANNEL)
            {
              cur_ccg_channel_id =
                rs->get_number<unsigned long>(POS_CHANNEL_ID);
            }
            else if(channel_type == CT_GEO_CHANNEL)
            {
              cur_ccg_geo_channels.insert(
                rs->get_number<unsigned long>(POS_CHANNEL_ID));
            }
            else
            {
              cur_ccg_platform_channels.insert(
                rs->get_number<unsigned long>(POS_CHANNEL_ID));
            }
          }

          rs_next = rs->next();
          cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_adv_actions_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_adv_actions_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum /* QueryActions */
      {
        QA_ACTION_ID = 1,
        QA_CUR_VALUE,
        QA_STATUS
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "action_id, "
            "cur_value, "
            "status "
          "FROM adserver.query_adv_actions() a");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        unsigned long action_id = rs->get_number<unsigned long>(QA_ACTION_ID);
        char cross_status = rs->get_char(QA_STATUS);

        if (cross_status == 'A')
        {
          AdvActionDef_var action;

          if(old_config)
          {
            AdvActionMap::ActiveMap::const_iterator ait =
              old_config->adv_actions.active().find(action_id);

            if(ait != old_config->adv_actions.active().end())
            {
              action = new AdvActionDef(*ait->second);
            }
          }

          if(!action.in())
          {
            action = new AdvActionDef();
          }

          action->action_id = action_id;
          action->cur_value = rs->get_decimal<RevenueDecimal>(QA_CUR_VALUE);

          new_config->adv_actions.activate(
            action_id, action, sysdate, old_config ? &old_config->adv_actions : 0);
        }
        else
        {
          new_config->adv_actions.deactivate(
            action_id, sysdate, old_config ? &old_config->adv_actions : 0);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at Action fetch: " << ex.what();
      throw Exception(ostr);
    }

    if(old_config)
    {
      new_config->adv_actions.deactivate_nonactive(
        old_config->adv_actions, sysdate);
    }

    try
    {
      /* fill ccg action links */

      enum /* QueryCCGActions */
      {
        QCA_ACTION_ID = 1,
        QCA_CCG_ID
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT action_id, "
          "ccg_id "
          "FROM adserver.query_ccg_action()");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_action_id =
        rs_next ? rs->get_number<unsigned long>(QCA_ACTION_ID) : 0;
      AdvActionDef::CCGIdSet cur_ccgs;

      for(AdvActionMap::ActiveMap::iterator ait =
            new_config->adv_actions.active().begin();
          ait != new_config->adv_actions.active().end(); )
      {
        if(!rs_next || ait->first < cur_action_id)
        {
          AdvActionDef_var action = new AdvActionDef(*ait->second);
          action->ccg_ids.swap(cur_ccgs);
          if(!(*action == *ait->second))
          {
            action->timestamp = sysdate;
            ait->second = action;
          }
          cur_ccgs.clear();
          ++ait;
        }
        else
        {
          if(ait->first == cur_action_id)
          {
            cur_ccgs.insert(rs->get_number<unsigned long>(QCA_CCG_ID));
          }

          rs_next = rs->next();
          cur_action_id =
            rs_next ? rs->get_number<unsigned long>(QCA_ACTION_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at CCGAction fetch: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_category_channels_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_category_channels_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum /* QueryCategoryChannels */
      {
        QCC_CHANNEL_ID = 1,
        QCC_NAME,
        QCC_NEWSGATE_NAME,
        QCC_STATUS,
        QCC_PARENT_CHANNEL_ID,
        QCC_FLAGS,
        QCC_TIMESTAMP
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "channel_id, "
            "name, "
            "newsgate_category_name, "
            "adserver_status, "
            "parent_channel_id, "
            "flags, "
            "version "
          "FROM adserver.query_category_channels()");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        unsigned long channel_id = rs->get_number<unsigned long>(QCC_CHANNEL_ID);
        CategoryChannelDef_var category_channel;

        if(old_config)
        {
          CategoryChannelMap::ActiveMap::const_iterator old_cc_it =
            old_config->category_channels.active().find(channel_id);

          if(old_cc_it != old_config->category_channels.active().end())
          {
            category_channel = new CategoryChannelDef(*(old_cc_it->second));
          }
        }

        if(!category_channel.in())
        {
          category_channel = new CategoryChannelDef();
        }

        category_channel->channel_id = channel_id;
        category_channel->name = rs->get_string(QCC_NAME);
        category_channel->newsgate_name = rs->get_string(QCC_NEWSGATE_NAME);
        category_channel->parent_channel_id = !rs->is_null(QCC_PARENT_CHANNEL_ID) ?
          rs->get_number<unsigned long>(QCC_PARENT_CHANNEL_ID) : 0;
        category_channel->flags = rs->get_number<unsigned long>(QCC_FLAGS);

        new_config->category_channels.activate(
          channel_id,
          category_channel,
          sysdate,
          old_config ? &old_config->category_channels : 0);
      }

      if(old_config)
      {
        new_config->category_channels.deactivate_nonactive(
          old_config->category_channels, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // fill category channel localizations
    try
    {
      enum /* QueryCategoryChannelLocalization */
      {
        QCCL_CHANNEL_ID = 1,
        QCCL_LANG,
        QCCL_NAME
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "SUBSTR(key, 17)::int, "
            "lang, "
            "value "
          "FROM DynamicResources WHERE key like 'CategoryChannel.%' "
          "ORDER BY SUBSTR(key, 17)::int");

      Commons::Postgres::ResultSet_var lc_rs = conn->execute_statement(stmt);

      CategoryChannelDef::LocalizationMap cur_localizations;
      bool rs_next = lc_rs->next();
      unsigned long cur_channel_id = rs_next ?
        lc_rs->get_number<unsigned long>(QCCL_CHANNEL_ID) : 0;

      for(CategoryChannelMap::ActiveMap::iterator cc_it =
            new_config->category_channels.active().begin();
          cc_it != new_config->category_channels.active().end(); )
      {
        if(!rs_next || cc_it->first < cur_channel_id)
        {
          if(cur_localizations.size() != cc_it->second->localizations.size() ||
             !std::equal(cur_localizations.begin(),
               cur_localizations.end(),
               cc_it->second->localizations.begin(),
               Algs::PairEqual()))
          {
            CategoryChannelDef_var category_channel(
              new CategoryChannelDef(*(cc_it->second)));
            category_channel->localizations.swap(cur_localizations);
            category_channel->timestamp = sysdate;
            cc_it->second = category_channel;
          }

          cur_localizations.clear();
          ++cc_it;
        }
        else
        {
          if(cc_it->first == cur_channel_id)
          {
            std::string lang = lc_rs->get_string(QCCL_LANG);
            std::string name = lc_rs->get_string(QCCL_NAME);
            cur_localizations.insert(std::make_pair(lang, name));
          }

          rs_next = lc_rs->next();
          cur_channel_id = rs_next ?
            lc_rs->get_number<unsigned long>(QCCL_CHANNEL_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception on query "
        "category localizations: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_fraud_conditions_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_fraud_conditions_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_FRAUD_CONDITION_ID = 1,
        POS_TYPE,
        POS_PERIOD,
        POS_LIMIT
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "fraud_condition_id, "
            "type, "
            "period, "
            "fc.limit "
          "FROM FraudCondition fc");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        FraudConditionDef_var fraud_condition(new FraudConditionDef());
        fraud_condition->id =
          rs->get_number<unsigned long>(POS_FRAUD_CONDITION_ID);
        fraud_condition->type = rs->get_string(POS_TYPE)[0];
        fraud_condition->period = 
          Generics::Time(rs->get_number<unsigned long>(POS_PERIOD));
        fraud_condition->limit =
          rs->get_number<unsigned long>(POS_LIMIT);

        new_config->fraud_conditions.activate(
          rs->get_number<unsigned long>(POS_FRAUD_CONDITION_ID),
          fraud_condition,
          sysdate,
          old_config ? &old_config->fraud_conditions : 0);
      }

      if(old_config)
      {
        new_config->fraud_conditions.deactivate_nonactive(
          old_config->fraud_conditions,
          sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_web_operations_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_web_operations_()";
    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);
    try
    {
      enum
      {
        POS_WEB_OPERATION_ID = 1,
        POS_APP,
        POS_SOURCE,
        POS_OPERATION,
        POS_FLAGS
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT web_operation_id, app, source, operation, flags "
          "FROM adserver.get_weboperation()");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
      const WebOperationMap* old_operations =
        old_config ? &(old_config->web_operations) : 0;

      while (rs->next())
      {
        unsigned long id = rs->get_number<int>(POS_WEB_OPERATION_ID);
        WebOperationDef_var web_operation = new WebOperationDef;
        web_operation->app = rs->get_string(POS_APP);
        web_operation->source = rs->get_string(POS_SOURCE);
        web_operation->operation = rs->get_string(POS_OPERATION);
        web_operation->flags = rs->get_number<unsigned int>(POS_FLAGS);
        new_config->web_operations.activate(
          id, web_operation, sysdate, old_operations);
      }
      if(old_config)
      {
        new_config->web_operations.deactivate_nonactive(
          old_config->web_operations, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": exception at fetcing web operations: " << ex.what();
      throw Exception(ostr);
    }
  }


  void CampaignConfigDBSource::query_search_engines_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "CampaignConfigDBSource::query_search_engines_()";
    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    enum
    {
      POS_SEARCH_ENGINE_ID = 1,
      POS_HOST_POSTFIX,
      POS_REGEXP,
      POS_ENC,
      POS_POST_ENC,
      POS_DEC_DEPTH
    };

    try
    {
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "search_engine_id,"
          "host,"
          "regexp,"
          "encoding,"
          "post_encoding,"
          "decoding_depth "
        "FROM SearchEngine "
        "ORDER BY search_engine_id, host, regexp, encoding, decoding_depth");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
      const SearchEngineMap* old_map =
        old_config ? &old_config->search_engines : 0;
      unsigned long search_engine_id, prev_search_engine_id = 0;
      SearchEngine_var value;

      while(rs->next())
      {
        search_engine_id = rs->get_number<unsigned long>(POS_SEARCH_ENGINE_ID);

        if(search_engine_id != prev_search_engine_id)
        {
          if(value)
          {
            new_config->search_engines.activate(
              prev_search_engine_id,
              value,
              sysdate,
              old_map);

            value.reset();
          }

          prev_search_engine_id = search_engine_id;
        }

        if(!value)
        {
          value = new SearchEngine;
        }

        SearchEngineRegExp reg_exp;
        reg_exp.host_postfix = rs->get_string(POS_HOST_POSTFIX);
        reg_exp.regexp = rs->get_string(POS_REGEXP);
        reg_exp.encoding = rs->get_string(POS_ENC);
        reg_exp.post_encoding = rs->get_string(POS_POST_ENC);
        reg_exp.decoding_depth = rs->get_number<unsigned long>(POS_DEC_DEPTH);
        value->regexps.push_back(reg_exp);
      }

      if(value)
      {
        new_config->search_engines.activate(
          prev_search_engine_id,
          value,
          sysdate,
          old_map);
      }

      if(old_config)
      {
        new_config->search_engines.deactivate_nonactive(
          old_config->search_engines, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_web_browsers_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "CampaignConfigDBSource::query_web_browsers_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    enum
    {
      POS_NAME = 1,
      POS_MARKER,
      POS_REGEXP,
      POS_REGEXP_REQUIRED,
      POS_PRIORITY
    };

    try
    {
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "name, "
            "marker, "
            "regexp, "
            "regexp_required, "
            "priority "
          "FROM WebBrowser "
          "ORDER BY name, marker, regexp, priority");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
      const WebBrowserMap* old_map =
        old_config ? &old_config->web_browsers : 0;
      std::string key, old_key;
      WebBrowser_var value;

      while(rs->next())
      {
        key = rs->get_string(POS_NAME);

        if(key != old_key)
        {
          if(value)
          {
            new_config->web_browsers.activate(
              old_key,
              value,
              sysdate,
              old_map);

            value.reset();
          }

          old_key = key;
        }

        if(!value)
        {
          value = new WebBrowser;
        }

        WebBrowser::Detector detector;
        detector.marker = rs->get_string(POS_MARKER);
        detector.regexp = rs->get_string(POS_REGEXP);
        detector.regexp_required = (rs->get_char(POS_REGEXP_REQUIRED) == 'Y');
        detector.priority = rs->get_number<unsigned long>(POS_PRIORITY);
        value->detectors.push_back(detector);
      }

      if(value)
      {
        new_config->web_browsers.activate(
          old_key,
          value,
          sysdate,
          old_map);
      }

      if(old_config)
      {
        new_config->web_browsers.deactivate_nonactive(
          old_config->web_browsers, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_platforms_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "CampaignConfigDBSource::query_platforms_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    enum
    {
      POS_PLATFORM_ID = 1,
      POS_NAME,
      POS_TYPE,
      POS_MARKER,
      POS_MATCH_REGEXP,
      POS_OUTPUT_REGEXP,
      POS_PRIORITY
    };

    try
    {
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "p.platform_id, "
          "lower(p.name), "
          "p.type, "
          "pd.match_marker, "
          "pd.match_regexp, "
          "pd.output_regexp, "
          "pd.priority "
        "FROM Platform p LEFT JOIN "
          "PlatformDetector pd ON (pd.platform_id = p.platform_id) "
        "ORDER BY "
          "p.platform_id, coalesce(pd.priority, 0) DESC, "
          "p.name, p.type, pd.match_marker, "
          "pd.match_regexp, pd.output_regexp");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
      const PlatformMap* old_map =
        old_config ? &old_config->platforms : 0;
      unsigned long platform_id;
      unsigned long prev_platform_id = 0;
      Platform_var value;

      while(rs->next())
      {
        platform_id = rs->get_number<unsigned long>(POS_PLATFORM_ID);

        if(platform_id != prev_platform_id)
        {
          if(value)
          {
            new_config->platforms.activate(
              prev_platform_id,
              value,
              sysdate,
              old_map);

            value.reset();
          }

          prev_platform_id = platform_id;
        }

        if(!value)
        {
          value = new Platform;
          value->name = rs->get_string(POS_NAME);
          value->type = rs->get_string(POS_TYPE);
          if(value->type == "OS")
          {
            value->type.clear();
          }
        }

        if(!rs->is_null(POS_PRIORITY))
        {
          Platform::Detector detector;
          detector.marker = rs->get_string(POS_MARKER);
          detector.match_regexp = rs->get_string(POS_MATCH_REGEXP);
          detector.output_regexp = rs->get_string(POS_OUTPUT_REGEXP);
          detector.priority = rs->get_number<unsigned long>(POS_PRIORITY);
          value->detectors.push_back(detector);
        }
        else if (value->type == "APPLICATION")
        {
          value->detectors.push_back(Platform::Detector());
        }
      }

      if(value)
      {
        new_config->platforms.activate(
          prev_platform_id,
          value,
          sysdate,
          old_map);
      }

      if(old_config)
      {
        new_config->platforms.deactivate_nonactive(
          old_config->platforms, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  static const std::string def("_");

  const std::string CampaignConfigDBSource::generate_key_(
    const std::list<BehavioralParameterDef>& bps)
    /*throw(eh::Exception)*/
  {
    if(bps.empty())
    {
      return def;
    }
    std::string res;
    res.reserve(128);
    res += def;
    for(std::list<BehavioralParameterDef>::const_iterator kiter = bps.begin();
        kiter != bps.end(); ++kiter)
    {
      if(kiter != bps.begin())
      {
        res.push_back('/');
      }
      res.push_back(kiter->trigger_type);
      String::StringManip::IntToStr(kiter->time_from).str().append_to(res);
      res.push_back('_');
      String::StringManip::IntToStr(kiter->time_to).str().append_to(res);
      res.push_back('_');
      String::StringManip::IntToStr(kiter->min_visits).str().append_to(res);
      res.push_back('_');
      String::StringManip::IntToStr(kiter->weight).str().append_to(res);
    }
    return res;
  }

  void CampaignConfigDBSource::query_behavioral_parameters_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_behavioral_parameters_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum BP_FIELDS1
      {
        BP_LIST_ID_LINK = 1,
        THRESHOLD,
        MIN_VISITS,
        TIME_FROM,
        TIME_TO,
        TRIGGER_TYPE,
        WEIGHT
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT behav_params_list_id, "
            "threshold, "
            "min_visits, "
            "t_from, "
            "t_to, "
            "trigger_type, "
            "weight "
          "FROM adserver.query_behavioral_parameters() ");

      Commons::Postgres::ResultSet_var rs1 = conn->execute_statement(stmt);

      bool new_lid = false, next = rs1->next(), last = false;
      unsigned long lid = 0, old_lid = 0;
      struct BehavioralParameterDef bp_def;
      BehavioralParameterListDef_var elem, old_elem;
      while(next || last)
      {
        if(next)
        {
          lid = rs1->get_number<unsigned long>(BP_LIST_ID_LINK);
          new_lid = lid != old_lid;
        }
        if(new_lid || last)
        {
          if(elem)
          {
            if(old_config)
            {
              new_config->behav_param_lists.activate(
                old_lid,
                elem,
                sysdate,
                &old_config->behav_param_lists);

            }
            else
            {
              new_config->behav_param_lists.activate(
                old_lid,
                elem,
                sysdate);
            }
          }
          last = false;
          if(next)
          {
            if(old_config)
            {
              BehavioralParameterMap::ActiveMap::const_iterator old_it =
                old_config->behav_param_lists.active().find(lid);
              if(old_it != old_config->behav_param_lists.active().end())
              {
                elem = new BehavioralParameterListDef(*old_it->second);
                elem->behave_params.clear();
              }
              else
              {
                elem = new BehavioralParameterListDef;
              }
            }
            else
            {
              elem = new BehavioralParameterListDef;
            }
            elem->threshold = rs1->get_number<unsigned long>(THRESHOLD);
            elem->timestamp = sysdate;
            old_lid = lid;
            new_lid = false;
          }
        }
        if(next)
        {
          bp_def.min_visits = rs1->get_number<unsigned long>(MIN_VISITS);
          bp_def.time_from = rs1->get_number<unsigned long>(TIME_FROM);
          bp_def.time_to = rs1->get_number<unsigned long>(TIME_TO);
          bp_def.weight = rs1->get_number<unsigned long>(WEIGHT);
          bp_def.trigger_type = rs1->get_char(TRIGGER_TYPE);
          elem->behave_params.push_back(bp_def);
          next = rs1->next();
          last = !next;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception on fetching "
        "adserver.query_behavioral_parameters() : " << ex.what();
      throw Exception(ostr);
    }
    try
    {
      enum BP_FIELDS2
      {
        CH_ID = 1,
        CH_TYPE,
        MIN_VISITS,
        TIME_FROM,
        TIME_TO,
        TRIGGER_TYPE,
        WEIGHT
      };

      Commons::Postgres::Statement_var stmt2 =
        new Commons::Postgres::Statement(
          "SELECT channel_id, "
            "channel_type, "
            "min_visits, "
            "t_from, "
            "t_to, "
            "trigger_type, "
            "weight "
          "FROM adserver.query_channel_behavioral_parameters()");

      Commons::Postgres::ResultSet_var rs2 = conn->execute_statement(stmt2);
      if(rs2->next())
      {
        bool next = true;
        struct BehavioralParameterDef bp_def;
        BehavioralParameterListDef_var elem;
        unsigned long ch_id, old_ch_id;
        ch_id = rs2->get_number<unsigned long>(CH_ID);
        old_ch_id = ch_id - 1;
        do
        {
          if(next)
          {
            ch_id = rs2->get_number<unsigned long>(CH_ID);
          }
          if(!next || ch_id != old_ch_id)
          {
            if(elem)
            {
              const std::string key = generate_key_(elem->behave_params);
              if(old_config)
              {
                new_config->str_behav_param_lists.activate(
                  key,
                  elem,
                  sysdate,
                  &old_config->str_behav_param_lists);
              }
              else
              {
                new_config->str_behav_param_lists.activate(
                  key,
                  elem,
                  sysdate);
              }
              SimpleChannelMap::ActiveMap::iterator sit =
                new_config->simple_channels.active().find(old_ch_id);
              if(sit != new_config->simple_channels.active().end())
              {
                SimpleChannelDef_var simple_channel(
                  new SimpleChannelDef(*(sit->second)));
                simple_channel->str_behav_param_list_id = key;

                new_config->simple_channels.activate(
                  old_ch_id,
                  simple_channel,
                  sysdate,
                  &new_config->simple_channels);
              }
            }
            if(next)
            {
              elem = new BehavioralParameterListDef;
              elem->threshold = 1;
              elem->timestamp = sysdate;
              old_ch_id = ch_id;
            }
            else
            {
              elem.reset();
            }
          }
          if (next)
          {
            if (rs2->is_null(TRIGGER_TYPE))
            {
              const char channel_type = rs2->get_char(CH_TYPE);

              if (channel_type == 'P')
              {//create fake BP for placement blacklist
                bp_def.min_visits = 1;
                bp_def.time_from = 0;
                bp_def.time_to = 0;
                bp_def.weight = 1;
                bp_def.trigger_type = 'U';
                elem->behave_params.push_back(bp_def);
              }
            }
            else
            {
              bp_def.min_visits = rs2->get_number<unsigned long>(MIN_VISITS);
              bp_def.time_from = rs2->get_number<unsigned long>(TIME_FROM);
              bp_def.time_to = rs2->get_number<unsigned long>(TIME_TO);
              bp_def.weight = rs2->get_number<unsigned long>(WEIGHT);
              bp_def.trigger_type = rs2->get_char(TRIGGER_TYPE);
              elem->behave_params.push_back(bp_def);
            }
          }
        }
        while((next = rs2->next()) || elem);
      }

      new_config->str_behav_param_lists.activate(
        audience_channel_behav_params_key_,
        audience_channel_behav_params_,
        sysdate,
        old_config ? &old_config->str_behav_param_lists : nullptr);

      if(old_config)
      {
        new_config->behav_param_lists.deactivate_nonactive(
          old_config->behav_param_lists,
          sysdate);

        new_config->str_behav_param_lists.deactivate_nonactive(
          old_config->str_behav_param_lists,
          sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception on fetching "
        "adserver.query_channel_behavioral_parameters(): " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_simple_channels_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_simple_channels_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      {
        /* collect simple channels
         * requirements: simple channel can't be deleted from DB,
         *   it deleted logicaly if status = 'D'
         */
        enum
        {
          POS_CHANNEL_ID = 1,
          POS_BP_LIST_ID,
          POS_THRESHOLD,
          POS_COUNTRY,
          POS_STATUS,
          POS_CHANNEL_TYPE
        };

// Commented due to 3.5.6 DB issue
//         Commons::Postgres::Statement_var stmt =
//           new Commons::Postgres::Statement(
//           "SELECT "
//             "channel_id, "
//             "behav_params_list_id, "
//             "threshold, "
//             "country_code, "
//             "adserver_status, "
//             "channel_type "
//           "FROM adserver.query_channel_behavioral_parameters_list($1)");

//         stmt->set_timestamp(1, sysdate - pending_expire_time_);

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
          "SELECT "
            "channel_id, "
            "behav_params_list_id, "
            "threshold, "
            "country_code, "
            "adserver_status, "
            "channel_type "
          "FROM adserver.query_channel_behavioral_parameters_list()");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

        while (rs->next())
        {
          const char status = rs->get_char(POS_STATUS);

          if(channel_statuses_.empty() ||
            strchr(channel_statuses_.c_str(), status) != 0)
          {
            const unsigned long channel_id = rs->get_number<unsigned long>(POS_CHANNEL_ID);
            const char channel_type = rs->get_char(POS_CHANNEL_TYPE);

            SimpleChannelDef_var s_channel;
            if(old_config)
            {
              SimpleChannelMap::ActiveMap::const_iterator sit_old =
                old_config->simple_channels.active().find(channel_id);
              if(sit_old != old_config->simple_channels.active().end())
              {
                s_channel = new SimpleChannelDef(*sit_old->second);
              }
            }

            if(!s_channel.in())
            {
              s_channel = new SimpleChannelDef();
            }

            s_channel->channel_id = channel_id;
            s_channel->status = status;
            s_channel->country = !rs->is_null(POS_COUNTRY) ?
              rs->get_string(POS_COUNTRY) : "";

            if(!rs->is_null(POS_BP_LIST_ID))
            {
              s_channel->behav_param_list_id = rs->get_number<unsigned long>(POS_BP_LIST_ID);
              s_channel->str_behav_param_list_id.clear();
            }
            else if (channel_type == AUDIENCE_CHANNEL_TYPE)
            {
              s_channel->str_behav_param_list_id = audience_channel_behav_params_key_;
            }
            else
            {
              s_channel->behav_param_list_id = 0;
            }

            s_channel->threshold = rs->get_number<unsigned long>(POS_THRESHOLD);

            config->simple_channels.activate(
              channel_id,
              s_channel,
              sysdate,
              old_config ? &old_config->simple_channels : 0);
          }
        }

        if(old_config)
        {
          config->simple_channels.deactivate_nonactive(
            old_config->simple_channels, sysdate);
        }
      }

      enum
      {
        CC_CHANNEL_ID = 1,
        CC_CHANNEL_CATEGORY_ID
      };

      /* fill channel categories */
      Commons::Postgres::Statement_var cat_stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "channel_id, "
            "category_channel_id "
          "FROM "
            "adserver.query_channel_categories()");

      Commons::Postgres::ResultSet_var cat_rs =
        conn->execute_statement(cat_stmt);

      SimpleChannelDef::CategoryIdSet cur_categories;

      bool rs_next = cat_rs->next();
      unsigned long cur_channel_id = rs_next ?
        cat_rs->get_number<unsigned long>(CC_CHANNEL_ID) : 0;

      for(SimpleChannelMap::ActiveMap::iterator sit =
            config->simple_channels.active().begin();
          sit != config->simple_channels.active().end(); )
      {
        if(!rs_next || sit->first < cur_channel_id)
        {
          SimpleChannelDef_var simple_channel(
            new SimpleChannelDef(*(sit->second)));
          simple_channel->categories.swap(cur_categories);
          simple_channel->timestamp = sysdate;
          config->simple_channels.activate(
            sit->first,
            simple_channel,
            sysdate,
            &config->simple_channels);

          cur_categories.clear();
          ++sit;
        }
        else
        {
          // rs_next is true
          if(sit->first == cur_channel_id)
          {
            cur_categories.insert(
              cat_rs->get_number<unsigned long>(CC_CHANNEL_CATEGORY_ID));
          }

          rs_next = cat_rs->next();
          cur_channel_id = rs_next ?
            cat_rs->get_number<unsigned long>(CC_CHANNEL_ID) : 0;
        }
      }
    }
    catch(const Commons::Postgres::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Commons::Postgres::Exception "
        "on fetching simple channels: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_geo_channels_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    UpdatingState* updating_state,
    const CampaignConfig* old_config,
    const UpdatingState* old_updating_state,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_geo_channels_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      {
        // force db fetching if geo channels isn't changed,
        // this optimization don't limit direct Channel deleting from DB
        // in this case will be changed number of channels,
        // or version (if created channels number compensate deleted number)
        // but any channel version must be updated, if updated geo channel
        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
          "SELECT max_version, cnt "
          "FROM adserver.query_geo_channels_max_version() ");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);
        if (rs->next())
        {
          updating_state->geo_channels_max_db_version = rs->get_date(1);
          updating_state->geo_channels_db_count = rs->get_number<unsigned long>(2);
        }
        else
        {
          updating_state->geo_channels_db_count = 0;
        }
      }

      if(!old_updating_state ||
         updating_state->geo_channels_max_db_version !=
           old_updating_state->geo_channels_max_db_version ||
         updating_state->geo_channels_db_count !=
           old_updating_state->geo_channels_db_count)
      {
        config->geo_channels = new GeoChannelMap();

        // required recheck db channels
        enum
        {
          POS_CHANNEL_ID = 1,
          POS_COUNTRY,
          POS_GEOIP_TARGET_REGION,
          POS_GEOIP_TARGET_CITIES,
        };

        Commons::Postgres::Statement_var stmt =
          new Commons::Postgres::Statement(
          "SELECT "
            "channel_id, "
            "country_code, "
            "name, "
            "city_list "
          "FROM adserver.query_geo_channels()");

        Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

        while (rs->next())
        {
          unsigned long channel_id = rs->get_number<unsigned long>(POS_CHANNEL_ID);

          GeoChannelDef_var channel(new GeoChannelDef());
          channel->country = rs->get_string(POS_COUNTRY);

          std::string parent_geotargets = rs->get_string(POS_GEOIP_TARGET_REGION);
          std::string geotargets = rs->get_string(POS_GEOIP_TARGET_CITIES);
          std::list<String::SubString> tgt_regions;
          std::list<String::SubString> tgt_cities;

          {
            String::StringManip::SplitNL ptokenizer(parent_geotargets);

            String::SubString tgt_region_cur;
            while(ptokenizer.get_token(tgt_region_cur))
            {
              tgt_regions.push_back(tgt_region_cur);
            }
          }

          if(tgt_regions.empty())
          {
            // cities without region
            tgt_regions.push_back(String::SubString());
          }

          {
            String::StringManip::SplitNL tokenizer(geotargets);

            String::SubString tgt_city_cur;
            while(tokenizer.get_token(tgt_city_cur))
            {
              String::SubString::SizeType pos = tgt_city_cur.find('#');
              if(pos != String::SubString::NPOS)
              {
                GeoChannelDef::GeoIPTarget geo_target;
                geo_target.region.assign(tgt_city_cur.begin(), pos);
                geo_target.city.assign(tgt_city_cur.begin() + pos + 1, tgt_city_cur.end());
                channel->geoip_targets.push_back(geo_target);
              }
              else
              {
                tgt_cities.push_back(tgt_city_cur);
              }
            }
          }

          // multiple regions & cities
          for(std::list<String::SubString>::const_iterator region_it =
                tgt_regions.begin();
              region_it != tgt_regions.end(); ++region_it)
          {
            if(tgt_cities.empty())
            {
              GeoChannelDef::GeoIPTarget geo_target;
              geo_target.region = region_it->str();
              channel->geoip_targets.push_back(geo_target);
            }
            else
            {
              for(std::list<String::SubString>::const_iterator city_it =
                    tgt_cities.begin();
                  city_it != tgt_cities.end(); ++city_it)
              {
                GeoChannelDef::GeoIPTarget geo_target;
                geo_target.region = region_it->str();
                geo_target.city = city_it->str();
                channel->geoip_targets.push_back(geo_target);
              }
            }
          }

          config->geo_channels->activate(
            channel_id,
            channel,
            sysdate,
            old_config ? old_config->geo_channels : GeoChannelMap_var());
        }

        if(old_config)
        {
          config->geo_channels->deactivate_nonactive(
            *(old_config->geo_channels), sysdate);
        }
      }
      else
      {
        config->geo_channels = old_config->geo_channels;
      }
    }
    catch(const Commons::Postgres::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught Commons::Postgres::Exception: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_geo_coord_channels_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_geo_coord_channels_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_CHANNEL_ID = 1,
        POS_LONGITUDE,
        POS_LATITUDE,
        POS_RADIUS,
        POS_RADIUS_UNIT
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "channel_id, "
          "longitude,"
          "latitude,"
          "radius,"
          "radius_units "
        "FROM adserver.query_geo_coord_channels() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        unsigned long channel_id = rs->get_number<unsigned long>(POS_CHANNEL_ID);

        GeoCoordChannelDef_var channel(new GeoCoordChannelDef());
        channel->longitude = rs->get_decimal<CoordDecimal>(POS_LONGITUDE);
        channel->latitude = rs->get_decimal<CoordDecimal>(POS_LATITUDE);
        AccuracyDecimal radius = rs->get_decimal<AccuracyDecimal>(POS_RADIUS);
        std::string radius_unit = rs->get_string(POS_RADIUS_UNIT);
        if(radius_unit == "km")
        {
          radius = AccuracyDecimal::mul(
            radius, AccuracyDecimal(false, 1000, 0), Generics::DMR_CEIL);
        }
        else if(radius_unit == "yd")
        {
          radius = AccuracyDecimal::mul(
            radius, YARD_IN_METERS, Generics::DMR_CEIL);
        }
        else if(radius_unit == "mi")
        {
          radius = AccuracyDecimal::mul(
            radius, MILE_IN_METERS, Generics::DMR_CEIL);
        }

        channel->radius = radius;

        config->geo_coord_channels.activate(
          channel_id,
          channel,
          sysdate,
          old_config ? &(old_config->geo_coord_channels) : 0);
      }

      if(old_config)
      {
        config->geo_channels->deactivate_nonactive(
          *(old_config->geo_channels), sysdate);
      }
    }
    catch(const Commons::Postgres::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught Commons::Postgres::Exception: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_channels_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_channels_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      ChannelIdSet targeted_channel_ids;
      for(CampaignMap::ActiveMap::const_iterator cmp_it =
          config->campaigns.active().begin();
        cmp_it != config->campaigns.active().end();
        ++cmp_it)
      {
        cmp_it->second->expression.all_channels(targeted_channel_ids);
      }

      enum
      {
        POS_CH_ID = 1,
        POS_CHANNEL_TYPE,
        POS_CH_COUNTRY_CODE,
        POS_CH_STATUS,
        POS_ACCOUNT_ID,
        POS_CH_NAME,
        POS_CH_FLAGS,
        POS_CH_VISIBILITY,
        POS_CH_FREQ_CAP_ID,
        POS_PARENT_CHANNEL_ID,
        POS_CH_EXPRESSION,
        POS_CH_DISCOVER_QUERY,
        POS_CH_DISCOVER_ANNOTATION,
        POS_CH_LANGUAGE,
        POS_CH_RATE_ID,
        POS_CH_CPM,
        POS_CH_CPC,
        POS_CH_BLOCK_SIZE_ID
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "channel_id, "
          "channel_type, "
          "country_code, "
          "adserver_status, "
          "account_id, "
          "name, "
          "flags, "
          "visibility, "
          "freq_cap_id, "
          "parent_channel_id, "
        // expression channel fields
          "expression, "
        // discover channel fields
          "discover_query, "
          "discover_annotation, "
          "language, "
        // cmp channel fields
          "channel_rate_id, "
          "cpm, "
          "cpc, "
          "size_id "
        "FROM adserver.query_channels()");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        char status = rs->get_char(POS_CH_STATUS);

        if(channel_statuses_.empty() ||
          strchr(channel_statuses_.c_str(), status) != 0)
        {
          unsigned long channel_id = rs->get_number<unsigned long>(POS_CH_ID);
          std::string expression =
            rs->is_null(POS_CH_EXPRESSION) ? "" : rs->get_string(POS_CH_EXPRESSION);
          std::string channel_name =
            rs->is_null(POS_CH_NAME) ? "" : rs->get_string(POS_CH_NAME);
          std::string country_code =
            rs->is_null(POS_CH_COUNTRY_CODE) ? "" : rs->get_string(POS_CH_COUNTRY_CODE);
          char channel_type = rs->get_char(POS_CHANNEL_TYPE);

          ChannelParams channel_params;
          channel_params.channel_id = channel_id;
          channel_params.type = channel_type;
          channel_params.country = country_code;
          channel_params.status = status;
          channel_params.timestamp = sysdate;

          ChannelParams::CommonParams_var ch_common_params(
            new ChannelParams::CommonParams());
          ch_common_params->account_id = rs->get_number<unsigned long>(POS_ACCOUNT_ID);
          ch_common_params->language = (!rs->is_null(POS_CH_LANGUAGE) ?
            rs->get_string(POS_CH_LANGUAGE) : std::string());
          ch_common_params->flags = rs->get_number<unsigned long>(POS_CH_FLAGS);
          if(targeted_channel_ids.find(channel_id) != targeted_channel_ids.end())
          {
            ch_common_params->flags = ch_common_params->flags | ChannelFlags::TARGETED;
          }
          ch_common_params->is_public = rs->get_string(POS_CH_VISIBILITY) == "PUB";
          ch_common_params->freq_cap_id = rs->get_number<unsigned long>(POS_CH_FREQ_CAP_ID);
          channel_params.common_params = ch_common_params;

          ChannelParams::DescriptiveParams_var ch_descriptive_params(
            new ChannelParams::DescriptiveParams());
          ch_descriptive_params->name = channel_name;
          ch_descriptive_params->parent_channel_id =
            rs->get_number<unsigned long>(POS_PARENT_CHANNEL_ID);
          channel_params.descriptive_params = ch_descriptive_params;

          if(!rs->is_null(POS_CH_RATE_ID))
          {
            ChannelParams::CMPParams_var cmp_params(
              new ChannelParams::CMPParams(
                rs->get_number<unsigned long>(POS_CH_RATE_ID),
                rs->get_decimal<RevenueDecimal>(POS_CH_CPM),
                rs->get_decimal<RevenueDecimal>(POS_CH_CPC)));
            channel_params.cmp_params = cmp_params;
          }

          if(!rs->is_null(POS_CH_DISCOVER_QUERY))
          {
            ChannelParams::DiscoverParams_var discover_params(
              new ChannelParams::DiscoverParams(
                rs->get_string(POS_CH_DISCOVER_QUERY).c_str(),
                rs->get_string(POS_CH_DISCOVER_ANNOTATION).c_str()));
            channel_params.discover_params = discover_params;
          }

          try
          {
            NonLinkedExpressionChannel_var result_channel;

            if(channel_type == 'E' || channel_type == 'V' || channel_type == 'T')
            {
              result_channel = ExpressionChannelParser::parse(expression);
              result_channel->params(channel_params);
            }
            else
            {
              result_channel = new NonLinkedExpressionChannel(channel_params);
            }

            if(old_config)
            {
              ChannelMap::ActiveMap::const_iterator ch_it =
                old_config->expression_channels.active().find(channel_id);
              if(ch_it != old_config->expression_channels.active().end())
              {
                // check equality : use new channel only
                // if channel changed (excluding status value)
                if(channel_equal(ch_it->second, result_channel, true))
                {
                  result_channel = ch_it->second;
                }
              }
            }

            if(channel_type == 'P')
            {
              BlockChannelDef_var block_channel(new BlockChannelDef());
              block_channel->size_id = rs->get_number<unsigned long>(POS_CH_BLOCK_SIZE_ID);
              block_channel->timestamp = sysdate;
              config->block_channels.activate(channel_id, block_channel);
            }

            config->expression_channels.activate(channel_id, result_channel);
          }
          catch(const ExpressionChannelParser::Exception& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": Channel with id = " << channel_id <<
              " have incorrect expression '" << expression << "': " <<
              ex.what();

            logger_->log(
              ostr.str(),
              Logging::Logger::ERROR,
              Aspect::CAMPAIGN_SERVER,
              "ADS-IMPL-6068");
          }
        } // channel_statuses_ check
      }
    }
    catch(const Commons::Postgres::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught Commons::Postgres::Exception "
        "on fetching channel info: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Expression channels fetching caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    if(old_config)
    {
      config->block_channels.deactivate_nonactive(
        old_config->block_channels, sysdate);
      config->expression_channels.deactivate_nonactive(
        old_config->expression_channels, sysdate);
    }
  }

  bool
  CampaignConfigDBSource::is_system_option_(
    CreativeDef::SystemOptions& sys_options,
    const CreativeOptionMap::ActiveMap& creative_options,
    long option_id,
    const String::SubString& value)
    /*throw(InvalidObject)*/
  {
    CreativeOptionMap::ActiveMap::const_iterator opt_it =
      creative_options.find(option_id);

    if(opt_it != creative_options.end())
    {
      if(opt_it->second->token == SpecialTokens::ST_CLICK_URL)
      {
        // click url always absolute (option have 'L' type),
        // but can contains other tokens usage - we can only check it
        std::string invalid_url_error;
        if(!HTTP::BrowserChecker()(value, &invalid_url_error))
        {
          Stream::Error ostr;
          ostr << ": invalid click url '" << value << "': " <<
            invalid_url_error;
          throw InvalidObject(ostr);
        }

        value.assign_to(sys_options.click_url);
        sys_options.click_url_option_id = option_id;
        return true;
      }
      else if(opt_it->second->token == SpecialTokens::ST_HTML_URL)
      {
        value.assign_to(sys_options.html_url);
        sys_options.html_url_option_id = option_id;
        return true;
      }
    }

    return false;
  }

  bool CampaignConfigDBSource::check_numeric_option_(
    const char* option,
    bool signed_cmp)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::check_numeric_option_()";

    try
    {
      Stream::Parser istr(option);

      if (signed_cmp)
      {
        long s_buf;
        istr >> s_buf;
      }
      else
      {
        unsigned long u_buf;
        istr >> u_buf;
      }

      if (istr.fail() || istr.bad())
      {
        return false;
      }

      return true;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_creative_option_values_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    typedef std::map<std::string, OptionValue> TokenOptionValueMap;

    static const char* FUN =
      "CampaignConfigDBSource::query_creative_option_values_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_OPTIONS_CCG_ID = 1,
        POS_OPTIONS_CC_ID,
        POS_OPTIONS_OPTION_ID,
        POS_OPTIONS_TOKEN,
        POS_OPTIONS_VALUE
      };

      // use option with highest priority (will be inserted primary)
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT ccg_id, cc_id, option_id, token, val "
          "FROM adserver.query_creative_option_values() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_options_next = rs->next();
      unsigned long cur_options_ccg_id = rs_options_next ?
        rs->get_number<unsigned long>(POS_OPTIONS_CCG_ID) : 0;
      unsigned long cur_options_cc_id = rs_options_next ?
        rs->get_number<unsigned long>(POS_OPTIONS_CC_ID) : 0;
      bool cur_deactivate_cc_id = false;

      // filter options with same token (use option with great priority)
      TokenOptionValueMap cur_tokens;
      CreativeDef::SystemOptions cur_sys_options;

      for (CampaignMap::ActiveMap::iterator ccg_it =
             config->campaigns.active().begin();
           ccg_it != config->campaigns.active().end(); ++ccg_it)
      {
        bool changed = false;
        CreativeList new_creative_list;

        for(CreativeList::const_iterator cc_it =
              ccg_it->second->creatives.begin();
            cc_it != ccg_it->second->creatives.end(); )
        {
          if(!rs_options_next ||
            ccg_it->first < cur_options_ccg_id ||
            (ccg_it->first == cur_options_ccg_id &&
             (*cc_it)->ccid < cur_options_cc_id))
          {
            cur_tokens.insert(std::make_pair(
              AD_IMAGE_PATH_TOKEN,
              OptionValue(
                AD_IMAGE_PATH_OPTION_ID,
                "##ADIMAGE-SERVER##/creatives##ADIMAGE-PATH-SUFFIX##")));

            cur_tokens.insert(std::make_pair(
              CRVBASE_TOKEN,
              OptionValue(
                CRVBASE_OPTION_ID,
                "##ADIMAGE-SERVER##/creatives##ADIMAGE-PATH-SUFFIX##")));

            // if defined MP4_DURATION add MP4_DURATION_HFORMAT
            auto mp4_duration_token_it = cur_tokens.find("MP4_DURATION");

            if(mp4_duration_token_it != cur_tokens.end())
            {
              unsigned long mp4_duration_seconds;
              if(String::StringManip::str_to_int(
                   mp4_duration_token_it->second.value, mp4_duration_seconds))
              {
                cur_tokens.insert(std::make_pair(
                  MP4_DURATION_HFORMAT_TOKEN,
                  OptionValue(
                    MP4_DURATION_HFORMAT_OPTION_ID,
                    Generics::Time(mp4_duration_seconds).get_gm_time().format("%H:%M:%S") + ".000")));
              }
            }

            OptionValueMap cur_options;

            for(TokenOptionValueMap::const_iterator opt_it =
                  cur_tokens.begin();
                opt_it != cur_tokens.end(); ++opt_it)
            {
              cur_options.insert(std::make_pair(
                opt_it->second.option_id,
                opt_it->second.value));
            }

            // load tokens for Pending creatives too for correct instantiate
            if((*cc_it)->tokens.size() != cur_options.size() ||
               !std::equal(cur_options.begin(),
                 cur_options.end(),
                 (*cc_it)->tokens.begin(),
                 Algs::PairEqual()) ||
               !(cur_sys_options == (*cc_it)->sys_options) ||
               (cur_deactivate_cc_id && (
                 (*cc_it)->status == 'A' || (*cc_it)->status == 'P'))
               )
            {
              CreativeDef_var new_creative(new CreativeDef(**cc_it));
              new_creative->tokens.swap(cur_options);
              new_creative->sys_options = cur_sys_options;
              if(cur_deactivate_cc_id)
              {
                new_creative->status = 'D';
              }
              new_creative_list.push_back(new_creative);
              changed = true;
            }
            else
            {
              new_creative_list.push_back(*cc_it);
            }

            cur_sys_options = CreativeDef::SystemOptions();
            cur_tokens.clear();

            cur_deactivate_cc_id = false;

            ++cc_it;
          }
          else
          {
            if(ccg_it->first == cur_options_ccg_id &&
               (*cc_it)->ccid == cur_options_cc_id)
            {
              long option_id = rs->get_number<long>(POS_OPTIONS_OPTION_ID);
              std::string token = rs->get_string(POS_OPTIONS_TOKEN);
              std::string value = rs->get_string(POS_OPTIONS_VALUE);

              try
              {
                if (!is_system_option_(
                  cur_sys_options,
                  config->creative_options.active(),
                  option_id,
                  value))
                {
                  cur_tokens.insert(std::make_pair(
                    token,
                    OptionValue(option_id, value.c_str())));
                }
              }
              catch(const InvalidObject& ex)
              {
                if (check_statuses_(
                      config->accounts.active(),
                      *cc_it,
                      ccg_it->second))
                {
                  cur_deactivate_cc_id = true;

                  Stream::Error ostr;
                  ostr << "Creative #" << cur_options_cc_id << " will be deactivated: " <<
                    ex.what();

                  logger_->log(
                    ostr.str(),
                    Logging::Logger::WARNING,
                    Aspect::TRAFFICKING_PROBLEM,
                    "ADS-TF-6");
                }
              }
            }

            rs_options_next = rs->next();
            cur_options_ccg_id = rs_options_next ?
              rs->get_number<unsigned long>(POS_OPTIONS_CCG_ID) : 0;
            cur_options_cc_id = rs_options_next ?
              rs->get_number<unsigned long>(POS_OPTIONS_CC_ID) : 0;
          }
        }

        if(changed)
        {
          Campaign_var new_campaign(new CampaignDef(*(ccg_it->second)));
          new_campaign->creatives.swap(new_creative_list);
          new_campaign->timestamp = sysdate;
          ccg_it->second = new_campaign;
        }
      }
    }
    catch(const InvalidObject&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_creative_category_values_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "CampaignConfigDBSource::query_creative_category_values_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_CCG_ID = 1,
        POS_CC_ID,
        POS_CATEGORY_ID
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT v.ccg_id, v.cc_id, v.creative_category_id "
          "FROM ("
            "SELECT "
              "cc.ccg_id ccg_id, "
              "cc.cc_id cc_id, "
              "ccc.creative_category_id creative_category_id "
            "FROM CreativeCategory_Creative ccc, "
              "CampaignCreative cc "
            "WHERE cc.creative_id = ccc.creative_id "
            "UNION "
            "SELECT "
              "cc.ccg_id ccg_id, "
              "cc.cc_id cc_id, "
              "cct.creative_category_id creative_category_id "
            "FROM CreativeCategory_Template cct, "
              "Creative cr, "
              "CampaignCreative cc "
            "WHERE cc.creative_id = cr.creative_id AND "
              "cr.template_id = cct.template_id) v "
          "ORDER BY v.ccg_id, v.cc_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
      unsigned long cur_cc_id = rs_next ? rs->get_number<unsigned long>(POS_CC_ID) : 0;
      CreativeCategorySet cur_categories;

      for (CampaignMap::ActiveMap::iterator ccg_it =
             config->campaigns.active().begin();
           ccg_it != config->campaigns.active().end(); ++ccg_it)
      {
        bool changed = false;
        CreativeList new_creative_list;

        for(CreativeList::const_iterator cc_it =
              ccg_it->second->creatives.begin();
            cc_it != ccg_it->second->creatives.end(); )
        {
          if(!rs_next ||
             ccg_it->first < cur_ccg_id ||
             (ccg_it->first == cur_ccg_id &&
               (*cc_it)->ccid < cur_cc_id))
          {
            if((*cc_it)->categories.size() != cur_categories.size() ||
               !std::equal(cur_categories.begin(),
                 cur_categories.end(),
                 (*cc_it)->categories.begin()))
            {
              CreativeDef_var new_creative(new CreativeDef(**cc_it));
              new_creative->categories.swap(cur_categories);
              new_creative_list.push_back(new_creative);
              changed = true;
            }
            else
            {
              new_creative_list.push_back(*cc_it);
            }

            cur_categories.clear();
            ++cc_it;
          }
          else
          {
            if(ccg_it->first == cur_ccg_id &&
               (*cc_it)->ccid == cur_cc_id)
            {
              cur_categories.insert(rs->get_number<unsigned long>(POS_CATEGORY_ID));
            }

            rs_next = rs->next();
            cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
            cur_cc_id = rs_next ? rs->get_number<unsigned long>(POS_CC_ID) : 0;
          }
        }

        if(changed)
        {
          Campaign_var new_campaign(new CampaignDef(*(ccg_it->second)));
          new_campaign->creatives.swap(new_creative_list);
          new_campaign->timestamp = sysdate;
          ccg_it->second = new_campaign;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignConfigDBSource::query_contracts_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_contracts_()";

    try
    {
      ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

      enum
      {
        POS_CONTRACT_ID = 1,
        POS_CONTRACT_DATE,
        POS_CONTRACT_NUMBER,
        POS_CONTRACT_TYPE,
        POS_ORD_CONRACT_ID,
        POS_ORD_ADO_ID,
        POS_VAT_INCLUDED,
        POS_SUBJECT_TYPE,
        POS_ACTION_TYPE,
        POS_AGENT_ACTING_FOR_PUBLISHER,
        POS_PARENT_CONTRACT_ID,
        POS_CONTRACTOR_ID,
        POS_CONTRACTOR_NAME,
        POS_CONTRACTOR_LEGAL_FORM,
        POS_CLIENT_ID,
        POS_CLIENT_NAME,
        POS_CLIENT_LEGAL_FORM
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "contract.id,"
            "to_char(contract.date, 'YYYY-MM-DD'),"
            "contract.number,"
            "contract.contract_type,"
            "contract.contract_id_ord::text as ord_contract_id,"
            "lower(contract.integration_system_name) AS ado_id,"
            "(case when contract.is_vat then 1 else 0 end) as vat_included,"
            "contract.subject as subject_type,"
            "contract.action as action_type,"
            "(case when contract.is_agent then 1 else 0 end) as agent_acting_for_publisher,"
            "contract.parent_contract_id AS parent_contract_id,"
            "contractor.inn,"
            "contractor.name,"
            "contractor.type as contractor_legal_form,"
            "client.inn,"
            "client.name,"
            "client.type as client_legal_form "
          "FROM contract "
            "JOIN organization AS client ON(client.id = contract.client_organization_id) "
            "JOIN organization AS contractor ON(contractor.id = contract.contractor_organization_id)"
          );

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while (rs->next())
      {
        ContractDef_var contract(new ContractDef());
        const unsigned long contract_id = rs->get_number<unsigned long>(POS_CONTRACT_ID);
        contract->contract_id = contract_id;

        contract->number = rs->get_string(POS_CONTRACT_NUMBER);
        contract->date = rs->get_string(POS_CONTRACT_DATE);
        contract->type = rs->get_string(POS_CONTRACT_TYPE);
        contract->vat_included = (
          !rs->is_null(POS_VAT_INCLUDED) ? (rs->get_number<unsigned int>(POS_VAT_INCLUDED) != 0) :
          false);

        contract->ord_contract_id = rs->get_string(POS_ORD_CONRACT_ID);
        contract->ord_ado_id = rs->get_string(POS_ORD_ADO_ID);
        contract->subject_type = rs->get_string(POS_SUBJECT_TYPE);
        contract->action_type = rs->get_string(POS_ACTION_TYPE);
        contract->agent_acting_for_publisher = rs->get_number<unsigned long>(POS_AGENT_ACTING_FOR_PUBLISHER);
        contract->parent_contract_id = (!rs->is_null(POS_PARENT_CONTRACT_ID) ?
          rs->get_number<unsigned long>(POS_PARENT_CONTRACT_ID) : 0);

        contract->client_id = rs->get_string(POS_CLIENT_ID);
        contract->client_name = rs->get_string(POS_CLIENT_NAME);
        contract->client_legal_form = rs->get_string(POS_CLIENT_LEGAL_FORM);

        contract->contractor_id = rs->get_string(POS_CONTRACTOR_ID);
        contract->contractor_name = rs->get_string(POS_CONTRACTOR_NAME);
        contract->contractor_legal_form = rs->get_string(POS_CONTRACTOR_LEGAL_FORM);

        config->contracts.activate(
          contract_id,
          contract,
          sysdate,
          old_config ? &old_config->contracts : 0);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_campaign_creatives_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_campaign_creatives_()";

    try
    {
      ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

      enum
      {
        POS_CCG_ID = 1,
        POS_CCID,
        POS_CREATIVE_ID,
        POS_FC_ID,
        POS_CRFORMAT,
        POS_WEIGHT,
        POS_SEQ_SET_ID,
        POS_VERSION_ID,
        POS_STATUS
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "ccg_id, "
          "cc_id, "
          "creative_id, "
          "fc_id, "
          "crformat, "
          "weight, "
          "set_number, "
          "version_id, "
          "status "
        "FROM "
          "adserver.query_campaign_creatives(NULL, 'all', $1)");

      stmt->set_timestamp(1, sysdate - pending_expire_time_);

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      CreativeList cur_creatives;
      bool rs_next = rs->next();
      unsigned long cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;

      for (CampaignMap::ActiveMap::iterator ccg_it =
             config->campaigns.active().begin();
           ccg_it != config->campaigns.active().end(); )
      {
        if(!rs_next || ccg_it->first < cur_ccg_id)
        {
          const CreativeList& current_campaign_creatives =
            ccg_it->second->creatives;

          bool list_changed = false;
          CreativeList::const_iterator left_cr_it =
            current_campaign_creatives.begin();
          CreativeList::iterator right_cr_it = cur_creatives.begin();
          CreativeList result_creatives;

          for(; left_cr_it != current_campaign_creatives.end() &&
                right_cr_it != cur_creatives.end(); )
          {
            if((*left_cr_it)->ccid < (*right_cr_it)->ccid)
            {
              list_changed = true;
              ++left_cr_it;
            }
            else if((*left_cr_it)->ccid > (*right_cr_it)->ccid)
            {
              list_changed = true;
              result_creatives.push_back(*right_cr_it);
              ++right_cr_it;
            }
            else
            {
              if((*left_cr_it)->ccid == (*right_cr_it)->ccid &&
                (*left_cr_it)->fc_id == (*right_cr_it)->fc_id &&
                (*left_cr_it)->format == (*right_cr_it)->format &&
                (*left_cr_it)->status == (*right_cr_it)->status &&
                (*left_cr_it)->order_set_id == (*right_cr_it)->order_set_id &&
                (*left_cr_it)->version_id == (*right_cr_it)->version_id && (
                  ccg_it->second->creative_optimization() ||
                  (*left_cr_it)->weight == (*right_cr_it)->weight))
              {
                result_creatives.push_back(*left_cr_it);
              }
              else
              {
                list_changed = true;
                result_creatives.push_back(*right_cr_it);
              }

              ++left_cr_it;
              ++right_cr_it;
            }
          }

          list_changed |= left_cr_it != current_campaign_creatives.end() ||
            right_cr_it != cur_creatives.end();

          std::copy(right_cr_it,
            cur_creatives.end(), std::back_inserter(result_creatives));

          if(list_changed)
          {
            Campaign_var new_campaign(new CampaignDef(*(ccg_it->second)));
            new_campaign->creatives.swap(result_creatives);
            new_campaign->timestamp = sysdate;
            ccg_it->second = new_campaign;
          }

          cur_creatives.clear();
          ++ccg_it;
        }
        else
        {
          if(ccg_it->first == cur_ccg_id)
          {
            unsigned long ccid = rs->get_number<unsigned long>(POS_CCID);

            try
            {
              char creative_status = rs->get_char(POS_STATUS);
              std::string crformat = rs->get_string(POS_CRFORMAT);

              if(ccg_it->second->ccg_type == CT_TEXT &&
                 crformat != TEXT_CREATIVE_FORMAT)
              {
                Stream::Error ostr;
                ostr << FUN << ": Creative linked to Text type CCG has incorrect format '" <<
                  crformat << "' (non " << TEXT_CREATIVE_FORMAT <<
                  ") and will be ignored: cc_id = " <<
                  ccid;

                logger_->log(
                  ostr.str(),
                  Logging::Logger::ERROR,
                  Aspect::CAMPAIGN_SERVER,
                  "ADS-IMPL-6068");

                creative_status = 'D';
              }

              if(creative_status == 'A' || creative_status == 'P')
              {
                CreativeDef_var new_creative(
                  new CreativeDef(
                    ccid,
                    rs->get_number<unsigned long>(POS_CREATIVE_ID),
                    (rs->is_null(POS_FC_ID) ? 0 : rs->get_number<unsigned long>(POS_FC_ID)),
                    rs->get_number<unsigned long>(POS_WEIGHT),
                    crformat.c_str(),
                    rs->get_string(POS_VERSION_ID).c_str(),
                    creative_status,
                    CreativeDef::SystemOptions() // sys options
                    ));

                new_creative->order_set_id = rs->get_number<unsigned long>(POS_SEQ_SET_ID);

                cur_creatives.push_back(new_creative);
              }
            }
            catch(const Commons::Postgres::Exception& e)
            {
              logger_->sstream(Logging::Logger::ERROR,
                Aspect::CAMPAIGN_SERVER,
                "ADS-IMPL-161") <<
                ": creative ccid = " << ccid <<
                ". Postgres::Exception: " << e.what();
            }
            catch(const InvalidObject& ex)
            {
              logger_->sstream(Logging::Logger::ERROR,
                Aspect::CAMPAIGN_SERVER,
               "ADS-IMPL-161") << FUN <<
                ": Invalid creative ccid = " << ccid <<
                ". Reason: " << ex.what();
            }
          }

          rs_next = rs->next();
          cur_ccg_id = rs_next ? rs->get_number<unsigned long>(POS_CCG_ID) : 0;
        }
      } // campaigns loop
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    query_creative_option_values_(
      conn,
      config,
      sysdate);

    // query creative sizes and its options
    try
    {
      ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

      enum
      {
        POS_SIZES_CCG_ID = 1,
        POS_SIZES_CC_ID,
        POS_SIZES_EXPANDABLE,
        POS_SIZES_EXPAND_DIRECTION
      };

      Commons::Postgres::Statement_var stmt_sizes =
        new Commons::Postgres::Statement(
          "SELECT "
            "ccg_id,"
            "cc_id,"
            "expandable,"
            "expansion "
          "FROM adserver.query_creative()");

      enum
      {
        POS_OPTIONS_CCG_ID = 1,
        POS_OPTIONS_CC_ID,
        POS_OPTIONS_SIZE_ID,
        POS_OPTIONS_OPTION_ID,
        POS_OPTIONS_TOKEN,
        POS_OPTIONS_VALUE
      };

      Commons::Postgres::Statement_var stmt_options =
        new Commons::Postgres::Statement(
        "SELECT "
          "ccg_id,"
          "cc_id,"
          "size_id,"
          "option_id,"
          "token,"
          "val "
        "FROM adserver.query_creative_option_value_with_sizes()");

      Commons::Postgres::ResultSet_var rs_sizes =
        conn->execute_statement(stmt_sizes, true);
      Commons::Postgres::ResultSet_var rs_options =
        conn->execute_statement(stmt_options, true);

      bool rs_sizes_next = rs_sizes->next();
      unsigned long cur_sizes_ccg_id = 0;
      unsigned long cur_sizes_cc_id = 0;

      bool cur_expandable = false;
      std::string cur_expand_direction;

      if(rs_sizes_next)
      {
        cur_sizes_ccg_id = rs_sizes->get_number<unsigned long>(POS_SIZES_CCG_ID);
        cur_sizes_cc_id = rs_sizes->get_number<unsigned long>(POS_SIZES_CC_ID);
        cur_expandable = (rs_sizes->get_char(POS_SIZES_EXPANDABLE) == 'Y');
        if(cur_expandable)
        {
          cur_expand_direction = rs_sizes->get_string(POS_SIZES_EXPAND_DIRECTION);
        }
      }

      bool rs_options_next = rs_options->next();
      CurrSizeMap cur_sizes;
      unsigned long cur_options_ccg_id = rs_options_next ?
        rs_options->get_number<unsigned long>(POS_OPTIONS_CCG_ID) : 0;
      unsigned long cur_options_cc_id = rs_options_next ?
        rs_options->get_number<unsigned long>(POS_OPTIONS_CC_ID) : 0;
      bool cur_deactivate_cc_id = false;

      for (CampaignMap::ActiveMap::iterator ccg_it =
             config->campaigns.active().begin();
           ccg_it != config->campaigns.active().end();
           ++ccg_it)
      {
        bool changed = false;
        CreativeList new_creative_list;

        for(CreativeList::const_iterator cc_it =
              ccg_it->second->creatives.begin();
            cc_it != ccg_it->second->creatives.end(); )
        {
          bool opt_less = !rs_options_next ||
            ccg_it->first < cur_options_ccg_id ||
            (ccg_it->first == cur_options_ccg_id &&
              (*cc_it)->ccid < cur_options_cc_id);

          bool cs_less = !rs_sizes_next ||
            ccg_it->first < cur_sizes_ccg_id ||
            (ccg_it->first == cur_sizes_ccg_id &&
              (*cc_it)->ccid < cur_sizes_cc_id);

          if(opt_less && cs_less)
          {
            // eval and setup WIDTH, HEIGHT
            for(auto cr_size_it = cur_sizes.begin();
                cr_size_it != cur_sizes.end(); )
            {
              SizeMap::ActiveMap::const_iterator size_it =
                config->sizes.active().find(cr_size_it->first);

              if(size_it != config->sizes.active().end())
              {
                CurrSize& curr_size = cr_size_it->second;
                unsigned long result_width = 0;
                unsigned long result_height = 0;

                if (curr_size.cur_options_width)
                {
                  result_width = curr_size.cur_options_width;
                  curr_size.tokens.insert(std::make_pair(
                    DEFAULT_CREATIVE_WIDTH_OPTION_ID,
                    IntToStr(result_width).str().str()));
                }
                else
                {
                  const auto ti = find_token(
                    **cc_it,
                    config->creative_options,
                    DEFAULT_CREATIVE_WIDTH_TOKEN);

                  if (ti != (*cc_it)->tokens.end() && !ti->second.empty())
                  {
                    String::StringManip::str_to_int(
                      ti->second, result_width);
                  }
                  else
                  {
                    result_width = size_it->second->width;
                    curr_size.tokens.insert(std::make_pair(
                      DEFAULT_CREATIVE_WIDTH_OPTION_ID,
                      IntToStr(result_width).str().str()));
                  }
                }

                if (curr_size.cur_options_height)
                {
                  result_height = curr_size.cur_options_height;
                  curr_size.tokens.insert(std::make_pair(
                    DEFAULT_CREATIVE_HEIGHT_OPTION_ID,
                    IntToStr(result_height).str().str()));
                }
                else
                {
                  const auto ti = find_token(
                    **cc_it,
                    config->creative_options,
                    DEFAULT_CREATIVE_HEIGHT_TOKEN);

                  if (ti != (*cc_it)->tokens.end() && !ti->second.empty())
                  {
                    String::StringManip::str_to_int(
                      ti->second, result_height);
                  }
                  else
                  {
                    result_height = size_it->second->height;
                    curr_size.tokens.insert(std::make_pair(
                      DEFAULT_CREATIVE_HEIGHT_OPTION_ID,
                      IntToStr(result_height).str().str()));
                  }
                }

                if(cur_expandable)
                {
                  const unsigned long result_max_width =
                    curr_size.cur_options_max_width != 0 ?
                      curr_size.cur_options_max_width : size_it->second->max_width;

                  const unsigned long result_max_height =
                    curr_size.cur_options_max_height != 0 ?
                      curr_size.cur_options_max_height : size_it->second->max_height;

                  curr_size.tokens.insert(std::make_pair(
                    CREATIVE_EXPAND_DIRECTION_OPTION_ID,
                    cur_expand_direction.c_str()));

                  curr_size.tokens.insert(std::make_pair(
                    CREATIVE_MAX_WIDTH_OPTION_ID,
                    IntToStr(result_max_width).str().str()));

                  curr_size.tokens.insert(std::make_pair(
                    CREATIVE_MAX_HEIGHT_OPTION_ID,
                    IntToStr(result_max_height).str().str()));

                  // calculate expand space values
                  if(result_max_height > result_height)
                  {
                    if(cur_expand_direction.find("UP") != std::string::npos)
                    {
                      curr_size.up_expand_space =
                        result_max_height - result_height;
                    }

                    if(cur_expand_direction.find("DOWN") != std::string::npos)
                    {
                      curr_size.down_expand_space =
                        result_max_height - result_height;
                    }
                  }

                  if(result_max_width > result_width)
                  {
                    if(cur_expand_direction.find("RIGHT") != std::string::npos)
                    {
                      curr_size.right_expand_space =
                        result_max_width - result_width;
                    }

                    if(cur_expand_direction.find("LEFT") != std::string::npos)
                    {
                      curr_size.left_expand_space =
                        result_max_width - result_width;
                    }
                  }
                }

                ++cr_size_it;
              }
              else
              {
                // size appeared after size query but before creative
                // skip it, it will be loaded at next iteration
                cur_sizes.erase(cr_size_it++);
              }
            }

            if((*cc_it)->sizes.size() != cur_sizes.size() ||
               !std::equal(cur_sizes.begin(),
                 cur_sizes.end(),
                 (*cc_it)->sizes.begin(),
                 CurrSizeEquals()) ||
               (cur_deactivate_cc_id && (*cc_it)->status == 'A')
               )
            {
              CreativeDef_var new_creative(new CreativeDef(**cc_it));
              new_creative->sizes.clear();
              new_creative->sizes.insert(cur_sizes.begin(), cur_sizes.end());

              if(cur_deactivate_cc_id)
              {
                new_creative->status = 'D';
              }

              new_creative_list.push_back(new_creative);
              changed = true;
            }
            else
            {
              new_creative_list.push_back(*cc_it);
            }

            cur_expandable = false;
            cur_expand_direction.clear();
            cur_deactivate_cc_id = false;
            cur_sizes.clear();

            ++cc_it;
          }
          else
          {
            if(!opt_less)
            {
              if(ccg_it->first == cur_options_ccg_id &&
                 (*cc_it)->ccid == cur_options_cc_id)
              {
                CurrSize& size = cur_sizes[rs_options->get_number<unsigned long>(POS_OPTIONS_SIZE_ID)];

                if(!rs_options->is_null(POS_OPTIONS_OPTION_ID))
                {
                  const long option_id = rs_options->get_number<long>(POS_OPTIONS_OPTION_ID);
                  std::string value = rs_options->get_string(POS_OPTIONS_VALUE);
                  std::string token = rs_options->get_string(POS_OPTIONS_TOKEN);

                  try
                  {
                    if(token == CREATIVE_MAX_WIDTH_TOKEN)
                    {
                      String::StringManip::str_to_int(value, size.cur_options_max_width);
                    }
                    else if(token == CREATIVE_MAX_HEIGHT_TOKEN)
                    {
                      String::StringManip::str_to_int(value, size.cur_options_max_height);
                    }
                    else if(token == DEFAULT_CREATIVE_WIDTH_TOKEN)
                    {
                      String::StringManip::str_to_int(value, size.cur_options_width);
                    }
                    else if(token == DEFAULT_CREATIVE_HEIGHT_TOKEN)
                    {
                      String::StringManip::str_to_int(value, size.cur_options_height);
                    }
                    else
                    {
                      size.tokens.insert(std::make_pair(
                        option_id, value.c_str()));
                    }
                  }
                  catch(const InvalidObject& ex)
                  {
                    if (check_statuses_(
                          config->accounts.active(),
                          *cc_it,
                          ccg_it->second))
                    {
                      cur_deactivate_cc_id = true;

                      Stream::Error ostr;
                      ostr << "Creative #" << cur_options_cc_id << " will be deactivated: " <<
                        ex.what();

                      logger_->log(
                        ostr.str(),
                        Logging::Logger::WARNING,
                        Aspect::TRAFFICKING_PROBLEM,
                        "ADS-TF-6");
                    }
                  }
                }
              }

              rs_options_next = rs_options->next();
              cur_options_ccg_id = rs_options_next ?
                rs_options->get_number<unsigned long>(POS_OPTIONS_CCG_ID) : 0;
              cur_options_cc_id = rs_options_next ?
                rs_options->get_number<unsigned long>(POS_OPTIONS_CC_ID) : 0;
            }

            if(!cs_less)
            {
              if(ccg_it->first == cur_sizes_ccg_id &&
                 (*cc_it)->ccid == cur_sizes_cc_id)
              {
                cur_expandable = (rs_sizes->get_char(POS_SIZES_EXPANDABLE) == 'Y');

                if(cur_expandable)
                {
                  cur_expand_direction = rs_sizes->get_string(POS_SIZES_EXPAND_DIRECTION);
                }
              }

              rs_sizes_next = rs_sizes->next();
              cur_sizes_ccg_id = rs_sizes_next ?
                rs_sizes->get_number<unsigned long>(POS_SIZES_CCG_ID) : 0;
              cur_sizes_cc_id = rs_sizes_next ?
                rs_sizes->get_number<unsigned long>(POS_SIZES_CC_ID) : 0;
            }
          }
        }

        if(changed)
        {
          Campaign_var new_campaign(new CampaignDef(*(ccg_it->second)));
          new_campaign->creatives.swap(new_creative_list);
          new_campaign->timestamp = sysdate;
          ccg_it->second = new_campaign;
        }
      } // campaigns loop
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    query_creative_category_values_(
      conn,
      config,
      sysdate);
  }

  void CampaignConfigDBSource::query_campaign_keywords_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_campaign_keywords_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_CCG_KEYWORD_ID = 1,
        POS_ORIGINAL_KEYWORD,
        POS_CLICK_URL
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "ccg_keyword_id, "
          "original_keyword, "
          "click_url "
        "FROM adserver.query_campaign_keywords() ");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        unsigned long ccg_keyword_id =
          rs->get_number<unsigned long>(POS_CCG_KEYWORD_ID);

        config->campaign_keywords.activate(
          ccg_keyword_id,
          CampaignKeyword_var(
            new CampaignKeyword(
              ccg_keyword_id,
              rs->get_string(POS_ORIGINAL_KEYWORD).c_str(),
              rs->get_string(POS_CLICK_URL).c_str())),
            sysdate,
            old_config ? &(old_config->campaign_keywords) : 0);
      }

      if(old_config)
      {
        config->campaign_keywords.deactivate_nonactive(
          old_config->campaign_keywords, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_freq_caps_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_freq_caps_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      std::set<unsigned long> used_fc;

      {
        /* collect used freq caps */
        for(SiteMap::ActiveMap::const_iterator s_it =
              new_config->sites.active().begin();
            s_it != new_config->sites.active().end(); ++s_it)
        {
          if (s_it->second->freq_cap_id)
          {
            used_fc.insert(s_it->second->freq_cap_id);
          }
        }

        for(CampaignMap::ActiveMap::const_iterator c_it =
              new_config->campaigns.active().begin();
            c_it != new_config->campaigns.active().end(); ++c_it)
        {
          if (c_it->second->fc_id)
          {
            used_fc.insert(c_it->second->fc_id);
          }

          if(c_it->second->group_fc_id)
          {
            used_fc.insert(c_it->second->group_fc_id);
          }

          for(CreativeList::const_iterator cr_it =
                c_it->second->creatives.begin();
              cr_it != c_it->second->creatives.end(); ++cr_it)
          {
            if((*cr_it)->fc_id)
            {
              used_fc.insert((*cr_it)->fc_id);
            }
          }
        }

        for(ChannelMap::ActiveMap::const_iterator ch_it =
              new_config->expression_channels.active().begin();
            ch_it != new_config->expression_channels.active().end();
            ++ch_it)
        {
          if(ch_it->second.in() &&
             ch_it->second->params().common_params.in() &&
             ch_it->second->params().common_params->freq_cap_id)
          {
            used_fc.insert(ch_it->second->params().common_params->freq_cap_id);
          }
        }
      }

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "freq_cap_id fc_id, "
            "coalesce(life_count, 0) as life_count, "
            "coalesce(period, 0) as period, "
            "coalesce(window_count, 0) as window_count, "
            "coalesce(window_length, 0) as window_length "
          "FROM FreqCap");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      enum
      {
        POS_FC_ID = 1,
        POS_LIFELIMIT,
        POS_PERIOD,
        POS_WINDOWLIMIT,
        POS_WINDOWTIME,
      };

      while (rs->next())
      {
        unsigned long fc_id = rs->get_number<unsigned long>(POS_FC_ID);

        if(used_fc.find(fc_id) != used_fc.end())
        {
          FreqCapDef_var new_elem(new FreqCapDef());
          new_elem->fc_id = fc_id;
          new_elem->lifelimit = rs->get_number<unsigned long>(POS_LIFELIMIT);
          new_elem->period = rs->get_number<unsigned long>(POS_PERIOD);
          new_elem->window_limit =
            rs->get_number<unsigned long>(POS_WINDOWLIMIT);
          new_elem->window_time = rs->get_number<unsigned long>(POS_WINDOWTIME);
          new_config->freq_caps.activate(
            fc_id,
            new_elem,
            sysdate,
            old_config ? &old_config->freq_caps : 0);
        }
      }

      if(old_config)
      {
        new_config->freq_caps.deactivate_nonactive(old_config->freq_caps, sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  CampaignConfigDBSource::query_creative_templates_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* new_config,
    const CampaignConfig* old_config,
    const Generics::Time& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_creative_templates_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    // query templates
    try
    {
      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "ct.template_id, "
            "ct.name crformat, "
            "coalesce(ctf.flags, 0), "
            "ctf.template_file templfile, "
            "ctf.template_type, "
            "cs.protocol_name sizename, "
            "af.name appformat, "
            "af.mime_type mime_type "
          "FROM "
            "Template ct "
              "LEFT JOIN TemplateFile ctf "
                "ON(ct.template_id = ctf.template_id) "
              "LEFT JOIN CreativeSize cs "
                "ON(ctf.size_id = cs.size_id) "
              "LEFT JOIN AppFormat af "
                "ON(ctf.app_format_id = af.app_format_id) "
          "WHERE ct.status = 'A' AND "
            "ctf.template_file_id IS NOT NULL "
          "ORDER BY ct.template_id, ctf.template_file_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      enum
      {
        POS_TEMPL_ID = 1,
        POS_CRFORMAT,
        POS_FLAGS,
        POS_TEMPLFILE,
        POS_TYPE,
        POS_SIZENAME,
        POS_APPFORMAT,
        POS_MIME_FORMAT,
      };

      unsigned long cur_template_id = 0;
      CreativeTemplateDef_var cur_template(new CreativeTemplateDef());

      while (rs->next())
      {
        unsigned long templ_id = rs->get_number<unsigned long>(POS_TEMPL_ID);
        std::string ct_appformat = rs->get_string(POS_APPFORMAT);
        std::string ct_size = rs->get_string(POS_SIZENAME);
        std::string ct_file = rs->get_string(POS_TEMPLFILE);
        std::string ct_mime_format = rs->get_string(POS_MIME_FORMAT);
        char ct_type = rs->get_char(POS_TYPE);

        // deactivate creative template if it isn't valid
        std::string ct_format = rs->get_string(POS_CRFORMAT);

        if(cur_template_id && cur_template_id != templ_id)
        {
          new_config->creative_templates.activate(
            cur_template_id,
            cur_template,
            sysdate,
            old_config ? &old_config->creative_templates : 0);

          cur_template.reset();

          if(old_config)
          {
            CreativeTemplateMap::ActiveMap::const_iterator old_templ_it =
              old_config->creative_templates.active().find(templ_id);
            if(old_templ_it != old_config->creative_templates.active().end())
            {
              cur_template = new CreativeTemplateDef(*(old_templ_it->second));
              cur_template->files.clear();
            }
          }

          if(!cur_template.in())
          {
            cur_template = new CreativeTemplateDef();
          }
        }

        cur_template_id = templ_id;

        AdServer::CampaignSvcs::CreativeTemplateType res_type;

        if(ct_type == 'T')
        {
          res_type = AdServer::CampaignSvcs::CTT_TEXT;
        }
        else if(ct_type == 'X')
        {
          res_type = AdServer::CampaignSvcs::CTT_XSLT;
        }
        else
        {
          Stream::Error ostr;
          ostr << "Invalid template type '" << ct_type <<
            "' in tempate with key (" << ct_format << ", " <<
            ct_size << ", " << ct_appformat << ")";

          throw Exception(ostr);
        }

        cur_template->files.push_back(
          CreativeTemplateFileDef(
            ct_format.c_str(),
            ct_size.c_str(),
            ct_appformat.c_str(),
            ct_mime_format.c_str(),
            ((rs->get_number<unsigned int>(POS_FLAGS) & 0x01) != 0),
            res_type,
            ct_file.c_str()));
      }

      if(cur_template_id)
      {
        new_config->creative_templates.activate(
          cur_template_id,
          cur_template,
          sysdate,
          old_config ? &old_config->creative_templates : 0);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // query template options
    try
    {
      enum
      {
        POS_TEMPLATE_ID = 1,
        POS_OPTION_ID,
        POS_OPTION_VALUE,
        POS_OPTION_TYPE
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "templ.template_id,"
            "o.option_id,"
            "o.default_value, "
            "SUBSTR(og.type, 1, 1)::char as type "
          "FROM Options o "
            "INNER JOIN OptionGroup og ON("
              "og.option_group_id = o.option_group_id) "
            "INNER JOIN Template templ ON(templ.template_id = og.template_id) "
          "WHERE og.type = 'Publisher' OR ("
            "og.type = 'Hidden' AND o.type <> 'Dynamic File') "
          "ORDER BY templ.template_id");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();
      unsigned long cur_template_id =
        rs_next ? rs->get_number<unsigned long>(POS_TEMPLATE_ID) : 0;
      OptionValueMap cur_tokens;
      OptionValueMap cur_hidden_tokens;

      for(CreativeTemplateMap::ActiveMap::iterator templ_it =
            new_config->creative_templates.active().begin();
          templ_it != new_config->creative_templates.active().end(); )
      {
        if(!rs_next || templ_it->first < cur_template_id)
        {
          if(templ_it->second->tokens.size() != cur_tokens.size() ||
             !std::equal(cur_tokens.begin(),
               cur_tokens.end(),
               templ_it->second->tokens.begin(),
               Algs::PairEqual()) ||
             templ_it->second->hidden_tokens.size() != cur_hidden_tokens.size() ||
             !std::equal(cur_hidden_tokens.begin(),
               cur_hidden_tokens.end(),
               templ_it->second->hidden_tokens.begin(),
               Algs::PairEqual()))
          {
            CreativeTemplateDef_var new_templ(
              new CreativeTemplateDef(*(templ_it->second)));
            new_templ->tokens.swap(cur_tokens);
            new_templ->hidden_tokens.swap(cur_hidden_tokens);
            new_templ->timestamp = sysdate;
            templ_it->second = new_templ;
          }

          cur_tokens.clear();
          cur_hidden_tokens.clear();
          ++templ_it;
        }
        else
        {
          if(templ_it->first == cur_template_id)
          {
            if(rs->get_char(POS_OPTION_TYPE) == 'P')
            {
              cur_tokens.insert(std::make_pair(
                rs->get_number<unsigned long>(POS_OPTION_ID),
                rs->get_string(POS_OPTION_VALUE)));
            }
            else
            {
              cur_hidden_tokens.insert(std::make_pair(
                rs->get_number<unsigned long>(POS_OPTION_ID),
                rs->get_string(POS_OPTION_VALUE)));
            }
          }

          rs_next = rs->next();
          cur_template_id =
            rs_next ? rs->get_number<unsigned long>(POS_TEMPLATE_ID) : 0;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception on options query: " << ex.what();
      throw Exception(ostr);
    }

    // deactivate templates that isn't present in query
    if(old_config)
    {
      new_config->creative_templates.deactivate_nonactive(
        old_config->creative_templates,
        sysdate);
    }
  }

  void CampaignConfigDBSource::query_creative_categories_(
    Commons::Postgres::Connection* conn,
    const CampaignConfig* old_config,
    CampaignConfig* new_config,
    const TimestampValue& sysdate)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_creative_categories_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    try
    {
      enum
      {
        POS_CREATIVE_CATEGORY_ID = 1,
        POS_CCT_ID,
        POS_NAME,
        POS_RTB_NAME,
        POS_KEY
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "creative_category_id,"
            "cct_id,"
            "name,"
            "rtb_name,"
            "key "
          "FROM adserver.query_creative_categories()");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool next;
      unsigned long cat_id = 0;
      CreativeCategoryDef_var cr_cat;
      while ((next = rs->next()) || cr_cat.in())
      {
        unsigned long cur_id =
          next ? rs->get_number<unsigned long>(POS_CREATIVE_CATEGORY_ID) : 0;
        if(cur_id != cat_id || !next)
        {
          if(cr_cat.in())
          {
            new_config->creative_categories.activate(
              cat_id,
              cr_cat,
              sysdate,
              old_config ? &old_config->creative_categories : 0);
            cr_cat = nullptr;
          }

          if(next)
          {
            cat_id = cur_id;
            cr_cat = new CreativeCategoryDef;
            cr_cat->cct_id = rs->get_number<unsigned long>(POS_CCT_ID);
            cr_cat->name = rs->get_string(POS_NAME);
          }
          else
          {
            break;
          }
        }

        if(!rs->is_null(POS_RTB_NAME))
        {
          bool ignore = false;
          const std::string rtb_name = rs->get_string(POS_RTB_NAME);
          AdRequestType rt_type = AR_NORMAL;
          if(rtb_name == RtbNames::IAB_NAME)
          {
            rt_type = AR_OPENRTB;
          }
          else if(rtb_name == RtbNames::OPENX_NAME)
          {
            rt_type = AR_OPENX;
          }
          else if(rtb_name == RtbNames::APPNEXUS_NAME)
          {
            rt_type = AR_APPNEXUS;
          }
          else if(rtb_name == RtbNames::GOOGLE_NAME)
          {
            rt_type = AR_GOOGLE;
          }
          else
          {
            ignore = true;
          }

          if(!ignore)
          {
            cr_cat->external_categories[rt_type].insert(
              rs->get_string(POS_KEY));
          }
        }
      }

      if(old_config)
      {
        new_config->creative_categories.deactivate_nonactive(
          old_config->creative_categories,
          sysdate);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void CampaignConfigDBSource::query_simple_channel_triggers_(
    Commons::Postgres::Connection* conn,
    CampaignConfig* config,
    const CampaignConfig* old_config,
    const TimestampValue& sysdate)
    /*throw(Exception)*/
  {
    static const char* FUN = "CampaignConfigDBSource::query_simple_channel_triggers_()";

    ExecutionTimeTracer exec_tracer(FUN, Aspect::CAMPAIGN_SERVER, logger_);

    // collect channels that assigned to ccg's
    ChannelIdSet check_channels;

    for(CampaignMap::ActiveMap::const_iterator cmp_it =
          config->campaigns.active().begin();
        cmp_it != config->campaigns.active().end(); ++cmp_it)
    {
      cmp_it->second->expression.all_channels(check_channels);
    }

    ChannelIdSet ad_linked_channels;
    ChannelIdSet checked_channels;

    while(!check_channels.empty())
    {
      ChannelIdSet new_check_channels;
      for(ChannelIdSet::const_iterator check_ch_it =
            check_channels.begin();
          check_ch_it != check_channels.end();
          ++check_ch_it)
      {
        SimpleChannelMap::ActiveMap::const_iterator sch_it =
          config->simple_channels.active().find(*check_ch_it);

        if(sch_it != config->simple_channels.active().end())
        {
          ad_linked_channels.insert(*check_ch_it);
        }
        else
        {
          ChannelMap::ActiveMap::const_iterator ch_it =
            config->expression_channels.active().find(*check_ch_it);

          if(ch_it != config->expression_channels.active().end() &&
             ch_it->second.in() &&
             ch_it->second->expression())
          {
            ch_it->second->expression()->all_channels(new_check_channels);
          }

          checked_channels.insert(*check_ch_it);
        }
      }

      ChannelIdSet filtered_check_channels;
      for(ChannelIdSet::iterator ch_it = new_check_channels.begin();
          ch_it != new_check_channels.end(); ++ch_it)
      {
        if(checked_channels.find(*ch_it) == checked_channels.end() &&
           ad_linked_channels.find(*ch_it) == ad_linked_channels.end())
        {
          filtered_check_channels.insert(*ch_it);
        }
      }

      check_channels.swap(filtered_check_channels);
    }

    try
    {
      enum
      {
        POS_CHANNEL_ID = 1,
        POS_CHANNEL_TRIGGER_ID,
        POS_TRIGGER_TYPE
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT channel_id, channel_trigger_id, trigger_type "
          "FROM adserver.get_channel_triggers($1)");

      Generics::Time start_date = old_config ?
        old_config->db_stamp - Generics::Time::ONE_DAY :
        Generics::Time::ZERO;

      stmt->set_timestamp(1, start_date);

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      bool rs_next = rs->next();

      SimpleChannelMap::ActiveMap::iterator sch_it =
        config->simple_channels.active().end();
      unsigned long cur_channel_id = 0;
      if(rs_next)
      {
        cur_channel_id = rs->get_number<long>(POS_CHANNEL_ID);
        sch_it = config->simple_channels.active().lower_bound(cur_channel_id);
      }
      unsigned long old_channel_id = 0;

      std::vector<unsigned long> cur_page_triggers;
      std::vector<unsigned long> cur_search_triggers;
      std::vector<unsigned long> cur_url_triggers;
      std::vector<unsigned long> cur_url_keyword_triggers;
      cur_page_triggers.reserve(10*1024);
      cur_search_triggers.reserve(10*1024);
      cur_url_triggers.reserve(10*1024);
      cur_url_keyword_triggers.reserve(10*1024);

      for (SimpleChannelMap::ActiveMap::iterator sch_it =
             config->simple_channels.active().begin();
           sch_it != config->simple_channels.active().end(); )
      {
        if(old_channel_id != cur_channel_id && old_channel_id)
        {
          SimpleChannelDef::MatchParams_var prev_match_params =
            sch_it->second->match_params;

          if((prev_match_params.in() &&
               (prev_match_params->page_triggers.size() !=
                 cur_page_triggers.size() ||
               prev_match_params->search_triggers.size() !=
                 cur_search_triggers.size() ||
               prev_match_params->url_triggers.size() !=
                 cur_url_triggers.size() ||
               prev_match_params->url_keyword_triggers.size() !=
                 cur_url_keyword_triggers.size() ||
               !std::equal(cur_page_triggers.begin(),
                 cur_page_triggers.end(),
                 prev_match_params->page_triggers.begin()) ||
               !std::equal(cur_search_triggers.begin(),
                 cur_search_triggers.end(),
                 prev_match_params->search_triggers.begin()) ||
               !std::equal(cur_url_triggers.begin(),
                 cur_url_triggers.end(),
                 prev_match_params->url_triggers.begin()) ||
               !std::equal(cur_url_keyword_triggers.begin(),
                 cur_url_keyword_triggers.end(),
                 prev_match_params->url_keyword_triggers.begin())
                )) ||
             (!prev_match_params.in() &&
               ((!cur_page_triggers.empty() ||
                !cur_search_triggers.empty() ||
                !cur_url_triggers.empty() ||
                !cur_url_keyword_triggers.empty()))))
          {
            SimpleChannelDef::MatchParams_var new_match_params;
            if(!cur_page_triggers.empty() ||
               !cur_search_triggers.empty() ||
               !cur_url_triggers.empty() ||
               !cur_url_keyword_triggers.empty())
            {
              new_match_params = new SimpleChannelDef::MatchParams();
              new_match_params->page_triggers.swap(cur_page_triggers);
              new_match_params->search_triggers.swap(cur_search_triggers);
              new_match_params->url_triggers.swap(cur_url_triggers);
              new_match_params->url_keyword_triggers.swap(cur_url_keyword_triggers);
            }

            SimpleChannelDef_var new_channel(new SimpleChannelDef(*(sch_it->second)));
            new_channel->timestamp = sysdate;
            new_channel->match_params = new_match_params;
            sch_it->second = new_channel;
          }

          if(!cur_channel_id)
          { //no more records from db
            break;
          }
          cur_page_triggers.clear();
          cur_search_triggers.clear();
          cur_url_triggers.clear();
          cur_url_keyword_triggers.clear();
          old_channel_id = 0;
        }
        else
        {
          if(sch_it->first < cur_channel_id || !cur_channel_id)
          {
            ++sch_it;
          }
          else
          {
            if(sch_it->first == cur_channel_id &&
               ad_linked_channels.find(sch_it->first) != ad_linked_channels.end())
            {
              if(!rs->is_null(POS_TRIGGER_TYPE) && !rs->is_null(POS_CHANNEL_TRIGGER_ID))
              {
                char trigger_type = rs->get_char(POS_TRIGGER_TYPE);
                if(trigger_type == 'P')
                {
                  cur_page_triggers.push_back(
                    rs->get_number<long>(POS_CHANNEL_TRIGGER_ID));
                }
                else if(trigger_type == 'S')
                {
                  cur_search_triggers.push_back(
                    rs->get_number<long>(POS_CHANNEL_TRIGGER_ID));
                }
                else if(trigger_type == 'U')
                {
                  cur_url_triggers.push_back(
                    rs->get_number<long>(POS_CHANNEL_TRIGGER_ID));
                }
                else
                {
                  cur_url_keyword_triggers.push_back(
                    rs->get_number<long>(POS_CHANNEL_TRIGGER_ID));
                }
              }
              old_channel_id = cur_channel_id;
            }
            rs_next = rs->next();
            cur_channel_id = rs_next ? rs->get_number<long>(POS_CHANNEL_ID) : 0;
          }
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
}
