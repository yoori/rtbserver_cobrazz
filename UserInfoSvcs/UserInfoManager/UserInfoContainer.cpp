#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>

#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Coro/TupleAwaitable.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

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
  constexpr std::uint64_t SLOW_TRANSACTION_THRESHOLD_US = 20'000;
  constexpr std::size_t MAX_SLOW_TRANSACTION_KEYS_TO_LOG = 50;

  struct UserIdRocksDBAccessor
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static unsigned int
    size(const Generics::Uuid&) noexcept
    {
      return Generics::Uuid::size();
    }

    static void
    load(const void* buf, unsigned int size, Generics::Uuid& key)
    {
      const char* buf_ptr = static_cast<const char*>(buf);
      if (size == Generics::Uuid::encoded_size())
      {
        key = Generics::Uuid(String::SubString(buf_ptr, size));
      }
      else
      {
        key = Generics::Uuid(buf_ptr, buf_ptr + size);
      }
    }

    static void
    save(const Generics::Uuid& key, void* buf, unsigned int size)
    {
      if (key.size() > size)
      {
        throw Exception("UserIdRocksDBAccessor::save(): buffer is too small");
      }

      std::memcpy(static_cast<char*>(buf), &*key.begin(), key.size());
    }
  };
}

namespace AdServer::UserInfoSvcs
{
  bool
  UserInfoContainer::SlowTransactionKey::operator<(const SlowTransactionKey& rhs) const noexcept
  {
    if (profile_type != rhs.profile_type)
    {
      return profile_type < rhs.profile_type;
    }

    return user_id < rhs.user_id;
  }

  void
  UserInfoContainer::OperationStateCollector::add(
    ProfileType profile_type,
    const UserId& user_id,
    std::uint64_t wait_us) noexcept
  {
    try
    {
      std::lock_guard lock(lock_);
      auto& state = states_[SlowTransactionKey{profile_type, user_id}];
      ++state.count;
      state.total_wait_us += wait_us;
      state.max_wait_us = std::max(state.max_wait_us, wait_us);
    }
    catch (...)
    {}
  }

  UserInfoContainer::OperationStateCollector::StateMap
  UserInfoContainer::OperationStateCollector::collect() noexcept
  {
    StateMap result;
    try
    {
      std::lock_guard lock(lock_);
      result.swap(states_);
    }
    catch (...)
    {}

    return result;
  }

  AdServer::Commons::StartableAwaitable<UserInfoContainer::UserProfileMap::Transaction_var>
  UserInfoContainer::co_get_transaction_(
    UserProfileMap* profile_map,
    const UserId& user_id,
    bool check_max_waiters,
    AdServer::ProfilingCommons::OperationPriority op_priority)
  {
    const auto start = std::chrono::steady_clock::now();
    const auto collect_slow_transaction = [&]() noexcept {
      const std::uint64_t wait_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
      if (wait_us >= SLOW_TRANSACTION_THRESHOLD_US)
      {
        operation_state_collector_.add(profile_type_(profile_map), user_id, wait_us);
      }
    };

    try
    {
      auto transaction = co_await profile_map->co_get_transaction(
        user_id, check_max_waiters, op_priority);
      collect_slow_transaction();

      co_return transaction;
    }
    catch (...)
    {
      collect_slow_transaction();
      throw;
    }
  }

  UserInfoContainer::ProfileType
  UserInfoContainer::profile_type_(const UserProfileMap* profile_map) const noexcept
  {
    if (profile_map == base_profiles_.in())
    {
      return ProfileType::BASE;
    }

    if (profile_map == temp_profiles_.in())
    {
      return ProfileType::TEMP;
    }

    if (profile_map == add_profiles_.in())
    {
      return ProfileType::ADD;
    }

    if (profile_map == history_profiles_.in())
    {
      return ProfileType::HISTORY;
    }

    if (profile_map == temp_history_profiles_.in())
    {
      return ProfileType::TEMP_HISTORY;
    }

    if (profile_map == freq_cap_profiles_.in())
    {
      return ProfileType::FREQ_CAP;
    }

    return ProfileType::UNKNOWN;
  }

  const char*
  UserInfoContainer::profile_type_name_(ProfileType profile_type) noexcept
  {
    switch (profile_type)
    {
      case ProfileType::BASE:
        return BASE_CHUNK_PREFIX;
      case ProfileType::TEMP:
        return TEMP_CHUNK_PREFIX;
      case ProfileType::ADD:
        return ADD_CHUNK_PREFIX;
      case ProfileType::HISTORY:
        return HISTORY_CHUNK_PREFIX;
      case ProfileType::TEMP_HISTORY:
        return TEMP_HISTORY_CHUNK_PREFIX;
      case ProfileType::FREQ_CAP:
        return FREQCAP_CHUNK_PREFIX;
      case ProfileType::UNKNOWN:
        return "Unknown";
    }

    return "Unknown";
  }

