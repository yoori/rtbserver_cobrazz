#pragma once

#include <list>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/UserInfoManip.hpp>

#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ChunkedExpireProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>


#include <UserInfoSvcs/UserInfoCommons/ChannelDictionary.hpp>
#include <UserInfoSvcs/UserInfoCommons/FreqCapConfig.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerLogger.hpp>

#include "UserOperationProcessor.hpp"

namespace AdServer::UserInfoSvcs
{
  struct UserStat
  {
    UserStat() noexcept;

    unsigned long users_count;
    unsigned long daily_users; // doesn't count by user info container

    unsigned long base_area_size;
    unsigned long temp_area_size;
    unsigned long add_area_size;
    unsigned long history_area_size;
    unsigned long freq_cap_area_size;
    unsigned long all_area_size;

    size_t allocator_cache_size;

    unsigned long ad_channels_count;
    unsigned long discover_channels_count;
  };

  class UserInfoContainer:
    public UserOperationProcessor,
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    typedef UserOperationProcessor::Exception Exception;
    DECLARE_EXCEPTION(UserIsFraud, Exception);
    DECLARE_EXCEPTION(ResourceExhausted, Exception);

  public:
    UserInfoContainer(
      Logging::Logger* logger,
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const AdServer::ProfilingCommons::LevelMapTraits& add_level_map_traits,
      const AdServer::ProfilingCommons::LevelMapTraits& temp_level_map_traits,
      const AdServer::ProfilingCommons::LevelMapTraits& history_level_map_traits,
      const AdServer::ProfilingCommons::LevelMapTraits& base_level_map_traits,
      const AdServer::ProfilingCommons::LevelMapTraits& freq_cap_level_map_traits,
      unsigned long colo_id,
      const Generics::Time& profile_request_timeout,
      const Generics::Time& history_optimization_period,
      bool avg_statistic,
      const Generics::Time& session_timeout,
      bool use_add_profile_on_match,
      unsigned long max_base_profile_waiters,
      unsigned long max_temp_profile_waiters,
      unsigned long max_freqcap_profile_waiters,
      unsigned long rocksdb_batching_threads)
      /*throw(Exception)*/;

    AdServer::Commons::StartableAwaitable<bool>
    co_pre_bid_process(
      UserFreqCapProfile::FreqCapIdArray& freq_caps,
      UserFreqCapProfile::FreqCapIdArray& virtual_freq_caps,
      UserFreqCapProfile::SeqOrderArray& seq_orders,
      UserFreqCapProfile::CampaignFreqs& campaign_freqs,
      std::vector<unsigned long>& optin_publishers,
      const UserId& user_id,
      const Generics::Time& now,
      bool fill_full_freq_caps,
      const Generics::Time& publishers_optin_timeout);

    virtual AdServer::Commons::StartableAwaitable<bool> co_update_freq_caps(
      const UserId& user_id,
      const Generics::Time& now,
      const Commons::RequestId& request_id,
      const UserFreqCapProfile::FreqCapIdArray& freq_caps,
      const UserFreqCapProfile::FreqCapIdArray& uc_freq_caps,
      const UserFreqCapProfile::FreqCapIdArray& virtual_freq_caps,
      const UserFreqCapProfile::SeqOrderArray& seq_orders,
      const UserFreqCapProfile::CampaignIds& campaign_ids,
      const UserFreqCapProfile::CampaignIds& uc_campaign_ids,
      AdServer::ProfilingCommons::OperationPriority op_priority)
      override;

    virtual AdServer::Commons::StartableAwaitable<bool> co_confirm_freq_caps(
      const UserId& user_id,
      const Generics::Time& now,
      const Commons::RequestId& request_id,
      const std::set<unsigned long>& exclude_pubpixel_accounts)
      override;

    AdServer::Commons::StartableAwaitable<bool> co_get_user_profile(
      const UserId& user_id,
      bool temporary,
      SmartMemBuf_var* mb_base_profile_out,
      SmartMemBuf_var* mb_add_profile_out,
      SmartMemBuf_var* mb_history_profile_out,
      SmartMemBuf_var* mb_fc_profile_out = 0);

