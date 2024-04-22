// STD
#include <filesystem>

// GTEST
#include <gtest/gtest.h>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

// THIS
#include <ProfilingCommons/PlainStorage3/FileController.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoContainer.hpp>

using UserInfoContainer = AdServer::UserInfoSvcs::UserInfoContainer;
using UserInfoContainer_var = ReferenceCounting::SmartPtr<UserInfoContainer>;
using ChunkPathMap = AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap;
using LevelMapTraits = AdServer::ProfilingCommons::LevelMapTraits;
using FileController = AdServer::ProfilingCommons::FileController;
using FileController_var = ReferenceCounting::SmartPtr<FileController>;
using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;
using FreqCapIdList = AdServer::UserInfoSvcs::UserFreqCapProfile::FreqCapIdList;
using SeqOrder = AdServer::UserInfoSvcs::UserFreqCapProfile::SeqOrder;
using SeqOrderList = AdServer::UserInfoSvcs::UserFreqCapProfile::SeqOrderList;
using CampaignIds = AdServer::UserInfoSvcs::UserFreqCapProfile::CampaignIds;
using OperationPriority = AdServer::ProfilingCommons::OperationPriority;
using SmartMemBuf_var = Generics::SmartMemBuf_var;
using SmartMemBuf = Generics::SmartMemBuf;
using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;
using ConstSmartMemBuf = Generics::ConstSmartMemBuf;
using FreqCapConfig = AdServer::UserInfoSvcs::FreqCapConfig;
using FreqCapConfig_var = AdServer::UserInfoSvcs::FreqCapConfig_var;
using UserFreqCapProfile = AdServer::UserInfoSvcs::UserFreqCapProfile;
using CampaignFreqs = AdServer::UserInfoSvcs::UserFreqCapProfile::CampaignFreqs;

namespace
{

UserInfoContainer_var create_user_info_container(
  const bool need_clear,
  const bool is_level_enable,
  const bool is_rocksdb_enable)
{
  ChunkPathMap chunk_path_map;
  const std::size_t common_chunks_number = 3;
  for (std::size_t i = 0; i < common_chunks_number; ++i)
  {
    chunk_path_map.try_emplace(i, "/tmp/test_user_info_container_chunk_" + std::to_string(i));
  }

  if (need_clear)
  {
    for (auto& [_, directory] : chunk_path_map)
    {
      std::filesystem::remove_all(directory);
      std::filesystem::create_directories(directory);
    }
  }

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::EMERGENCY)));

  FileController_var file_controller = new AdServer::ProfilingCommons::SSDFileController(
    AdServer::ProfilingCommons::FileController_var(
      new AdServer::ProfilingCommons::PosixFileController(nullptr)));

  const LevelMapTraits add_level_map_traits(
    AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
    1 * 1024 * 1024,
    1 * 1024 * 1024,
    20 * 1024 * 1024,
    20,
    Generics::Time::ONE_DAY,
    file_controller);
  const LevelMapTraits temp_level_map_traits(
    AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
    1 * 1024 * 1024,
    1 * 1024 * 1024,
    20 * 1024 * 1024,
    20,
    Generics::Time::ONE_DAY,
    file_controller);
  const LevelMapTraits history_level_map_traits(
    AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
    1 * 1024 * 1024,
    1 * 1024 * 1024,
    20 * 1024 * 1024,
    20,
    Generics::Time::ONE_DAY,
    file_controller);
  const LevelMapTraits base_level_map_traits(
    AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
    1 * 1024 * 1024,
    1 * 1024 * 1024,
    20 * 1024 * 1024,
    20,
    Generics::Time::ONE_DAY,
    file_controller);
  const LevelMapTraits freq_cap_level_map_traits(
    AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
    1 * 1024 * 1024,
    1 * 1024 * 1024,
    20 * 1024 * 1024,
    20,
    Generics::Time::ONE_DAY,
    file_controller);

  const RocksDBParams add_rocksdb_params(
    1,
    1000,
    rocksdb::kCompactionStyleLevel);
  const RocksDBParams temp_rocksdb_params(
    1,
    1000,
    rocksdb::kCompactionStyleLevel);
  const RocksDBParams history_rocksdb_params(
    1,
    1000,
    rocksdb::kCompactionStyleLevel);
  const RocksDBParams base_rocksdb_params(
    1,
    1000,
    rocksdb::kCompactionStyleLevel);
  const RocksDBParams freq_cap_rocksdb_params(
    1,
    1000,
    rocksdb::kCompactionStyleLevel);

  const unsigned long colo_id = 1;
  const Generics::Time profile_request_timeout = Generics::Time::ONE_MINUTE;
  const Generics::Time history_optimization_period = Generics::Time::ONE_DAY;
  const bool avg_statistic = false;
  const Generics::Time session_timeout = Generics::Time::ONE_MINUTE;
  const unsigned long max_base_profile_waiters = 10;
  const unsigned long max_temp_profile_waiters = 10;
  const unsigned long max_freqcap_profile_waiters = 10;

  AdServer::ProfilingCommons::LoadingProgressCallbackBase_var progress_processor_parent;

  UserInfoContainer_var container = new UserInfoContainer(
    logger.in(),
    common_chunks_number,
    chunk_path_map,
    is_level_enable,
    add_level_map_traits,
    temp_level_map_traits,
    history_level_map_traits,
    base_level_map_traits,
    freq_cap_level_map_traits,
    is_rocksdb_enable,
    add_rocksdb_params,
    temp_rocksdb_params,
    history_rocksdb_params,
    base_rocksdb_params,
    freq_cap_rocksdb_params,
    colo_id,
    profile_request_timeout,
    history_optimization_period,
    avg_statistic,
    session_timeout,
    max_base_profile_waiters,
    max_temp_profile_waiters,
    max_freqcap_profile_waiters,
    progress_processor_parent);

  return container;
}

} // namespace

