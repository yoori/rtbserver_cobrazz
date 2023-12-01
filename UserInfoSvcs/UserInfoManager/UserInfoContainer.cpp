#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>

#include <Commons/Algs.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

#include <Logger/ActiveObjectCallback.hpp>

#include <String/AsciiStringManip.hpp>
#include <String/Tokenizer.hpp>

#include "Compatibility/UserBaseProfileAdapter.hpp"
#include "Compatibility/UserHistoryProfileAdapter.hpp"
#include "Compatibility/UserOperationProfilesAdapter.hpp"
#include "Compatibility/UserFreqCapProfileAdapter.hpp"

#include "UserInfoContainer.hpp"

namespace Aspect
{
  const char USER_INFO_CONTAINER[] = "UserInfoContainer";
}

namespace
{
  const long DEFAULT_COLO_ID = -1;
  const char BASE_CHUNK_PREFIX[] = "Base";
  const char ADD_CHUNK_PREFIX[] = "Add";
  const char TEMP_CHUNK_PREFIX[] = "Temp";
  const char HISTORY_CHUNK_PREFIX[] = "History";
  const char TEMP_HISTORY_CHUNK_PREFIX[] = "TempHistory";
  const char FREQCAP_CHUNK_PREFIX[] = "FreqCap";
  const unsigned int CHUNKED_MAPS_COUNT = 8;
}

namespace AdServer
{
namespace UserInfoSvcs
{
  /* UserInfoContainer */
  UserInfoContainer::UserInfoContainer(
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
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id),
      profile_request_timeout_(profile_request_timeout),
      time_offset_(Generics::Time::ZERO),
      profile_avg_statistic_(avg_statistic),
      ad_channels_count_(0),
      discover_channels_count_(0),
      history_optimization_period_(history_optimization_period),
      session_timeout_(session_timeout),
      base_profile_expire_time_(base_level_map_traits.expire_time)
  {
    using AdaptBaseProfile =
      AdServer::ProfilingCommons::OptionalProfileAdapter<BaseProfileAdapter>;
    using AdaptHistoryProfile =
      AdServer::ProfilingCommons::OptionalProfileAdapter<HistoryProfileAdapter>;
    using AdaptFreqCapProfile =
      AdServer::ProfilingCommons::OptionalProfileAdapter<UserFreqCapProfileAdapter>;
    using ManagerPoolConfig = UServerUtils::Grpc::RocksDB::Config;

    if (progress_processor_parent.in())
    {
      loading_progress_processor_ =
        new AdServer::ProfilingCommons::LoadingProgressCallback(
          progress_processor_parent,
          CHUNKED_MAPS_COUNT * chunk_folders.size());
    }
    else
    {
      loading_progress_processor_ =
        new AdServer::ProfilingCommons::LoadingProgressCallbackBase();
    }

    auto number_threads = std::thread::hardware_concurrency();
    if (number_threads == 0)
    {
      number_threads = 30;
    }

    ManagerPoolConfig config;
    config.event_queue_max_size = 10000000;
    config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
    config.io_uring_size = 6400;
    config.number_io_urings = 2 * number_threads;
    auto rocksdb_manager_pool = std::make_shared<RocksdbManagerPool>(
      config,
      logger);

    auto key_adapter = [] (const UserId& key) {
      return std::string(reinterpret_cast<const char*>(key.begin()), key.size());
    };

    base_profiles_ = open_chunked_map_<UserProfileMap, UserId>(
      rocksdb_manager_pool,
      common_chunks_number,
      chunk_folders,
      BASE_CHUNK_PREFIX,
      is_rocksdb_enable,
      base_rocksdb_params,
      is_level_enable,
      base_level_map_traits,
      max_base_profile_waiters,
      key_adapter,
      AdaptBaseProfile{});

    temp_profiles_ = open_chunked_map_<UserProfileMap, UserId>(
      rocksdb_manager_pool,
      common_chunks_number,
      chunk_folders,
      TEMP_CHUNK_PREFIX,
      is_rocksdb_enable,
      temp_rocksdb_params,
      is_level_enable,
      temp_level_map_traits,
      max_temp_profile_waiters,
      key_adapter,
      AdaptBaseProfile{});

    add_profiles_ = open_chunked_map_<UserProfileMap, UserId>(
      rocksdb_manager_pool,
      common_chunks_number,
      chunk_folders,
      ADD_CHUNK_PREFIX,
      is_rocksdb_enable,
      add_rocksdb_params,
      is_level_enable,
      add_level_map_traits,
      0,
      key_adapter,
      AdaptBaseProfile{});

    history_profiles_ = open_chunked_map_<UserProfileMap, UserId>(
      rocksdb_manager_pool,
      common_chunks_number,
      chunk_folders,
      HISTORY_CHUNK_PREFIX,
      is_rocksdb_enable,
      history_rocksdb_params,
      is_level_enable,
      history_level_map_traits,
      0,
      key_adapter,
      AdaptHistoryProfile{});

    temp_history_profiles_ = open_chunked_map_<UserProfileMap, UserId>(
      rocksdb_manager_pool,
      common_chunks_number,
      chunk_folders,
      TEMP_HISTORY_CHUNK_PREFIX,
      is_rocksdb_enable,
      temp_rocksdb_params,
      is_level_enable,
      temp_level_map_traits,
      0,
      key_adapter,
      AdaptHistoryProfile{});

    freq_cap_profiles_ = open_chunked_map_<UserProfileMap, UserId>(
      rocksdb_manager_pool,
      common_chunks_number,
      chunk_folders,
      FREQCAP_CHUNK_PREFIX,
      is_rocksdb_enable,
      freq_cap_rocksdb_params,
      is_level_enable,
      freq_cap_level_map_traits,
      max_freqcap_profile_waiters,
      key_adapter,
      AdaptFreqCapProfile{});
  }