    virtual AdServer::Commons::StartableAwaitable<bool> co_remove_user_profile(
      const UserId& user_id)
      override;

    virtual AdServer::Commons::StartableAwaitable<bool> co_exchange_merge(
      const UserId& user_id,
      const Generics::MemBuf& base_profile_buf,
      const Generics::MemBuf& history_profile_buf,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
      override;

    virtual AdServer::Commons::StartableAwaitable<bool> co_merge(
      const RequestMatchParams& request_params,
      const Generics::MemBuf& merge_base_profile,
      Generics::MemBuf& merge_add_profile,
      const Generics::MemBuf& merge_history_profile,
      const Generics::MemBuf& merge_freq_cap_profile,
      UserAppearance& user_app,
      long last_colo_id,
      long current_placement_colo_id,
      AdServer::ProfilingCommons::OperationPriority op_priority,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info = 0)
      override;

    virtual AdServer::Commons::StartableAwaitable<bool> co_fraud_user(
      const UserId& user_id,
      const Generics::Time& now)
      override;

    virtual AdServer::Commons::StartableAwaitable<bool> co_match(
      const RequestMatchParams& request_params,
      long last_colo_id,
      long current_placement_colo_id,
      ColoUserId& colo_user_id,
      const ChannelIdPack& matched_channels,
      ChannelMatchMap& result_channels,
      UserAppearance& user_app,
      ProfileProperties& properties,
      AdServer::ProfilingCommons::OperationPriority op_priority,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
      UniqueChannelsResult* unique_channels_result = 0)
      override;

    virtual AdServer::Commons::StartableAwaitable<bool> co_consider_publishers_optin(
      const UserId& user_id,
      const std::set<unsigned long>& publisher_account_ids,
      const Generics::Time& now,
      AdServer::ProfilingCommons::OperationPriority op_priority)
      override;

    void config(
      const Generics::Time& time_offset,
      const Generics::Time& master_stamp,
      ChannelDictionary* channels_config,
      FreqCapConfig* freq_caps)
      noexcept;

    ChannelDictionary_var channels_config() const noexcept;

    Generics::Time time_offset() const noexcept;
    Generics::Time master_stamp() const noexcept;

    UserStat get_stats() const /*throw(eh::Exception)*/;

    void delete_old_profiles(
      const Generics::Time& persistent_lifetime)
      /*throw(NotReady, Exception)*/;

    void delete_old_temporary_profiles(
      const Generics::Time& temp_lifetime)
      /*throw(NotReady, Exception)*/;

  protected:
    typedef AdServer::ProfilingCommons::
      TransactionProfileMap<UserId>::MaxWaitersReached
      MaxWaitersReached;

    typedef AdServer::ProfilingCommons::ChunkedProfileMap<
      UserId,
      AdServer::ProfilingCommons::TransactionProfileMap<UserId>,
      unsigned long (*)(const Generics::Uuid& uuid) >
      UserProfileMap;

    typedef ReferenceCounting::SmartPtr<UserProfileMap>
      UserProfileMap_var;

    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    virtual ~UserInfoContainer() noexcept {};

  private:
    FreqCapConfig_var get_freq_cap_config_() const;

    void filter_channel_thresholds_(ChannelMatchMap& channels)
      /*throw(NotReady, Exception)*/;

    void update_history_(
      Generics::SmartMemBuf* profile,
      Generics::SmartMemBuf* history_profile,
      const Generics::Time& current_time,
      const ChannelDictionary& channel_rules,
      const Generics::Time& time_offset,
      bool update_force = false)
      /*throw(ChannelsMatcher::InvalidProfileException)*/;

    Generics::Time get_last_request_(Generics::SmartMemBuf* profile)
      /*throw(ChannelsMatcher::InvalidProfileException)*/;

    Generics::Time get_create_time_(Generics::SmartMemBuf* profile)
      /*throw(ChannelsMatcher::InvalidProfileException)*/;

    bool profiles_merged_(
      const RequestMatchParams& request_params,
      Generics::SmartMemBuf* base_profile,
      Generics::SmartMemBuf* add_profile,
      const Generics::Time& now,
      const Generics::Time& timeout)
      /*throw(NotReady, ChannelsMatcher::InvalidProfileException)*/;

    template<typename ProfileMapType>
    ReferenceCounting::SmartPtr<ProfileMapType>
    open_chunked_map_(
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const char* chunk_prefix,
      const AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits& profile_map_traits,
      unsigned long max_waiters = 0,
      unsigned long rocksdb_batching_threads = 2)
      /*throw(Exception)*/;

    void trace_match_request_(
      std::ostream& ostr,
      const RequestMatchParams& request_params,
      const ChannelIdPack& matched_channels,
      const Generics::MemBuf& base_profile,
      const Generics::MemBuf* add_profile)
      noexcept;

  private:
    Logging::Logger_var logger_;

    const unsigned long colo_id_;
    const Generics::Time profile_request_timeout_;

    std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>
      rocksdb_processor_;

    UserProfileMap_var base_profiles_;
    UserProfileMap_var temp_profiles_;
    UserProfileMap_var add_profiles_;
    UserProfileMap_var history_profiles_;
    UserProfileMap_var temp_history_profiles_;
    UserProfileMap_var freq_cap_profiles_;

    mutable SyncPolicy::Mutex config_lock_;
    Generics::Time time_offset_;
    Generics::Time master_stamp_;

    ChannelDictionary_var channels_config_;
    FreqCapConfig_var freq_cap_config_;

    mutable SyncPolicy::Mutex stat_lock_;
    const bool profile_avg_statistic_;
    unsigned long ad_channels_count_;
    unsigned long discover_channels_count_;

    mutable SyncPolicy::Mutex clear_expired_lock_;
    const Generics::Time history_optimization_period_;

    const Generics::Time session_timeout_;
    const bool use_add_profile_on_match_;
    const Generics::Time base_profile_expire_time_;
  };

