#ifndef USERINFOSVCS_USERINFOCONTAINER_HPP
#define USERINFOSVCS_USERINFOCONTAINER_HPP

#include <list>
#include <span>
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
#include <ProfilingCommons/ProfileMap/AsyncRocksDBProfileMap.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <ProfilingCommons/PlainStorage3/LoadingProgressCallback.hpp>


#include <UserInfoSvcs/UserInfoCommons/ChannelDictionary.hpp>
#include <UserInfoSvcs/UserInfoCommons/FreqCapConfig.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerLogger.hpp>

#include "UserOperationProcessor.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
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
      public Generics::CompositeActiveObject,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      using RocksdbManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
      using RocksdbManagerPoolPtr = std::shared_ptr<RocksdbManagerPool>;
      using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;
      using Exception = UserOperationProcessor::Exception;
      using UserIdList = std::list<UserId>;
      using ChanelIds = std::vector<std::uint32_t>;
      using WlChannelIds = std::span<const std::uint32_t>;

      DECLARE_EXCEPTION(UserIsFraud, Exception);

      struct AllUsersProcessingState: public ReferenceCounting::AtomicImpl
      {
        Generics::Time start_time;
        Generics::Time end_time;
        long user_processing_portion;
        bool collect_unique_channels;

        // processing state user counters (including invalid)
        unsigned long start_user_count;
        unsigned long processed_user_count;

        long chunk_i;
        UserIdList chunk_users_to_process;
        unsigned long chunk_users_to_process_count;

        // processing result stats (users count contains only valid users)
        unsigned long user_channels_count;
        unsigned long user_discover_channels_count;
        unsigned long users_count;

      protected:
        virtual ~AllUsersProcessingState() noexcept {}
      };

      using AllUsersProcessingState_var = ReferenceCounting::SmartPtr<AllUsersProcessingState>;

    public:
      UserInfoContainer(
        Logging::Logger* logger,
        unsigned long common_chunks_number,
        const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
        const bool is_level_enable,
        const AdServer::ProfilingCommons::LevelMapTraits& add_level_map_traits,
        const AdServer::ProfilingCommons::LevelMapTraits& temp_level_map_traits,
        const AdServer::ProfilingCommons::LevelMapTraits& history_level_map_traits,
        const AdServer::ProfilingCommons::LevelMapTraits& base_level_map_traits,
        const AdServer::ProfilingCommons::LevelMapTraits& freq_cap_level_map_traits,
        const bool is_rocksdb_enable,
        const RocksDBParams& add_rocksdb_params,
        const RocksDBParams& temp_rocksdb_params,
        const RocksDBParams& history_rocksdb_params,
        const RocksDBParams& base_rocksdb_params,
        const RocksDBParams& freq_cap_rocksdb_params,
        unsigned long colo_id,
        const Generics::Time& profile_request_timeout,
        const Generics::Time& history_optimization_period,
        bool avg_statistic,
        const Generics::Time& session_timeout,
        unsigned long max_base_profile_waiters,
        unsigned long max_temp_profile_waiters,
        unsigned long max_freqcap_profile_waiters,
        AdServer::ProfilingCommons::LoadingProgressCallbackBase_var progress_processor_parent)
        /*throw(Exception)*/;

      void
      get_full_freq_caps(
        const UserId& user_id,
        const Generics::Time& now,
        UserFreqCapProfile::FreqCapIdList& freq_caps,
        UserFreqCapProfile::FreqCapIdList& virtual_freq_caps,
        UserFreqCapProfile::SeqOrderList& seq_orders,
        UserFreqCapProfile::CampaignFreqs& campaign_freqs)
        /*throw(ChunkNotFound, UserIsFraud, Exception)*/;

      virtual void update_freq_caps(
        const UserId& user_id,
        const Generics::Time& now,
        const Commons::RequestId& request_id,
        const UserFreqCapProfile::FreqCapIdList& freq_caps,
        const UserFreqCapProfile::FreqCapIdList& uc_freq_caps,
        const UserFreqCapProfile::FreqCapIdList& virtual_freq_caps,
        const UserFreqCapProfile::SeqOrderList& seq_orders,
        const UserFreqCapProfile::CampaignIds& campaign_ids,
        const UserFreqCapProfile::CampaignIds& uc_campaign_ids,
        AdServer::ProfilingCommons::OperationPriority op_priority)
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      virtual void
      confirm_freq_caps(
        const UserId& user_id,
        const Generics::Time& now,
        const Commons::RequestId& request_id,
        const std::set<unsigned long>& exclude_pubpixel_accounts)
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      virtual bool get_user_profile(
        const UserId& user_id,
        bool temporary,
        SmartMemBuf_var* mb_base_profile_out,
        SmartMemBuf_var* mb_add_profile_out,
        SmartMemBuf_var* mb_history_profile_out,
        SmartMemBuf_var* mb_fc_profile_out = 0)
        /*throw(ChunkNotFound, Exception)*/;

      virtual bool remove_user_profile(
        const UserId& user_id)
        /*throw(ChunkNotFound, Exception)*/;

      virtual void exchange_merge(
        const UserId& user_id,
        const Generics::MemBuf& base_profile_buf,
        const Generics::MemBuf& history_profile_buf,
        UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      void merge(
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
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      virtual void
      fraud_user(
        const UserId& user_id,
        const Generics::Time& now)
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      virtual void
      get_optin_publishers(
        const UserId& user_id,
        const Generics::Time& publishers_optin_timeout,
        std::list<unsigned long>& optin_publishers)
        /*throw(ChunkNotFound, Exception)*/;

      virtual void
      remove_audience_channels(
        const UserId& user_id,
        const AudienceChannelSet& audience_channels)
        /*throw(ChunkNotFound, Exception)*/;

      virtual void
      add_audience_channels(
        const UserId& user_id,
        const AudienceChannelSet& audience_channels)
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      virtual void match(
        const RequestMatchParams& request_params,
        long last_colo_id,
        long current_placement_colo_id,
        ColoUserId& colo_user_id,
        const ChannelMatchPack& matched_channels,
        ChannelMatchMap& result_channels,
        UserAppearance& user_app,
        //PartlyMatchResult& partly_match_result,
        ProfileProperties& properties,
        AdServer::ProfilingCommons::OperationPriority op_priority,
        UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
        UniqueChannelsResult* unique_channels_result = 0)
        /*throw(NotReady, ChunkNotFound, Exception)*/;

      void consider_publishers_optin(
        const UserId& user_id,
        const std::set<unsigned long>& publisher_account_ids,
        const Generics::Time& now,
        AdServer::ProfilingCommons::OperationPriority op_priority)
        /*throw(ChunkNotFound, Exception)*/;

      ChanelIds get_user_channels(
        const UserId& user_id,
        const bool base_profile,
        const bool add_profile,
        const bool history_profile,
        const bool fc_profile,
        const WlChannelIds& wl_channel_ids);
      
      void dump_colo_users() /*throw(Exception)*/;

      bool dispose_user(const UserId& user_id) noexcept;

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

      AllUsersProcessingState_var
      start_all_users_processing(
        const Generics::Time& processing_time,
        unsigned long user_processing_portion,
        bool collect_unique_channels)
        /*throw(NotReady, Exception)*/;

      Generics::Time
      continue_all_users_processing(
        AllUsersProcessingState& state,
        Generics::ActiveObject* interrupter)
        /*throw(NotReady, Exception)*/;

    protected:
      using MaxWaitersReached = AdServer::ProfilingCommons::
        TransactionProfileMap<UserId>::MaxWaitersReached;

      using UserProfileMap = AdServer::ProfilingCommons::ChunkedProfileMap<
        UserId,
        AdServer::ProfilingCommons::TransactionProfileMap<UserId>,
        unsigned long (*)(const Generics::Uuid& uuid)>;

      using UserProfileMap_var = ReferenceCounting::SmartPtr<UserProfileMap>;

      using SyncPolicy = Sync::Policy::PosixThread;

    protected:
      virtual ~UserInfoContainer() noexcept {};

    private:
      void filter_channel_thresholds_(
        ChannelMatchMap& channels)
        /*throw(NotReady, Exception)*/;

      void update_history_(
        Generics::SmartMemBuf* profile,
        Generics::SmartMemBuf* history_profile,
        const Generics::Time& current_time,
        const ChannelDictionary& channel_rules,
        const Generics::Time& time_offset,
        bool update_force = false)
        /*throw(ChannelsMatcher::InvalidProfileException)*/;

      Generics::Time get_last_request_(
        Generics::SmartMemBuf* profile)
        /*throw(ChannelsMatcher::InvalidProfileException)*/;

      Generics::Time get_create_time_(
        Generics::SmartMemBuf* profile)
        /*throw(ChannelsMatcher::InvalidProfileException)*/;

      bool profiles_merged_(
        const RequestMatchParams& request_params,
        Generics::SmartMemBuf* base_profile,
        Generics::SmartMemBuf* add_profile,
        const Generics::Time& now,
        const Generics::Time& timeout)
        /*throw(NotReady, ChannelsMatcher::InvalidProfileException)*/;

      template<typename ProfileMapType, typename AdapterOptionalType>
      ReferenceCounting::SmartPtr<ProfileMapType>
      open_chunked_map_(
        unsigned long common_chunks_number,
        const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
        const char* chunk_prefix,
        const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
        unsigned long max_waiters = 0)
        /*throw(Exception)*/;

      template<
        typename ProfileMap,
        typename Key,
        typename KeyAdapter,
        typename AdapterOptional>
      ReferenceCounting::SmartPtr<ProfileMap> open_chunked_map_(
        const RocksdbManagerPoolPtr& rocksdb_manager_pool,
        unsigned long common_chunks_number,
        const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
        const char* chunk_prefix,
        const bool is_rocksdb_enable,
        const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
        const bool is_level_enable,
        const AdServer::ProfilingCommons::LevelMapTraits& level_map_traits,
        unsigned long max_waiters,
        const KeyAdapter& key_adapter,
        const AdapterOptional& optional_adapter);

      void trace_match_request_(
        std::ostream& ostr,
        const RequestMatchParams& request_params,
        const ChannelMatchPack& matched_channels,
        const Generics::MemBuf& base_profile,
        const Generics::MemBuf* add_profile)
        noexcept;

      bool
      process_user_(
        unsigned long* channels_count,
        unsigned long* discover_channels_count,
        const UserId& user_id_val,
        const Generics::Time& now);

    private:
      Logging::Logger_var logger_;

      const unsigned long colo_id_;
      const Generics::Time profile_request_timeout_;

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
      const Generics::Time base_profile_expire_time_;

      AdServer::ProfilingCommons::LoadingProgressCallbackBase_var
        loading_progress_processor_;
    };

    typedef ReferenceCounting::SmartPtr<UserInfoContainer>
      UserInfoContainer_var;

  } /* UserInfoSvcs */
} /* AdServer */

namespace AdServer
{
namespace UserInfoSvcs
{
  inline
  void UserInfoContainer::config(
    const Generics::Time& time_offset,
    const Generics::Time& master_stamp,
    ChannelDictionary* channels_config,
    FreqCapConfig* freq_cap_config)
    noexcept
  {
    SyncPolicy::WriteGuard lock(config_lock_);

    time_offset_ = time_offset;
    master_stamp_ = master_stamp;
    
    channels_config_ = ReferenceCounting::add_ref(channels_config);
    freq_cap_config_ = ReferenceCounting::add_ref(freq_cap_config);
  }

  inline
  ChannelDictionary_var
  UserInfoContainer::channels_config() const
    noexcept
  {
    SyncPolicy::ReadGuard lock(config_lock_);
    return channels_config_;
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
}

#endif /*USERINFOSVCS_USERINFOCONTAINER_HPP*/
