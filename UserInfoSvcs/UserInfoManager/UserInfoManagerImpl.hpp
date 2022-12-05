#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_IMPL_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_IMPL_HPP_

#include <list>
#include <vector>
#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/AccessActiveObject.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <UserInfoSvcs/UserInfoExchanger/UserInfoExchanger.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserInfoManagerConfig.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager_s.hpp>
#include <ProfilingCommons/PlainStorage3/LoadingProgressCallbackBase.hpp>

#include "FileRWStats.hpp"
#include "UserInfoContainer.hpp"
#include "UserOperationLoader.hpp"
#include "UserOperationSaver.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /**
     * Implementation of UserInfoManager.
     */
    class UserInfoManagerImpl:
      public virtual CORBACommons::ReferenceCounting::ServantImpl<
        POA_AdServer::UserInfoSvcs::UserInfoManager>,
      public virtual Generics::CompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef xsd::AdServer::Configuration::UserInfoManagerConfigType
        UserInfoManagerConfig;

      typedef std::list<unsigned long> ChunkIdList;

      typedef std::vector<ColoUserIds> ColoUserIdVector; // TO RECHECK !!!

    public:
      UserInfoManagerImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        const UserInfoManagerConfig& user_info_manager_config)
        /*throw(Exception)*/;

      // ActiveObject interface
      virtual void
      wait_object()
        /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

      // UserInfoMatcher interface
      virtual void get_master_stamp(
        CORBACommons::TimestampInfo_out master_stamp)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean
      uim_ready() noexcept;

      char* get_progress() noexcept;

      virtual CORBA::Boolean merge(
        const AdServer::UserInfoSvcs::UserInfo& user_info,
        const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
        const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
        CORBA::Boolean_out merge_success,
        CORBACommons::TimestampInfo_out last_request)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual CORBA::Boolean fraud_user(
        const CORBACommons::UserIdInfo& user_id,
        const CORBACommons::TimestampInfo& time)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;
      
      virtual CORBA::Boolean match(
        const AdServer::UserInfoSvcs::UserInfo& user_info,
        const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
        AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual CORBA::Boolean get_user_profile(
        const CORBACommons::UserIdInfo& user_id,
        CORBA::Boolean temporary,
        const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
        AdServer::UserInfoSvcs::UserProfiles_out user_profile)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual CORBA::Boolean remove_user_profile(
        const CORBACommons::UserIdInfo& user_info)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual void
      update_user_freq_caps(
        const CORBACommons::UserIdInfo& user_id,
        const CORBACommons::TimestampInfo& time,
        const CORBACommons::RequestIdInfo& request_id,
        const FreqCapIdSeq& freq_caps,
        const FreqCapIdSeq& uc_freq_caps,
        const FreqCapIdSeq& virtual_freq_caps,
        const AdServer::UserInfoSvcs::UserInfoManager::SeqOrderSeq& seq_orders,
        const CampaignIdSeq& campaign_ids_seq,
        const CampaignIdSeq& uc_campaign_ids_seq)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual void confirm_user_freq_caps(
        const CORBACommons::UserIdInfo& user_id,
        const CORBACommons::TimestampInfo& time,
        const CORBACommons::RequestIdInfo& request_id,
        const CORBACommons::IdSeq& exclude_pubpixel_accounts)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      
      virtual void consider_publishers_optin(
        const CORBACommons::UserIdInfo& user_id_info,
        const CORBACommons::IdSeq& exclude_pubpixel_accounts,
        const CORBACommons::TimestampInfo& now)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
              AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      void clear_expired(
        CORBA::Boolean synch,
        const CORBACommons::TimestampInfo& cleanup_time,
        CORBA::Long portion)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

      UserStat get_stats()
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          eh::Exception)*/;

      void
      get_controllable_chunks(
        ChunkIdList& chunk_ids,
        unsigned long& common_chunks_number)
        /*throw(Exception)*/;

      Logging::Logger*
      logger() noexcept;

      FileRWStats::IntervalStats
      get_rw_stats() /*throw(eh::Exception)*/;

    protected:
      typedef Sync::Policy::PosixThreadRW SyncPolicy;
      typedef Generics::TaskGoal TaskBase;
      typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

      class LoadingProgressProcessor
        : public AdServer::ProfilingCommons::LoadingProgressCallbackBase
      {
      public:
        LoadingProgressProcessor(double range) noexcept;
        virtual void post_progress(double value) noexcept;
        std::string get_progress_in_percents() noexcept;

      private:
        double range_;
        double progress_;
        SyncPolicy::Mutex progress_lock_;
      };
      typedef ReferenceCounting::SmartPtr<LoadingProgressProcessor>
        LoadingProgressProcessor_var;

      class FlushLogsTask: public TaskBase
      {
      public:
        FlushLogsTask(
          UserInfoManagerImpl* manager,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual ~FlushLogsTask() noexcept {};

      private:
        UserInfoManagerImpl* manager_;
      };
      
      class GetLastColoProfilesTask : public TaskBase
      {
      public:
        GetLastColoProfilesTask(
          UserInfoManagerImpl* user_info_manager_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
      };

      class UpdateChannelsConfigTask : public TaskBase
      {
      public:
        UpdateChannelsConfigTask(
          UserInfoManagerImpl* user_info_manager_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
      };

      class RotateUserOperationsBackupTask: public TaskBase
      {
      public:
        RotateUserOperationsBackupTask(
          UserInfoManagerImpl* user_info_manager_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
      };

      class DeleteOldProfilesTask : public TaskBase
      {
      public:
        DeleteOldProfilesTask(
          Generics::TaskRunner* task_runner,
          UserInfoManagerImpl* user_info_manager_impl,
          bool reschedule)
          noexcept;

        virtual void execute() noexcept;
        
      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
        bool reschedule_;
      };
      
      class DeleteOldTemporaryProfilesTask : public TaskBase
      {
      public:
        DeleteOldTemporaryProfilesTask(
          Generics::TaskRunner* task_runner,
          UserInfoManagerImpl* user_info_manager_impl,
          bool reschedule)
          noexcept;

        virtual void execute() noexcept;
        
      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
        bool reschedule_;
      };

      class AllUsersProcessingTask: public TaskBase
      {
      public:
        AllUsersProcessingTask(
          Generics::TaskRunner* task_runner,
          UserInfoManagerImpl* user_info_manager_impl,
          bool reschedule,
          UserInfoContainer::AllUsersProcessingState* processing_state,
          const Generics::Time cleanup_time =  Generics::Time(-1),
          long portion = -1)
          noexcept;

        virtual void execute() noexcept;
        
      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
        bool reschedule_;
        UserInfoContainer::AllUsersProcessingState_var processing_state_;
        Generics::Time cleanup_time_;
        long portion_;
      };

      class LoadChunksDataTask : public TaskBase
      {
      public:
        LoadChunksDataTask(UserInfoManagerImpl* user_info_manager_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        UserInfoManagerImpl* user_info_manager_impl_;
      };

      typedef CORBACommons::ObjectPoolRefConfiguration
        CampaignServerPoolConfig;
      typedef CORBACommons::ObjectPool<
        AdServer::CampaignSvcs::CampaignServer,
        CampaignServerPoolConfig>
        CampaignServerPool;
      typedef std::unique_ptr<CampaignServerPool> CampaignServerPoolPtr;

      typedef AdServer::Commons::AccessActiveObject<
        UserInfoContainer_var>
        UserInfoContainerHolder;
      typedef ReferenceCounting::SmartPtr<UserInfoContainerHolder>
        UserInfoContainerHolder_var;
      typedef UserInfoContainerHolder::Accessor
        UserInfoContainerAccessor;

      typedef AdServer::Commons::AccessActiveObject<
        UserOperationProcessor_var>
        UserOperationProcessorHolder;
      typedef ReferenceCounting::SmartPtr<UserOperationProcessorHolder>
        UserOperationProcessorHolder_var;
      typedef UserOperationProcessorHolder::Accessor
        UserOperationProcessorAccessor;

    protected:
      virtual ~UserInfoManagerImpl() noexcept {};

      UserOperationProcessorAccessor
      get_user_operation_processor_(bool throw_not_ready = true)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady)*/;

      UserInfoContainerAccessor
      get_user_info_container_(bool throw_not_ready = true)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady)*/;

      void load_chunk_files_() noexcept;

      void update_config_() noexcept;

      void rotate_user_operations_backup_() noexcept;

      void load_user_operations_() noexcept;

      void delete_old_profiles_(bool reschedule) noexcept;
      
      void delete_old_temporary_profiles_(bool reschedule) noexcept;

      void get_last_colo_profiles_() noexcept;

      void
      update_channels_config_(
        UserInfoContainer* user_info_container,
        UserOperationProcessor* user_operation_processor)
        noexcept;

      void apply_freq_caps_(
        FreqCapConfig& freq_cap_config,
        const AdServer::CampaignSvcs::FreqCapConfigInfo& freq_cap_seq)
        noexcept;

      static void push_channel_interval_(
        ChannelIntervalsPack* cip,
        const ChannelInterval& ci)
        noexcept;
      
      CampaignServerPoolPtr
      resolve_campaign_servers_(
        const CORBACommons::CorbaObjectRefList& campaign_server_refs)
        /*throw(Exception, eh::Exception)*/;

      void calc_user_daily_stat_(
        const Generics::Time& current_time,
        const Generics::Time& request_tiem,
        const Generics::Time& user_time,
        const Generics::Time& time_offset)
        noexcept;

      void flush_logs_() noexcept;

      void
      all_users_process_step_(
        bool reschedule,
        UserInfoContainer::AllUsersProcessingState* state,
        bool fast_mode,
        const Generics::Time& cleanup_time,
        long portion)
        noexcept;

      AdServer::ProfilingCommons::LevelMapTraits
      fill_level_map_traits_(
        const xsd::AdServer::Configuration::ChunksConfigType& chunks_config)
        noexcept;

    private:
      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;

      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var task_runner_;

      UserInfoManagerConfig user_info_manager_config_;
      AdServer::ProfilingCommons::FileController_var file_controller_;
      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders_;
      unsigned long placement_colo_id_;

      InternalUserOperationLoader_var user_operation_loader_;
      UserOperationSaver_var user_operation_saver_;
      ExternalUserOperationLoader_var external_user_operation_loader_;

      UserOperationProcessorHolder_var user_operation_processor_;
      UserInfoContainerHolder_var user_info_container_;
      // objects that can be deactivated only after
      // user_info_container_ deactivation
      Generics::CompositeActiveObject_var user_info_container_dependent_active_object_;

      mutable SyncPolicy::Mutex lock_;
      bool campaignserver_ready_; // REVIEW

      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      CampaignServerPoolPtr campaign_servers_;

      // user profiles exchange parameters
      bool uie_presents_;
      std::string exchange_customer_id_;
      std::string exchange_provider_id_; // equal to placement colo id
      UserInfoExchanger_var user_info_exchanger_;
      AdServer::UserInfoSvcs::ReceiveCriteria opt_uie_receive_criteria_;

      bool clean_user_profiles_;
      Generics::Time profile_lifetime_;
      Generics::Time temp_profile_lifetime_;

      Generics::Time repeat_trigger_timeout_;
      
      bool provide_channel_counters_;

      mutable SyncPolicy::Mutex colo_lock_;
      ColoUserIdVector colo_profiles_vector_;

      mutable SyncPolicy::Mutex daily_stat_lock_;
      unsigned long current_day_;
      unsigned long daily_users_;

      UserInfoManagerLogger_var user_info_manager_logger_;

      Generics::ActiveObjectCallback_var check_operations_callback_;

      LoadingProgressProcessor_var loading_progress_processor_;
      ReferenceCounting::SmartPtr<FileRWStats> file_rw_stats_;
    };

    typedef ReferenceCounting::SmartPtr<UserInfoManagerImpl>
      UserInfoManagerImpl_var;

  } /* UserInfoSvcs */
} /* AdServer */

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /* Inlines */
    inline
    UserInfoManagerImpl::FlushLogsTask::FlushLogsTask(
      UserInfoManagerImpl* manager,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        manager_(manager)
    {}

    inline
    void
    UserInfoManagerImpl::FlushLogsTask::execute()
      noexcept
    {
      manager_->flush_logs_();
    }
    
    inline
    UserInfoManagerImpl::GetLastColoProfilesTask::GetLastColoProfilesTask(
      UserInfoManagerImpl* user_info_manager_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl)
    {}

    inline
    void
    UserInfoManagerImpl::GetLastColoProfilesTask::execute()
      noexcept
    {
      user_info_manager_impl_->get_last_colo_profiles_();
    }

    inline
    UserInfoManagerImpl::UpdateChannelsConfigTask::UpdateChannelsConfigTask(
      UserInfoManagerImpl* user_info_manager_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl)
    {}

    inline
    void
    UserInfoManagerImpl::UpdateChannelsConfigTask::execute()
      noexcept
    {
      user_info_manager_impl_->update_config_();
    }

    inline
    UserInfoManagerImpl::
    RotateUserOperationsBackupTask::RotateUserOperationsBackupTask(
      UserInfoManagerImpl* user_info_manager_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl)
    {}

    inline
    void
    UserInfoManagerImpl::RotateUserOperationsBackupTask::execute()
      noexcept
    {
      user_info_manager_impl_->rotate_user_operations_backup_();
    }

    // UserInfoManagerImpl::DeleteOldProfilesTask
    inline
    UserInfoManagerImpl::DeleteOldProfilesTask::DeleteOldProfilesTask(
      Generics::TaskRunner* task_runner,
      UserInfoManagerImpl* user_info_manager_impl,
      bool reschedule)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl),
        reschedule_(reschedule)
    {}

    inline
    void
    UserInfoManagerImpl::DeleteOldProfilesTask::execute()
      noexcept
    {
      user_info_manager_impl_->delete_old_profiles_(reschedule_);
    }

    inline
    UserInfoManagerImpl::
    DeleteOldTemporaryProfilesTask::DeleteOldTemporaryProfilesTask(
      Generics::TaskRunner* task_runner,
      UserInfoManagerImpl* user_info_manager_impl,
      bool reschedule)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl),
        reschedule_(reschedule)
    {}

    inline
    void
    UserInfoManagerImpl::DeleteOldTemporaryProfilesTask::execute()
      noexcept
    {
      user_info_manager_impl_->delete_old_temporary_profiles_(reschedule_);
    }

    // UserInfoManagerImpl::AllUsersProcessingTask
    inline
    UserInfoManagerImpl::AllUsersProcessingTask::AllUsersProcessingTask(
      Generics::TaskRunner* task_runner,
      UserInfoManagerImpl* user_info_manager_impl,
      bool reschedule,
      UserInfoContainer::AllUsersProcessingState* processing_state,
      const Generics::Time cleanup_time,
      long portion)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl),
        reschedule_(reschedule),
        processing_state_(ReferenceCounting::add_ref(processing_state)),
        cleanup_time_(cleanup_time),
        portion_(portion)
    {}

    inline
    void
    UserInfoManagerImpl::AllUsersProcessingTask::execute() noexcept
    {
      user_info_manager_impl_->all_users_process_step_(
        reschedule_,
        processing_state_,
        false,
        cleanup_time_,
        portion_);
    }

    // UserInfoManagerImpl::LoadChunksDataTask
    inline
    UserInfoManagerImpl::LoadChunksDataTask::LoadChunksDataTask(
      UserInfoManagerImpl* user_info_manager_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_impl_(user_info_manager_impl)
    {}

    inline
    void
    UserInfoManagerImpl::LoadChunksDataTask::execute()
      noexcept
    {
      user_info_manager_impl_->load_chunk_files_();
    }

    inline
    Logging::Logger*
    UserInfoManagerImpl::logger() noexcept
    {
      return logger_;
    }

    inline
    FileRWStats::IntervalStats
    UserInfoManagerImpl::get_rw_stats() /*throw(eh::Exception)*/
    {
      FileRWStats::IntervalStats res;

      if (file_rw_stats_)
      {
        res = file_rw_stats_->get_stats();
      }

      return res;
    }
  }
}

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_IMPL_HPP_*/
