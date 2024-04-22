// GTEST
#include <gtest/gtest.h>

// STD
#include <filesystem>
#include <string>
#include <thread>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

// THIS
#include <RequestInfoSvcs/ExpressionMatcher/ChannelMatcher.hpp>

namespace
{
using ChannelIdSet = AdServer::CampaignSvcs::ChannelIdSet;
using ChannelActionMap = std::map<unsigned long, unsigned long>;

const ChannelIdSet result_cpm_channels_origin{11, 22, 33};
const ChannelActionMap result_channel_actions_origin{
  {111, 1111},
  {222, 2222},
  {333, 3333}
};

[[maybe_unused]] void remove_directory(const std::string& str_path)
{
  std::filesystem::path path(str_path);
  std::filesystem::remove_all(path);
}

} // namespace

class ChannelMatcherTest final :
  public AdServer::RequestInfoSvcs::ChannelMatcher,
  public ReferenceCounting::AtomicImpl
{
public:
  ChannelMatcherTest() = default;

  void process_request(
    const ChannelIdSet& history_channels,
    ChannelIdSet& result_channels,
    ChannelIdSet* result_cpm_channels,
    ChannelActionMap* result_channel_actions) override
  {
    if (counter_ == 1)
    {
      result_channels = history_channels;
      if (result_cpm_channels)
      {
        *result_cpm_channels = result_cpm_channels_origin;
      }
      if (result_channel_actions)
      {
        *result_channel_actions = result_channel_actions_origin;
      }
    }
    else if (counter_ == 2)
    {
      std::transform(
        std::begin(history_channels),
        std::end(history_channels),
        std::inserter(result_channels, std::end(result_channels)),
        [] (const auto v) { return v * 2;});

      if (result_cpm_channels)
      {
        std::transform(
          std::begin(result_cpm_channels_origin),
          std::end(result_cpm_channels_origin),
          std::inserter(*result_cpm_channels, std::end(*result_cpm_channels)),
          [] (const auto v) { return v * 2;});
      }
      if (result_channel_actions)
      {
        for (const auto& [k, v] : result_channel_actions_origin)
        {
          result_channel_actions->emplace(k, 2 * v);
        }
      }
    }
    else if (counter_ >= 3)
    {
      throw std::runtime_error("Logic error");
    }

    counter_ += 1;
  }

  Config_var config() const override
  {
    return {};
  }

  void config(Config* /*config*/) override
  {
  }

protected:
  ~ChannelMatcherTest() override = default;

private:
  int counter_ = 1;
};

