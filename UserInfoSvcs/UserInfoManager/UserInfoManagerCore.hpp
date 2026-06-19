#pragma once

#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <vector>
#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <Commons/AccessActiveObject.hpp>
#include <Commons/Coro.hpp>
#include <Commons/UserInfoManip.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserInfoManagerConfig.hpp>

#include <ProfilingCommons/PlainStorage3/LoadingProgressCallbackBase.hpp>

#include "FileRWStats.hpp"
#include "UserInfoConfigSource.hpp"
#include "UserInfoContainer.hpp"
#include "UserOperationLoader.hpp"
#include "UserOperationSaver.hpp"

namespace AdServer::UserInfoSvcs
{
  /**
   * Implementation of UserInfoManager.
   */
  class UserInfoManagerCore:
    public virtual Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);

    typedef xsd::AdServer::Configuration::UserInfoManagerConfigType
      UserInfoManagerConfig;

    typedef std::list<unsigned long> ChunkIdList;
    typedef std::vector<unsigned char> ByteArray;

    struct UserInfo
    {
      AdServer::Commons::UserId user_id;
      AdServer::Commons::UserId huser_id;
      Generics::Time time;
      long request_colo_id = 0;
      long current_colo_id = 0;
      bool temporary = false;
    };

    struct GeoData
    {
      CampaignSvcs::CoordDecimal latitude;
      CampaignSvcs::CoordDecimal longitude;
      CampaignSvcs::AccuracyDecimal accuracy;
    };

    struct MatchParams
    {
      explicit MatchParams(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : matched_channels(resource),
          geo_data_seq(resource)
      {}

      ChannelIdPack matched_channels;
      std::string cohort;
      std::string cohort2;
      std::pmr::vector<GeoData> geo_data_seq;
      Generics::Time publishers_optin_timeout;
      bool use_empty_profile = false;
      bool filter_contextual_triggers = false;
      bool silent_match = false;
      bool no_match = false;
      bool no_result = false;
      bool provide_channel_count = false;
      bool provide_persistent_channels = false;
      bool change_last_request = false;
      bool ret_freq_caps = false;
    };

    struct ProfilesRequest
    {
      bool base_profile = false;
      bool add_profile = false;
      bool history_profile = false;
      bool freq_cap_profile = false;
    };

    struct UserProfiles
    {
      ByteArray base_user_profile;
      ByteArray add_user_profile;
      ByteArray history_user_profile;
      ByteArray freq_cap;
      ByteArray pref_profile;
    };

    struct ChannelWeight
    {
      unsigned long channel_id = 0;
      unsigned long weight = 0;
    };

    struct SeqOrder
    {
      unsigned long ccg_id = 0;
      unsigned long set_id = 0;
      unsigned long imps = 0;
    };

    struct CampaignFreq
    {
      unsigned long campaign_id = 0;
      unsigned long imps = 0;
    };

    struct MatchResult
    {
      explicit MatchResult(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : geo_data_seq(resource),
          exclude_pubpixel_accounts(resource),
          full_freq_caps(resource),
          full_virtual_freq_caps(resource),
          seq_orders(resource),
          campaign_freqs(resource),
          channels(resource),
          hid_channels(resource)
      {}

      unsigned long adv_channel_count = 0;
      unsigned long discover_channel_count = 0;
      long colo_id = -1;
      bool times_inited = false;
      Generics::Time last_request_time;
      Generics::Time create_time;
      Generics::Time session_start;
      bool fraud_request = false;
      std::string cohort;
      std::string cohort2;
      std::pmr::vector<GeoData> geo_data_seq;
      std::pmr::vector<unsigned long> exclude_pubpixel_accounts;
      std::pmr::vector<unsigned long> full_freq_caps;
      std::pmr::vector<unsigned long> full_virtual_freq_caps;
      std::pmr::vector<SeqOrder> seq_orders;
      std::pmr::vector<CampaignFreq> campaign_freqs;
      std::pmr::vector<ChannelWeight> channels;
      std::pmr::vector<ChannelWeight> hid_channels;
      Generics::Time process_time;
    };

  public:
    UserInfoManagerCore(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const UserInfoManagerConfig& user_info_manager_config)
      /*throw(Exception)*/;

    UserInfoManagerCore(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const UserInfoManagerConfig& user_info_manager_config,
      UserInfoConfigSourcePtr config_source)
      /*throw(Exception)*/;

    // ActiveObject interface
    virtual void
    wait_object()
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

    Generics::Time get_master_stamp();

    bool uim_ready() noexcept;

    std::string get_progress() noexcept;

    AdServer::Commons::Task<bool> co_merge(
      const UserInfo& user_info,
      const MatchParams& match_params,
      const UserProfiles& merge_user_profile,
      bool& merge_success,
      Generics::Time& last_request);

    AdServer::Commons::Task<bool> co_fraud_user(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time);

    AdServer::Commons::Task<bool> co_match(
      const UserInfo& user_info,
      const MatchParams& match_params,
      MatchResult& match_result);

    AdServer::Commons::Task<bool> co_get_user_profile(
      const AdServer::Commons::UserId& user_id,
      bool temporary,
      const ProfilesRequest& profile_request,
      UserProfiles& user_profile);

    bool remove_user_profile(const AdServer::Commons::UserId& user_id);

    AdServer::Commons::Task<bool> co_remove_user_profile(
      const AdServer::Commons::UserId& user_id);

    AdServer::Commons::Task<bool> co_update_user_freq_caps(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      const Generics::Uuid& request_id,
      const std::vector<unsigned long>& freq_caps,
      const std::vector<unsigned long>& uc_freq_caps,
      const std::vector<unsigned long>& virtual_freq_caps,
      const std::vector<SeqOrder>& seq_orders,
      const std::vector<unsigned long>& campaign_ids,
      const std::vector<unsigned long>& uc_campaign_ids);

    AdServer::Commons::Task<bool> co_confirm_user_freq_caps(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      const Generics::Uuid& request_id,
      const std::set<unsigned long>& exclude_pubpixel_accounts);

    AdServer::Commons::Task<bool> co_consider_publishers_optin(
      const AdServer::Commons::UserId& user_id,
      const std::set<unsigned long>& exclude_pubpixel_accounts,
      const Generics::Time& now);

    void clear_expired(
      bool synch,
      const Generics::Time& cleanup_time,
      long portion);

    UserStat get_stats()
      /*throw(NotReady, eh::Exception)*/;

    void
    get_controllable_chunks(
      ChunkIdList& chunk_ids,
      unsigned long& common_chunks_number)
      /*throw(Exception)*/;

    Logging::Logger*
    logger() noexcept;

    FileRWStats::IntervalStats
    get_rw_stats() /*throw(eh::Exception)*/;

    virtual ~UserInfoManagerCore() noexcept;

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
        UserInfoManagerCore* manager,
        Generics::TaskRunner* task_runner)
        noexcept;

      virtual void execute() noexcept;

    protected:
      virtual ~FlushLogsTask() noexcept {};

    private:
      UserInfoManagerCore* manager_;
    };

    class UpdateChannelsConfigTask : public TaskBase
    {
    public:
      UpdateChannelsConfigTask(
        UserInfoManagerCore* user_info_manager_impl,
        Generics::TaskRunner* task_runner)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserInfoManagerCore* user_info_manager_impl_;
    };

    class RotateUserOperationsBackupTask: public TaskBase
    {
    public:
      RotateUserOperationsBackupTask(
        UserInfoManagerCore* user_info_manager_impl,
        Generics::TaskRunner* task_runner)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserInfoManagerCore* user_info_manager_impl_;
    };

    class DeleteOldProfilesTask : public TaskBase
    {
    public:
      DeleteOldProfilesTask(
        Generics::TaskRunner* task_runner,
        UserInfoManagerCore* user_info_manager_impl,
        bool reschedule)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserInfoManagerCore* user_info_manager_impl_;
      bool reschedule_;
    };

    class DeleteOldTemporaryProfilesTask : public TaskBase
    {
    public:
      DeleteOldTemporaryProfilesTask(
        Generics::TaskRunner* task_runner,
        UserInfoManagerCore* user_info_manager_impl,
        bool reschedule)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserInfoManagerCore* user_info_manager_impl_;
      bool reschedule_;
    };

    class LoadChunksDataTask : public TaskBase
    {
    public:
      LoadChunksDataTask(UserInfoManagerCore* user_info_manager_impl,
        Generics::TaskRunner* task_runner)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserInfoManagerCore* user_info_manager_impl_;
    };

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
    UserOperationProcessorAccessor
    get_user_operation_processor_(bool throw_not_ready = true)
      /*throw(NotReady)*/;

    UserInfoContainerAccessor
    get_user_info_container_(bool throw_not_ready = true)
      /*throw(NotReady)*/;

    void load_chunk_files_() noexcept;

    void update_config_() noexcept;

    void rotate_user_operations_backup_() noexcept;

    void load_user_operations_() noexcept;

    void delete_old_profiles_(bool reschedule) noexcept;

    void delete_old_temporary_profiles_(bool reschedule) noexcept;

    void
    update_channels_config_(
      UserInfoContainer* user_info_container,
      UserOperationProcessor* user_operation_processor)
      noexcept;

    void calc_user_daily_stat_(
      const Generics::Time& current_time,
      const Generics::Time& request_tiem,
      const Generics::Time& user_time,
      const Generics::Time& time_offset)
      noexcept;

    void flush_logs_() noexcept;

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
    UserInfoConfigSourcePtr config_source_;

    mutable SyncPolicy::Mutex lock_;
    bool campaignserver_ready_; // REVIEW

    bool clean_user_profiles_;
    Generics::Time profile_lifetime_;
    Generics::Time temp_profile_lifetime_;

    Generics::Time repeat_trigger_timeout_;

    bool provide_channel_counters_;

    mutable SyncPolicy::Mutex daily_stat_lock_;
    unsigned long current_day_;
    unsigned long daily_users_;

    UserInfoManagerLogger_var user_info_manager_logger_;

    Generics::ActiveObjectCallback_var check_operations_callback_;

    LoadingProgressProcessor_var loading_progress_processor_;
    ReferenceCounting::SmartPtr<FileRWStats> file_rw_stats_;
  };

  using UserInfoManagerCorePtr = std::shared_ptr<UserInfoManagerCore>;

} /* AdServer::UserInfoSvcs */