  using UserInfoContainer_var = ReferenceCounting::SmartPtr<UserInfoContainer>;

} /* AdServer::UserInfoSvcs */

namespace AdServer::UserInfoSvcs
{
  inline
  void UserInfoContainer::config(
    const Generics::Time& time_offset,
    const Generics::Time& master_stamp,
    ChannelDictionary* channels_config,
    FreqCapConfig* freq_cap_config)
    noexcept
  {
    ChannelDictionary_var new_channels_config =
      ReferenceCounting::add_ref(channels_config);
    FreqCapConfig_var new_freq_cap_config =
      ReferenceCounting::add_ref(freq_cap_config);

    {
      SyncPolicy::WriteGuard lock(config_lock_);

      time_offset_ = time_offset;
      master_stamp_ = master_stamp;
      channels_config_.swap(new_channels_config);
      freq_cap_config_.swap(new_freq_cap_config);
    }
  }

  inline
  ChannelDictionary_var
  UserInfoContainer::channels_config() const noexcept
  {
    SyncPolicy::ReadGuard lock(config_lock_);
    return channels_config_;
  }

  inline
  FreqCapConfig_var
  UserInfoContainer::get_freq_cap_config_() const
  {
    SyncPolicy::ReadGuard lock(config_lock_);
    FreqCapConfig_var freq_cap_config = freq_cap_config_;

    if(freq_cap_config.in() == 0)
    {
      throw NotReady("Unable to get freq caps configuration.");
    }

    return freq_cap_config;
  }

  inline
  Generics::Time
  UserInfoContainer::time_offset() const noexcept
  {
    SyncPolicy::ReadGuard lock(config_lock_);
    return time_offset_;
  }

  inline
  Generics::Time
  UserInfoContainer::master_stamp() const noexcept
  {
    SyncPolicy::ReadGuard lock(config_lock_);
    return master_stamp_;
  }
}
