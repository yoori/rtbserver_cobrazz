// BOOST
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// THREAD
#include <thread>

#include <Generics/BoundedMap.hpp>
#include "ChannelMatcher.hpp"

namespace Aspect
{
  const char CHANNEL_MATCHER[] = "ChannelMatcher";
  const char ROCKSDB_CACHE[] = "RocksdbCache";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    template<typename ArrayType>
    struct ArrayHelper
    {
      template<typename SourceContainerType>
      static
      ArrayType
      init_array(const SourceContainerType& source)
      {
        ArrayType result(source.size());
        std::copy(source.begin(), source.end(), result.begin());
        return result;
      }
    };
  };

  struct CalculateChannelMatcher::MatchKeyHolder: public ReferenceCounting::AtomicImpl
  {
    typedef std::vector<unsigned long> ChannelIdArray;

    MatchKeyHolder(const CampaignSvcs::ChannelIdSet& history_channels_val)
      noexcept
      : history_channels(
          ArrayHelper<ChannelIdArray>::init_array(history_channels_val))
    {}

    const ChannelIdArray history_channels;

  protected:
    virtual
    ~MatchKeyHolder() noexcept = default;
  };

  class CalculateChannelMatcher::MatchCache: public ReferenceCounting::AtomicImpl
  {
  protected:
    class MatchSizePolicy;

  public:
    typedef ReferenceCounting::SmartPtr<MatchKeyHolder>
      MatchKeyHolder_var;

    struct MatchKeyHashAdapter
    {
      friend class MatchSizePolicy;

    public:
      MatchKeyHashAdapter()
      {}

      MatchKeyHashAdapter(MatchKeyHolder* match_key_holder)
        : holder_(ReferenceCounting::add_ref(match_key_holder))
      {
        Generics::Murmur32v3Hasher hasher(0);

        for(MatchKeyHolder::ChannelIdArray::const_iterator ch_it =
              holder_->history_channels.begin();
            ch_it != holder_->history_channels.end(); ++ch_it)
        {
          hasher.add(&*ch_it, sizeof(*ch_it));
        }

        hash_ = hasher.finalize();
      }

      size_t
      hash() const noexcept
      {
        return hash_;
      }

      bool
      operator==(const MatchKeyHashAdapter& right)
        const noexcept
      {
        return hash_ == right.hash_ &&
          holder_->history_channels.size() ==
            right.holder_->history_channels.size() &&
          std::equal(holder_->history_channels.begin(),
            holder_->history_channels.end(),
            right.holder_->history_channels.begin());
      }

    protected:
      MatchKeyHolder_var holder_;
      size_t hash_;
    };

  public:
    MatchCache(
      unsigned long size_limit)
      noexcept
      : cache_map_(size_limit, Generics::Time::ZERO, MatchSizePolicy())
    {}

    MatchResult_var
    get(
      const MatchKeyHashAdapter& key,
      const Generics::Time& /*now*/
      ) noexcept
    {
      CacheMap::const_iterator it = cache_map_.find(key);
      if(it != cache_map_.end())
      {
        return it->second->match_result;
      }

      return MatchResult_var();
    }

    void
    insert(
      const MatchKeyHashAdapter& key,
      MatchResult* match_result,
      const Generics::Time& now)
      noexcept
    {
      CacheMap::value_type ins(
        key,
        new MatchResultHolder(now, match_result));
      cache_map_.insert(std::move(ins));
    }

  protected:
    class MatchResultHolder: public ReferenceCounting::AtomicImpl
    {
    public:
      MatchResultHolder(
        const Generics::Time& actual_time_val,
        MatchResult* match_result_val)
        : actual_time(actual_time_val),
          match_result(ReferenceCounting::add_ref(match_result_val))
      {}

      const Generics::Time actual_time;
      MatchResult_var match_result;

    protected:
      virtual
      ~MatchResultHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<MatchResultHolder>
      MatchResultHolder_var;

    struct MatchSizePolicy
    {
      size_t
      operator()(const MatchKeyHashAdapter& key, const MatchResultHolder_var& value) const noexcept
      {
        return sizeof(void*) +
          sizeof(unsigned long) * key.holder_->history_channels.size() +
          sizeof(void*) +
          sizeof(unsigned long) * value->match_result->result_channels.size() +
          sizeof(unsigned long) * value->match_result->result_estimate_channels.size();
      }
    };

    typedef Generics::BoundedMap<
      MatchKeyHashAdapter,
      MatchResultHolder_var,
      MatchSizePolicy,
      Sync::Policy::PosixThread,
      ReferenceCounting::HashTable<
        MatchKeyHashAdapter,
        Generics::BoundedMapTypes<MatchKeyHashAdapter, MatchResultHolder_var>::Item> >
      CacheMap;

  protected:
    virtual
    ~MatchCache() noexcept = default;

  protected:
    const Generics::Time timeout_;
    CacheMap cache_map_;
  };

  CalculateChannelMatcher::MatchKey::MatchKey(const CampaignSvcs::ChannelIdSet& history_channels)
    : match_key_holder_(new MatchKeyHolder(history_channels))
  {}

  CalculateChannelMatcher::MatchKey::~MatchKey() noexcept
  {}

  CalculateChannelMatcher::MatchResult::MatchResult(
    const ChannelIdSet& result_channels_val,
    const ChannelIdSet& result_estimate_channels_val,
    const ChannelActionMap& result_channel_actions_val)
    noexcept
    : result_channels(
        ArrayHelper<ChannelIdArray>::init_array(result_channels_val)),
      result_estimate_channels(
        ArrayHelper<ChannelIdArray>::init_array(result_estimate_channels_val)),
      result_channel_actions(result_channel_actions_val)
  {}

  // ChannelMatcher
  CalculateChannelMatcher::CalculateChannelMatcher(
    Logging::Logger* logger,
    unsigned long cache_limit,
    const Generics::Time& /*cache_timeout*/)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      cache_limit_(cache_limit)
  {}

  CalculateChannelMatcher::Config_var
  CalculateChannelMatcher::config() const /*throw(Exception)*/
  {
    try
    {
      SyncPolicy::ReadGuard lock(lock_);
      return ReferenceCounting::add_ref(config_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "CalculateChannelMatcher::config(): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  CalculateChannelMatcher::ExpressionChannelIndex_var
  CalculateChannelMatcher::get_channel_index_() const /*throw(Exception)*/
  {
    try
    {
      SyncPolicy::ReadGuard lock(lock_);
      return ReferenceCounting::add_ref(channel_index_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "CalculateChannelMatcher::get_channel_index_(): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  CalculateChannelMatcher::config(Config* new_config) /*throw(Exception)*/
  {
    try
    {
      ExpressionChannelIndex_var ch_index =
        new AdServer::CampaignSvcs::ExpressionChannelIndex();
      ch_index->index(new_config->expression_channels);
      Config_var config = ReferenceCounting::add_ref(new_config);
      MatchCache_var match_cache;
      if(cache_limit_ > 0)
      {
        match_cache = new MatchCache(cache_limit_);
      }

      ChannelActionConfig_var channel_action_config =
        new ChannelActionConfig();

      {
        for(auto ch_it = new_config->expression_channels.begin();
          ch_it != new_config->expression_channels.end(); ++ch_it)
        {
          if(ch_it->second->has_params())
          {
            channel_action_config->channel_actions.insert(
              std::make_pair(
                ch_it->first,
                ch_it->second->params().action_id));
          }
        }
      }

      // cache cache syncronously with config for avoid inconsistent match result
      SyncPolicy::WriteGuard lock(lock_);
      config_.swap(config);
      channel_index_.swap(ch_index);
      match_cache_.swap(match_cache);
      channel_action_config_.swap(channel_action_config);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "CalculateChannelMatcher::config(...): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  CalculateChannelMatcher::process_request_(
    const AdServer::CampaignSvcs::ExpressionChannelIndex* channel_index,
    const ChannelActionConfig* channel_action_config,
    ChannelIdSet& result_channels,
    ChannelIdSet* result_estimate_channels,
    ChannelActionMap* channel_actions,
    const AdServer::CampaignSvcs::ChannelIdHashSet& history_channels)
  {
    channel_index->match(
      result_channels,
      history_channels,
      result_estimate_channels);

    if(channel_actions)
    {
      for(auto ch_it = result_channels.begin(); ch_it != result_channels.end(); ++ch_it)
      {
        auto conv_it = channel_action_config->channel_actions.find(*ch_it);
        if(conv_it != channel_action_config->channel_actions.end())
        {
          channel_actions->insert(*conv_it);
        }
      }
    }
  }

  void CalculateChannelMatcher::process_request(
    const ChannelIdSet& history_channels,
    ChannelIdSet& result_channels,
    ChannelIdSet* result_estimate_channels,
    ChannelActionMap* result_channel_actions)
    /*throw(Exception)*/
  {
    ExpressionChannelIndex_var channel_index;
    MatchCache_var match_cache;
    ChannelActionConfig_var channel_action_config;

    {
      SyncPolicy::ReadGuard lock(lock_);
      channel_index = ReferenceCounting::add_ref(channel_index_);
      match_cache = ReferenceCounting::add_ref(match_cache_);
      channel_action_config = ReferenceCounting::add_ref(channel_action_config_);
    }

    if(match_cache.in())
    {
      Generics::Time now = Generics::Time::ZERO; //Generics::Time::get_time_of_day();

      MatchCache::MatchKeyHashAdapter match_key_hash_adapter(
        MatchCache::MatchKeyHolder_var(
          new MatchKeyHolder(history_channels)));

      MatchResult_var match_result = match_cache->get(match_key_hash_adapter, now);

      if(!match_result)
      {
        ChannelIdSet local_result_channels;
        ChannelIdSet local_result_estimate_channels;
        ChannelActionMap local_result_channel_actions;
        AdServer::CampaignSvcs::ChannelIdHashSet history_channels_hash;
        std::copy(
          history_channels.begin(),
          history_channels.end(),
          std::inserter(history_channels_hash, history_channels_hash.begin()));

        process_request_(
          channel_index,
          channel_action_config.in(),
          local_result_channels,
          &local_result_estimate_channels,
          &local_result_channel_actions,
          history_channels_hash);

        match_result = new MatchResult(
          local_result_channels,
          local_result_estimate_channels,
          local_result_channel_actions);

        match_cache->insert(match_key_hash_adapter, match_result, now);

        result_channels.swap(local_result_channels);

        if(result_estimate_channels)
        {
          result_estimate_channels->swap(local_result_estimate_channels);
        }

        if(result_channel_actions)
        {
          result_channel_actions->swap(local_result_channel_actions);
        }
      }
      else
      {
        std::copy(match_result->result_channels.begin(),
          match_result->result_channels.end(),
          std::inserter(result_channels, result_channels.begin()));

        if(result_estimate_channels)
        {
          std::copy(match_result->result_estimate_channels.begin(),
            match_result->result_estimate_channels.end(),
            std::inserter(*result_estimate_channels, result_estimate_channels->begin()));
        }

        if(result_channel_actions)
        {
          std::copy(match_result->result_channel_actions.begin(),
            match_result->result_channel_actions.end(),
            std::inserter(*result_channel_actions, result_channel_actions->begin()));
        }
      }
    }
    else
    {
      AdServer::CampaignSvcs::ChannelIdHashSet history_channels_hash;
      std::copy(
        history_channels.begin(),
        history_channels.end(),
        std::inserter(history_channels_hash, history_channels_hash.begin()));
      process_request_(
        channel_index,
        channel_action_config,
        result_channels,
        result_estimate_channels,
        result_channel_actions,
        history_channels_hash);
    }

    // push history channels into result even if it isn't indexed
    //   geo channels and already deactivated simple channels
    std::copy(
      history_channels.begin(),
      history_channels.end(),
      std::inserter(result_channels, result_channels.begin()));
  }

  CacheChannelMatcher::CacheChannelMatcher(
    ChannelMatcher* delegate,
    Logging::Logger* logger,
    const std::uint32_t cache_recheck_period,
    const std::string& db_path,
    const std::uint32_t block_сache_size_mb,
    const std::uint32_t ttl)
    : delegate_(ReferenceCounting::add_ref(delegate)),
      logger_(ReferenceCounting::add_ref(logger)),
      cache_(std::make_unique<RocksdbCache>(
        logger,
        cache_recheck_period,
        db_path,
        block_сache_size_mb,
        ttl))
  {
  }

  void CacheChannelMatcher::process_request(
    const ChannelIdSet& history_channels,
    ChannelIdSet& result_channels,
    ChannelIdSet* result_estimate_channels,
    ChannelActionMap* result_channel_actions)
  {
    const std::string key = cache_->create_key(history_channels);
    DataPtr data = cache_->get(key);
    if (!data)
    {
      data = std::make_unique<Data>();
      delegate_->process_request(
        history_channels,
        data->channels,
        &data->estimate_channels,
        &data->channel_actions);
      cache_->set(key, *data);
    }

    result_channels.merge(std::move(data->channels));
    if (result_estimate_channels)
    {
      result_estimate_channels->merge(std::move(data->estimate_channels));
    }
    if (result_channel_actions)
    {
      result_channel_actions->merge(std::move(data->channel_actions));
    }
  }

  CacheChannelMatcher::Config_var
  CacheChannelMatcher::config() const
  {
    return delegate_->config();
  }

  void CacheChannelMatcher::config(Config* config)
  {
    delegate_->config(config);
  }

  CacheChannelMatcher::RocksdbCache::RocksdbCache(
    Logger* logger,
    const std::uint32_t cache_recheck_period,
    const std::string& db_path,
    const std::uint32_t block_сache_size_mb,
    const std::uint32_t ttl)
    : logger_(ReferenceCounting::add_ref(logger)),
      cache_recheck_period_(cache_recheck_period)
  {
    std::size_t number_threads = std::thread::hardware_concurrency();
    if (number_threads == 0)
    {
      number_threads = 16;
    }

    rocksdb::DBOptions db_options;
    db_options.IncreaseParallelism(number_threads);
    db_options.create_if_missing = true;

    rocksdb::ColumnFamilyOptions column_family_options;
    column_family_options.OptimizeForPointLookup(block_сache_size_mb);
    column_family_options.compaction_style = rocksdb::kCompactionStyleLevel;
    column_family_options.ttl = ttl;

    std::optional<std::vector<int>> ttls{{static_cast<int>(ttl)}};

    std::vector<rocksdb::ColumnFamilyDescriptor> descriptors{
      {rocksdb::kDefaultColumnFamilyName, column_family_options}
    };

    data_base_ = std::make_unique<DataBase>(
      logger,
      db_path,
      db_options,
      descriptors,
      true,
      ttls);

    write_options_.disableWAL = true;
    write_options_.sync = false;
  }

  std::string CacheChannelMatcher::RocksdbCache::create_key(
    const ChannelIdSet& history_channels)
  {
    return md5(std::begin(history_channels), std::end(history_channels));
  }

  CacheChannelMatcher::DataPtr
  CacheChannelMatcher::RocksdbCache::get(const std::string& key) noexcept
  {
    try
    {
      std::string result;
      const auto status = data_base_->get().Get(
        read_options_,
        rocksdb::Slice(key.data(), key.size()),
        &result);

      if (status.ok())
      {
        std::istringstream stream(std::move(result));
        boost::archive::binary_iarchive iarchive(
          stream,
          boost::archive::no_header);
        auto data = std::make_unique<Data>();
        iarchive >> *data;

        const auto now = std::chrono::system_clock::now();
        const auto duration =
          std::chrono::duration_cast<std::chrono::seconds>(
            now - data->time).count();
        if (duration >= cache_recheck_period_)
        {
          return {};
        }

        return data;
      }
      else
      {
        if (!status.IsNotFound())
        {
          Stream::Error stream;
          stream << FNS
                 << status.ToString();
          logger_->error(stream.str(), Aspect::ROCKSDB_CACHE);
        }

        return {};
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::ROCKSDB_CACHE);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger_->error(stream.str(), Aspect::ROCKSDB_CACHE);
    }

    return {};
  }

  void CacheChannelMatcher::RocksdbCache::set(
    const std::string& key,
    const Data& data) noexcept
  {
    try
    {
      std::string string_buffer;
      string_buffer.reserve(200);
      std::ostringstream stream(std::move(string_buffer));
      boost::archive::binary_oarchive oarchive(
        stream,
        boost::archive::no_header);
      oarchive << data;
      const auto value = stream.str();

      const auto status = data_base_->get().Put(
        write_options_,
        rocksdb::Slice(key.data(), key.size()),
        rocksdb::Slice(value.data(), value.size()));
      if (!status.ok())
      {
        Stream::Error stream;
        stream << FNS
               << status.ToString();
        logger_->error(stream.str(), Aspect::ROCKSDB_CACHE);
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::ROCKSDB_CACHE);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger_->error(stream.str(), Aspect::ROCKSDB_CACHE);
    }
  }
}
}
