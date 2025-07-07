// GTEST
#include <gtest/gtest.h>

// STD
#include <filesystem>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

// THIS
#include <RequestInfoSvcs/ExpressionMatcher/ExpressionMatcherOutLogger.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/UserInventoryContainer.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserChannelInventoryProfileAdapter.hpp>

using ChunkPathMap = AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap;
using UserInventoryInfoContainer = AdServer::RequestInfoSvcs::UserInventoryInfoContainer;
using UserInventoryInfoContainer_var = AdServer::RequestInfoSvcs::UserInventoryInfoContainer_var;
using ExpressionMatcherOutLogger = AdServer::RequestInfoSvcs::ExpressionMatcherOutLogger;
using ExpressionMatcherOutLogger_var = AdServer::RequestInfoSvcs::ExpressionMatcherOutLogger_var;
using RevenueDecimal = AdServer::RequestInfoSvcs::RevenueDecimal;
using Cache_var = AdServer::ProfilingCommons::ProfileMapFactory::Cache_var;
using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;
using SmartMemBuf_var = Generics::SmartMemBuf_var;
using SmartMemBuf = Generics::SmartMemBuf;
using UserChannelInventoryProfileWriter = AdServer::RequestInfoSvcs::UserChannelInventoryProfileWriter;
using UserChannelInventoryProfileReader = AdServer::RequestInfoSvcs::UserChannelInventoryProfileReader;

namespace
{

UserInventoryInfoContainer_var create_user_inventory_container(const bool need_clear)
{
  ChunkPathMap chunk_folders;
  const std::size_t common_chunks_number = 3;
  for (std::size_t i = 0; i < common_chunks_number; ++i)
  {
    chunk_folders.try_emplace(i, "/tmp/test_user_inventory_container_chunk_" + std::to_string(i));
  }

  if (need_clear)
  {
    for (auto& [_, directory] : chunk_folders)
    {
      std::filesystem::remove_all(directory);
      std::filesystem::create_directories(directory);
    }
  }

  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(
      std::cerr,
      Logging::Logger::EMERGENCY));

  AdServer::LogProcessing::LogFlushTraits log_flush_traits(
    Generics::Time::ONE_DAY,
    "/tmp",
    std::nullopt);

  ExpressionMatcherOutLogger_var expression_matcher_out_logger = new ExpressionMatcherOutLogger(
    logger.in(),
    693,
    RevenueDecimal::ZERO,
    log_flush_traits,
    log_flush_traits,
    log_flush_traits,
    log_flush_traits,
    log_flush_traits,
    log_flush_traits,
    log_flush_traits,
    log_flush_traits);

  const RocksDBParams add_rocksdb_params(
    1,
    1000,
    rocksdb::kCompactionStyleLevel);

  UserInventoryInfoContainer_var container = new UserInventoryInfoContainer(
    logger.in(),
    Generics::Time::ONE_DAY,
    expression_matcher_out_logger.in(),
    expression_matcher_out_logger.in(),
    false,
    common_chunks_number,
    chunk_folders,
    "invfile_prefix",
    Cache_var{},
    add_rocksdb_params);

  return container;
}

} // namespace

TEST(UserInventoryInfoContainer, Test)
{
  const AdServer::Commons::UserId user_id =
    AdServer::Commons::UserId::create_random_based();
  const std::string original_sum_revenue = "sum_revenue";
  const Generics::Time now = Generics::Time::get_time_of_day();

  {
    UserChannelInventoryProfileWriter profile_writer;
    profile_writer.version() = AdServer::RequestInfoSvcs::CURRENT_CHANNEL_INVENTORY_PROFILE_VERSION;
    profile_writer.sum_revenue() = original_sum_revenue;

    Generics::SmartMemBuf_var mem_buf(
      new Generics::SmartMemBuf(profile_writer.size()));
    profile_writer.save(
      mem_buf->membuf().data(),
      mem_buf->membuf().size());

    auto container = create_user_inventory_container(true);
    EXPECT_NO_THROW(container->save_profile_(
      user_id,
      Generics::transfer_membuf(mem_buf),
      now));

    auto result_mem_buf = container->get_profile(user_id);
    EXPECT_TRUE(result_mem_buf);

    UserChannelInventoryProfileReader profile_reader(
      result_mem_buf->membuf().data(),
      result_mem_buf->membuf().size());
    EXPECT_EQ(profile_reader.sum_revenue(), original_sum_revenue);
  }

  {
    auto container = create_user_inventory_container(false);
    auto mem_buf = container->get_profile(user_id);
    EXPECT_TRUE(mem_buf);

    UserChannelInventoryProfileReader profile_reader(
      mem_buf->membuf().data(),
      mem_buf->membuf().size());
    EXPECT_EQ(profile_reader.sum_revenue(), original_sum_revenue);
  }
}