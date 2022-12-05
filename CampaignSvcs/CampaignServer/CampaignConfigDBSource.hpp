#ifndef _CAMPAIGNCONFIGDBSOURCE_HPP_
#define _CAMPAIGNCONFIGDBSOURCE_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/Postgres/ConnectionPool.hpp>

#include "CampaignConfig.hpp"
#include "ModifyConfigSource.hpp"
#include "CampaignConfigModifier.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    class CampaignConfigDBSource:
      public CampaignConfigSource,
      public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignConfigSource::Exception);
      DECLARE_EXCEPTION(NotReady, Exception);

      CampaignConfigDBSource(
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
        /*throw(Exception)*/;

      virtual CampaignConfig_var update(bool* need_logging) /*throw(Exception)*/;

      StatSource::CStat_var stat() const /*throw(NotReady)*/;

      CampaignConfigModifier::CState_var modify_state() const /*throw(NotReady)*/;

      void update_stat() /*throw(Exception)*/;

    private:
      DECLARE_EXCEPTION(InvalidObject, eh::DescriptiveException);

      // internal state of updating, shared between update operations
      class UpdatingState: public virtual ReferenceCounting::AtomicImpl
      {
      public:
        // loading from DB optimization,
        // required for force geo channels reloading if no changes
        Generics::Time geo_channels_max_db_version;
        unsigned long geo_channels_db_count;

      private:
        virtual
        ~UpdatingState() noexcept = default;
      };

      typedef ReferenceCounting::SmartPtr<UpdatingState>
        UpdatingState_var;

      typedef Sync::Policy::PosixThread SyncPolicy;

    private:
      virtual
      ~CampaignConfigDBSource() noexcept;

      static bool check_cost_(
        const RevenueDecimal& val,
        const CampaignConfig* config,
        const CurrencyDef* currency)
        noexcept;

      bool update_ora_config_(
        CampaignConfig* new_config,
        UpdatingState* updating_state)
        /*throw(Exception, eh::Exception)*/;

      Generics::Time 
      query_db_stamp_(
        Commons::Postgres::Connection* conn)
        /*throw(Exception)*/;

      void
      query_app_formats_(
        CampaignConfig* new_config,
        Commons::Postgres::Connection* conn,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void
      query_sizes_(
        CampaignConfig* new_config,
        Commons::Postgres::Connection* conn,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void
      query_global_params_(
        CampaignConfig* new_config,
        Commons::Postgres::Connection* conn,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void
      query_accounts_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_recursive_tokens_(
        Commons::Postgres::Connection* conn)
        /*throw(Exception)*/;

      void query_creative_options_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_sites_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_currency_exchange_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config)
        /*throw(Exception, eh::Exception)*/;

      void query_currencies_(
        Commons::Postgres::Connection* connection,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_tags_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_tag_option_values_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_countries_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_colocations_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_campaigns_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_campaign_expressions_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_campaign_creatives_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_adv_actions_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_geo_coord_channels_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_channels_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_simple_channels_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_simple_channel_triggers_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_geo_channels_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        UpdatingState* updating_state,
        const CampaignConfig* old_config,
        const UpdatingState* updating_old_state,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_campaign_keywords_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_freq_caps_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_creative_templates_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const Generics::Time& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_creative_categories_(
        Commons::Postgres::Connection* conn,
        const CampaignConfig* old_config,
        CampaignConfig* new_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_category_channels_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_behavioral_parameters_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception, eh::Exception)*/;

      void query_fraud_conditions_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_search_engines_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_web_browsers_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_platforms_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_web_operations_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* new_config,
        const CampaignConfig* old_config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_creative_option_values_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      void query_creative_category_values_(
        Commons::Postgres::Connection* conn,
        CampaignConfig* config,
        const TimestampValue& sysdate)
        /*throw(Exception)*/;

      static const std::string generate_key_(
        const std::list<BehavioralParameterDef>& bps)
        /*throw(eh::Exception)*/;

      static bool
      is_system_option_(
        CreativeDef::SystemOptions& sys_options,
        const CreativeOptionMap::ActiveMap& creative_options,
        long option_id,
        const String::SubString& value)
        /*throw(InvalidObject)*/;

      static
      bool check_numeric_option_(
        const char* option,
        bool signed_cmp = false)
        /*throw(Exception, eh::Exception)*/;

      static
      bool check_statuses_(
        const AccountMap::ActiveMap& account_map,
        const CreativeDef* creative,
        const CampaignDef* campaign) noexcept;
       
    private:
      Logging::Logger_var logger_;
      unsigned long server_id_;
      const std::string campaign_statuses_;
      const std::string channel_statuses_;
      const Generics::Time pending_expire_time_;

      AdServer::Commons::Postgres::Environment_var pg_env_;
      AdServer::Commons::Postgres::ConnectionPool_var pg_pool_;

      StatSource_var db_stat_source_;
      CampaignConfigModifier_var campaign_config_modifier_;

      ReferenceCounting::PtrHolder<CampaignConfig_var> campaign_config_;
      UpdatingState_var updating_state_;

      // creative options that can't be defined at DB side
      StringSet generic_tokens_;
      StringSet advertiser_tokens_;
      StringSet publisher_tokens_;
      StringSet internal_tokens_;
      CreativeOptionMap::ActiveMap predefined_options_;

      BehavioralParameterListDef_var audience_channel_behav_params_;
      std::string audience_channel_behav_params_key_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignConfigDBSource>
      CampaignConfigDBSource_var;
  }
}

#endif /*_CAMPAIGNCONFIGDBSOURCE_HPP_*/
