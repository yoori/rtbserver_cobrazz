#ifndef ADSERVER_REQUESTINFOSVCS_EXPRESSIONMATCHERIMPL_HPP
#define ADSERVER_REQUESTINFOSVCS_EXPRESSIONMATCHERIMPL_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <Commons/CorbaConfig.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/LogProcessing/ExpressionMatcherConfig.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServerPool.hpp>

#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <LogCommons/RequestBasicChannels.hpp>

#include "ExpressionMatcher_s.hpp"

#include "ChannelMatcher.hpp"
#include "UserInventoryContainer.hpp"
#include "UserTriggerMatchContainer.hpp"
#include "UserColoReachContainer.hpp"
#include "ExpressionMatcherOutLogger.hpp"
#include "ExpressionMatcherStats.hpp"
#include "ExpressionMatcherLogLoader.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    /** ExpressionMatcherImpl
     *   tasks running order:
     *     LoadDataTask ->
     *       ResolveUserInfoManagerSessionTask (in loop while non successfull)
     *       FlushLogsTask (in loop)
     *       UpdateExpressionChannelsTask ->
     *         ProcessRequestBasicChannelsTask (in loop)
     */
    class ExpressionMatcherImpl :
      public virtual CORBACommons::ReferenceCounting::
        ServantImpl<POA_AdServer::RequestInfoSvcs::ExpressionMatcher>,
      public virtual Generics::CompositeActiveObject,
      private RequestBasicChannelsProcessor,
      private ConsiderInterface
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(InvalidArgument, Exception);

      typedef xsd::AdServer::Configuration::ExpressionMatcherConfigType
        ExpressionMatcherConfig;

    public:
      ExpressionMatcherImpl(
        Logging::Logger* logger,
        const ExpressionMatcherConfig& expression_matcher_config,
        ProcStatImpl* proc_stat_impl)
        /*throw(Exception)*/;

      virtual CORBA::Boolean
      get_inventory_profile(
        const char* user_id,
        AdServer::RequestInfoSvcs::UserInventoryProfile_out inv_profile)
        /*throw(
          AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
          AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/;

      virtual CORBA::Boolean
      get_user_trigger_match_profile(
        const char* user_id,
        bool temporary_user,
        AdServer::RequestInfoSvcs::UserTriggerMatchProfile_out profile)
        /*throw(
          AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
          AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/;

      virtual CORBA::Boolean
      get_request_trigger_match_profile(
        const char* request_id,
        AdServer::RequestInfoSvcs::RequestTriggerMatchProfile_out profile)
        /*throw(
          AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
          AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/;

      virtual CORBA::Boolean
      get_household_colo_reach_profile(
        const char* request_id,
        AdServer::RequestInfoSvcs::HouseholdColoReachProfile_out profile)
        /*throw(
          AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
          AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/;

      virtual void
      run_daily_processing(bool sync)
        /*throw(
          AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
          AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/;

      virtual void
      consider_click(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time)
        noexcept;

      virtual void
      consider_impression(
        const AdServer::Commons::UserId& user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time,
        const ChannelIdSet& channels)
        noexcept;

    protected:
      virtual
      ~ExpressionMatcherImpl() noexcept;

    protected:
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

      struct UsersChunk
      {
        UserIdList users;
        UserIdList unprocessed_users;
      };

      typedef std::vector<UsersChunk> UsersChunkArray;

      class DailyCheckState: public ReferenceCounting::AtomicImpl
      {
      public:
        Generics::Time start_time;
        UserIdList users;
        UserIdList unprocessed_users;

      private:
        virtual
        ~DailyCheckState() noexcept = default;
      };

      typedef ReferenceCounting::SmartPtr<DailyCheckState> DailyCheckState_var;

      static
      size_t
      split_users(DailyCheckState& state, UsersChunkArray& chunk_array, size_t blocks);

      static
      void
      merge_users(ExpressionMatcherImpl::UsersChunkArray& chunk_array, ExpressionMatcherImpl::DailyCheckState& state);

      /* Tasks */
      class TaskBase: public Generics::TaskGoal
      {
      public:
        TaskBase(ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

      protected:
        virtual
        ~TaskBase() noexcept = default;

        ExpressionMatcherImpl* expression_matcher_impl_;
      };

      class ClearExpiredUsersTask: public TaskBase
      {
      public:
        ClearExpiredUsersTask(
          ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner,
          bool set_next_task = true)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~ClearExpiredUsersTask() noexcept = default;

        bool set_next_task_;
      };

      class DailyCheckTask: public TaskBase
      {
      public:
        DailyCheckTask(
          ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner,
          bool set_next_task = true,
          DailyCheckState* state = 0)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~DailyCheckTask() noexcept = default;

        bool set_next_task_;
        DailyCheckState_var state_;
      };

      /* Task that periodically flushes stats */
      class FlushLogsTask: public TaskBase
      {
      public:
        FlushLogsTask(
          ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~FlushLogsTask() noexcept = default;
      };

      class ResolveUserInfoManagerSessionTask : public TaskBase
      {
      public:
        ResolveUserInfoManagerSessionTask(
          ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~ResolveUserInfoManagerSessionTask() noexcept = default;
      };

      class UpdateExpressionChannelsTask : public TaskBase
      {
      public:
        UpdateExpressionChannelsTask(
          ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~UpdateExpressionChannelsTask() noexcept = default;
      };

      class LoadDataTask : public TaskBase
      {
      public:
        LoadDataTask(
          ExpressionMatcherImpl* expression_matcher_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~LoadDataTask() noexcept = default;
      };

      class PlacementColo : public ReferenceCounting::AtomicImpl
      {
      public:
        Generics::Time time_offset;

        PlacementColo(const Generics::Time time) noexcept
          : time_offset(time)
        {}

      protected:
        virtual
        ~PlacementColo() noexcept = default;
      };

      typedef ReferenceCounting::SmartPtr<PlacementColo> PlacementColo_var;

    private:
      template<
        typename ContainerPtrHolderType,
        typename KeyType,
        typename GetProfileAdapterType>
      bool
      get_profile_(
        CORBACommons::OctSeq_out result_profile,
        const char* FUN,
        const ContainerPtrHolderType& container_ptr_holder,
        const KeyType& id,
        const GetProfileAdapterType& get_profile_adapter)
        /*throw(AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
          AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/;

      void
      load_data_() noexcept;

      bool
      resolve_user_info_manager_session_() noexcept;

      void
      update_expression_channels_() noexcept;

      void
      update_campaign_config_() noexcept;

      void
      daily_check_users_(
        bool set_next_task,
        DailyCheckState* state) noexcept;

      void
      try_start_daily_processing_loop_() noexcept;

      void
      clear_expired_users_(bool set_next_task) noexcept;

      DailyCheckState_var
      daily_check_users_impl_(DailyCheckState* state) noexcept;

      void
      daily_check_users_thread_(
        int thread_number,
        const Generics::Time& now,
        UserInventoryInfoContainer* user_inventory_container,
        UsersChunk& users,
        size_t& processed_count) noexcept;

      void
      daily_check_user_impl_(
        const UserId& user_id,
        const Generics::Time& now,
        const Generics::Time& placement_colo_now_date,
        UserInventoryInfoContainer* user_inventory_container);

      void
      flush_logs_() noexcept;

      void
      update_stats_() noexcept;

      virtual bool
      process_requests(
        LogProcessing::FileReceiver::FileGuard* file_ptr,
        std::size_t& processed_lines_count)
        /*throw(eh::Exception)*/;

      bool
      check_sampling_(const UserId& user_id) const noexcept;

      void
      process_request_basic_channels_record_(
        UserInventoryInfoContainer* user_inventory_container,
        UserTriggerMatchContainer* user_trigger_match_container,
        UserTriggerMatchContainer* temp_user_trigger_match_container,
        UserColoReachContainer* household_colo_reach_container,
        const AdServer::LogProcessing::
          RequestBasicChannelsCollector::KeyT& key,
        const AdServer::LogProcessing::
          RequestBasicChannelsCollector::DataT::DataT& record)
        /*throw(Exception)*/;

      AdServer::ProfilingCommons::LevelMapTraits
      fill_level_map_traits_(
        const xsd::AdServer::Configuration::LevelChunksConfigType& chunks_config)
        noexcept;

      Logging::Logger*
      logger() noexcept;

    private:
      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;

      AdServer::CampaignSvcs::CampaignServerPoolPtr campaign_pool_;

      AdServer::UserInfoSvcs::UserInfoManagerController_var
        user_info_manager_controller_;
      AdServer::UserInfoSvcs::UserInfoManagerSession_var
        user_info_manager_session_;

      ExpressionMatcherConfig expression_matcher_config_;
      Generics::Time check_loggers_period_;

      mutable SyncPolicy::Mutex lock_;
      mutable SyncPolicy::Mutex daily_processing_lock_;
      bool daily_processing_loop_started_;

      ProfilingCommons::FileController_var file_controller_;
      ChannelMatcher_var channel_matcher_;
      ReferenceCounting::PtrHolder<UserInventoryInfoContainer_var>
        user_inventory_container_;
      ReferenceCounting::PtrHolder<UserTriggerMatchContainer_var>
        temp_user_trigger_match_container_;
      UserTriggerMatchProfileProvider_var user_trigger_match_profile_provider_;
      ReferenceCounting::PtrHolder<UserTriggerMatchContainer_var>
        user_trigger_match_container_;
      ReferenceCounting::PtrHolder<UserColoReachContainer_var>
        household_colo_reach_container_;
      ExpressionMatcherOutLogger_var expression_matcher_out_logger_;

      Logging::ActiveObjectCallbackImpl_var callback_;
      Generics::TaskRunner_var task_runner_;
      Generics::TaskRunner_var daily_processing_task_runner_;
      Generics::Planner_var scheduler_;
      ProfilingCommons::ProfileMapFactory::Cache_var profile_cache_;

      ProcStatImpl_var proc_stat_impl_;
      StatsCounters stats_;

      ExpressionMatcherLogLoader_var log_loader_;
      ReferenceCounting::PtrHolder<PlacementColo_var> placement_colo_;
    };

    typedef ReferenceCounting::SmartPtr<ExpressionMatcherImpl>
      ExpressionMatcherImpl_var;
  }
}

//
// Inlines
//
namespace AdServer
{
  namespace RequestInfoSvcs
  {
    inline
    Logging::Logger*
    ExpressionMatcherImpl::logger()
      noexcept
    {
      return callback_->logger();
    }

    inline
    ExpressionMatcherImpl::
    TaskBase::TaskBase(ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : Generics::TaskGoal(task_runner),
        expression_matcher_impl_(expression_matcher_impl)
    {}

    inline
    ExpressionMatcherImpl::
    ClearExpiredUsersTask::ClearExpiredUsersTask(
      ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner,
      bool set_next_task)
      noexcept
      : TaskBase(expression_matcher_impl, task_runner),
        set_next_task_(set_next_task)
    {}

    inline
    void
    ExpressionMatcherImpl::
    ClearExpiredUsersTask::execute() noexcept
    {
      expression_matcher_impl_->clear_expired_users_(set_next_task_);
    }

    inline
    ExpressionMatcherImpl::
    DailyCheckTask::DailyCheckTask(
      ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner,
      bool set_next_task,
      DailyCheckState* state)
      noexcept
      : TaskBase(expression_matcher_impl, task_runner),
        set_next_task_(set_next_task),
        state_(ReferenceCounting::add_ref(state))
    {}

    inline
    void
    ExpressionMatcherImpl::
    DailyCheckTask::execute() noexcept
    {
      expression_matcher_impl_->daily_check_users_(set_next_task_, state_);
    }

    inline
    ExpressionMatcherImpl::
    FlushLogsTask::FlushLogsTask(
      ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(expression_matcher_impl, task_runner)
    {}

    inline
    void
    ExpressionMatcherImpl::
    FlushLogsTask::execute() noexcept
    {
      expression_matcher_impl_->flush_logs_();
    }

    inline
    ExpressionMatcherImpl::
    ResolveUserInfoManagerSessionTask::ResolveUserInfoManagerSessionTask(
      ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(expression_matcher_impl, task_runner)
    {}

    inline
    void
    ExpressionMatcherImpl::
    ResolveUserInfoManagerSessionTask::execute() noexcept
    {
      expression_matcher_impl_->resolve_user_info_manager_session_();
    }

    inline
    ExpressionMatcherImpl::
    UpdateExpressionChannelsTask::UpdateExpressionChannelsTask(
      ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(expression_matcher_impl, task_runner)
    {}

    inline
    void
    ExpressionMatcherImpl::
    UpdateExpressionChannelsTask::execute() noexcept
    {
      expression_matcher_impl_->update_expression_channels_();
    }

    inline
    ExpressionMatcherImpl::
    LoadDataTask::LoadDataTask(
      ExpressionMatcherImpl* expression_matcher_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(expression_matcher_impl, task_runner)
    {}

    inline
    void
    ExpressionMatcherImpl::
    LoadDataTask::execute() noexcept
    {
      expression_matcher_impl_->load_data_();
    }
  }
}

#endif /*ADSERVER_REQUESTINFOSVCS_EXPRESSIONMATCHERIMPL_HPP*/
