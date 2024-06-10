#ifndef _EXPRESSION_MATCHER_CHANNEL_MATCHER_HPP_
#define _EXPRESSION_MATCHER_CHANNEL_MATCHER_HPP_

#include <chrono>

#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

#include <openssl/md5.h>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBase.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class ChannelMatcher : virtual public ReferenceCounting::Interface
    {
    public:
      using ChannelMap = CampaignSvcs::ExpressionChannelHolderMap;
      using ChannelIdSet = AdServer::CampaignSvcs::ChannelIdSet;
      using ChannelActionMap = std::map<unsigned long, unsigned long>;

      struct Config: public ReferenceCounting::AtomicImpl
      {
        Generics::Time fill_time;
        ChannelMap expression_channels;
        ChannelIdSet all_channels;

      private:
        virtual ~Config() noexcept = default;
      };
      using Config_var = ReferenceCounting::SmartPtr<Config>;

    public:
      ChannelMatcher() = default;

      virtual void process_request(
        const ChannelIdSet& history_channels,
        ChannelIdSet& result_channels,
        ChannelIdSet* result_cpm_channels = nullptr,
        ChannelActionMap* channel_actions = nullptr) = 0;

      virtual Config_var config() const = 0;

      virtual void config(Config* config) = 0;

    protected:
      virtual ~ChannelMatcher() = default;
    };

    using ChannelMatcher_var = ReferenceCounting::SmartPtr<ChannelMatcher>;

    class CalculateChannelMatcher final :
      public ChannelMatcher,
      public ReferenceCounting::AtomicImpl
    {
    public:
      using Logger_var = Logging::Logger_var;

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      class MatchKeyHolder;

      struct MatchKey
      {
      public:
        MatchKey(const CampaignSvcs::ChannelIdSet& history_channels);

        virtual ~MatchKey() noexcept;

      protected:
        ReferenceCounting::SmartPtr<MatchKeyHolder> match_key_holder_;
      };

      struct MatchResult: public ReferenceCounting::AtomicImpl
      {
        typedef std::vector<unsigned long> ChannelIdArray;

        MatchResult(
          const ChannelIdSet& result_channels_val,
          const ChannelIdSet& result_estimate_channels_val,
          const ChannelActionMap& result_channel_actions_val)
          noexcept;

        const ChannelIdArray result_channels;
        const ChannelIdArray result_estimate_channels;
        const ChannelActionMap result_channel_actions;

      protected:
        virtual ~MatchResult() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<MatchResult>
        MatchResult_var;

    public:
      CalculateChannelMatcher(
        Logging::Logger* logger,
        unsigned long cache_limit,
        const Generics::Time& cache_timeout)
        /*throw(Exception)*/;

      Config_var config() const override /*throw(Exception)*/;

      void config(Config* config) override /*throw(Exception)*/;

      void process_request(
        const ChannelIdSet& history_channels,
        ChannelIdSet& result_channels,
        ChannelIdSet* result_estimate_channels = nullptr,
        ChannelActionMap* channel_actions = nullptr) override
        /*throw(Exception)*/;

    private:
      typedef Sync::Policy::PosixThread SyncPolicy;
      typedef AdServer::CampaignSvcs::ExpressionChannelIndex_var
        ExpressionChannelIndex_var;

      class MatchCache;
      typedef ReferenceCounting::SmartPtr<MatchCache>
        MatchCache_var;

      struct ChannelActionConfig: public ReferenceCounting::AtomicImpl
      {
        typedef std::map<unsigned long, unsigned long> ChannelActionMap;

        ChannelActionMap channel_actions;

      protected:
        virtual ~ChannelActionConfig() noexcept = default;
      };

      typedef ReferenceCounting::SmartPtr<ChannelActionConfig>
        ChannelActionConfig_var;

    private:
      ~CalculateChannelMatcher() override = default;

    void
    process_request_(
      const AdServer::CampaignSvcs::ExpressionChannelIndex* channel_index,
      const ChannelActionConfig* channel_action_config,
      ChannelIdSet& result_channels,
      ChannelIdSet* result_estimate_channels,
      ChannelActionMap* channel_actions,
      const AdServer::CampaignSvcs::ChannelIdHashSet& history_channels);

    ExpressionChannelIndex_var get_channel_index_() const
        /*throw(Exception)*/;

    private:
      Logging::Logger_var logger_;
      const unsigned long cache_limit_;

      mutable SyncPolicy::Mutex lock_;
      Config_var config_;
      MatchCache_var match_cache_;
      ExpressionChannelIndex_var channel_index_;
      ChannelActionConfig_var channel_action_config_;
    };

    class CacheChannelMatcher final :
      public ChannelMatcher,
      public ReferenceCounting::AtomicImpl
    {
    public:
      struct Data final
      {
        using Time = std::chrono::system_clock::time_point;

        ChannelIdSet channels;
        ChannelIdSet estimate_channels;
        ChannelActionMap channel_actions;
        Time time = std::chrono::system_clock::now();

      private:
        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
          ar & channels;
          ar & estimate_channels;
          ar & channel_actions;
          ar & boost::serialization::make_binary_object(&time, sizeof(Time));
        }

        friend class boost::serialization::access;
      };

      using DataPtr = std::unique_ptr<Data>;
      using Logger = Logging::Logger;
      using Logger_var = Logging::Logger_var;

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    private:
      class RocksdbCache;
      using RocksdbCachePtr = std::unique_ptr<RocksdbCache>;

    public:
      CacheChannelMatcher(
        ChannelMatcher* delegate,
        Logger* logger,
        const std::uint32_t cache_recheck_period,
        const std::string& db_path,
        const std::uint32_t block_сache_size_mb,
        const std::uint32_t ttl);

      void process_request(
        const ChannelIdSet& history_channels,
        ChannelIdSet& result_channels,
        ChannelIdSet* result_estimate_channels = nullptr,
        ChannelActionMap* channel_actions = nullptr) override;

      Config_var config() const override;

      void config(Config* config) override;

    protected:
      ~CacheChannelMatcher() override = default;

    private:
      ChannelMatcher_var delegate_;

      Logger_var logger_;

      RocksdbCachePtr cache_;
    };

    class CacheChannelMatcher::RocksdbCache final
    {
    private:
      using Data = CacheChannelMatcher::Data;
      using DataPtr = CacheChannelMatcher::DataPtr;
      using ChannelIdSet = CacheChannelMatcher::ChannelIdSet;
      using Logger = Logging::Logger;
      using Logger_var = Logging::Logger_var;
      using DataBase = UServerUtils::Grpc::RocksDB::DataBase;
      using DataBasePtr = std::unique_ptr<DataBase>;
      using ReadOptions = rocksdb::ReadOptions;
      using WriteOptions = rocksdb::WriteOptions;

    public:
      RocksdbCache(
        Logger* logger,
        const std::uint32_t cache_recheck_period,
        const std::string& db_path,
        const std::uint32_t block_сache_size_mb,
        const std::uint32_t ttl);

      ~RocksdbCache() = default;

      std::string create_key(const ChannelIdSet& history_channels);

      DataPtr get(const std::string& key) noexcept;

      void set(const std::string& key, const Data& data) noexcept;
      
    private:
      template<class It>
      std::string md5(It begin, It end)
      {
        using Value = typename std::iterator_traits<It>::value_type;

        MD5_CTX ctx;
        MD5_Init(&ctx);
        for (; begin != end; ++begin)
        {
          MD5_Update(&ctx, std::addressof(*begin), sizeof(Value));
        }

        std::string result;
        result.resize(MD5_DIGEST_LENGTH);
        MD5_Final(reinterpret_cast<unsigned char*>(result.data()), &ctx);

        return result;
      }

    private:
      const Logger_var logger_;

      const std::uint32_t cache_recheck_period_;

      DataBasePtr data_base_;

      ReadOptions read_options_;

      WriteOptions write_options_;
    };

  } // namespace RequestInfoSvcs
} // namespace AdServer

#endif /*_EXPRESSION_MATCHER_CHANNEL_MATCHER_HPP_*/