TEST(ChannelMatcher, CacheChannelMatcher_1)
{
  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(
      std::cerr,
      Logging::Logger::EMERGENCY));

  const std::uint32_t cache_recheck_period = 1;
  const std::string db_path = "/tmp/rocksdb_test";
  const std::uint32_t block_сache_size_mb = 100;
  const std::uint32_t ttl = 5;

  remove_directory(db_path);

  AdServer::RequestInfoSvcs::ChannelMatcher_var delegate = new ChannelMatcherTest;
  AdServer::RequestInfoSvcs::ChannelMatcher_var matcher =
    new AdServer::RequestInfoSvcs::CacheChannelMatcher(
      delegate.in(),
      logger.in(),
      cache_recheck_period,
      db_path,
      block_сache_size_mb,
      ttl);

  for (int i = 1; i <= 10; i += 1)
  {
    const ChannelIdSet history_channels = {1, 2, 3};
    ChannelIdSet result_channels;
    ChannelIdSet result_cpm_channels;
    ChannelActionMap result_channel_actions;

    matcher->process_request(
      history_channels,
      result_channels,
      &result_cpm_channels,
      &result_channel_actions);

    EXPECT_EQ(result_channels, history_channels);
    EXPECT_EQ(result_cpm_channels, result_cpm_channels_origin);
    EXPECT_EQ(result_channel_actions, result_channel_actions_origin);
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  for (int i = 1; i <= 10; i += 1)
  {
    const ChannelIdSet history_channels = {1, 2, 3};
    ChannelIdSet result_channels;
    ChannelIdSet result_cpm_channels;
    ChannelActionMap result_channel_actions;

    matcher->process_request(
      history_channels,
      result_channels,
      &result_cpm_channels,
      &result_channel_actions);

    ChannelIdSet result_channels_expected;
    std::transform(
      std::begin(history_channels),
      std::end(history_channels),
      std::inserter(result_channels_expected, std::end(result_channels_expected)),
      [] (const auto v) {
        return v * 2;
      });
    EXPECT_EQ(result_channels, result_channels_expected);

    ChannelIdSet result_cpm_channels_expected;
    std::transform(
      std::begin(result_cpm_channels_origin),
      std::end(result_cpm_channels_origin),
      std::inserter(result_cpm_channels_expected, std::end(result_cpm_channels_expected)),
    [] (const auto v) {
      return v * 2;
    });
    EXPECT_EQ(result_cpm_channels, result_cpm_channels_expected);

    ChannelActionMap result_channel_actions_expected;
    for (const auto& [k, v] : result_channel_actions_origin)
    {
      result_channel_actions_expected.emplace(k, 2 * v);
    }
    EXPECT_EQ(result_channel_actions, result_channel_actions_expected);
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  {
    const ChannelIdSet history_channels = {1, 2, 3};
    ChannelIdSet result_channels;
    ChannelIdSet result_cpm_channels;
    ChannelActionMap result_channel_actions;

    EXPECT_ANY_THROW(
      matcher->process_request(
        history_channels,
        result_channels,
        &result_cpm_channels,
        &result_channel_actions));
  }
}

TEST(ChannelMatcher, CacheChannelMatcher_2)
{
  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(
      std::cerr,
      Logging::Logger::EMERGENCY));

  const std::uint32_t cache_recheck_period = 1;
  const std::string db_path = "/tmp/rocksdb_test";
  const std::uint32_t block_сache_size_mb = 100;
  const std::uint32_t ttl = 1;

  remove_directory(db_path);

  {
    const ChannelIdSet history_channels = {1, 2, 3};
    ChannelIdSet result_channels;

    AdServer::RequestInfoSvcs::ChannelMatcher_var delegate = new ChannelMatcherTest;
    AdServer::RequestInfoSvcs::ChannelMatcher_var matcher =
      new AdServer::RequestInfoSvcs::CacheChannelMatcher(
        delegate.in(),
        logger.in(),
        cache_recheck_period,
        db_path,
        block_сache_size_mb,
        ttl);

    for (int i = 1; i <= 2; ++i)
    {
      matcher->process_request(
        history_channels,
        result_channels,
        nullptr,
        nullptr);
      if (i == 1)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
  }

  for (int k = 1; k <= 3; ++k)
  {
    AdServer::RequestInfoSvcs::ChannelMatcher_var delegate = new ChannelMatcherTest;
    AdServer::RequestInfoSvcs::ChannelMatcher_var matcher =
      new AdServer::RequestInfoSvcs::CacheChannelMatcher(
        delegate.in(),
        logger.in(),
        cache_recheck_period,
        db_path,
        block_сache_size_mb,
        ttl);

    for (int i = 1; i <= 3; i += 1)
    {
      const ChannelIdSet history_channels = {1, 2, 3};
      ChannelIdSet result_channels;

      matcher->process_request(
        history_channels,
        result_channels,
        nullptr,
        nullptr);

      ChannelIdSet result_channels_expected;
      std::transform(
        std::begin(history_channels),
        std::end(history_channels),
        std::inserter(result_channels_expected, std::end(result_channels_expected)),
        [] (const auto v) {
          return v * 2;
        });
      EXPECT_EQ(result_channels, result_channels_expected);
    }
  }
}