TEST(UserInfoContainer, Test)
{
  const AdServer::Commons::UserId user_id =
    AdServer::Commons::UserId::create_random_based();
  const AdServer::Commons::RequestId request_id =
    AdServer::Commons::UserId::create_random_based();
  const Generics::Time now = Generics::Time::get_time_of_day();
  const FreqCapIdList freq_caps{1, 2, 3};
  const FreqCapIdList uc_freq_caps{1, 2, 3, 4};
  const FreqCapIdList virtual_freq_caps{1, 2, 3, 5};
  SeqOrderList seq_orders{
    SeqOrder{.imps = 1, .ccg_id = 1, .set_id = 1},
    SeqOrder{.imps = 22, .ccg_id = 22, .set_id = 22},
    SeqOrder{.imps = 333, .ccg_id = 333, .set_id = 333}};
  const CampaignIds campaign_ids{1, 2, 3, 4, 5};
  const CampaignIds uc_campaign_ids{6, 7, 8};
  const OperationPriority op_priority = OperationPriority::OP_RUNTIME;

  FreqCapConfig_var fc_config(new FreqCapConfig);
  const unsigned long fc_id_val = 1;
  const unsigned long lifelimit_val = 100;
  const Generics::Time period_val = Generics::Time(3000);
  const unsigned long window_limit_val = 1000;
  const Generics::Time window_time_val = Generics::Time(3000);
  fc_config->freq_caps.insert(
    std::make_pair(
      1,
      AdServer::Commons::FreqCap(
        fc_id_val,
        lifelimit_val,
        period_val,
        window_limit_val,
        window_time_val)));

  {
    const bool need_clear = true;
    const bool is_level_enable = true;
    const bool is_rocksdb_enable = false;
    auto user_info_container = create_user_info_container(
      need_clear,
      is_level_enable,
      is_rocksdb_enable);
    user_info_container->activate_object();

    user_info_container->config(
      Generics::Time(3000),
      Generics::Time(3000),
      nullptr,
      fc_config.in());

    user_info_container->update_freq_caps(
      user_id,
      now,
      request_id,
      freq_caps,
      uc_freq_caps,
      virtual_freq_caps,
      seq_orders,
      campaign_ids,
      uc_campaign_ids,
      op_priority);

    SmartMemBuf_var mb_fc_profile_out = new SmartMemBuf;
    const bool temporary = false;
    auto result = user_info_container->get_user_profile(
      user_id,
      temporary,
      nullptr,
      nullptr,
      nullptr,
      &mb_fc_profile_out);
    EXPECT_TRUE(result);

    ConstSmartMemBuf_var mb_fc_profile = Generics::transfer_membuf(mb_fc_profile_out.in());
    UserFreqCapProfile freq_cap_profile(mb_fc_profile);

    FreqCapIdList freq_caps_out;
    SeqOrderList seq_orders_out;
    CampaignFreqs campaign_freqs_out;
    const Generics::Time now = Generics::Time::get_time_of_day();

    result = freq_cap_profile.full(
      freq_caps_out,
      nullptr,
      seq_orders_out,
      campaign_freqs_out,
      now,
      *fc_config);
    EXPECT_TRUE(result);

    EXPECT_EQ(seq_orders.size(), seq_orders_out.size());
    auto it = std::begin(seq_orders);
    auto it_out = std::begin(seq_orders_out);
    const auto it_end = std::end(seq_orders);
    for (; it != it_end; ++it, ++it_out)
    {
      EXPECT_EQ(it->imps, it_out->imps);
      EXPECT_EQ(it->set_id, it_out->set_id);
      EXPECT_EQ(it->ccg_id, it_out->ccg_id);
    }

    user_info_container->deactivate_object();
    user_info_container->wait_object();
  }

  {
    const bool need_clear = false;
    const bool is_level_enable = true;
    const bool is_rocksdb_enable = true;
    auto user_info_container = create_user_info_container(
      need_clear,
      is_level_enable,
      is_rocksdb_enable);
    user_info_container->activate_object();

    user_info_container->config(
      Generics::Time(3000),
      Generics::Time(3000),
      nullptr,
      fc_config.in());

    SmartMemBuf_var mb_fc_profile_out = new SmartMemBuf;
    const bool temporary = false;
    auto result = user_info_container->get_user_profile(
      user_id,
      temporary,
      nullptr,
      nullptr,
      nullptr,
      &mb_fc_profile_out);
    EXPECT_TRUE(result);

    ConstSmartMemBuf_var mb_fc_profile = Generics::transfer_membuf(mb_fc_profile_out.in());
    UserFreqCapProfile freq_cap_profile(mb_fc_profile);

    FreqCapIdList freq_caps_out;
    SeqOrderList seq_orders_out;
    CampaignFreqs campaign_freqs_out;
    const Generics::Time now = Generics::Time::get_time_of_day();

    result = freq_cap_profile.full(
      freq_caps_out,
      nullptr,
      seq_orders_out,
      campaign_freqs_out,
      now,
      *fc_config);
    EXPECT_TRUE(result);

    EXPECT_EQ(seq_orders.size(), seq_orders_out.size());
    auto it = std::begin(seq_orders);
    auto it_out = std::begin(seq_orders_out);
    const auto it_end = std::end(seq_orders);
    for (; it != it_end; ++it, ++it_out)
    {
      EXPECT_EQ(it->imps, it_out->imps);
      EXPECT_EQ(it->set_id, it_out->set_id);
      EXPECT_EQ(it->ccg_id, it_out->ccg_id);
    }

    user_info_container->deactivate_object();
    user_info_container->wait_object();
  }

  {
    const bool need_clear = false;
    const bool is_level_enable = false;
    const bool is_rocksdb_enable = true;
    auto user_info_container = create_user_info_container(
      need_clear,
      is_level_enable,
      is_rocksdb_enable);
    user_info_container->activate_object();

    user_info_container->config(
      Generics::Time(3000),
      Generics::Time(3000),
      nullptr,
      fc_config.in());

    SmartMemBuf_var mb_fc_profile_out = new SmartMemBuf;
    const bool temporary = false;
    auto result = user_info_container->get_user_profile(
      user_id,
      temporary,
      nullptr,
      nullptr,
      nullptr,
      &mb_fc_profile_out);
    EXPECT_TRUE(result);
    EXPECT_TRUE(mb_fc_profile_out->membuf().empty());

    user_info_container->deactivate_object();
    user_info_container->wait_object();
  }

  {
    const bool need_clear = true;
    const bool is_level_enable = true;
    const bool is_rocksdb_enable = true;
    auto user_info_container = create_user_info_container(
      need_clear,
      is_level_enable,
      is_rocksdb_enable);
    user_info_container->activate_object();

    user_info_container->config(
      Generics::Time(3000),
      Generics::Time(3000),
      nullptr,
      fc_config.in());

    user_info_container->update_freq_caps(
      user_id,
      now,
      request_id,
      freq_caps,
      uc_freq_caps,
      virtual_freq_caps,
      seq_orders,
      campaign_ids,
      uc_campaign_ids,
      op_priority);

    SmartMemBuf_var mb_fc_profile_out = new SmartMemBuf;
    const bool temporary = false;
    auto result = user_info_container->get_user_profile(
      user_id,
      temporary,
      nullptr,
      nullptr,
      nullptr,
      &mb_fc_profile_out);
    EXPECT_TRUE(result);

    ConstSmartMemBuf_var mb_fc_profile = Generics::transfer_membuf(mb_fc_profile_out.in());
    UserFreqCapProfile freq_cap_profile(mb_fc_profile);

    FreqCapIdList freq_caps_out;
    SeqOrderList seq_orders_out;
    CampaignFreqs campaign_freqs_out;
    const Generics::Time now = Generics::Time::get_time_of_day();

    result = freq_cap_profile.full(
      freq_caps_out,
      nullptr,
      seq_orders_out,
      campaign_freqs_out,
      now,
      *fc_config);
    EXPECT_TRUE(result);

    EXPECT_EQ(seq_orders.size(), seq_orders_out.size());
    auto it = std::begin(seq_orders);
    auto it_out = std::begin(seq_orders_out);
    const auto it_end = std::end(seq_orders);
    for (; it != it_end; ++it, ++it_out)
    {
      EXPECT_EQ(it->imps, it_out->imps);
      EXPECT_EQ(it->set_id, it_out->set_id);
      EXPECT_EQ(it->ccg_id, it_out->ccg_id);
    }

    user_info_container->deactivate_object();
    user_info_container->wait_object();
  }

  {
    const bool need_clear = false;
    const bool is_level_enable = false;
    const bool is_rocksdb_enable = true;
    auto user_info_container = create_user_info_container(
      need_clear,
      is_level_enable,
      is_rocksdb_enable);
    user_info_container->activate_object();

    user_info_container->config(
      Generics::Time(3000),
      Generics::Time(3000),
      nullptr,
      fc_config.in());

    SmartMemBuf_var mb_fc_profile_out = new SmartMemBuf;
    const bool temporary = false;
    auto result = user_info_container->get_user_profile(
      user_id,
      temporary,
      nullptr,
      nullptr,
      nullptr,
      &mb_fc_profile_out);
    EXPECT_TRUE(result);

    ConstSmartMemBuf_var mb_fc_profile = Generics::transfer_membuf(mb_fc_profile_out.in());
    UserFreqCapProfile freq_cap_profile(mb_fc_profile);

    FreqCapIdList freq_caps_out;
    SeqOrderList seq_orders_out;
    CampaignFreqs campaign_freqs_out;
    const Generics::Time now = Generics::Time::get_time_of_day();

    result = freq_cap_profile.full(
      freq_caps_out,
      nullptr,
      seq_orders_out,
      campaign_freqs_out,
      now,
      *fc_config);
    EXPECT_TRUE(result);

    EXPECT_EQ(seq_orders.size(), seq_orders_out.size());
    auto it = std::begin(seq_orders);
    auto it_out = std::begin(seq_orders_out);
    const auto it_end = std::end(seq_orders);
    for (; it != it_end; ++it, ++it_out)
    {
      EXPECT_EQ(it->imps, it_out->imps);
      EXPECT_EQ(it->set_id, it_out->set_id);
      EXPECT_EQ(it->ccg_id, it_out->ccg_id);
    }

    user_info_container->deactivate_object();
    user_info_container->wait_object();
  }
}