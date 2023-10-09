#ifndef REQUESTINFOSVCS_REQUESTINFOMANAGERIMPL_HPP
#define REQUESTINFOSVCS_REQUESTINFOMANAGERIMPL_HPP

#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Generics/Values.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <Commons/CorbaConfig.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/RequestInfoSvcs/RequestInfoManagerConfig.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <RequestInfoSvcs/RequestInfoManager/RequestInfoManager_s.hpp>

#include "CompositeRequestActionProcessor.hpp"
#include "UserCampaignReachContainer.hpp"
#include "UserActionInfoContainer.hpp"
#include "UserFraudProtectionContainer.hpp"
#include "RequestInfoContainer.hpp"
#include "UserSiteReachContainer.hpp"
#include "UserTagRequestMergeContainer.hpp"
#include "RequestLogLoader.hpp"
#include "RequestOutLogger.hpp"
#include "ExpressionMatcherNotifier.hpp"
#include "RequestOperationDistributor.hpp"
#include <Generics/CompositeMetricsProvider.hpp>

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class RequestInfoManagerStatsImpl;
    typedef ReferenceCounting::SmartPtr<RequestInfoManagerStatsImpl>
      RequestInfoManagerStatsImpl_var;
    class UserFraudDeactivator;

    typedef
      ReferenceCounting::SmartPtr<UserFraudDeactivator>
      UserFraudDeactivator_var;

    /**
     * Implementation of RequestInfoManager.
     * Child objects relations:
     * RequestLogLoader ->
     *   UserActionInfoContainer (process_adv_action, process_custom_action) ->
     *     RequestInfoContainer (process_action)
     *   RequestInfoContainer (merge requests, clicks and actions) ->
     *     UserFraudProtectionContainer -> RequestInfoContainer::proxy
     *     UserActionInfoContainer::RequestActionProcessorImpl (process_click)
     *     ExpressionMatcherNotifier (notify other services about actions),
     *     UserCampaignReachContainer (hook campaign reach appear) ->
     *       RequestOutLogger::process_reach
     *     RequestOutLogger
     *   CompositeTagRequestProcessor ->
     *     UserSiteReachContainer ->
     *       RequestOutLogger::process_site_reach
     *     PassbackContainer::process_tag_request ->
     *       RequestOutLogger::process_passback
     *   PassbackContainer::process_passback_request ->
     *     RequestOutLogger::process_passback
     */
    class RequestInfoManagerImpl:
      public virtual CORBACommons::ReferenceCounting::
        ServantImpl<POA_AdServer::RequestInfoSvcs::RequestInfoManager>,
      public virtual Generics::CompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(NotReady, Exception);

      typedef
        xsd::AdServer::Configuration::RequestInfoManagerConfigType
        RequestInfoManagerConfig;

      typedef std::list<unsigned long> ChunkIdList;

    public:
      RequestInfoManagerImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        const RequestInfoManagerConfig& request_info_manager_config,
        const RequestInfoManagerStatsImpl_var& rim_stats_impl,
        Generics::CompositeMetricsProvider_var composite_metrics_provider)
        /*throw(Exception)*/;

      virtual CORBA::Boolean get_profile(
        const char* request_id,
        AdServer::RequestInfoSvcs::RequestProfile_out request_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean get_user_campaign_reach_profile(
        const char* user_id,
        AdServer::RequestInfoSvcs::UserProfile_out user_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean get_user_action_profile(
        const char* user_id,
        AdServer::RequestInfoSvcs::UserProfile_out user_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean get_passback_profile(
        const char* request_id,
        AdServer::RequestInfoSvcs::PassbackProfile_out passback_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean get_user_fraud_protection_profile(
        const char* user_id,
        AdServer::RequestInfoSvcs::UserProfile_out user_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean get_user_site_reach_profile(
        const char* user_id,
        AdServer::RequestInfoSvcs::UserProfile_out user_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean get_user_tag_request_group_profile(
        const char* user_id,
        AdServer::RequestInfoSvcs::UserProfile_out user_profile)
        /*throw(
          AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      virtual void clear_expired(CORBA::Boolean synch) noexcept;

      void get_controllable_chunks(
        ChunkIdList& chunk_ids,
        unsigned long& common_chunks_number)
        /*throw(Exception)*/;

    protected:
      virtual ~RequestInfoManagerImpl() noexcept;

    private:
      typedef Generics::TaskGoal TaskBase;
      typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

      Generics::CompositeMetricsProvider_var composite_metrics_provider_;

      struct LoadDataState: public ReferenceCounting::AtomicImpl
      {
      public:
        LoadDataState() noexcept;

        bool
        fully_loaded_i() const noexcept;

      public:
        typedef Sync::Policy::PosixThread SyncPolicy;

      public:
        mutable SyncPolicy::Mutex lock;
        bool request_loaded;
        bool user_campaign_reach_loaded;
        bool user_action_info_loaded;
        bool user_fraud_protection_loaded;
        bool passback_loaded;
        bool user_site_reach_loaded;
        bool user_tag_request_merge_loaded;
        bool billing_processor_loaded;

      protected:
        virtual ~LoadDataState() noexcept {}
      };

      typedef ReferenceCounting::SmartPtr<LoadDataState>
        LoadDataState_var;

      typedef bool (RequestInfoManagerImpl::*LoadDataMethod)();

      class LoadTask: public TaskBase
      {
      public:
        LoadTask(
          LoadDataState* load_data_state,
          bool LoadDataState::* data_loaded,
          LoadDataMethod load_data,
          Generics::TaskRunner* task_runner,
          RequestInfoManagerImpl* request_info_manager_impl)
          noexcept;

        virtual void
        execute() noexcept;

      protected:
      protected:
        LoadDataState_var load_data_state_;
        bool LoadDataState::* data_loaded_;
        LoadDataMethod load_data_;
        RequestInfoManagerImpl* request_info_manager_impl_;
      };

      class ClearExpiredDataTask : public TaskBase
      {
      public:
        ClearExpiredDataTask(
          Generics::TaskRunner& task_runner,
          RequestInfoManagerImpl* request_info_manager_impl,
          bool reschedule)
          noexcept;

        virtual void execute() noexcept;

      protected:
        RequestInfoManagerImpl* request_info_manager_impl_;
        bool reschedule_;
      };

      class FlushLogsTask : public TaskBase
      {
      public:
        FlushLogsTask(
          Generics::TaskRunner& task_runner,
          RequestInfoManagerImpl* request_info_manager_impl)
          noexcept;

        virtual void execute() noexcept;

      protected:
        RequestInfoManagerImpl* request_info_manager_impl_;
      };

      class UpdateFraudRulesTask : public TaskBase
      {
      public:
        UpdateFraudRulesTask(
          Generics::TaskRunner* task_runner,
          RequestInfoManagerImpl* request_info_manager_impl)
          noexcept;

        virtual void execute() noexcept;

      protected:
        RequestInfoManagerImpl* request_info_manager_impl_;
      };

      typedef Sync::Policy::PosixThread SyncPolicy;

    private:
      typedef CORBACommons::ObjectPoolRefConfiguration
        CampaignServerPoolConfig;
      typedef CORBACommons::ObjectPool<
          AdServer::CampaignSvcs::CampaignServer,
          CampaignServerPoolConfig>
        CampaignServerPool;
      typedef std::unique_ptr<CampaignServerPool> CampaignServerPoolPtr;

      CampaignServerPoolPtr
      resolve_campaign_servers_() /*throw(Exception, eh::Exception)*/;

      template<typename ContainerPtrHolderType, typename KeyType>
      bool get_profile_(
        CORBACommons::OctSeq_out result_profile,
        const char* FUN,
        const ContainerPtrHolderType& container_ptr,
        const KeyType& id)
        /*throw(AdServer::RequestInfoSvcs::RequestInfoManager::NotReady,
          AdServer::RequestInfoSvcs::RequestInfoManager::ImplementationException)*/;

      // initialization after start methods
      void
      load_data_(
        LoadDataState* load_data_state,
        bool LoadDataState::* data_loaded,
        LoadDataMethod load_data)
        noexcept;

      bool load_request_chunk_files_() noexcept;

      bool load_user_campaign_reach_chunk_files_() noexcept;

      bool load_user_action_info_chunk_files_() noexcept;

      bool load_user_fraud_protection_chunk_files_() noexcept;

      bool init_passback_container_() noexcept;

      bool init_user_site_reach_container_() noexcept;

      bool init_user_tag_request_merge_container_() noexcept;

      bool init_billing_processor_() noexcept;

      bool update_fraud_rules_(bool loop_if_fail) noexcept;

      void start_logs_processing_() noexcept;

      // process methods
      void check_logs_() noexcept;

      void flush_logs_() noexcept;

      void clear_expired_data_(bool reschedule) noexcept;

      void
      init_(
        const xsd::AdServer::Configuration::InLogsType& lp_config,
        InLogs& in_logs)
        noexcept;

      void
      init_(
        const xsd::AdServer::Configuration::InLogType& config,
        const std::string& log_root,
        const char* default_in_dir,
        InLog& in_log)
        noexcept;

    private:
      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;
      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      const unsigned SERVICE_INDEX_;
      CampaignServerPoolPtr campaign_servers_;

      /// Callback with fixes Aspect and Code for log records of check_logs()
      Generics::ActiveObjectCallback_var check_logs_callback_;

      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var task_runner_;

      RequestInfoManagerConfig request_info_manager_config_;

      RequestInfoManagerStatsImpl_var rim_stats_impl_;

      RequestOutLogger_var request_out_logger_;
      ExpressionMatcherNotifier_var expression_matcher_notifier_;
      UserFraudDeactivator_var user_fraud_deactivator_;
      ProfilingCommons::ProfileMapFactory::Cache_var profile_cache_;

      // delay initialization processors
      ReferenceCounting::PtrHolder<UserCampaignReachContainer_var>
        user_campaign_reach_container_;
      ReferenceCounting::PtrHolder<RequestInfoContainer_var>
        request_info_container_;
      RequestOperationSaver_var request_operation_saver_;
      CompositeRequestActionProcessor_var processing_distributor_;
      ReferenceCounting::PtrHolder<UserActionInfoContainer_var>
        user_action_info_container_;
      ReferenceCounting::PtrHolder<UserFraudProtectionContainer_var>
        user_fraud_protection_container_;

      ReferenceCounting::PtrHolder<PassbackContainer_var>
        passback_container_;
      ReferenceCounting::PtrHolder<UserSiteReachContainer_var>
        user_site_reach_container_;
      ReferenceCounting::PtrHolder<UserTagRequestMergeContainer_var>
        user_tag_request_merge_container_;

      RequestOperationDistributor_var request_operation_distributor_;
      RequestLogLoader_var request_log_loader_;

    };

    typedef
      ReferenceCounting::SmartPtr<RequestInfoManagerImpl>
      RequestInfoManagerImpl_var;

  } /* RequestInfoSvcs */
} /* AdServer */

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    inline
    RequestInfoManagerImpl::LoadDataState::LoadDataState()
      noexcept
      : request_loaded(false),
        user_campaign_reach_loaded(false),
        user_action_info_loaded(false),
        user_fraud_protection_loaded(false),
        passback_loaded(false),
        user_site_reach_loaded(false),
        user_tag_request_merge_loaded(false),
        billing_processor_loaded(false)
    {}

    inline
    bool
    RequestInfoManagerImpl::LoadDataState::fully_loaded_i() const
      noexcept
    {
      return user_campaign_reach_loaded &&
        request_loaded &&
        user_action_info_loaded &&
        user_fraud_protection_loaded &&
        passback_loaded &&
        user_site_reach_loaded &&
        user_tag_request_merge_loaded &&
        billing_processor_loaded;
    }

    inline
    RequestInfoManagerImpl::LoadTask::LoadTask(
      LoadDataState* load_data_state,
      bool LoadDataState::* data_loaded,
      LoadDataMethod load_data,
      Generics::TaskRunner* task_runner,
      RequestInfoManagerImpl* request_info_manager_impl)
      noexcept
      : TaskBase(task_runner),
        load_data_state_(ReferenceCounting::add_ref(load_data_state)),
        data_loaded_(data_loaded),
        load_data_(load_data),
        request_info_manager_impl_(request_info_manager_impl)
    {}

    inline
    void
    RequestInfoManagerImpl::LoadTask::execute()
      noexcept
    {
      request_info_manager_impl_->load_data_(
        load_data_state_,
        data_loaded_,
        load_data_);
    }

    inline
    RequestInfoManagerImpl::ClearExpiredDataTask::ClearExpiredDataTask(
      Generics::TaskRunner& task_runner,
      RequestInfoManagerImpl* request_info_manager_impl,
      bool reschedule)
      noexcept
      : TaskBase(&task_runner),
        request_info_manager_impl_(request_info_manager_impl),
        reschedule_(reschedule)
    {}

    inline
    void
    RequestInfoManagerImpl::ClearExpiredDataTask::execute()
      noexcept
    {
      request_info_manager_impl_->clear_expired_data_(reschedule_);
    }

    inline
    RequestInfoManagerImpl::FlushLogsTask::FlushLogsTask(
      Generics::TaskRunner& task_runner,
      RequestInfoManagerImpl* request_info_manager_impl)
      noexcept
      : TaskBase(&task_runner),
        request_info_manager_impl_(request_info_manager_impl)
    {}

    inline
    void
    RequestInfoManagerImpl::FlushLogsTask::execute()
      noexcept
    {
      request_info_manager_impl_->flush_logs_();
    }

    inline
    RequestInfoManagerImpl::
    UpdateFraudRulesTask::UpdateFraudRulesTask(
      Generics::TaskRunner* task_runner,
      RequestInfoManagerImpl* request_info_manager_impl)
      noexcept
      : TaskBase(task_runner),
        request_info_manager_impl_(request_info_manager_impl)
    {}

    inline
    void
    RequestInfoManagerImpl::
    UpdateFraudRulesTask::execute()
      noexcept
    {
      request_info_manager_impl_->update_fraud_rules_(true);
    }
  }
}

#endif /*REQUESTINFOSVCS_REQUESTINFOMANAGERIMPL_HPP*/