  void
  UserInfoContainer::flush_slow_transactions() noexcept
  {
    try
    {
      auto states = operation_state_collector_.collect();
      if (states.empty())
      {
        return;
      }

      using StateMap = OperationStateCollector::StateMap;
      std::vector<const StateMap::value_type*> ordered_states;
      ordered_states.reserve(states.size());

      std::uint64_t total_count = 0;
      for (const auto& state : states)
      {
        ordered_states.push_back(&state);
        total_count += state.second.count;
      }

      std::sort(
        ordered_states.begin(),
        ordered_states.end(),
        [](const auto* lhs, const auto* rhs)
        {
          if (lhs->second.max_wait_us != rhs->second.max_wait_us)
          {
            return lhs->second.max_wait_us > rhs->second.max_wait_us;
          }

          return lhs->second.total_wait_us > rhs->second.total_wait_us;
        });

      const auto keys_to_log = std::min(ordered_states.size(), MAX_SLOW_TRANSACTION_KEYS_TO_LOG);

      Stream::Error ostr;
      ostr << "Slow co_get_transaction waits: threshold_us=" <<
        SLOW_TRANSACTION_THRESHOLD_US <<
        ", events=" << total_count <<
        ", keys=" << ordered_states.size() << ", shown_keys=" << keys_to_log;

      for (std::size_t i = 0; i < keys_to_log; ++i)
      {
        const auto& [key, state] = *ordered_states[i];
        ostr << "\n  profile=" << profile_type_name_(key.profile_type) <<
          ", uid=" << key.user_id <<
          ", count=" << state.count <<
          ", total_wait_us=" << state.total_wait_us <<
          ", avg_wait_us=" << state.total_wait_us / state.count <<
          ", max_wait_us=" << state.max_wait_us;
      }

      logger_->log(ostr.str(), Logging::Logger::NOTICE, Aspect::USER_INFO_CONTAINER);
    }
    catch (...)
    {}
  }

  /* UserInfoContainer */
  UserInfoContainer::UserInfoContainer(
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
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id),
      profile_request_timeout_(profile_request_timeout),
      rocksdb_processor_(std::make_shared<
        AdServer::ProfilingCommons::RocksDBProfileMapProcessor>(rocksdb_batching_threads)),
      time_offset_(Generics::Time::ZERO),
      profile_avg_statistic_(avg_statistic),
      ad_channels_count_(0),
      discover_channels_count_(0),
      history_optimization_period_(history_optimization_period),
      session_timeout_(session_timeout),
      use_add_profile_on_match_(use_add_profile_on_match),
      base_profile_expire_time_(base_level_map_traits.expire_time)
  {
    add_child_object(rocksdb_processor_);

    base_profiles_ = open_chunked_map_<UserProfileMap>(
      common_chunks_number,
      chunk_folders,
      BASE_CHUNK_PREFIX,
      AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
        base_level_map_traits.expire_time),
      max_base_profile_waiters,
      rocksdb_batching_threads);