namespace AdServer::UserInfoSvcs
{
  /* Inlines */
  inline
  UserInfoManagerCore::FlushLogsTask::FlushLogsTask(
    UserInfoManagerCore* manager,
    Generics::TaskRunner* task_runner)
    noexcept
    : TaskBase(task_runner),
      manager_(manager)
  {}

  inline
  void
  UserInfoManagerCore::FlushLogsTask::execute()
    noexcept
  {
    manager_->flush_logs_();
  }

  inline
  UserInfoManagerCore::UpdateChannelsConfigTask::UpdateChannelsConfigTask(
    UserInfoManagerCore* user_info_manager_impl,
    Generics::TaskRunner* task_runner)
    noexcept
    : TaskBase(task_runner),
      user_info_manager_impl_(user_info_manager_impl)
  {}

  inline
  void
  UserInfoManagerCore::UpdateChannelsConfigTask::execute()
    noexcept
  {
    user_info_manager_impl_->update_config_();
  }

  inline
  UserInfoManagerCore::
  RotateUserOperationsBackupTask::RotateUserOperationsBackupTask(
    UserInfoManagerCore* user_info_manager_impl,
    Generics::TaskRunner* task_runner)
    noexcept
    : TaskBase(task_runner),
      user_info_manager_impl_(user_info_manager_impl)
  {}