  void
  UserInfoContainer::remove_audience_channels(
    const UserId& user_id,
    const AudienceChannelSet& audience_channels)
    /*throw(ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::remove_audience_channels()";

    if (audience_channels.empty())
    {
      return;
    }
    
    try
    {
      UserProfileMap::Transaction_var base_profile_trans =
        base_profiles_->get_transaction(
          user_id,
          true,
          ProfilingCommons::OP_BACKGROUND // not runtime request have lower priority
          );

      Generics::Time access_time;
      SmartMemBuf_var base_mem_buf = Algs::copy_membuf(
        base_profile_trans->get_profile(&access_time));

      if (base_mem_buf.in() == 0)
      {
        return;
      }

      SmartMemBuf_var add_mem_buf(new SmartMemBuf);
      
      ChannelsMatcher matching(base_mem_buf.in(), add_mem_buf.in()); 

      matching.remove_audience_channels(audience_channels);

      base_profile_trans->save_profile(
        Generics::transfer_membuf(base_mem_buf),
        access_time);
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
  
  void
  UserInfoContainer::add_audience_channels(
    const UserId& user_id,
    const AudienceChannelSet& audience_channels)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::add_audience_channels()";

    if (audience_channels.size() == 0)
    {
      return;
    }
    
    ChannelDictionary_var channel_rules = channels_config();

    if (channel_rules.in() == 0)
    {
      throw NotReady("Unable to get channels configuration.");
    }

    try
    {
      UserProfileMap::Transaction_var base_profile_trans =
        base_profiles_->get_transaction(
          user_id,
          true,
          ProfilingCommons::OP_BACKGROUND // not runtime request have lower priority
          );

      SmartMemBuf_var base_mem_buf = Algs::copy_membuf(
        base_profile_trans->get_profile());

      if (base_mem_buf.in() == 0)
      {
        base_mem_buf = new SmartMemBuf;
      }

      Generics::Time now = Generics::Time::get_time_of_day();

      SmartMemBuf_var add_mem_buf(new SmartMemBuf);

      ChannelsMatcher matching(base_mem_buf.in(), add_mem_buf.in());

      matching.add_audience_channels(
        audience_channels,
        *channel_rules,
        now - base_profile_expire_time_);

      base_profile_trans->save_profile(
        Generics::transfer_membuf(base_mem_buf),
        now);
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
  
  void
  UserInfoContainer::get_full_freq_caps(
    const UserId& user_id,
    const Generics::Time& now,
    UserFreqCapProfile::FreqCapIdList& freq_caps,
    UserFreqCapProfile::FreqCapIdList& virtual_freq_caps,
    UserFreqCapProfile::SeqOrderList& seq_orders,
    UserFreqCapProfile::CampaignFreqs& campaign_freqs)
    /*throw(ChunkNotFound, UserIsFraud, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::get_full_freq_caps()";

    FreqCapConfig_var freq_cap_config;

    {
      SyncPolicy::ReadGuard lock(config_lock_);
      freq_cap_config = freq_cap_config_;
    }

    if(freq_cap_config.in() == 0)
    {
      throw NotReady("Unable to get channels configuration.");
    }

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        freq_cap_profiles_->get_transaction(
          user_id,
          true, // check max waiters
          ProfilingCommons::OP_RUNTIME);
      UserFreqCapProfile profile(fc_profile_trans->get_profile());

      if(profile.full(
           freq_caps,
           &virtual_freq_caps,
           seq_orders,
           campaign_freqs,
           now,
           *freq_cap_config))
      {
        fc_profile_trans->save_profile(
          profile.transfer_membuf(),
          now);
      }
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserFreqCapProfile::Invalid: " << ex.what();
      throw UserIsFraud(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoContainer::update_freq_caps(
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
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::update_freq_caps()";

    FreqCapConfig_var freq_cap_config;

    {
      SyncPolicy::ReadGuard lock(config_lock_);
      freq_cap_config = freq_cap_config_;
    }

    if(freq_cap_config.in() == 0)
    {
      throw NotReady("Unable to get channels configuration.");
    }

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        freq_cap_profiles_->get_transaction(user_id, true, op_priority);

      ConstSmartMemBuf_var fc_mem_buf = fc_profile_trans->get_profile();

      UserFreqCapProfile profile(fc_mem_buf);

      profile.consider(
        request_id,
        now,
        freq_caps,
        uc_freq_caps,
        virtual_freq_caps,
        seq_orders,
        campaign_ids,
        uc_campaign_ids,
        *freq_cap_config);

      fc_profile_trans->save_profile(
        profile.transfer_membuf(),
        now);
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {}
    catch (const MaxWaitersReached&)
    {}
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void UserInfoContainer::confirm_freq_caps(
    const UserId& user_id,
    const Generics::Time& now,
    const Commons::RequestId& request_id,
    const std::set<unsigned long>& exclude_pubpixel_accounts)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::confirm_freq_caps()";

    FreqCapConfig_var freq_cap_config;

    {
      SyncPolicy::ReadGuard lock(config_lock_);
      freq_cap_config = freq_cap_config_;
    }

    if(freq_cap_config.in() == 0)
    {
      throw NotReady("Unable to get freq caps configuration.");
    }

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        freq_cap_profiles_->get_transaction(user_id, false);

      ConstSmartMemBuf_var fc_mem_buf = fc_profile_trans->get_profile();

      if(!fc_mem_buf.in())
      {
        fc_mem_buf = new ConstSmartMemBuf();
      }

      UserFreqCapProfile profile(fc_mem_buf.in());

      bool res = profile.consider_publishers_optin(
        exclude_pubpixel_accounts,
        now);

      res |= profile.confirm_request(
        Commons::RequestId(request_id),
        now,
        *freq_cap_config);

      if(res)
      {
        fc_profile_trans->save_profile(
          profile.transfer_membuf(),
          now);
      }
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {}
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  UserInfoContainer::dispose_user(const UserId& user_id)
    noexcept
  {
    return base_profiles_->dispose_profile(user_id);
  }

  bool
  UserInfoContainer::get_user_profile(
    const UserId& user_id,
    bool temporary,
    SmartMemBuf_var* mb_base_profile_out,
    SmartMemBuf_var* mb_add_profile_out,
    SmartMemBuf_var* mb_history_profile_out,
    SmartMemBuf_var* mb_fc_profile_out)
    /*throw(ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::get_user_profile()";

    try
    {
      if(mb_base_profile_out)
      {
        if(!temporary)
        {
          *mb_base_profile_out = Algs::copy_membuf(
            base_profiles_->get_profile(user_id));
        }
        else
        {
          *mb_base_profile_out = Algs::copy_membuf(
            temp_profiles_->get_profile(user_id));
        }
      }

      if(mb_add_profile_out)
      {
        *mb_add_profile_out = Algs::copy_membuf(
          add_profiles_->get_profile(user_id));
      }

      if(mb_history_profile_out)
      {
        if(!temporary)
        {
          *mb_history_profile_out = Algs::copy_membuf(
            history_profiles_->get_profile(user_id));
        }
        else
        {
          *mb_history_profile_out = Algs::copy_membuf(
            temp_history_profiles_->get_profile(user_id));
        }
      }

      if(mb_fc_profile_out)
      {
        Generics::ConstSmartMemBuf_var mb = freq_cap_profiles_->get_profile(user_id);

        if(mb && mb->membuf().size() <= 40*1024*1024)
        {
          *mb_fc_profile_out = Algs::copy_membuf(mb);
        }
      }

      return true;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  UserInfoContainer::remove_user_profile(const UserId& user_id)
    /*throw(ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::remove_user_profile()";

    try
    {
      base_profiles_->remove_profile(user_id);
      temp_profiles_->remove_profile(user_id);
      add_profiles_->remove_profile(user_id);
      history_profiles_->remove_profile(user_id);
      temp_history_profiles_->remove_profile(user_id);
      freq_cap_profiles_->remove_profile(user_id);

      return true;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoContainer::exchange_merge(
    const UserId& user_id,
    const Generics::MemBuf& other_base_profile_buf,
    const Generics::MemBuf& other_history_profile_buf,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::merge_profiles()";

    ChannelDictionary_var channel_rules = channels_config();
    Generics::Time current_time_offset = time_offset();

    if (channel_rules.in() == 0)
    {
      throw NotReady("Unable to get channels configuration.");
    }

    try
    {
      UserProfileMap::Transaction_var base_profile_trans =
        base_profiles_->get_transaction(
          user_id,
          true, // check max waiters
          AdServer::ProfilingCommons::OP_BACKGROUND);

      UserProfileMap::Transaction_var add_profile_trans =
        add_profiles_->get_transaction(
          user_id,
          true, // check max waiters
          AdServer::ProfilingCommons::OP_BACKGROUND);

      SmartMemBuf_var add_buf = Algs::copy_membuf(
        add_profile_trans->get_profile());

      if(add_buf.in() && !other_base_profile_buf.empty())
      {
        try
        {
          Generics::SmartMemBuf_var base_profile(
            new Generics::SmartMemBuf(other_base_profile_buf));
          if (other_base_profile_buf.size() != 0)
          {
            BaseProfileAdapter base_adapter;
            base_profile = Algs::copy_membuf(
              base_adapter(
                Generics::transfer_membuf(base_profile),
                true // ignore_future_versions
                ));
          }

          Generics::SmartMemBuf_var history_profile(
            new Generics::SmartMemBuf(other_history_profile_buf));
          if (other_history_profile_buf.size() != 0)
          {
            HistoryProfileAdapter history_adapter;
            history_profile = Algs::copy_membuf(
              history_adapter(
                Generics::transfer_membuf(history_profile)));
          }
          
          const Generics::Time now(
            std::max(
              get_last_request_(base_profile.in()),
              get_last_request_(add_buf.in())));
          
          update_history_(base_profile.in(),
                          history_profile.in(), now, *channel_rules, current_time_offset);
          
          SmartMemBuf_var empty_buf(new SmartMemBuf);
          
          ChannelsMatcher matching(add_buf.in(), empty_buf.in());
          
          ProfileMatchParams profile_match_params;
          matching.merge(
            0, // skip local history profile, replace it with other history profile
            base_profile->membuf(),
            history_profile->membuf(),
            *channel_rules,
            profile_match_params);
          
          add_profile_trans->remove_profile();
          
          if (add_buf->membuf().size() != 0)
          {
            base_profile_trans->save_profile(
              Generics::transfer_membuf(
                Algs::copy_membuf(add_buf)),
              now);
          }
          
          if (history_profile->membuf().size() != 0)
          {
            history_profiles_->save_profile(
              user_id,
              Generics::transfer_membuf(
                Algs::copy_membuf(history_profile)),
              now);
          }
          
          if (ho_info != 0 && add_buf->membuf().size() != 0)
          {
            ChannelsMatcher cm(add_buf.in(), empty_buf.in());
            
            if (cm.need_channel_count_stats_logging(now, current_time_offset))
            {
              UniqueChannelsResult ucr;
              matching.unique_channels(
                add_buf->membuf(),
                history_profile->membuf().size() != 0 ? &history_profile->membuf() : 0,
                *channel_rules,
                ucr);
              
              ho_info->isp_date = now + current_time_offset;
              ho_info->adv_channel_count = ucr.simple_channels;
              ho_info->discover_channel_count = ucr.discover_channels;
            }
          }
        }
        catch(const ChannelsMatcher::InvalidProfileException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Caught ChannelsMatcher::InvalidProfileException "
            "at user: " << user_id << ": " << ex.what();
          throw Exception(ostr);
        }
      }
    }
    catch(const MaxWaitersReached&)
    {
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
  }

  void
  UserInfoContainer::merge(
    const RequestMatchParams& request_params,
    const Generics::MemBuf& merge_base_profile_buf,
    Generics::MemBuf& merge_add_profile_buf,
    const Generics::MemBuf& merge_history_profile_buf,
    const Generics::MemBuf& merge_freq_cap_profile_buf,
    UserAppearance& user_app,
    long last_colo_id,
    long current_placement_colo_id,
    AdServer::ProfilingCommons::OperationPriority op_priority,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::merge()";

    try
    {
      ChannelDictionary_var channel_rules = channels_config();

      if (channel_rules.in() == 0)
      {
        throw NotReady("Unable to get channels configuration.");
      }

      FreqCapConfig_var freq_cap_config;

      {
        SyncPolicy::ReadGuard lock(config_lock_);
        freq_cap_config = freq_cap_config_;
      }

      if(freq_cap_config.in() == 0)
      {
        throw NotReady("Unable to get freq caps configuration.");
      }

      Generics::Time current_time_offset = time_offset();

      const UserId& user_id = request_params.user_id;
      bool temporary = request_params.temporary;
      bool merge_to_additional = false;
      UserProfileMap::Transaction_var target_profile_trans;
      UserProfileMap::Transaction_var target_history_profile_trans;
      SmartMemBuf_var target_profile;
      SmartMemBuf_var target_history_profile;

      bool new_user =
        !temporary && request_params.change_last_request && !request_params.household;

      try
      {
        target_profile_trans = temporary ?
          temp_profiles_->get_transaction(user_id, true, op_priority) :
          base_profiles_->get_transaction(user_id, true, op_priority);

        UserProfileMap::Transaction_var add_profile_trans =
          add_profiles_->get_transaction(user_id, true, op_priority);

        target_profile = Algs::copy_membuf(
          add_profile_trans->get_profile());

        if ((!request_params.use_empty_profile &&
             last_colo_id != current_placement_colo_id &&
             last_colo_id != DEFAULT_COLO_ID) ||
            target_profile.in())
        {
          /* if additional profile exist merge buffers to it */
          target_profile_trans = add_profile_trans;

