#ifndef _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_SERVER_CAMPAIGN_SERVER_IMPL_HPP_
#define _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_SERVER_CAMPAIGN_SERVER_IMPL_HPP_

#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer_s.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignCommons_s.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include "CampaignConfig.hpp"
#include "CampaignConfigDBSource.hpp"
#include "CampaignConfigServerSource.hpp"
#include "CampaignServerStatValues.hpp"
#include "CampaignServerLogger.hpp"

#include "BillStatSource.hpp"
#include "BillStatDBSource.hpp"
#include "BillStatServerSource.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    typedef CampaignServerStatValues ProcStatImpl;

    typedef ::ReferenceCounting::SmartPtr<ProcStatImpl> ProcStatImpl_var;

    /** Implements corba interface CampaignServer
     *  to stored configuration.
     *  Function to fill configuration was on inherited classes
     */
    class CampaignServerBaseImpl:
      public virtual Generics::CompositeActiveObject,
      public virtual CORBACommons::ReferenceCounting::
        ServantImpl<POA_AdServer::CampaignSvcs::CampaignServer>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(InvalidArgument, Exception);

    public:
      /** Parametric constructor, provides CampaignServer the callback
       *  and logger pointers.
       *
       * @param callback Pointer to a callback object instance.
       * @param logger Pointer to a logger object instance.
       */
      CampaignServerBaseImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        ProcStatImpl* proc_stat_impl,
        unsigned int config_update_period,
        unsigned int ecpm_update_period,
        const Generics::Time& bill_stat_update_period,
        const LogFlushTraits& colo_update_flush_traits,
        unsigned long colo_id,
        const char* version)
        /*throw(InvalidArgument, Exception, eh::Exception)*/;

      virtual void change_db_state(bool new_state) /*throw(Exception)*/;

      virtual bool get_db_state() /*throw(Commons::DbStateChanger::NotSupported)*/;

      void update_config() noexcept;

      virtual CampaignConfigUpdateInfo* get_config(
        const CampaignGetConfigSettings& settings)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual CORBA::Boolean need_config(
        const TimestampInfo& req_timestamp)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/;

      virtual EcpmSeq* get_ecpms(
        const TimestampInfo& request_timestamp)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/;

      virtual AdServer::CampaignSvcs::BriefSimpleChannelAnswer*
      brief_simple_channels(
        const AdServer::CampaignSvcs::CampaignServer::
          GetSimpleChannelsInfo& settings)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual AdServer::CampaignSvcs::SimpleChannelAnswer*
      simple_channels(
        const AdServer::CampaignSvcs::CampaignServer::
          GetSimpleChannelsInfo& settings)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual AdServer::CampaignSvcs::ChannelServerChannelAnswer*
      chsv_simple_channels(
        const AdServer::CampaignSvcs::CampaignServer::
        GetSimpleChannelsInfo& settings)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;


      virtual
      AdServer::CampaignSvcs::ExpressionChannelsInfo*
      get_expression_channels(
        const AdServer::CampaignSvcs::CampaignServer::
          GetExpressionChannelsInfo& request_settings)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::CampaignServer::PassbackInfo*
      get_tag_passback(CORBA::ULong tag_id, const char* app_format)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::FraudConditionConfig*
      fraud_conditions()
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::DetectorsConfig*
      detectors(const AdServer::CampaignSvcs::TimestampInfo& request_timestamp)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::FreqCapConfigInfo*
      freq_caps()
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      ColocationFlagsSeq*
      get_colocation_flags()
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::CampaignServer::ColocationPropInfo*
      get_colocation_prop(CORBA::ULong colo_id)
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::CampaignServer::DeliveryLimitConfigInfo*
      get_delivery_limit_config()
        /*throw(AdServer::CampaignSvcs::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs::BillStatInfo*
      get_bill_stat()
        /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

    protected:
      virtual
      ~CampaignServerBaseImpl() noexcept;

      typedef Sync::Policy::PosixThread SyncPolicy;

      struct TraceLevel
      {
        enum
        {
          LOW = Logging::Logger::TRACE,
          MIDDLE,
          HIGH
        };
      };

      typedef Generics::TaskGoal TaskMessageBase;
      typedef ReferenceCounting::SmartPtr<TaskMessageBase>
        TaskMessageBase_var;

      class FlushLogsTask: public TaskMessageBase
      {
      public:
        FlushLogsTask(
          CampaignServerBaseImpl* server,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;

      protected:
        virtual
        ~FlushLogsTask() noexcept = default;

      private:
        CampaignServerBaseImpl* server_;
      };

      class UpdateConfigTaskMessage: public TaskMessageBase
      {
      public:
        UpdateConfigTaskMessage(
          CampaignServerBaseImpl* server,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;

      protected:
        virtual
        ~UpdateConfigTaskMessage() noexcept = default;

      private:
        CampaignServerBaseImpl* server_;
      };

      class UpdateBillStatTask: public TaskMessageBase
      {
      public:
        UpdateBillStatTask(
          CampaignServerBaseImpl* server,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;

      protected:
        virtual
        ~UpdateBillStatTask() noexcept = default;

      private:
        CampaignServerBaseImpl* server_;
      };

    protected:
      virtual void report_error(
        Generics::ActiveObject* object,
        Generics::ActiveObjectCallback::Severity severity,
        const char* description, const char* error_code = 0) noexcept;

      CampaignConfig_var campaign_config() /*throw(eh::Exception)*/;

      virtual CampaignConfigSource_var
      get_campaign_config_source() noexcept = 0;

      virtual BillStatSource_var
      get_bill_stat_source() noexcept = 0;

      // tasks
      void update_bill_stat_() noexcept;

      /* get_config help methods */
      /* ObjectAdapterType:
       *   bool to_fill(const Object)
       *   bool to_deactivate(const Object)
       *   void activate(ObjectActiveSeqType::value_type&, const Object)
       *   void deactivate(ObjectDeletedSeqType::value_type&, const Object)
       *   void deactivate(ObjectDeletedSeqType::value_type&, const DeletedObject)
       */
      template<
        typename ObjectActiveSeqType,
        typename ObjectDeletedSeqType,
        typename ObjectContainerType,
        typename ObjectAdapterType>
      void fill_object_update_sequences_(
        ObjectActiveSeqType& active_object_seq,
        ObjectDeletedSeqType* deleted_object_seq,
        const ObjectContainerType& object_container,
        const ObjectAdapterType& object_adapter,
        const char* FILL_OBJECT_TYPE)
        /*throw(eh::Exception)*/;

      void
      fill_app_formats_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void
      fill_sizes_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void
      fill_countries_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;


      void fill_creative_options_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_creative_categories_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_adv_actions_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_simple_channels_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const StringSet& countries,
        const char* channel_statuses,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void
      fill_geo_coord_channels_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void
      fill_block_channels_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_expression_channels_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const StringSet& countries,
        const char* channel_statuses,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_category_channels_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_campaigns_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const StringSet& countries,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_ecpms_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const char* campaign_types,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_accounts_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_colocations_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_freq_caps_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_active_freq_caps_(
        FreqCapSeq& freq_cap_seq,
        const TimestampValue& request_timestamp,
        const FreqCapMap::ActiveMap& freq_caps,
        unsigned long portion,
        unsigned long portions_number)
        /*throw(Exception)*/;

      void
      fill_active_campaign_ids_(
        CampaignIdSeq& campaign_ids,
        const CampaignMap::ActiveMap& campaigns)
        /*throw(Exception)*/;

      void fill_creative_templates_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_sites_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_tags_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_currencies_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_campaign_keywords_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_behavioral_parameters_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        /*throw(Exception)*/;

      void fill_brief_bp_parameters_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        unsigned long portion,
        unsigned long portions_number,
        AdServer::CampaignSvcs::BriefBehavParamInfoSeq& bp_ids,
        AdServer::CampaignSvcs::BriefKeyBehavParamInfoSeq& bp_keys)
          /*throw(eh::Exception)*/;

      void fill_bp_parameters_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        unsigned long portion,
        unsigned long portions_number,
        AdServer::CampaignSvcs::BehavParamInfoSeq& bp_ids,
        AdServer::CampaignSvcs::KeyBehavParamInfoSeq& bp_keys)
          /*throw(eh::Exception)*/;

      void fill_fraud_conditions_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        FraudConditionSeq& fraud_condition_seq,
        DeletedIdSeq* deleted_fraud_condition_seq)
        /*throw(Exception)*/;

      void fill_search_enginies_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        SearchEngineSeq& search_engines,
        AdServer::CampaignSvcs::DeletedIdSeq* del_seq = 0)
        const
        /*throw(Exception)*/;

      static bool filter_campaign_(
        const CampaignConfig* campaign_config,
        const CampaignDef& campaign,
        const char* campaign_types,
        const StringSet& countries)
        noexcept;

      void
      fill_amount_distribution_(
        AdServer::CampaignSvcs::AmountDistributionInfo& amount_distribution_info,
        const BillStatSource::Stat::AmountDistribution& amount_distribution)
        noexcept;

      void
      fill_amount_count_distribution_(
        AdServer::CampaignSvcs::AmountCountDistributionInfo& amount_count_distribution_info,
        const BillStatSource::Stat::AmountCountDistribution& amount_count_distribution)
        noexcept;

      static void
      fill_contracts_(
        const TimestampValue& request_timestamp,
        const CampaignConfig* campaign_config,
        const CampaignGetConfigSettings& settings,
        CampaignConfigUpdateInfo& update_info)
        noexcept;

      /* logging methods */
      void flush_logs_() noexcept;

      template<typename SourceType, typename TimestampOpType>
      unsigned long
      count_updated_instances_(
        const SourceType& src,
        const Generics::Time& request_timestamp,
        unsigned long portion,
        unsigned long portions_number,
        const TimestampOpType& ts_op)
        noexcept;

    protected:
      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;
      ProcStatImpl_var proc_stat_impl_;
      ReferenceCounting::PtrHolder<CampaignConfig_var> campaign_config_;
      ReferenceCounting::PtrHolder<BillStatSource::Stat_var> bill_stat_;

      Generics::TaskRunner_var task_runner_;
      Generics::Planner_var scheduler_;

      Generics::Time config_update_period_;
      Generics::Time ecpm_update_period_;
      Generics::Time flush_period_;
      Generics::Time bill_stat_update_period_;

      CampaignServerLogger_var campaign_server_logger_;
      unsigned long colo_id_;
      std::string version_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignServerBaseImpl>
      CampaignServerBaseImpl_var;

    /**
     * Implements interaction with the Database Server and provides
     * Campaign Managers with campaign updates.
     */
    class CampaignServerImpl: public CampaignServerBaseImpl
    {
    public:
      /** Parametric constructor, provides CampaignServer the callback
       *  and logger pointers as well as database access parameters.
       *
       * @param callback Pointer to a callback object instance.
       * @param logger Pointer to a logger object instance.
       * @param db_conn_info The information for DB connection:
       *   service, name, password
       */
      CampaignServerImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        ProcStatImpl* proc_stat_impl,
        unsigned long colo_id,
        const char* version,
        unsigned int config_update_period,
        unsigned int ecpm_update_period,
        const Generics::Time& bill_stat_update_period,
        const LogFlushTraits& colo_update_flush_traits,
        unsigned long server_id,
        const String::SubString& campaign_statuses,
        const String::SubString& channel_statuses,
        const char* pg_connection_string,
        const Generics::Time& stat_stamp_sync_period,
        const CORBACommons::CorbaObjectRefList& stat_providers,
        const Generics::Time& audience_expiration_time,
        const Generics::Time& pending_expire_time,
        bool enable_delivery_thresholds)
        /*throw(InvalidArgument, Exception, eh::Exception)*/;

      virtual void change_db_state(bool new_state) /*throw(Exception)*/;

      virtual bool get_db_state() noexcept;

      virtual
      void update_stat()
        /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
          AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/;

      virtual
      AdServer::CampaignSvcs::StatInfo*
      get_stat()
        /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;
 
    protected:
      virtual
      ~CampaignServerImpl() noexcept;

      virtual CampaignConfigSource_var
      get_campaign_config_source() noexcept;

      virtual BillStatSource_var
      get_bill_stat_source() noexcept;

    private:
      Commons::Postgres::Environment_var pg_env_;
      CampaignConfigDBSource_var campaign_config_db_source_;
      BillStatDBSource_var bill_stat_db_source_;
    };

    /** Implements campaign proxy server.
     *  Campaign updates was received from other CampaignServer object
     */
    class CampaignServerProxyImpl: public CampaignServerBaseImpl
    {
    public:
      CampaignServerProxyImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        ProcStatImpl* proc_stat_impl,
        unsigned long colo_id,
        const char* version,
        unsigned int config_update_period,
        unsigned int ecpm_update_period,
        const Generics::Time& bill_stat_update_period,
        const LogFlushTraits& colo_update_flush_traits,
        unsigned long server_id,
        const String::SubString& channel_statuses,
        const CORBACommons::CorbaObjectRefList& campaign_server_refs,
        const char* campaigns_types,
        const char* countries,
        bool only_tags)
        /*throw(InvalidArgument, Exception, eh::Exception)*/;

      virtual void change_db_state(bool new_state) /*throw(Exception)*/;

      virtual
      AdServer::CampaignSvcs::StatInfo*
      get_stat()
        /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
          AdServer::CampaignSvcs::CampaignServer::NotReady)*/;

      virtual
      void update_stat()
        /*throw(AdServer::CampaignSvcs::CampaignServer::NotSupport,
          AdServer::CampaignSvcs::CampaignServer::ImplementationException)*/;

    protected:
      virtual
      ~CampaignServerProxyImpl() noexcept;

      virtual CampaignConfigSource_var
      get_campaign_config_source() noexcept;

      virtual BillStatSource_var
      get_bill_stat_source() noexcept;

    private:
      CampaignConfigServerSource_var campaign_config_server_source_;
      BillStatServerSource_var bill_stat_server_source_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignServerImpl>
      CampaignServerProxyImpl_var;
  }
}