    temp_profiles_ = open_chunked_map_<UserProfileMap>(
      common_chunks_number,
      chunk_folders,
      TEMP_CHUNK_PREFIX,
      AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
        temp_level_map_traits.expire_time),
      max_temp_profile_waiters,
      rocksdb_batching_threads);

    add_profiles_ = open_chunked_map_<UserProfileMap>(
      common_chunks_number,
      chunk_folders,
      ADD_CHUNK_PREFIX,
      AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
        add_level_map_traits.expire_time),
      0,
      rocksdb_batching_threads);

    history_profiles_ = open_chunked_map_<UserProfileMap>(
      common_chunks_number,
      chunk_folders,
      HISTORY_CHUNK_PREFIX,
      AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
        history_level_map_traits.expire_time),
      0,
      rocksdb_batching_threads);

    temp_history_profiles_ = open_chunked_map_<UserProfileMap>(
      common_chunks_number,
      chunk_folders,
      TEMP_HISTORY_CHUNK_PREFIX,
      AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
        temp_level_map_traits.expire_time),
      0,
      rocksdb_batching_threads);

    freq_cap_profiles_ = open_chunked_map_<UserProfileMap>(
      common_chunks_number,
      chunk_folders,
      FREQCAP_CHUNK_PREFIX,
      AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
        freq_cap_level_map_traits.expire_time),
      max_freqcap_profile_waiters,
      rocksdb_batching_threads);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_pre_bid_process(
    UserFreqCapProfile::FreqCapIdArray& freq_caps,
    UserFreqCapProfile::FreqCapIdArray& virtual_freq_caps,
    UserFreqCapProfile::SeqOrderArray& seq_orders,
    UserFreqCapProfile::CampaignFreqs& campaign_freqs,
    std::vector<unsigned long>& optin_publishers,
    const UserId& user_id,
    const Generics::Time& now,
    bool fill_full_freq_caps,
    const Generics::Time& publishers_optin_timeout)
    /*throw(ChunkNotFound, UserIsFraud, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::co_pre_bid_process()";

    FreqCapConfig_var freq_cap_config = get_freq_cap_config_();

    try
    {
      if (!fill_full_freq_caps && publishers_optin_timeout == Generics::Time::ZERO)
      {
        co_return true;
      }

      UserProfileMap::Transaction_var fc_profile_trans =
        co_await co_get_transaction_(freq_cap_profiles_.in(),
          user_id,
          true, // check max waiters
          ProfilingCommons::OP_RUNTIME);
      SmartMemBuf_var fc_mem_buf = co_await fc_profile_trans->co_get_own_profile();
      UserFreqCapProfile profile(fc_mem_buf, true);

      bool profile_changed = false;

      if (fill_full_freq_caps)
      {
        profile_changed = profile.full(
          freq_caps,
          &virtual_freq_caps,
          seq_orders,
          campaign_freqs,
          now,
          *freq_cap_config);
      }

      if (publishers_optin_timeout != Generics::Time::ZERO)
      {
        profile.get_optin_publishers(optin_publishers, publishers_optin_timeout);
      }

      if (profile_changed)
      {
        fc_profile_trans->save_profile_async(profile.transfer_membuf(), now);
      }

      co_return true;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const MaxWaitersReached& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught MaxWaitersReached: " << ex.what();
      throw ResourceExhausted(ostr);
    }
    catch(const UserFreqCapProfile::Invalid& ex)
    {
      if (fill_full_freq_caps)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught UserFreqCapProfile::Invalid: " << ex.what();
        throw UserIsFraud(ostr);
      }

      co_return true;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_update_freq_caps(
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
  {
    static const char* FUN = "UserInfoContainer::co_update_freq_caps()";

    FreqCapConfig_var freq_cap_config = get_freq_cap_config_();

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        co_await co_get_transaction_(freq_cap_profiles_.in(), user_id, true, op_priority);

      SmartMemBuf_var fc_mem_buf = co_await fc_profile_trans->co_get_own_profile();
      UserFreqCapProfile profile(fc_mem_buf, true);
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

      fc_profile_trans->save_profile_async(profile.transfer_membuf(), now);

      co_return true;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {
      co_return false;
    }
    catch (const MaxWaitersReached&)
    {
      co_return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_confirm_freq_caps(
    const UserId& user_id,
    const Generics::Time& now,
    const Commons::RequestId& request_id,
    const std::set<unsigned long>& exclude_pubpixel_accounts)
  {
    static const char* FUN = "UserInfoContainer::co_confirm_freq_caps()";

    FreqCapConfig_var freq_cap_config = get_freq_cap_config_();

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        co_await co_get_transaction_(freq_cap_profiles_.in(), user_id, false);

      SmartMemBuf_var fc_mem_buf = co_await fc_profile_trans->co_get_own_profile();
      UserFreqCapProfile profile(fc_mem_buf, true);
      bool res = profile.consider_publishers_optin(exclude_pubpixel_accounts, now);
      res |= profile.confirm_request(request_id, now, *freq_cap_config);

      if (res)
      {
        fc_profile_trans->save_profile_async(profile.transfer_membuf(), now);
      }

      co_return true;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {
      co_return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_get_user_profile(
    const UserId& user_id,
    bool temporary,
    SmartMemBuf_var* mb_base_profile_out,
    SmartMemBuf_var* mb_add_profile_out,
    SmartMemBuf_var* mb_history_profile_out,
    SmartMemBuf_var* mb_fc_profile_out)
  {
    static const char* FUN = "UserInfoContainer::co_get_user_profile()";

    try
    {
      if (mb_base_profile_out)
      {
        if (!temporary)
        {
          *mb_base_profile_out = co_await base_profiles_->co_get_own_profile(user_id);
        }
        else
        {
          *mb_base_profile_out = co_await temp_profiles_->co_get_own_profile(user_id);
        }
      }

      if (mb_add_profile_out)
      {
        *mb_add_profile_out = co_await add_profiles_->co_get_own_profile(user_id);
      }

      if (mb_history_profile_out)
      {
        if (!temporary)
        {
          *mb_history_profile_out = co_await history_profiles_->co_get_own_profile(user_id);
        }
        else
        {
          *mb_history_profile_out = co_await temp_history_profiles_->co_get_own_profile(user_id);
        }
      }

      if (mb_fc_profile_out)
      {
        SmartMemBuf_var mb = co_await freq_cap_profiles_->co_get_own_profile(user_id);

        if (mb && mb->membuf().size() <= 40 * 1024 * 1024)
        {
          *mb_fc_profile_out = mb;
        }
      }

      co_return true;
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

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_remove_user_profile(const UserId& user_id)
  {
    static const char* FUN = "UserInfoContainer::co_remove_user_profile()";

    try
    {
      auto remove_profile = [&user_id](
        UserProfileMap* profiles) -> AdServer::Commons::StartableAwaitable<bool>
      {
        co_await profiles->co_remove_profile(user_id);
        co_return true;
      };

      co_await AdServer::Commons::TupleAwaitable(
        remove_profile(base_profiles_.in()),
        remove_profile(temp_profiles_.in()),
        remove_profile(add_profiles_.in()),
        remove_profile(history_profiles_.in()),
        remove_profile(temp_history_profiles_.in()),
        remove_profile(freq_cap_profiles_.in()));

      co_return true;
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

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_exchange_merge(
    const UserId& user_id,
    const Generics::MemBuf& other_base_profile_buf,
    const Generics::MemBuf& other_history_profile_buf,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
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
      UserProfileMap::Transaction_var base_profile_trans = co_await co_get_transaction_(
        base_profiles_.in(),
        user_id,
        true, // check max waiters
        AdServer::ProfilingCommons::OP_BACKGROUND);

      UserProfileMap::Transaction_var add_profile_trans = co_await co_get_transaction_(
        add_profiles_.in(),
        user_id,
        true, // check max waiters
        AdServer::ProfilingCommons::OP_BACKGROUND);

      SmartMemBuf_var add_buf = co_await add_profile_trans->co_get_own_profile();

      if (add_buf.in() && !other_base_profile_buf.empty())
      {
        try
        {
          Generics::SmartMemBuf_var base_profile(new Generics::SmartMemBuf(other_base_profile_buf));
          if (other_base_profile_buf.size() != 0)
          {
            BaseProfileAdapter base_adapter;
            base_profile = base_adapter(
              base_profile.in(),
              true // ignore_future_versions
              );
          }

          Generics::SmartMemBuf_var history_profile(
            new Generics::SmartMemBuf(other_history_profile_buf));
          if (other_history_profile_buf.size() != 0)
          {
            HistoryProfileAdapter history_adapter;
            history_profile = history_adapter(history_profile.in());
          }

          const Generics::Time now(
            std::max(get_last_request_(base_profile.in()), get_last_request_(add_buf.in())));

          update_history_(
            base_profile.in(),
            history_profile.in(),
            now,
            *channel_rules,
            current_time_offset);

          SmartMemBuf_var empty_buf(new SmartMemBuf);

          ChannelsMatcher matching(add_buf.in(), empty_buf.in());

          ProfileMatchParams profile_match_params;
          matching.merge(
            0, // skip local history profile, replace it with other history profile
            base_profile->membuf(),
            history_profile->membuf(),
            *channel_rules,
            profile_match_params);

          co_await add_profile_trans->co_remove_profile();

          if (add_buf->membuf().size() != 0)
          {
            base_profile_trans->save_profile_async(
              Generics::transfer_membuf(Algs::copy_membuf(add_buf)),
              now);
          }

          if (history_profile->membuf().size() != 0)
          {
            history_profiles_->save_profile_async(
              user_id,
              Generics::transfer_membuf(Algs::copy_membuf(history_profile)),
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

      co_return true;
    }
    catch(const MaxWaitersReached&)
    {
      co_return false;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_merge(
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
    static const char* FUN = "UserInfoContainer::co_merge()";

    try
    {
      ChannelDictionary_var channel_rules = channels_config();

      if (channel_rules.in() == 0)
      {
        throw NotReady("Unable to get channels configuration.");
      }

      FreqCapConfig_var freq_cap_config = get_freq_cap_config_();

      Generics::Time current_time_offset = time_offset();

      const UserId& user_id = request_params.user_id;
      bool temporary = request_params.temporary;
      bool merge_to_additional = false;
      UserProfileMap::Transaction_var target_profile_trans;
      UserProfileMap::Transaction_var target_history_profile_trans;
      SmartMemBuf_var target_profile;
      SmartMemBuf_var target_history_profile;

      bool new_user = !temporary && request_params.change_last_request && !request_params.household;

      try
      {
        if (temporary)
        {
          target_profile_trans = co_await co_get_transaction_(temp_profiles_.in(),
            user_id, true, op_priority);
        }
        else
        {
          target_profile_trans = co_await co_get_transaction_(base_profiles_.in(),
            user_id, true, op_priority);
        }

        UserProfileMap::Transaction_var add_profile_trans =
          co_await co_get_transaction_(add_profiles_.in(), user_id, true, op_priority);

        target_profile = co_await add_profile_trans->co_get_own_profile();

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
          target_profile = co_await target_profile_trans->co_get_own_profile();
        }
      }
      catch(const MaxWaitersReached&)
      {
        co_return false;
      }

      Generics::SmartMemBuf_var merge_base_profile(
        new Generics::SmartMemBuf(merge_base_profile_buf));
      if (merge_base_profile_buf.size() != 0)
      {
        BaseProfileAdapter base_adapter;
        merge_base_profile = base_adapter(merge_base_profile.in());
      }

      Generics::SmartMemBuf_var merge_history_profile(
        new Generics::SmartMemBuf(merge_history_profile_buf));
      if (merge_history_profile_buf.size() != 0)
      {
        HistoryProfileAdapter history_adapter;
        merge_history_profile = history_adapter(merge_history_profile.in());
      }

      Generics::ConstSmartMemBuf_var merge_freq_cap_profile;

      if (!merge_freq_cap_profile_buf.empty())
      {
        Generics::SmartMemBuf_var v(new SmartMemBuf(merge_freq_cap_profile_buf));
        UserFreqCapProfileAdapter freq_cap_adapter;
        Generics::SmartMemBuf_var adapted_freq_cap_profile = freq_cap_adapter(v.in());
        merge_freq_cap_profile = Generics::transfer_membuf(adapted_freq_cap_profile);
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

      if (temporary)
      {
        target_history_profile_trans =
          co_await co_get_transaction_(temp_history_profiles_.in(), user_id, true, op_priority);
      }
      else
      {
        target_history_profile_trans =
          co_await co_get_transaction_(history_profiles_.in(), user_id, true, op_priority);
      }

      target_history_profile = co_await target_history_profile_trans->co_get_own_profile();

      if (target_history_profile.in() == 0)
      {
        target_history_profile = new SmartMemBuf;
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
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

        tracing_ostr << std::endl << "History profile before merging: " << std::endl;

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

        tracing_ostr << std::endl << "Base profile for merge user: " << std::endl;

        ChannelsMatcher::print(
          merge_base_profile_buf.get<unsigned char>(),
          merge_base_profile_buf.size(),
          tracing_ostr,
          true,
          true);

        tracing_ostr << std::endl << "Additional profile for merge user: " << std::endl;

        ChannelsMatcher::print(
          merge_add_profile_buf.get<unsigned char>(),
          merge_add_profile_buf.size(),
          tracing_ostr,
          true,
          true);

        tracing_ostr << std::endl << "History profile for merge user: " << std::endl;

        ChannelsMatcher::history_print(
          merge_history_profile_buf.get<unsigned char>(),
          merge_history_profile_buf.size(),
          tracing_ostr,
          true);

        tracing_ostr << std::endl;

        logger_->log(tracing_ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
      }

      SmartMemBuf_var add_mb(new SmartMemBuf);

      ChannelsMatcher user_profile_adapter(target_profile.in(), add_mb.in());

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

      if (merge_add_profile_buf.size() > 0)
      {
        SmartMemBuf_var empty_hp(new SmartMemBuf);

        Generics::SmartMemBuf_var merge_add_profile(
          new Generics::SmartMemBuf(merge_add_profile_buf));

        BaseProfileAdapter add_adapter;
        merge_add_profile = add_adapter(merge_add_profile.in());

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
        co_await co_get_transaction_(freq_cap_profiles_.in(), user_id, true, op_priority);

      SmartMemBuf_var target_freq_cap_profile =
        co_await target_freq_cap_profile_trans->co_get_own_profile();
      ConstSmartMemBuf_var result_freq_cap_profile;

      if (merge_freq_cap_profile)
      {
        try
        {
          UserFreqCapProfile freq_cap_profile(target_freq_cap_profile, true);
          freq_cap_profile.merge(merge_freq_cap_profile, merge_time, *freq_cap_config);
          result_freq_cap_profile = freq_cap_profile.transfer_membuf();
        }
        catch(const UserFreqCapProfile::Invalid&)
        {}
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
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

        logger_->log(tracing_ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
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
        target_history_profile_trans->save_profile_async(
          Generics::transfer_membuf(target_history_profile),
          merge_time);
      }

      if (target_profile->membuf().size() != 0)
      {
        target_profile_trans->save_profile_async(
          Generics::transfer_membuf(target_profile),
          merge_time);
      }

      if (result_freq_cap_profile)
      {
        target_freq_cap_profile_trans->save_profile_async(result_freq_cap_profile, merge_time);
      }

      co_return true;
    }
    catch(const MaxWaitersReached&)
    {
      co_return false;
    }
    catch(const ChannelsMatcher::InvalidProfileException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ChannelsMatcher::InvalidProfileException"
        " at user '" << request_params.user_id.to_string() << ": " << ex.what();
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

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_fraud_user(const UserId& user_id, const Generics::Time& now)
  {
    static const char* FUN = "UserInfoContainer::co_fraud_user()";

    try
    {
      UserProfileMap::Transaction_var base_profile_trans = co_await co_get_transaction_(
        base_profiles_.in(),
        user_id,
        true,
        AdServer::ProfilingCommons::OP_RUNTIME);
      SmartMemBuf_var base_mem_buf = co_await base_profile_trans->co_get_own_profile();

      if (base_mem_buf.in() == 0)
      {
        base_mem_buf = new SmartMemBuf;
      }

      SmartMemBuf_var add_mem_buf(new SmartMemBuf);
      ChannelsMatcher cm(base_mem_buf.in(), add_mem_buf.in());

      if (cm.fraud_user(now))
      {
        base_profile_trans->save_profile_async(
          Generics::transfer_membuf(base_mem_buf),
          cm.last_request());
      }

      co_return true;
    }
    catch(const MaxWaitersReached&)
    {
      co_return false;
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

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_match(
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
    UniqueChannelsResult* unique_channels_result)
    /*throw(NotReady, ChunkNotFound, Exception)*/
  {
    static const char* FUN = "UserInfoContainer::co_match()";

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

      UserProfileMap* base_profiles = temporary ? temp_profiles_.in() : base_profiles_.in();

      UserProfileMap::Transaction_var base_profile_trans;
      UserProfileMap::Transaction_var add_profile_trans;
      SmartMemBuf_var base_mem_buf;
      SmartMemBuf_var add_mem_buf;

      if (use_add_profile_on_match_)
      {
        auto [loaded_base_profile_trans, loaded_add_profile_trans] =
          co_await AdServer::Commons::TupleAwaitable(
            co_get_transaction_(base_profiles, user_id, true, op_priority),
            co_get_transaction_(add_profiles_.in(), user_id, true, op_priority));

        base_profile_trans = std::move(loaded_base_profile_trans);
        add_profile_trans = std::move(loaded_add_profile_trans);

        auto [loaded_base_mem_buf, loaded_add_mem_buf] =
          co_await AdServer::Commons::TupleAwaitable(
            base_profile_trans->co_get_own_profile(),
            add_profile_trans->co_get_own_profile());

        base_mem_buf = std::move(loaded_base_mem_buf);
        add_mem_buf = std::move(loaded_add_mem_buf);
      }
      else
      {
        base_profile_trans =
          co_await co_get_transaction_(base_profiles, user_id, true, op_priority);
        base_mem_buf = co_await base_profile_trans->co_get_own_profile();
      }

      bool match_to_additional =
        use_add_profile_on_match_ &&
        !request_params.use_empty_profile &&
        ((last_colo_id != current_placement_colo_id && last_colo_id != DEFAULT_COLO_ID) ||
         add_mem_buf.in());

      if (add_mem_buf.in() == 0)
      {
        add_mem_buf = new SmartMemBuf;

        if (match_to_additional && logger_->log_level() >= Logging::Logger::TRACE)
        {
          std::ostringstream ostr;
          ostr << "Additional profile for user with uid = " << user_id <<
            "was created with create_time " << match_time.get_gm_time();

          logger_->log(ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
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

          co_await add_profile_trans->co_remove_profile();

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

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream tracing_ostr;
        tracing_ostr << "Input match request: " << std::endl;

        trace_match_request_(
          tracing_ostr,
          request_params,
          matched_channels,
          base_mem_buf->membuf(),
          &add_mem_buf->membuf());

        logger_->log(tracing_ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
      }

      if (!request_params.use_empty_profile)
      {
        if (request_params.request_colo_id != -1)
        {
          user_app.last_request = matching.last_request();
        }

        bool need_history_optimize = matching.need_history_optimization(
          match_time,
          history_optimization_period_,
          current_time_offset);

        new_user =
          !match_to_additional &&
          request_params.change_last_request &&
          !request_params.household &&
          (new_user || matching.need_channel_count_stats_logging(match_time, current_time_offset));

        if (need_history_optimize)
        {
          /* first request today : history optimization */
          UserProfileMap::Transaction_var history_profile_trans;
          if (temporary)
          {
            history_profile_trans = co_await co_get_transaction_(temp_history_profiles_.in(),
              user_id, true, op_priority);
          }
          else
          {
            history_profile_trans = co_await co_get_transaction_(history_profiles_.in(),
              user_id, true, op_priority);
          }

          SmartMemBuf_var hist_mem_buf = co_await history_profile_trans->co_get_own_profile();

          if (hist_mem_buf.in() == 0)
          {
            hist_mem_buf = new SmartMemBuf;
          }

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            std::ostringstream ostr;
            ostr << "To history optimize user '" << user_id << "':" <<
              std::endl << "  History Profile:" << std::endl;

            matching.history_print(
              hist_mem_buf->membuf().get<unsigned char>(),
              hist_mem_buf->membuf().size(),
              ostr,
              true);

            logger_->log(ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
          }

          bool first_today_history_optimization;

          matching.history_optimize(
            hist_mem_buf.in(),
            match_time,
            current_time_offset,
            *channel_rules,
            &first_today_history_optimization);

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            std::ostringstream ostr;
            ostr << "From history optimize user '" << user_id << "':" <<
              std::endl << "  History Profile:" << std::endl;

            matching.history_print(
              hist_mem_buf->membuf().get<unsigned char>(),
              hist_mem_buf->membuf().size(), ostr, true);

            logger_->log(ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
          }

          if (!request_params.silent_match)
          {
            history_profile_trans->save_profile_async(
              Generics::transfer_membuf(hist_mem_buf),
              match_time);
          }
        }
      }

      matching.match(
        result_channels,
        match_time,
        matched_channels,
        *channel_rules,
        //request_params.request_colo_id,
        request_params,
        properties,
        session_timeout_);

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
          co_await co_get_transaction_(history_profiles_.in(), user_id, true, op_priority);

        SmartMemBuf_var hist_mem_buf = co_await history_profile_trans->co_get_own_profile();

        UniqueChannelsResult ucr;
        matching.unique_channels(
          base_mem_buf->membuf(),
          hist_mem_buf.in() ? &hist_mem_buf->membuf() : 0,
          *channel_rules,
          *unique_channels_result);
      }

      if (!request_params.use_empty_profile && !request_params.silent_match)
      {
        user_app.session_start = matching.session_start();

        if (new_user && !temporary && ho_info != 0)
        {
          UserProfileMap::Transaction_var history_profile_trans =
            co_await co_get_transaction_(history_profiles_.in(), user_id, true, op_priority);

          SmartMemBuf_var hist_mem_buf = co_await history_profile_trans->co_get_own_profile();

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
          if (add_mem_buf->membuf().empty())
          {
            co_await add_profile_trans->co_remove_profile();
          }
          else
          {
            add_profile_trans->save_profile_async(
              Generics::transfer_membuf(add_mem_buf),
              match_time);
          }
        }

        /* save profiles */
        if (base_mem_buf->membuf().size() != 0)
        {
          base_profile_trans->save_profile_async(
            Generics::transfer_membuf(base_mem_buf),
            match_time);
        }
      }

      filter_channel_thresholds_(result_channels);

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream tracing_ostr;
        tracing_ostr << "Match request result: " << std::endl << "  Result channels:";

        for (ChannelMatchMap::const_iterator ch_it = result_channels.begin();
            ch_it != result_channels.end(); ++ch_it)
        {
          tracing_ostr << " " << ch_it->first << "->" << ch_it->second;
        }

        tracing_ostr << std::endl << " Base user profile after matching: " << std::endl;
        matching.print(
          base_mem_buf->membuf().get<unsigned char>(),
          base_mem_buf->membuf().size(), tracing_ostr, true, true);
        tracing_ostr << std::endl << "  Add user profile after matching: " << std::endl;
        matching.print(
          add_mem_buf->membuf().get<unsigned char>(),
          add_mem_buf->membuf().size(), tracing_ostr, true, true);

        logger_->log(tracing_ostr.str(), Logging::Logger::TRACE, Aspect::USER_INFO_CONTAINER);
      }

      co_return true;
    }
    catch(const MaxWaitersReached& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught MaxWaitersReached: " << ex.what();
      throw ResourceExhausted(ostr);
    }
    catch(const ChannelsMatcher::InvalidProfileException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ChannelsMatcher::InvalidProfileException"
        " at user '" << request_params.user_id << ": " << ex.what();
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

  void UserInfoContainer::delete_old_profiles(const Generics::Time& persistent_lifetime)
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

  void UserInfoContainer::delete_old_temporary_profiles(const Generics::Time& temp_lifetime)
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
      res.temp_area_size = temp_profiles_->area_size() + temp_history_profiles_->area_size();
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

  AdServer::ProfilingCommons::RocksDBProfileMapProcessor::Stats
  UserInfoContainer::rocksdb_stats() const noexcept
  {
    return rocksdb_processor_->stats();
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
      ucm.history_optimize(history_profile, current_time, current_time_offset, channel_rules);
    }
  }

  Generics::Time
  UserInfoContainer::get_last_request_(Generics::SmartMemBuf* profile)
    /*throw(ChannelsMatcher::InvalidProfileException)*/
  {
    ChannelsMatcher cm(profile, 0);
    return cm.last_request();
  }

  Generics::Time
  UserInfoContainer::get_create_time_(Generics::SmartMemBuf* profile)
    /*throw(ChannelsMatcher::InvalidProfileException)*/
  {
    ChannelsMatcher cm(profile, 0);
    return cm.create_time();
  }

  void UserInfoContainer::filter_channel_thresholds_(ChannelMatchMap& channels)
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

      while (ch_it != channels.end())
      {
        auto f_it = channel_features.find(ch_it->first);

        if (f_it != channel_features.end() && ch_it->second < f_it->second.threshold)
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

  AdServer::Commons::StartableAwaitable<bool>
  UserInfoContainer::co_consider_publishers_optin(
    const UserId& user_id,
    const std::set<unsigned long>& publisher_account_ids,
    const Generics::Time& now,
    AdServer::ProfilingCommons::OperationPriority op_priority)
  {
    static const char* FUN = "UserInfoContainer::co_consider_publishers_optin()";

    try
    {
      UserProfileMap::Transaction_var fc_profile_trans =
        co_await co_get_transaction_(freq_cap_profiles_.in(), user_id, true, op_priority);

      SmartMemBuf_var fc_mem_buf = co_await fc_profile_trans->co_get_own_profile();
      UserFreqCapProfile profile(fc_mem_buf, true);
      profile.consider_publishers_optin(publisher_account_ids, now);
      fc_profile_trans->save_profile_async(profile.transfer_membuf(), now);

      co_return true;
    }
    catch(const UserProfileMap::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserProfileMap::ChunkNotFound: " << ex.what();
      throw ChunkNotFound(ostr);
    }
    catch(const UserFreqCapProfile::Invalid&)
    {
      co_return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: "<< ex.what();
      throw Exception(ostr);
    }
  }

  template<typename ProfileMapType>
  ReferenceCounting::SmartPtr<ProfileMapType>
  UserInfoContainer::open_chunked_map_(
    unsigned long common_chunks_number,
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const char* chunk_prefix,
    const AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits& profile_map_traits,
    unsigned long max_waiters,
    unsigned long rocksdb_batching_threads)
    /*throw(Exception)*/
  {
    static const char* FUN = "open_chunked_map_()";

    try
    {
      auto profile_map =
        AdServer::ProfilingCommons::ProfileMapFactory::
        open_rocksdb_chunked_map<
          UserId,
          UserIdRocksDBAccessor,
          unsigned long (*)(const Generics::Uuid& uuid)>(
            common_chunks_number,
            chunk_folders,
            chunk_prefix,
            profile_map_traits,
            AdServer::Commons::uuid_distribution_hash,
            max_waiters,
            true,
            ".rocksdb",
            rocksdb_batching_threads,
            rocksdb_processor_);
      add_child_object(profile_map.second);
      return profile_map.first;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't open '" << chunk_prefix << "' profiles : " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoContainer::trace_match_request_(
    std::ostream& ostr,
    const RequestMatchParams& request_params,
    const ChannelIdPack& matched_channels,
    const Generics::MemBuf& base_profile,
    const Generics::MemBuf* add_profile)
    noexcept
  {
    ostr << "Matching with params: " << std::endl <<
      "  uid = '" << PrivacyFilter::filter(request_params.user_id.to_string().c_str(), "USER_ID") <<
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
    Algs::print(ostr, matched_channels.url_channels.begin(), matched_channels.url_channels.end());

    ostr << std::endl << "  Page channels: ";
    Algs::print(ostr, matched_channels.page_channels.begin(), matched_channels.page_channels.end());

    ostr << std::endl << "  Search channels: ";
    Algs::print(
      ostr,
      matched_channels.search_channels.begin(),
      matched_channels.search_channels.end());

    ostr << std::endl << "  Url keyword channels: ";
    Algs::print(
      ostr,
      matched_channels.url_keyword_channels.begin(),
      matched_channels.url_keyword_channels.end());

    ostr << std::endl << "  Base user profile: " << std::endl;
    ChannelsMatcher::print(
      base_profile.get<unsigned char>(),
      base_profile.size(),
      ostr, true, true);

    if (add_profile)
    {
      ostr << std::endl << "  Add user profile: " << std::endl;
      ChannelsMatcher::print(
        add_profile->get<unsigned char>(),
        add_profile->size(),
        ostr, true, true);
    }

    ostr << std::endl;
  }
} /* namespace AdServer::UserInfoSvcs */