  inline
  void
  UserInfoManagerCore::RotateUserOperationsBackupTask::execute()
    noexcept
  {
    user_info_manager_impl_->rotate_user_operations_backup_();
  }

  // UserInfoManagerCore::DeleteOldProfilesTask
  inline
  UserInfoManagerCore::DeleteOldProfilesTask::DeleteOldProfilesTask(
    Generics::TaskRunner* task_runner,
    UserInfoManagerCore* user_info_manager_impl,
    bool reschedule)
    noexcept
    : TaskBase(task_runner),
      user_info_manager_impl_(user_info_manager_impl),
      reschedule_(reschedule)
  {}

  inline
  void
  UserInfoManagerCore::DeleteOldProfilesTask::execute()
    noexcept
  {
    user_info_manager_impl_->delete_old_profiles_(reschedule_);
  }

  inline
  UserInfoManagerCore::
  DeleteOldTemporaryProfilesTask::DeleteOldTemporaryProfilesTask(
    Generics::TaskRunner* task_runner,
    UserInfoManagerCore* user_info_manager_impl,
    bool reschedule)
    noexcept
    : TaskBase(task_runner),
      user_info_manager_impl_(user_info_manager_impl),
      reschedule_(reschedule)
  {}

  inline
  void
  UserInfoManagerCore::DeleteOldTemporaryProfilesTask::execute()
    noexcept
  {
    user_info_manager_impl_->delete_old_temporary_profiles_(reschedule_);
  }

  // UserInfoManagerCore::LoadChunksDataTask
  inline
  UserInfoManagerCore::LoadChunksDataTask::LoadChunksDataTask(
    UserInfoManagerCore* user_info_manager_impl,
    Generics::TaskRunner* task_runner)
    noexcept
    : TaskBase(task_runner),
      user_info_manager_impl_(user_info_manager_impl)
  {}

  inline
  void
  UserInfoManagerCore::LoadChunksDataTask::execute()
    noexcept
  {
    user_info_manager_impl_->load_chunk_files_();
  }

  inline
  Logging::Logger*
  UserInfoManagerCore::logger() noexcept
  {
    return logger_;
  }

  inline
  FileRWStats::IntervalStats
  UserInfoManagerCore::get_rw_stats() /*throw(eh::Exception)*/
  {
    FileRWStats::IntervalStats res;

    if (file_rw_stats_)
    {
      res = file_rw_stats_->get_stats();
    }

    return res;
  }
} /* AdServer::UserInfoSvcs */