          merge_to_additional = true;
        }
        else
        {
          target_profile = Algs::copy_membuf(
            target_profile_trans->get_profile());
        }
      }
      catch(const MaxWaitersReached&)
      {
        return;
      }

      Generics::SmartMemBuf_var merge_base_profile(
        new Generics::SmartMemBuf(merge_base_profile_buf));
      if (merge_base_profile_buf.size() != 0)
      {
        BaseProfileAdapter base_adapter;
        merge_base_profile = Algs::copy_membuf(
          base_adapter(
            Generics::transfer_membuf(merge_base_profile)));
      }

      Generics::SmartMemBuf_var merge_history_profile(
        new Generics::SmartMemBuf(merge_history_profile_buf));
      if (merge_history_profile_buf.size() != 0)
      {
        HistoryProfileAdapter history_adapter;
        merge_history_profile = Algs::copy_membuf(
          history_adapter(
            Generics::transfer_membuf(merge_history_profile)));
      }

      Generics::ConstSmartMemBuf_var merge_freq_cap_profile;

      if(!merge_freq_cap_profile_buf.empty())
      {
        Generics::SmartMemBuf_var v(new SmartMemBuf(merge_freq_cap_profile_buf));
        UserFreqCapProfileAdapter freq_cap_adapter;
        merge_freq_cap_profile = freq_cap_adapter(
          Generics::transfer_membuf(v));
      }

      Generics::Time merge_time = request_params.current_time;

      if (target_profile.in() == 0)
      {
        target_profile = new SmartMemBuf;
      }
      else
      {
        user_app.last_request = get_last_request_(target_profile.in());
        user_app.create_time = get_create_time_(target_profile.in());
      }

      target_history_profile_trans = temporary ?
        temp_history_profiles_->get_transaction(user_id, true, op_priority) :
        history_profiles_->get_transaction(user_id, true, op_priority);

      target_history_profile = Algs::copy_membuf(
        target_history_profile_trans->get_profile());

      if (target_history_profile.in() == 0)
      {
        target_history_profile = new SmartMemBuf;
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream tracing_ostr;

        tracing_ostr <<
          "Merge Tracing: " << std::endl <<
          "Merge to " << (merge_to_additional ? "additional" : "base") <<
          " profile for target user '" <<
          PrivacyFilter::filter(request_params.user_id.to_string().c_str(), "USER_ID") <<
          "' before merging: " << std::endl;

        if (target_profile.in())
        {
          ChannelsMatcher::print(
            target_profile->membuf().get<unsigned char>(),
            target_profile->membuf().size(),
            tracing_ostr,
            true,
            true);
        }
        else
        {
          tracing_ostr << "Profile is not exist";
        }

        tracing_ostr << std::endl <<
          "History profile before merging: " << std::endl;

        if (target_history_profile.in())
        {
          ChannelsMatcher::history_print(
            target_history_profile->membuf().get<unsigned char>(),
            target_history_profile->membuf().size(),
            tracing_ostr,
            true);
        }
        else
        {
          tracing_ostr << "Profile is not exist";
        }

        tracing_ostr << std::endl <<
          "Base profile for merge user: " << std::endl;

        ChannelsMatcher::print(
          merge_base_profile_buf.get<unsigned char>(),
          merge_base_profile_buf.size(),
          tracing_ostr,
          true,
          true);

        tracing_ostr << std::endl <<
          "Additional profile for merge user: " << std::endl;

        ChannelsMatcher::print(
          merge_add_profile_buf.get<unsigned char>(),
          merge_add_profile_buf.size(),
          tracing_ostr,
          true,
          true);

        tracing_ostr << std::endl <<
          "History profile for merge user: " << std::endl;

        ChannelsMatcher::history_print(
          merge_history_profile_buf.get<unsigned char>(),
          merge_history_profile_buf.size(),
          tracing_ostr,
          true);

        tracing_ostr << std::endl;

        logger_->log(tracing_ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_CONTAINER);
      }

      SmartMemBuf_var add_mb(new SmartMemBuf);

      ChannelsMatcher user_profile_adapter(
        target_profile.in(), add_mb.in());

      if (target_profile.in())
      {
        update_history_(
          target_profile.in(),
          target_history_profile.in(),
          merge_time,
          *channel_rules,
          current_time_offset);
      }

      if (target_profile.in() == 0)
      {
        target_profile = new SmartMemBuf;
      }

      if (target_history_profile.in() == 0)
      {
        target_history_profile = new SmartMemBuf;
      }

      if ((merge_time + current_time_offset).get_gm_time().get_date() <=
          (user_app.last_request + current_time_offset).get_gm_time().get_date())
      {
        new_user = false;
      }

      update_history_(
        merge_base_profile.in(),
        merge_history_profile.in(),
        merge_time,
        *channel_rules,
        current_time_offset);

      user_profile_adapter.merge(
        target_history_profile.in(),
        merge_base_profile->membuf(),
        merge_history_profile->membuf(),
        *channel_rules,
        request_params,
        merge_time);

      if(merge_add_profile_buf.size() > 0)
      {
        SmartMemBuf_var empty_hp(new SmartMemBuf);

        Generics::SmartMemBuf_var merge_add_profile(
          new Generics::SmartMemBuf(merge_add_profile_buf));

        BaseProfileAdapter add_adapter;
        merge_add_profile = Algs::copy_membuf(
          add_adapter(
            Generics::transfer_membuf(merge_add_profile)));

        user_profile_adapter.merge(
          target_history_profile.in(),
          merge_add_profile->membuf(),
          empty_hp->membuf(),
          *channel_rules,
          request_params,
          merge_time);
      }

      if (target_history_profile->membuf().size() != 0)
      {
        update_history_(
          target_profile.in(),
          target_history_profile.in(),
          merge_time,
          *channel_rules,
          current_time_offset,
          true);
      }

      UserProfileMap::Transaction_var target_freq_cap_profile_trans =
        freq_cap_profiles_->get_transaction(user_id, true, op_priority);

      ConstSmartMemBuf_var target_freq_cap_profile =
        target_freq_cap_profile_trans->get_profile();

      if(merge_freq_cap_profile)
      {
        try
        {
          UserFreqCapProfile freq_cap_profile(target_freq_cap_profile);
          freq_cap_profile.merge(
            merge_freq_cap_profile,
            merge_time,
            *freq_cap_config);
          target_freq_cap_profile = freq_cap_profile.transfer_membuf();
        }
        catch(const UserFreqCapProfile::Invalid&)
        {}
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream tracing_ostr;

        tracing_ostr <<
          "Profile for user '" <<
          PrivacyFilter::filter(request_params.user_id.to_string().c_str(), "USER_ID") <<
          "' after merging: " << std::endl;

        user_profile_adapter.print(
          target_profile->membuf().get<unsigned char>(),
          target_profile->membuf().size(),
          tracing_ostr,
          true,
          true);

        tracing_ostr << std::endl <<
          "History profile for user '" <<
          PrivacyFilter::filter(request_params.user_id.to_string().c_str(), "USER_ID") <<
          "' after merging: " << std::endl;

        user_profile_adapter.history_print(
          target_history_profile->membuf().get<unsigned char>(),
          target_history_profile->membuf().size(),
          tracing_ostr,
          true);

        logger_->log(tracing_ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_CONTAINER);
      }

      if (!merge_to_additional && new_user && ho_info != 0)
      {
        UniqueChannelsResult ucr;
        user_profile_adapter.unique_channels(
          target_profile->membuf(),
          target_history_profile->membuf().size() != 0 ?
            &target_history_profile->membuf() : 0,
          *channel_rules,
          ucr);

        ho_info->isp_date = merge_time + current_time_offset;
        ho_info->adv_channel_count = ucr.simple_channels;
        ho_info->discover_channel_count = ucr.discover_channels;
      }

      if (target_history_profile->membuf().size() != 0)
      {
        target_history_profile_trans->save_profile(
          Generics::transfer_membuf(target_history_profile),
          merge_time);
      }

      if (target_profile->membuf().size() != 0)
      {
        target_profile_trans->save_profile(
          Generics::transfer_membuf(target_profile),
          merge_time);
      }

      if(target_freq_cap_profile && merge_freq_cap_profile)
      {
        target_freq_cap_profile_trans->save_profile(
          target_freq_cap_profile,
          merge_time);
      }
    }
    catch(const MaxWaitersReached&)
    {}
    catch(const ChannelsMatcher::InvalidProfileException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ChannelsMatcher::InvalidProfileException"
        " at user '" << request_params.user_id.to_string() <<
        ": " << ex.what();
      throw Exception(ostr);
    }
    catch(const NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught NotReady: " << ex.what();
      throw NotReady(ostr);
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoContainer::fraud_user(
    const UserId& user_id,
    const Generics::Time& now)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::fraud_user()";

    try
    {
      UserProfileMap::Transaction_var base_profile_trans =
        base_profiles_->get_transaction(
          user_id,
          true, // check max waiters
          AdServer::ProfilingCommons::OP_RUNTIME);
      SmartMemBuf_var base_mem_buf = Algs::copy_membuf(
        base_profile_trans->get_profile());

      if(base_mem_buf.in() == 0)
      {
        base_mem_buf = new SmartMemBuf;
      }

      SmartMemBuf_var add_mem_buf(new SmartMemBuf);
      ChannelsMatcher cm(base_mem_buf.in(), add_mem_buf.in());

      if(cm.fraud_user(now))
      {
        base_profile_trans->save_profile(
          Generics::transfer_membuf(base_mem_buf),
          cm.last_request());
      }
    }
    catch(const MaxWaitersReached&)
    {}
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoContainer::get_optin_publishers(
    const UserId& user_id,
    const Generics::Time& publishers_optin_timeout,
    std::list<unsigned long>& optin_publishers)
    /*throw(ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::get_optin_publishers()";

    try
    {
      if (publishers_optin_timeout != Generics::Time::ZERO)
      {
        ConstSmartMemBuf_var fc_mem_buf = freq_cap_profiles_->get_profile(user_id);
        
        if(fc_mem_buf.in() && fc_mem_buf->membuf().size() > 0)
        {
          UserFreqCapProfile profile(fc_mem_buf);
          
          profile.get_optin_publishers(optin_publishers, publishers_optin_timeout);
        }
      }
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {}
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
  
  void
  UserInfoContainer::match(
    const RequestMatchParams& request_params,
    long last_colo_id,
    long current_placement_colo_id,
    ColoUserId& colo_user_id,
    const ChannelMatchPack& matched_channels,
    ChannelMatchMap& result_channels,
    UserAppearance& user_app,
    ProfileProperties& properties,
    AdServer::ProfilingCommons::OperationPriority op_priority,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
    UniqueChannelsResult* unique_channels_result)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::match()";

    try
    {
      bool new_user = false;
      ChannelDictionary_var channel_rules = channels_config();

      if (channel_rules.in() == 0)
      {
        throw NotReady("Unable to get channels configuration.");
      }

      Generics::Time current_time_offset = time_offset();

      const UserId& user_id = request_params.user_id;
      bool temporary = request_params.temporary;
      Generics::Time match_time = request_params.current_time;

      UserProfileMap::Transaction_var base_profile_trans =
        temporary ?
        temp_profiles_->get_transaction(user_id, true, op_priority) :
        base_profiles_->get_transaction(user_id, true, op_priority);
      UserProfileMap::Transaction_var add_profile_trans =
        add_profiles_->get_transaction(user_id, true, op_priority);

      SmartMemBuf_var base_mem_buf = Algs::copy_membuf(
        base_profile_trans->get_profile());
      SmartMemBuf_var add_mem_buf = Algs::copy_membuf(
        add_profile_trans->get_profile());

      bool match_to_additional =
        !request_params.use_empty_profile &&
        ((last_colo_id != current_placement_colo_id &&
         last_colo_id != DEFAULT_COLO_ID) ||
         add_mem_buf.in());

      if (add_mem_buf.in() == 0)
      {
        add_mem_buf = new SmartMemBuf;

        if (match_to_additional && logger_->log_level() >= Logging::Logger::TRACE)
        {
          std::ostringstream ostr;
          ostr << "Additional profile for user with uid = " << user_id <<
            "was created with create_time " <<
            match_time.get_gm_time();

          logger_->log(
            ostr.str(),
            Logging::Logger::TRACE,
            Aspect::USER_INFO_CONTAINER);
        }
      }
      if (base_mem_buf.in() == 0)
      {
        new_user = request_params.change_last_request;

        base_mem_buf = new SmartMemBuf;
      }

      if (match_to_additional)
      {
        new_user = false;

        if (profiles_merged_(
              request_params,
              base_mem_buf.in(),
              add_mem_buf.in(),
              match_time,
              profile_request_timeout_))
        {
          match_to_additional = false;

          add_profile_trans->remove_profile();

          Stream::Error ostr;
          ostr << FUN << ": Base profile from other colo did not received. "
            "Base and additional profiles were merged on current colo "
            "for uid = " << user_id << ".";

          logger_->log(ostr.str(),
            Logging::Logger::WARNING,
            Aspect::USER_INFO_CONTAINER,
            "ADS-IMPL-76");
        }
      }

      ChannelsMatcher matching(base_mem_buf.in(), add_mem_buf.in());

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream tracing_ostr;
        tracing_ostr << "Input match request: " << std::endl;

        trace_match_request_(
          tracing_ostr,
          request_params,
          matched_channels,
          base_mem_buf->membuf(),
          &add_mem_buf->membuf());

        logger_->log(tracing_ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_CONTAINER);
      }

      if(!request_params.use_empty_profile)
      {
        if(request_params.request_colo_id != -1)
        {
          user_app.last_request = matching.last_request();
        }

        bool need_history_optimize =
          matching.need_history_optimization(
            match_time,
            history_optimization_period_,
            current_time_offset);

        new_user =
          !match_to_additional &&
          request_params.change_last_request &&
          !request_params.household &&
          (new_user ||
           matching.need_channel_count_stats_logging(
             match_time, current_time_offset));

        if(need_history_optimize)
        {
          /* first request today : history optimization */
          UserProfileMap::Transaction_var history_profile_trans =
            temporary ?
            temp_history_profiles_->get_transaction(user_id, true, op_priority) :
            history_profiles_->get_transaction(user_id, true, op_priority);

          SmartMemBuf_var hist_mem_buf = Algs::copy_membuf(
            history_profile_trans->get_profile());

          if (hist_mem_buf.in() == 0)
          {
            hist_mem_buf = new SmartMemBuf;
          }

          if(logger_->log_level() >= Logging::Logger::TRACE)
          {
            std::ostringstream ostr;
            ostr << "To history optimize user '" << user_id << "':" <<
              std::endl << "  History Profile:" << std::endl;

            matching.history_print(
              hist_mem_buf->membuf().get<unsigned char>(),
              hist_mem_buf->membuf().size(),
              ostr,
              true);

            logger_->log(
              ostr.str(),
              Logging::Logger::TRACE,
              Aspect::USER_INFO_CONTAINER);
          }

          bool first_today_history_optimization;

          matching.history_optimize(
            hist_mem_buf.in(),
            match_time,
            current_time_offset,
            *channel_rules,
            &first_today_history_optimization);

          if(logger_->log_level() >= Logging::Logger::TRACE)
          {
            std::ostringstream ostr;
            ostr << "From history optimize user '" << user_id << "':" <<
              std::endl << "  History Profile:" << std::endl;

            matching.history_print(
              hist_mem_buf->membuf().get<unsigned char>(),
              hist_mem_buf->membuf().size(), ostr, true);

            logger_->log(ostr.str(),
              Logging::Logger::TRACE,
              Aspect::USER_INFO_CONTAINER);
          }

          if (!request_params.silent_match)
          {
            history_profile_trans->save_profile(
              Generics::transfer_membuf(hist_mem_buf),
              match_time);
          }
        }
      }

      user_app.session_start = matching.session_start();

      matching.match(
        result_channels,
        match_time,
        matched_channels,
        *channel_rules,
        //request_params.request_colo_id,
        request_params,
        properties,
        session_timeout_,
        match_to_additional);

      user_app.create_time = matching.create_time();

      if (last_colo_id != current_placement_colo_id)
      {
        user_id.to_string().swap(colo_user_id.user_id);
        colo_user_id.colo_id = last_colo_id;
        colo_user_id.need_profile = true;
      }
      else
      {
        colo_user_id.need_profile = false;
      }

      if (request_params.provide_channel_count && unique_channels_result != 0)
      {
        UserProfileMap::Transaction_var history_profile_trans =
          history_profiles_->get_transaction(user_id, true, op_priority);

        SmartMemBuf_var hist_mem_buf = Algs::copy_membuf(
          history_profile_trans->get_profile());

        UniqueChannelsResult ucr;
        matching.unique_channels(
          base_mem_buf->membuf(),
          hist_mem_buf.in() ? &hist_mem_buf->membuf() : 0,
          *channel_rules,
          *unique_channels_result);
      }

      if(!request_params.use_empty_profile && !request_params.silent_match)
      {
        user_app.session_start = matching.session_start();

        if (new_user && !temporary && ho_info != 0)
        {
          UserProfileMap::Transaction_var history_profile_trans =
            history_profiles_->get_transaction(user_id, true, op_priority);

          SmartMemBuf_var hist_mem_buf = Algs::copy_membuf(
            history_profile_trans->get_profile());

          UniqueChannelsResult ucr;
          matching.unique_channels(
            base_mem_buf->membuf(),
            hist_mem_buf.in() ? &hist_mem_buf->membuf() : 0,
            *channel_rules,
            ucr);

          ho_info->isp_date = match_time + current_time_offset;
          ho_info->adv_channel_count = ucr.simple_channels;
          ho_info->discover_channel_count = ucr.discover_channels;
        }

        if (match_to_additional)
        {
          if(add_mem_buf->membuf().empty())
          {
            add_profile_trans->remove_profile();
          }
          else
          {
            add_profile_trans->save_profile(
              Generics::transfer_membuf(add_mem_buf),
              match_time);
          }
        }

        /* save profiles */
        if (base_mem_buf->membuf().size() != 0)
        {
          base_profile_trans->save_profile(
            Generics::transfer_membuf(base_mem_buf),
            match_time);
        }
      }

      filter_channel_thresholds_(result_channels);

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream tracing_ostr;
        tracing_ostr << "Match request result: " << std::endl <<
          "  Result channels:";

        for(ChannelMatchMap::const_iterator ch_it = result_channels.begin();
            ch_it != result_channels.end(); ++ch_it)
        {
          tracing_ostr << " " << ch_it->first << "->" << ch_it->second;
        }

        tracing_ostr << std::endl <<
          " Base user profile after matching: " << std::endl;
        matching.print(
          base_mem_buf->membuf().get<unsigned char>(),
          base_mem_buf->membuf().size(), tracing_ostr, true, true);
        tracing_ostr << std::endl <<
          "  Add user profile after matching: " << std::endl;
        matching.print(
          add_mem_buf->membuf().get<unsigned char>(),
          add_mem_buf->membuf().size(), tracing_ostr, true, true);

        logger_->log(tracing_ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_CONTAINER);
      }
    }
    catch(const MaxWaitersReached&)
    {}
    catch(const ChannelsMatcher::InvalidProfileException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ChannelsMatcher::InvalidProfileException"
        " at user '" << request_params.user_id << ": " <<
        ex.what();
      throw Exception(ostr);
    }
    catch(const NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught NotReady: " << ex.what();
      throw NotReady(ostr);
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void UserInfoContainer::delete_old_profiles(
    const Generics::Time& persistent_lifetime)
    /*throw(NotReady, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::delete_old_profiles()";

    try
    {
      SyncPolicy::WriteGuard lock(clear_expired_lock_);
      Generics::Time now = Generics::Time::get_time_of_day();
      Generics::Time persistent_clear_time(now - persistent_lifetime);

      base_profiles_->clear_expired(persistent_clear_time);
      add_profiles_->clear_expired(persistent_clear_time);
      history_profiles_->clear_expired(persistent_clear_time);
      freq_cap_profiles_->clear_expired(persistent_clear_time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw Exception(ostr);
    }
  }

  void UserInfoContainer::delete_old_temporary_profiles(
    const Generics::Time& temp_lifetime)
    /*throw(NotReady, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::delete_old_temporary_profiles()";

    try
    {
      Generics::Time now = Generics::Time::get_time_of_day();
      temp_profiles_->clear_expired(now - temp_lifetime);
      temp_history_profiles_->clear_expired(now - temp_lifetime);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw Exception(ostr);
    }
  }

  UserStat::UserStat() noexcept
    : users_count(0),
      daily_users(0),
      base_area_size(0),
      temp_area_size(0),
      add_area_size(0),
      history_area_size(0),
      freq_cap_area_size(0),
      all_area_size(0),
      allocator_cache_size(0),
      ad_channels_count(0),
      discover_channels_count(0)
  {}

  UserStat
  UserInfoContainer::get_stats() const
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UserInfoContainer::get_stats()";

    try
    {
      UserStat res;
      res.users_count += base_profiles_->size();
      res.users_count += temp_profiles_->size();
      res.base_area_size = base_profiles_->area_size();
      res.temp_area_size =
        temp_profiles_->area_size() + temp_history_profiles_->area_size();
      res.add_area_size = add_profiles_->area_size();
      res.history_area_size = history_profiles_->area_size();
      res.freq_cap_area_size = freq_cap_profiles_->area_size();
      res.all_area_size = res.base_area_size + res.temp_area_size +
        res.add_area_size + res.history_area_size + res.freq_cap_area_size;

      res.allocator_cache_size = MembufAllocator::ALLOCATOR->cached();
      
      {
        SyncPolicy::ReadGuard guard(stat_lock_);
        res.ad_channels_count = ad_channels_count_;
        res.discover_channels_count = discover_channels_count_;
      }

      return res;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  UserInfoContainer::profiles_merged_(
    const RequestMatchParams& request_params,
    Generics::SmartMemBuf* base_profile,
    Generics::SmartMemBuf* add_profile,
    const Generics::Time& now,
    const Generics::Time& timeout)
    /*throw(NotReady, ChannelsMatcher::InvalidProfileException)*/
  {
    bool res = false;

    if (!add_profile->membuf().empty())
    {
      Generics::Time create_time = get_create_time_(add_profile);

      if (now - create_time >= timeout)
      {
        SmartMemBuf_var empty_buf(new SmartMemBuf);
        SmartMemBuf_var base_history_buf(new SmartMemBuf);
        SmartMemBuf_var add_history_buf(new SmartMemBuf);

        ChannelDictionary_var channel_rules = channels_config();

        if (channel_rules.in() == 0)
        {
          throw NotReady("Unable to get channels configuration.");
        }

        Generics::Time current_time_offset = time_offset();

        update_history_(base_profile,
          base_history_buf.in(), now, *channel_rules, current_time_offset);
        update_history_(add_profile,
          add_history_buf.in(), now, *channel_rules, current_time_offset);

        ChannelsMatcher matching(base_profile, empty_buf.in());
        matching.merge(
          base_history_buf.in(),
          add_profile->membuf(),
          add_history_buf->membuf(),
          *channel_rules,
          request_params);

        add_profile->membuf().clear();

        res = true;
      }
    }

    return res;
  }

  void
  UserInfoContainer::update_history_(
    Generics::SmartMemBuf* profile,
    Generics::SmartMemBuf* history_profile,
    const Generics::Time& current_time,
    const ChannelDictionary& channel_rules,
    const Generics::Time& current_time_offset,
    bool update_force)
    /*throw(ChannelsMatcher::InvalidProfileException)*/
  {
    SmartMemBuf_var empty_prof(new SmartMemBuf);

    ChannelsMatcher ucm(profile, empty_prof.in());

    if (update_force || ucm.need_history_optimization(
          current_time, Generics::Time::ZERO, current_time_offset))
    {
      ucm.history_optimize(history_profile,
        current_time, current_time_offset, channel_rules);
    }
  }

  Generics::Time
  UserInfoContainer::get_last_request_(
    Generics::SmartMemBuf* profile)
    /*throw(ChannelsMatcher::InvalidProfileException)*/
  {
    ChannelsMatcher cm(profile, 0);
    return cm.last_request();
  }

  Generics::Time
  UserInfoContainer::get_create_time_(
    Generics::SmartMemBuf* profile)
    /*throw(ChannelsMatcher::InvalidProfileException)*/
  {
    ChannelsMatcher cm(profile, 0);
    return cm.create_time();
  }

  void UserInfoContainer::filter_channel_thresholds_(
    ChannelMatchMap& channels)
    /*throw(NotReady, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::filter_channel_thresholds_()";

    ChannelDictionary_var channel_rules = channels_config();

    if (channel_rules.in() == 0)
    {
      throw NotReady("Unable to get channels configuration.");
    }

    try
    {
      ChannelMatchMap::iterator ch_it = channels.begin();
      const ChannelFeaturesMap& channel_features = channel_rules->channel_features;

      while(ch_it != channels.end())
      {
        ChannelFeaturesMap::const_iterator f_it =
          channel_features.find(ch_it->first);

        if(f_it != channel_features.end() &&
           ch_it->second < f_it->second.threshold)
        {
          channels.erase(ch_it++);
        }
        else
        {
          ++ch_it;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: "<< ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoContainer::consider_publishers_optin(
    const UserId& user_id,
    const std::set<unsigned long>& publisher_account_ids,
    const Generics::Time& now,
    AdServer::ProfilingCommons::OperationPriority op_priority)
    /*throw(ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::consider_publishers_optin()";

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        freq_cap_profiles_->get_transaction(user_id, true, op_priority);
      
      ConstSmartMemBuf_var fc_mem_buf = fc_profile_trans->get_profile();
      
      UserFreqCapProfile profile(fc_mem_buf);

      profile.consider_publishers_optin(
        publisher_account_ids,
        now);

      fc_profile_trans->save_profile(
        profile.transfer_membuf(),
        now);
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {}
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: "<< ex.what();
      throw Exception(ostr);
    }
  }
  
  UserInfoContainer::AllUsersProcessingState_var
  UserInfoContainer::start_all_users_processing(
    const Generics::Time& processing_time,
    unsigned long user_processing_portion,
    bool collect_unique_channels)
    /*throw(NotReady, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::start_all_users_processing()";

    AllUsersProcessingState_var new_state = new AllUsersProcessingState();
    new_state->start_time = Generics::Time::get_time_of_day();
    new_state->end_time = new_state->start_time + processing_time;
    new_state->user_processing_portion = user_processing_portion;
    new_state->collect_unique_channels = collect_unique_channels;

    new_state->start_user_count = 0;
    new_state->processed_user_count = 0;

    new_state->chunk_i = -1;
    new_state->chunk_users_to_process_count = 0;

    new_state->user_channels_count = 0;
    new_state->user_discover_channels_count = 0;
    new_state->users_count = 0;

    try
    {
      SyncPolicy::WriteGuard lock(clear_expired_lock_); // serialize all users processing

      for(UserProfileMap::ChunkList::const_iterator chunk_it =
            base_profiles_->chunks().begin();
          chunk_it != base_profiles_->chunks().end(); ++chunk_it)
      {
        if(chunk_it->in())
        {
          new_state->start_user_count += (*chunk_it)->size();
        }
      }

      return new_state;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw Exception(ostr);
    }
  }

  Generics::Time
  UserInfoContainer::continue_all_users_processing(
    AllUsersProcessingState& state,
    Generics::ActiveObject* interrupter)
    /*throw(NotReady, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::continue_all_users_processing()";

    unsigned long res = 0;

    try
    {
      Generics::Time now = Generics::Time::get_time_of_day();

      ChannelDictionary_var channel_rules = channels_config();

      if(channel_rules.in() == 0)
      {
        // delay processing
        return now + Generics::Time(10);
      }

      unsigned long count_channels = 0;
      unsigned long count_discover_channels = 0;

      {
        SyncPolicy::WriteGuard lock(clear_expired_lock_); // serialize history optimization

        if(state.chunk_i < 0 || state.chunk_users_to_process_count == 0)
        {
          // switch chunk
          state.chunk_i = state.chunk_i < 0 ? 0 : state.chunk_i + 1;

          if(static_cast<unsigned long>(state.chunk_i) >=
               base_profiles_->chunks().size())
          {
            return Generics::Time::ZERO;
          }

          UserProfileMap::ChunkList::const_iterator chunk_it =
            base_profiles_->chunks().begin() + state.chunk_i;
          while(chunk_it != base_profiles_->chunks().end() && !chunk_it->in())
          {
            ++chunk_it;
          }

          if(chunk_it == base_profiles_->chunks().end())
          {
            // processing finished
            if(profile_avg_statistic_)
            {
              SyncPolicy::WriteGuard guard(stat_lock_);
              ad_channels_count_ = count_channels;
              discover_channels_count_ = count_discover_channels;
            }

            return Generics::Time::ZERO;
          }

          state.chunk_i = chunk_it - base_profiles_->chunks().begin();
          state.chunk_users_to_process.clear();
          (*chunk_it)->copy_keys(state.chunk_users_to_process);
          state.chunk_users_to_process_count = state.chunk_users_to_process.size();
        }

        unsigned long user_i = 0;

        UserIdList::iterator it = state.chunk_users_to_process.begin();
        while((!interrupter || interrupter->active()) &&
          it != state.chunk_users_to_process.end() &&
          (state.user_processing_portion < 0 ||
           user_i < static_cast<unsigned long>(state.user_processing_portion)))
        {
          unsigned long channels_count;
          unsigned long discover_channels_count;

          try
          {
            if(process_user_(
                 state.collect_unique_channels ? &channels_count : 0,
                 state.collect_unique_channels ? &discover_channels_count : 0,
                 *it,
                 now))
            {
              // fill processing stats
              state.user_channels_count += channels_count;
              state.user_discover_channels_count += discover_channels_count;
              ++state.users_count;
            }
          }
          catch(const eh::Exception& e)
          {
            logger_->sstream(Logging::Logger::ERROR,
              Aspect::USER_INFO_CONTAINER,
              "ADS-IMPL-80") <<
              FUN << ": Can't process user '"
              << *it << "'. eh::Exception: " << e.what();
          }

          ++it;
          ++user_i;
        }

        state.chunk_users_to_process.erase(
          state.chunk_users_to_process.begin(),
          it);
        state.chunk_users_to_process_count -= user_i;

        // calculate next processing time
        if(state.end_time < now)
        {
          // passing processing
          return now;
        }

        state.processed_user_count += user_i;
        if(state.user_processing_portion < 0 ||
           state.processed_user_count >= state.start_user_count)
        {
          // start_user_count reached, new users appeared, process its
          return now;
        }

        return std::min(
          now + (state.end_time - now) * state.user_processing_portion / (
            state.start_user_count + user_i - state.processed_user_count),
          state.end_time);
      }

      return Generics::Time(res);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  UserInfoContainer::process_user_(
    unsigned long* channels_count,
    unsigned long* discover_channels_count,
    const UserId& user_id,
    const Generics::Time& now)
  {
    try
    {
      UserProfileMap::Transaction_var base_profile_trans =
        base_profiles_->get_transaction(user_id, true, ProfilingCommons::OP_BACKGROUND);
      UserProfileMap::Transaction_var add_profile_trans =
        add_profiles_->get_transaction(user_id, true, ProfilingCommons::OP_BACKGROUND);
      SmartMemBuf_var add_mem_buf = Algs::copy_membuf(
        add_profile_trans->get_profile());

      if(add_mem_buf.in() != 0)
      {
        return false;
      }

      SmartMemBuf_var base_mem_buf = Algs::copy_membuf(
        base_profile_trans->get_profile());

      if(base_mem_buf.in() == 0 || base_mem_buf->membuf().size() == 0)
      {
        return false;
      }

      add_mem_buf = new SmartMemBuf();

      Generics::Time current_time_offset = time_offset();

      ChannelsMatcher user_matcher(base_mem_buf.in(), add_mem_buf.in());

      ChannelDictionary_var channel_rules = channels_config();

      SmartMemBuf_var hist_mem_buf = Algs::copy_membuf(
        history_profiles_->get_profile(user_id));
        // TO CHECK: isn't required if no channels count

      if(user_matcher.need_history_optimization(
           now,
           Generics::Time::ZERO,
           current_time_offset))
      {
        if(hist_mem_buf.in() == 0)
        {
          hist_mem_buf = new SmartMemBuf();
        }

        user_matcher.history_optimize(
          hist_mem_buf.in(),
          now,
          current_time_offset,
          *channel_rules);

        history_profiles_->save_profile(
          user_id,
          Generics::transfer_membuf(hist_mem_buf),
          user_matcher.last_request());

        base_profile_trans->save_profile(
          Generics::transfer_membuf(base_mem_buf),
          user_matcher.last_request());
      }

      if(channels_count || discover_channels_count)
      {
        UniqueChannelsResult unique_channel_counters;
        user_matcher.unique_channels(
          base_mem_buf->membuf(),
          hist_mem_buf.in() ? &hist_mem_buf->membuf() : 0,
          *channel_rules,
          unique_channel_counters);

        if(channels_count)
        {
          *channels_count = unique_channel_counters.simple_channels;
        }

        if(discover_channels_count)
        {
          *discover_channels_count = unique_channel_counters.discover_channels;
        }
      }
    }
    catch(const MaxWaitersReached&)
    {}

    return true;
  }

  template<typename ProfileMapType,
    typename AdapterOptionalType>
  ReferenceCounting::SmartPtr<ProfileMapType>
  UserInfoContainer::open_chunked_map_(
    unsigned long common_chunks_number,
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const char* chunk_prefix,
    const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
    unsigned long max_waiters)
    /*throw(Exception)*/
  {
    static const char* FUN = "open_chunked_map_()";

    try
    {
      return AdServer::ProfilingCommons::ProfileMapFactory::
        open_chunked_map<
          UserId,
          AdServer::ProfilingCommons::UserIdAccessor,
          unsigned long (*)(const Generics::Uuid& uuid),
          AdapterOptionalType>(
            common_chunks_number,
            chunk_folders,
            chunk_prefix,
            user_level_map_traits,
            *this,
            Generics::ActiveObjectCallback_var(
              new Logging::ActiveObjectCallbackImpl(
                logger_,
                "UserInfoContainer",
                "UserInfo",
                "ADS-IMPL-84")),
            AdServer::Commons::uuid_distribution_hash,
            loading_progress_processor_,
            max_waiters);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't open '" << chunk_prefix <<
        "' profiles : " << ex.what();
      throw Exception(ostr);
    }
  }

  template<
    typename ProfileMap,
    typename Key,
    typename KeyAdapter,
    typename AdapterOptional>
  ReferenceCounting::SmartPtr<ProfileMap>
  UserInfoContainer::open_chunked_map_(
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
    const AdapterOptional& optional_adapter)
  {
    try
    {
      return AdServer::ProfilingCommons::ProfileMapFactory::open_chunked_map<
        Key,
        AdServer::ProfilingCommons::UserIdAccessor,
        unsigned long (*)(const Generics::Uuid& uuid),
        KeyAdapter,
        AdapterOptional>(
          *this,
          Generics::ActiveObjectCallback_var(
            new Logging::ActiveObjectCallbackImpl(
              logger_.in(),
              "UserInfoContainer",
              "UserInfo",
              "ADS-IMPL-84")),
          logger_.in(),
          rocksdb_manager_pool,
          common_chunks_number,
          chunk_folders,
          chunk_prefix,
          is_rocksdb_enable,
          rocksdb_params,
          is_level_enable,
          level_map_traits,
          &AdServer::Commons::uuid_distribution_hash,
          max_waiters,
          key_adapter,
          optional_adapter,
          loading_progress_processor_.in());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error stream;
      stream << FNS
             << ": Can't open '"
             << chunk_prefix
             << "' profiles : "
             << ex.what();
      throw Exception(stream);
    }
  }

  void
  UserInfoContainer::trace_match_request_(
    std::ostream& ostr,
    const RequestMatchParams& request_params,
    const ChannelMatchPack& matched_channels,
    const Generics::MemBuf& base_profile,
    const Generics::MemBuf* add_profile)
    noexcept
  {
    ostr << "Matching with params: " << std::endl <<
      "  uid = '" << PrivacyFilter::filter(
        request_params.user_id.to_string().c_str(), "USER_ID") <<
      "'" << std::endl <<
      "  current-time = " << request_params.current_time.get_gm_time() <<
        std::endl <<
      "  silent = " << request_params.silent_match << std::endl <<
      "  colo-id = " << request_params.request_colo_id << std::endl;

    ostr << std::endl << "  Persistent channels: ";
    Algs::print(
      ostr,
      matched_channels.persistent_channels.begin(),
      matched_channels.persistent_channels.end());

    ostr << std::endl << "  Url channels: ";
    Algs::print_fields(
      ostr,
      matched_channels.url_channels,
      &ChannelMatch::channel_id,
      &ChannelMatch::channel_trigger_id);

    ostr << std::endl << "  Page channels: ";
    Algs::print_fields(
      ostr,
      matched_channels.page_channels,
      &ChannelMatch::channel_id,
      &ChannelMatch::channel_trigger_id);

    ostr << std::endl << "  Search channels: ";
    Algs::print_fields(
      ostr,
      matched_channels.search_channels,
      &ChannelMatch::channel_id,
      &ChannelMatch::channel_trigger_id);

    ostr << std::endl << "  Url keyword channels: ";
    Algs::print_fields(
      ostr,
      matched_channels.url_keyword_channels,
      &ChannelMatch::channel_id,
      &ChannelMatch::channel_trigger_id);
    
    ostr << std::endl << "  Base user profile: " << std::endl;
    ChannelsMatcher::print(
      base_profile.get<unsigned char>(),
      base_profile.size(),
      ostr, true, true);

    if(add_profile)
    {
      ostr << std::endl << "  Add user profile: " << std::endl;
      ChannelsMatcher::print(
        add_profile->get<unsigned char>(),
        add_profile->size(),
        ostr, true, true);
    }

    ostr << std::endl;
  }
} /* namespace UserInfoSvcs */
} /* namespace AdServer */