/* Inlines */
namespace AdServer
{
  namespace CampaignSvcs
  {
    /* CampaignServerBaseImpl::FlushLogsTask */
    inline
    CampaignServerImpl::FlushLogsTask::FlushLogsTask(
      CampaignServerBaseImpl* server,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        server_(server)
    {}

    inline
    void
    CampaignServerBaseImpl::FlushLogsTask::execute() noexcept
    {
      server_->flush_logs_();
    }

    /* CampaignServerImpl::UpdateConfigTaskMessage */
    inline
    CampaignServerBaseImpl::
    UpdateConfigTaskMessage::UpdateConfigTaskMessage(
      CampaignServerBaseImpl* server,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        server_(server)
    {}

    inline
    void
    CampaignServerBaseImpl::UpdateConfigTaskMessage::execute() noexcept
    {
      server_->update_config();
    }

    // CampaignServerImpl::UpdateBillStatTask
    inline
    CampaignServerBaseImpl::
    UpdateBillStatTask::UpdateBillStatTask(
      CampaignServerBaseImpl* server,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        server_(server)
    {}

    inline
    void
    CampaignServerBaseImpl::UpdateBillStatTask::execute() noexcept
    {
      server_->update_bill_stat_();
    }

    template<typename SourceType, typename TimestampOpType>
    unsigned long
    CampaignServerBaseImpl::count_updated_instances_(
      const SourceType& src,
      const TimestampValue& request_timestamp,
      unsigned long portion,
      unsigned long portions_number,
      const TimestampOpType& ts_op)
      noexcept
    {
      unsigned long result = 0;
      for (typename SourceType::const_iterator it = src.begin();
           it != src.end();
           ++it)
      {
        if(it->first % portions_number == portion &&
           ts_op(it->second) > request_timestamp)
        {
          ++result;
        }
      }
      return result;
    }
  }
}

#endif /*_AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_SERVER_CAMPAIGN_SERVER_IMPL_HPP_*/
