#include "ExpressionMatcherImpl.hpp"

#include <fstream>
#include <algorithm>
#include <iterator>
#include <string>

#include <Commons/Algs.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/DelegateTaskGoal.hpp>

#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>
#include <CampaignSvcs/CampaignManager/CampaignConfigSource.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Utils.hpp>

#include "UserTriggerMatchProfileProviderImpl.hpp"
#include "ConversionProcessor.hpp"

namespace
{
  const char DEFAULT_REQUEST_BASIC_CHANNELS_BASE_NAME[] =
    "RequestBasicChannels";
  const char DEFAULT_CONSIDER_CLICK_BASE_NAME[] =
    "ConsiderClick";
  const char DEFAULT_CONSIDER_IMPRESSION_BASE_NAME[] =
    "ConsiderImpression";

  const char DEFAULT_ERROR_DIR[] = "Error";

  const char CHANNEL_INVENTORY_OUT_DIR[] = "ChannelInventory";
  const char CHANNEL_IMP_INVENTORY_OUT_DIR[] = "ChannelImpInventory";
  const char CHANNEL_PRICE_RANGE_OUT_DIR[] = "ChannelPriceRange";
  const char CHANNEL_PERFORMANCE_OUT_DIR[] = "ChannelPerformance";
  const char CHANNEL_TRIGGER_IMP_STAT_OUT_DIR[] = "ChannelTriggerImpStat";
  const char GLOBAL_COLO_USER_STAT_OUT_DIR[] = "GlobalColoUserStat";
  const char COLO_USER_STAT_OUT_DIR[] = "ColoUserStat";

  const unsigned long MAX_CHANNEL_LEVEL = 20;

  typedef const String::AsciiStringManip::Char1Category<','> Sep;
}

namespace Aspect
{
  const char EXPRESSION_MATCHER[] = "ExpressionMatcher";
  const char EXPRESSION_MATCHER_DAILY_CHECK[] = "ExpressionMatcher:DailyCheck";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    template<typename _T>
    _T gcd(_T first, _T second)
    {
      while (second != 0)
      {
        _T t = first % second;
        first = second;
        second = t;
      }
      return first;
    }

    std::string
    fetch_dir_path(const xsd::AdServer::Configuration::InLogsType& lp_config) /*throw(eh::Exception)*/
    {
      return
        AdServer::LogProcessing::FileReceiverConfig::make_path(
          lp_config.log_root().c_str(),
          lp_config.RequestBasicChannels().path().present() ? lp_config.RequestBasicChannels().path()->c_str() :
            DEFAULT_REQUEST_BASIC_CHANNELS_BASE_NAME);
    }

    ExpressionMatcherLogLoader::LogReadTraits
    make_log_read_traits(
      ExpressionMatcherLogLoader::LogType log_type,
      const char* log_root,
      const ::xsd::AdServer::Configuration::InLogType& log_type_config,
      const char* prefix)
      /*throw(eh::Exception)*/
    {
      const std::string in_dir =
        AdServer::LogProcessing::FileReceiverConfig::make_path(
          log_root,
          log_type_config.path().present() ?
            log_type_config.path()->c_str() :
            prefix);

      return ExpressionMatcherLogLoader::LogReadTraits(
        log_type,
        in_dir,
        prefix,
        LogProcessing::FileReceiverConfig::make_path(
          in_dir.c_str(),
          log_type_config.intermediate().c_str()));
    }
  }

  AdServer::LogProcessing::LogFlushTraits
  read_flush_policy(
    const xsd::AdServer::Configuration::LogFlushPolicyType&
      log_flush_policy,
    const char* default_path,
    Generics::Time& check_loggers_period)
  {
    check_loggers_period.tv_sec =
      ( check_loggers_period.tv_sec == 0 ?
        log_flush_policy.period() :
        gcd((unsigned long)check_loggers_period.tv_sec,
            (unsigned long)log_flush_policy.period()));

    AdServer::LogProcessing::LogFlushTraits res;
    res.period = Generics::Time(log_flush_policy.period());
    res.out_dir = log_flush_policy.path().present() ?
      log_flush_policy.path()->c_str() : default_path;
    return res;
  }

  /**
   * ExpressionMatcherImpl
   * info:
   *   task scheduling order:
   *   UpdateExpressionChannelsTask
   *     -> ProcessRequestBasicChannelsTask (in loop)
   *   ResolveUserInfoManagerSessionTask
   *     -> DailyProcessingTask
   */

  ExpressionMatcherImpl::ExpressionMatcherImpl(
    Logging::Logger* init_logger,
    const ExpressionMatcherConfig& expression_matcher_config,
    ProcStatImpl* proc_stat_impl)
    /*throw(Exception)*/
    : corba_client_adapter_(new CORBACommons::CorbaClientAdapter),
      expression_matcher_config_(expression_matcher_config),
      check_loggers_period_(0),
      daily_processing_loop_started_(false),
      file_controller_(new ProfilingCommons::PosixFileController(0,
        expression_matcher_config.Storage().min_free_space().present() ?
          *expression_matcher_config.Storage().min_free_space() : 0)),
      callback_(new Logging::ActiveObjectCallbackImpl(init_logger,
        "AdServer::CampaignSvcs::ExpressionMatcherImpl",
        Aspect::EXPRESSION_MATCHER, "ADS-IMPL-4016")),
      task_runner_(new Generics::TaskRunner(callback_, 3)),
      daily_processing_task_runner_(new Generics::TaskRunner(callback_,
        expression_matcher_config_.DailyProcessing().thread_pool_size())),
      scheduler_(new Generics::Planner(callback_)),
      profile_cache_(
        expression_matcher_config.LogProcessing().cache_blocks() > 0 ?
        ProfilingCommons::ProfileMapFactory::Cache_var(
          new ProfilingCommons::ProfileMapFactory::Cache(
            expression_matcher_config.LogProcessing().cache_blocks())) :
        ProfilingCommons::ProfileMapFactory::Cache_var()),
      proc_stat_impl_(ReferenceCounting::add_ref(proc_stat_impl))
  {
    static const char* FUN = "ExpressionMatcherImpl::ExpressionMatcherImpl()";

    if(logger() == 0)
    {
      throw Exception(
        "ExpressionMatcherImpl::ExpressionMatcherImpl(): "
        "Logger must be defined.");
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(daily_processing_task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CompositeActiveObject::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }

    /* init processors */
    channel_matcher_ = new ChannelMatcher(
      logger(),
      expression_matcher_config_.LogProcessing().channel_match_cache_size(),
      Generics::Time::ZERO);

    try
    {
      const xsd::AdServer::Configuration::OutLogsType&
        lp_config = expression_matcher_config_.LogProcessing().OutLogs();

      RevenueDecimal div_simpl_factor_reminder;
      RevenueDecimal simpl_factor =
        expression_matcher_config_.inventory_users_percentage() != 0 ?
        RevenueDecimal::div(
          RevenueDecimal(false, 100, 0),
          RevenueDecimal(false,
            expression_matcher_config_.inventory_users_percentage(),
            0),
          div_simpl_factor_reminder) :
        RevenueDecimal::ZERO;

      // FIXME: temporary check, before all sampled stats will be make double
      if (simpl_factor != RevenueDecimal(false, simpl_factor.integer<long>(), 0))
      {
        throw Exception("inventory_users_percentage does not correspond to an integral number");
      }

      const std::string log_root = lp_config.log_root();

      expression_matcher_out_logger_ = new ExpressionMatcherOutLogger(
        logger(),
        expression_matcher_config_.colo_id(),
        simpl_factor,
        read_flush_policy(
          lp_config.ChannelInventory(),
          (log_root + CHANNEL_INVENTORY_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.ChannelImpInventory(),
          (log_root + CHANNEL_IMP_INVENTORY_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.ChannelPriceRange(),
          (log_root + CHANNEL_PRICE_RANGE_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.ChannelInventoryActivity(),
          (log_root + CHANNEL_INVENTORY_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.ChannelPerformance(),
          (log_root + CHANNEL_PERFORMANCE_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.ChannelTriggerImpStat(),
          (log_root + CHANNEL_TRIGGER_IMP_STAT_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.GlobalColoUserStat(),
          (log_root + GLOBAL_COLO_USER_STAT_OUT_DIR).c_str(),
          check_loggers_period_),
        read_flush_policy(
          lp_config.ColoUserStat(),
          (log_root + COLO_USER_STAT_OUT_DIR).c_str(),
          check_loggers_period_));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't init ExpressionMatcherOutLogger. eh::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }

    try
    {
      Generics::Task_var load_data_msg(new LoadDataTask(this, task_runner_));
      task_runner_->enqueue_task(load_data_msg);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't set task for UserInfoManagerSession resolving."
        "Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    try
    {
      campaign_pool_.reset(new AdServer::CampaignSvcs::CampaignServerPool(
        Config::CorbaConfigReader::read_multi_corba_ref(
          expression_matcher_config_.CampaignServerCorbaRef()),
        corba_client_adapter_,
        CORBACommons::ChoosePolicyType::PT_PERSISTENT,
        Generics::Time(expression_matcher_config_.update_period() / 2) // bad period
        ));
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't resolve campaign server."
        "Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    try
    {
      auto number_threads = std::thread::hardware_concurrency();
      if (number_threads == 0)
      {
        number_threads = 30;
      }

      UServerUtils::Grpc::RocksDB::Config config;
      config.event_queue_max_size = 10000000;
      config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
      config.io_uring_size = 12800;
      config.number_io_urings = 2 * number_threads;
      rocksdb_manager_pool_ = std::make_shared<RocksdbManagerPool>(
        config,
        logger());
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't create rocksdb_manager_pool. Caught eh::Exception: " << exc.what();
      throw Exception(ostr);
    }
  }

  ExpressionMatcherImpl::~ExpressionMatcherImpl() noexcept
  {}

  struct GetProfileAdapter
  {
    template<typename ContainerPtrHolderType, typename KeyType>
    Generics::ConstSmartMemBuf_var operator()(
      ContainerPtrHolderType* container,
      const KeyType& key) const
    {
      return container->get_profile(key);
    }
  };

  struct GetUserProfileAdapter
  {
    template<typename ContainerPtrHolderType, typename KeyType>
    Generics::ConstSmartMemBuf_var operator()(
      ContainerPtrHolderType* container,
      const KeyType& key) const
    {
      return container->get_user_profile(key);
    }
  };

  struct GetRequestProfileAdapter
  {
    template<typename ContainerPtrHolderType, typename KeyType>
    Generics::ConstSmartMemBuf_var operator()(
      ContainerPtrHolderType* container,
      const KeyType& key) const
    {
      return container->get_request_profile(key);
    }
  };

  template<
    typename ContainerPtrHolderType,
    typename KeyType,
    typename GetProfileAdapterType>
  bool
  ExpressionMatcherImpl::get_profile_(
    CORBACommons::OctSeq_out result_profile,
    const char* FUN,
    const ContainerPtrHolderType& container_ptr_holder,
    const KeyType& id,
    const GetProfileAdapterType& get_profile_adapter)
    /*throw(AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
      AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/
  {
    try
    {
      auto container = container_ptr_holder.get();

      if(!container.in())
      {
        AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady exc;
        exc.description = "Container is not ready";
        throw exc;
      }

      result_profile = new CORBACommons::OctSeq();

      Generics::ConstSmartMemBuf_var mb_profile = get_profile_adapter(container.in(), id);

      if(mb_profile.in())
      {
        CorbaAlgs::convert_mem_buf(*result_profile, mb_profile->membuf());
        return true;
      }

      return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't get profile. Caught eh::Exception: " << ex.what();

      CORBACommons::throw_desc<
        RequestInfoSvcs::ExpressionMatcher::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't get profile. Caught CORBA::SystemException: " << e;

      CORBACommons::throw_desc<
        RequestInfoSvcs::ExpressionMatcher::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  CORBA::Boolean
  ExpressionMatcherImpl::get_inventory_profile(
    const char* user_id,
    AdServer::RequestInfoSvcs::UserInventoryProfile_out inv_profile)
    /*throw(
      AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
      AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/
  {
    static const char* FUN = "ExpressionMatcherImpl::get_profile()";

    return get_profile_(
      inv_profile,
      FUN,
      user_inventory_container_,
      AdServer::Commons::UserId(user_id),
      GetProfileAdapter());
  }

  CORBA::Boolean
  ExpressionMatcherImpl::get_user_trigger_match_profile(
    const char* user_id,
    bool temporary_user,
    AdServer::RequestInfoSvcs::UserTriggerMatchProfile_out user_trigger_profile)
    /*throw(AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
      AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/
  {
    static const char* FUN = "ExpressionMatcherImpl::get_user_trigger_match_profile()";

    if(!temporary_user)
    {
      return get_profile_(
        user_trigger_profile,
        FUN,
        user_trigger_match_container_,
        AdServer::Commons::UserId(user_id),
        GetUserProfileAdapter());
    }
    else
    {
      return get_profile_(
        user_trigger_profile,
        FUN,
        temp_user_trigger_match_container_,
        AdServer::Commons::UserId(user_id),
        GetUserProfileAdapter());
    }
  }

  CORBA::Boolean
  ExpressionMatcherImpl::get_request_trigger_match_profile(
    const char* request_id,
    AdServer::RequestInfoSvcs::RequestTriggerMatchProfile_out request_trigger_profile)
    /*throw(AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
      AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/
  {
    static const char* FUN = "ExpressionMatcherImpl::get_request_trigger_match_profile()";

    return get_profile_(
      request_trigger_profile,
      FUN,
      user_trigger_match_container_,
      AdServer::Commons::RequestId(request_id),
      GetRequestProfileAdapter());
  }

  CORBA::Boolean
  ExpressionMatcherImpl::get_household_colo_reach_profile(
    const char* user_id,
    AdServer::RequestInfoSvcs::HouseholdColoReachProfile_out profile)
    /*throw(AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
      AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/
  {
    static const char* FUN = "ExpressionMatcherImpl::get_household_colo_reach_profile()";

    return get_profile_(
      profile,
      FUN,
      household_colo_reach_container_,
      AdServer::Commons::RequestId(user_id),
      GetProfileAdapter());
  }

  void ExpressionMatcherImpl::load_data_() noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::load_data_()";

    try
    {
      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_inventory_folders;

      AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
        chunk_inventory_folders,
        expression_matcher_config_.ChunksConfig().chunks_root().c_str(),
        "Chunk");
      for(auto iter = chunk_inventory_folders.begin();
          iter != chunk_inventory_folders.end();
          ++iter)
      {
        iter->second += "/Inventory";
      }

      if(!user_inventory_container_.get().in())
      {
        UserInventoryInfoContainer_var user_inventory_container = new UserInventoryInfoContainer(
          logger(),
          Generics::Time::ONE_DAY * expression_matcher_config_.ChunksConfig().days_to_keep(),
          expression_matcher_out_logger_,
          expression_matcher_out_logger_,
          expression_matcher_config_.LogProcessing().adrequest_anonymize(),
          expression_matcher_config_.ChunksConfig().chunks_number(),
          chunk_inventory_folders,
          expression_matcher_config_.ChunksConfig().chunks_prefix().c_str(),
          profile_cache_,
          fill_level_map_traits_(expression_matcher_config_.ChunksConfig())
          );

        add_child_object(user_inventory_container, true);

        user_inventory_container_ = user_inventory_container;
      }
    }
    catch(const UserInventoryInfoContainer::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4023") << FUN <<
        ": caught Exception on creating UserInventoryInfoContainer: " << ex.what();
    }

    if(expression_matcher_config_.TriggerImpsConfig().present())
    {
      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_trigger_folders;
      try
      {
        AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
          chunk_trigger_folders,
          expression_matcher_config_.TriggerImpsConfig()->UserChunksConfig().chunks_root().c_str(),
          "Chunk");
        for(auto iter = chunk_trigger_folders.begin();
            iter != chunk_trigger_folders.end();
            ++iter)
        {
          iter->second += "/UserTriggerMatch";
        }
      }
      catch(const eh::Exception& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-806") << FUN <<
          ": caught Exception on fetching UserTriggerMatchchunks : " << ex.what();
      }

      try
      {
        if(!temp_user_trigger_match_container_.get().in())
        {
          const auto& request_chunks_rocksdb_config =
            expression_matcher_config_.TriggerImpsConfig()->RequestChunksRocksDBConfig();
          bool is_request_rocksdb_enable = false;
          const auto& is_request_rocksdb_enable_optional = request_chunks_rocksdb_config.is_enable();
          if (is_request_rocksdb_enable_optional.present())
          {
            is_request_rocksdb_enable = is_request_rocksdb_enable_optional.get();
          }
          const auto request_rocksdb_params = AdServer::UserInfoSvcs::fill_rocksdb_map_params(
            request_chunks_rocksdb_config);

          UserTriggerMatchContainer_var temp_user_trigger_match_container = new UserTriggerMatchContainer(
            logger(),
            0,
            0, // no providers
            rocksdb_manager_pool_,
            expression_matcher_config_.ChunksConfig().chunks_number(),
            chunk_trigger_folders,
            expression_matcher_config_.TriggerImpsConfig()->TempUserChunksConfig().chunks_prefix().c_str(),
            "",
            0,
            is_request_rocksdb_enable,
            request_rocksdb_params,
            expression_matcher_config_.TriggerImpsConfig()->positive_triggers_group_size(),
            expression_matcher_config_.TriggerImpsConfig()->negative_triggers_group_size(),
            expression_matcher_config_.TriggerImpsConfig()->max_trigger_visits(),
            profile_cache_,
            fill_level_map_traits_(expression_matcher_config_.TriggerImpsConfig()->TempUserChunksConfig()),
            fill_level_map_traits_(expression_matcher_config_.TriggerImpsConfig()->RequestChunksConfig())
            );

          add_child_object(temp_user_trigger_match_container, true);

          temp_user_trigger_match_container_ = temp_user_trigger_match_container;

          // init UserTriggerMatchProfileProvider
          CORBACommons::CorbaObjectRefList expression_matcher_refs;

          Config::CorbaConfigReader::read_multi_corba_ref(
            expression_matcher_config_.ExpressionMatcherGroup(),
            expression_matcher_refs);

          Commons::HostDistributionFile_var host_distr = new Commons::HostDistributionFile(
            expression_matcher_config_.ChunksDistribution().distribution_file_path().c_str(),
            expression_matcher_config_.ChunksDistribution().distribution_file_schema().c_str());

          user_trigger_match_profile_provider_ = new UserTriggerMatchProfileProviderImpl(
            corba_client_adapter_.in(),
            expression_matcher_refs,
            host_distr,
            expression_matcher_config_.service_host_name().c_str(),
            temp_user_trigger_match_container_.get(),
            expression_matcher_config_.ChunksConfig().chunks_number());
        }
      }
      catch(const Commons::HostDistributionFile::InvalidFile& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4023") << FUN <<
          ": HostDistributionFile::InvalidFile: '" <<
          expression_matcher_config_.ChunksDistribution().distribution_file_path()
          << "': " << ex.what();
      }
      catch(const UserTriggerMatchContainer::Exception& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4023") << FUN <<
          ": caught Exception on creating UserTriggerMatchContainer: " << ex.what();
      }

      try
      {
        if(user_trigger_match_profile_provider_.in() &&
           !user_trigger_match_container_.get().in())
        {
          const auto& request_chunks_rocksdb_config =
            expression_matcher_config_.TriggerImpsConfig()->RequestChunksRocksDBConfig();
          bool is_request_rocksdb_enable = false;
          const auto& is_request_rocksdb_enable_optional = request_chunks_rocksdb_config.is_enable();
          if (is_request_rocksdb_enable_optional.present())
          {
            is_request_rocksdb_enable = is_request_rocksdb_enable_optional.get();
          }
          const auto request_rocksdb_params = AdServer::UserInfoSvcs::fill_rocksdb_map_params(
            request_chunks_rocksdb_config);

          UserTriggerMatchContainer_var user_trigger_match_container = new UserTriggerMatchContainer(
            logger(),
            expression_matcher_out_logger_,
            user_trigger_match_profile_provider_,
            rocksdb_manager_pool_,
            expression_matcher_config_.ChunksConfig().chunks_number(),
            chunk_trigger_folders,
            expression_matcher_config_.TriggerImpsConfig()->UserChunksConfig().chunks_prefix().c_str(),
            expression_matcher_config_.TriggerImpsConfig()->RequestChunksConfig().chunks_root().c_str(),
            expression_matcher_config_.TriggerImpsConfig()->RequestChunksConfig().chunks_prefix().c_str(),
            is_request_rocksdb_enable,
            request_rocksdb_params,
            expression_matcher_config_.TriggerImpsConfig()->positive_triggers_group_size(),
            expression_matcher_config_.TriggerImpsConfig()->negative_triggers_group_size(),
            expression_matcher_config_.TriggerImpsConfig()->max_trigger_visits(),
            profile_cache_,
            fill_level_map_traits_(expression_matcher_config_.TriggerImpsConfig()->UserChunksConfig()),
            fill_level_map_traits_(expression_matcher_config_.TriggerImpsConfig()->RequestChunksConfig())
            );

          add_child_object(user_trigger_match_container, true);

          user_trigger_match_container_ = user_trigger_match_container;
        }
      }
      catch(const eh::Exception& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4023") << FUN <<
          ": caught Exception on creating UserTriggerMatchContainer: " << ex.what();
      }
    }

    try
    {
      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_colo_reach_folders;
      AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
        chunk_colo_reach_folders,
        expression_matcher_config_.HouseholdColoReachChunksConfig().chunks_root().c_str(),
        "Chunk");
      for(auto iter = chunk_colo_reach_folders.begin();
          iter != chunk_colo_reach_folders.end();
          ++iter)
      {
        iter->second += "/HouseholdColoReach";
      }

      if(!household_colo_reach_container_.get().in())
      {
        UserColoReachContainer_var household_colo_reach_container = new UserColoReachContainer(
          logger(),
          expression_matcher_out_logger_,
          true, // household
          expression_matcher_config_.ChunksConfig().chunks_number(),
          chunk_colo_reach_folders,
          expression_matcher_config_.HouseholdColoReachChunksConfig().chunks_prefix().c_str(),
          profile_cache_,
          fill_level_map_traits_(expression_matcher_config_.HouseholdColoReachChunksConfig())
          );

        add_child_object(household_colo_reach_container, true);

        household_colo_reach_container_ = household_colo_reach_container;
      }
    }
    catch(const UserInventoryInfoContainer::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4023") << FUN <<
        ": caught Exception on creating UserInventoryInfoContainer: " << ex.what();
    }

    if(!user_inventory_container_.get().in() ||
       (expression_matcher_config_.TriggerImpsConfig().present() && (
         !user_trigger_match_container_.get().in() ||
         !temp_user_trigger_match_container_.get().in())))
    {
      // loading problems: try reload
      Generics::Goal_var load_data_msg(new LoadDataTask(this, task_runner_));
      scheduler_->schedule(load_data_msg, Generics::Time::get_time_of_day() + 5);
    }
    else
    {
      try
      {
        /* set task for getting UserInfoManagerSession */
        Generics::Task_var msg(
          new ResolveUserInfoManagerSessionTask(this, task_runner_));
        task_runner_->enqueue_task(msg);
      }
      catch(const eh::Exception& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4007") << FUN <<
          ": Can't set task for UserInfoManagerSession resolving."
          "Caught eh::Exception: " << ex.what();
      }

      try
      {
        /* set loop task for flush logs */
        Generics::Task_var msg(new FlushLogsTask(this, task_runner_));
        task_runner_->enqueue_task(msg);
      }
      catch(const eh::Exception& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4007") << FUN <<
          ": Can't set task for flush logs. Caught eh::Exception: " << ex.what();
      }

      try
      {
        /* if session resolved without problem
         * set task to update expression channels */
        Generics::Task_var update_expressions_msg(
          new UpdateExpressionChannelsTask(this, task_runner_));
        task_runner_->enqueue_task(update_expressions_msg);
      }
      catch(const eh::Exception& ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4007") << FUN << ": "
          "Can't set task for UserInfoManagerSession resolving."
          "Caught eh::Exception: " << ex.what();
      }
    }
  }

  bool ExpressionMatcherImpl::resolve_user_info_manager_session_() noexcept
  {
//  static const char* FUN = "ExpressionMatcherImpl::resolve_user_info_manager_session_()";

    typedef xsd::AdServer::Configuration::
      ExpressionMatcherConfigType::UserInfoManagerControllerGroup_sequence
      UserInfoManagerControllerGroupSeq;

    AdServer::UserInfoSvcs::UserInfoOperationDistributor::
      ControllerRefList controller_groups;

    for(UserInfoManagerControllerGroupSeq::const_iterator cg_it =
          expression_matcher_config_.UserInfoManagerControllerGroup().begin();
        cg_it != expression_matcher_config_.UserInfoManagerControllerGroup().end();
        ++cg_it)
    {
      AdServer::UserInfoSvcs::UserInfoOperationDistributor::
        ControllerRef controller_ref_group;

      Config::CorbaConfigReader::read_multi_corba_ref(
        *cg_it,
        controller_ref_group);

      controller_groups.push_back(controller_ref_group);
    }

    AdServer::UserInfoSvcs::UserInfoOperationDistributor_var distributor =
      new AdServer::UserInfoSvcs::UserInfoOperationDistributor(
        callback_->logger(),
        controller_groups,
        corba_client_adapter_.in(),
        Generics::Time::ZERO // pool timeout
      );

    user_info_manager_session_ = ReferenceCounting::add_ref(distributor);
    add_child_object(distributor);

    try_start_daily_processing_loop_();

    return true;
  }

  void
  ExpressionMatcherImpl::try_start_daily_processing_loop_() noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::try_start_daily_processing_loop_()";

    try
    {
      bool need_start = false;
      AdServer::UserInfoSvcs::UserInfoManagerSession_var user_info_manager_session;

      {
        ChannelMatcher::Config_var configuration = channel_matcher_->config();

        SyncPolicy::WriteGuard lock(lock_);
        need_start = user_info_manager_session_.in() &&
          configuration.in() &&
          !daily_processing_loop_started_;

        if(need_start)
        {
          daily_processing_loop_started_ = true;
        }
      }

      if(need_start)
      {
        Generics::Task_var clear_expired_msg(new ClearExpiredUsersTask(
          this, task_runner_, true));
        task_runner_->enqueue_task(clear_expired_msg);

        Generics::Task_var msg(new DailyCheckTask(this, task_runner_, true));
        task_runner_->enqueue_task(msg);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4008") << FUN <<
        ": Can't set daily processing task. Caught eh::Exception: " <<
        ex.what();
    }
  }

  void
  ExpressionMatcherImpl::update_expression_channels_() noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::update_expression_channels_()";
    const unsigned long PORTIONS_NUMBER = 20;

    try
    {
      logger()->log(String::SubString("To update expression channels."),
        Logging::Logger::TRACE,
        Aspect::EXPRESSION_MATCHER);

      ChannelMatcher::Config_var old_config = channel_matcher_->config();
      ChannelMatcher::Config_var new_config;

      bool interrupted = false;

      bool load_trigger_match_config = user_trigger_match_container_.get().in();

      UserTriggerMatchContainer::Config_var new_trigger_match_config;

      for (;;)
      {
        new_config = new ChannelMatcher::Config;
        if (load_trigger_match_config)
        {
          new_trigger_match_config = new UserTriggerMatchContainer::Config();
        }

        AdServer::CampaignSvcs::CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_pool_->get_object<AdServer::CampaignSvcs::CampaignServerPool::Exception>(
            logger(),
            Logging::Logger::EMERGENCY,
            Aspect::EXPRESSION_MATCHER,
            "ADS_ICON-4001",
            expression_matcher_config_.service_index(),
            expression_matcher_config_.service_index());

        try
        {
          CampaignSvcs::CampaignServer::GetExpressionChannelsInfo request_settings;
          request_settings.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
          request_settings.portions_number = PORTIONS_NUMBER;
          request_settings.channel_types = "KBEGVTA";
          request_settings.channel_statuses = "AW";
          request_settings.provide_ccg_links = false;
          request_settings.provide_channel_triggers =
            expression_matcher_config_.TriggerImpsConfig().present();
          request_settings.provide_overlap_channel_ids = false;

          for(unsigned long portion = 0;
              portion < PORTIONS_NUMBER && !(interrupted = !active());
              ++portion)
          {
            // update expressions
            CampaignSvcs::ExpressionChannelsInfo_var expression_channels_info;
            request_settings.portion = portion;
            expression_channels_info =
              campaign_server->get_expression_channels(request_settings);

            // fill expression channels index
            const CampaignSvcs::ExpressionChannelSeq& expression_channels =
              expression_channels_info->expression_channels;

            for(CORBA::ULong i = 0; i < expression_channels.length(); ++i)
            {
              CampaignSvcs::ExpressionChannelBase_var new_channel =
                CampaignSvcs::unpack_channel(
                  expression_channels[i],
                  new_config->expression_channels);
              const unsigned long CHANNEL_ID = new_channel->params().channel_id;
              new_config->all_channels.insert(CHANNEL_ID);

              ChannelMatcher::ChannelMap::iterator ch_it =
                new_config->expression_channels.find(CHANNEL_ID);

              if(ch_it != new_config->expression_channels.end())
              {
                ch_it->second->channel = new_channel;
              }
              else
              {
                new_config->expression_channels.insert(
                  std::make_pair(
                    CHANNEL_ID,
                    CampaignSvcs::ExpressionChannelHolder_var(
                      new CampaignSvcs::ExpressionChannelHolder(new_channel))));
              }
            }

            // fill trigger match index
            const CampaignSvcs::ChannelTriggersSeq& channels =
              expression_channels_info->activate_channel_triggers;

            for(CORBA::ULong i = 0; i < channels.length(); ++i)
            {
              if(channels[i].page_triggers.length() ||
                 channels[i].search_triggers.length() ||
                 channels[i].url_triggers.length() ||
                 channels[i].url_keyword_triggers.length())
              {
                UserTriggerMatchContainer::Config::ChannelInfo_var channel =
                  new UserTriggerMatchContainer::Config::ChannelInfo();

                channel->page_time_to = channels[i].page_time_to;
                channel->search_time_to = channels[i].search_time_to;
                channel->url_time_to = channels[i].url_time_to;
                channel->url_keyword_time_to = channels[i].url_keyword_time_to;
                channel->page_min_visits = channels[i].page_min_visits;
                channel->search_min_visits = channels[i].search_min_visits;
                channel->url_min_visits = channels[i].url_min_visits;
                channel->url_keyword_min_visits = channels[i].url_keyword_min_visits;

                channel->page_triggers.reserve(channels[i].page_triggers.length());
                channel->search_triggers.reserve(channels[i].search_triggers.length());
                channel->url_triggers.reserve(channels[i].url_triggers.length());
                channel->url_keyword_triggers.reserve(channels[i].url_keyword_triggers.length());
                std::copy(
                  channels[i].page_triggers.get_buffer(),
                  channels[i].page_triggers.get_buffer() + channels[i].page_triggers.length(),
                  std::back_inserter(channel->page_triggers));
                std::copy(
                  channels[i].search_triggers.get_buffer(),
                  channels[i].search_triggers.get_buffer() + channels[i].search_triggers.length(),
                  std::back_inserter(channel->search_triggers));
                std::copy(
                  channels[i].url_triggers.get_buffer(),
                  channels[i].url_triggers.get_buffer() + channels[i].url_triggers.length(),
                  std::back_inserter(channel->url_triggers));
                std::copy(
                  channels[i].url_keyword_triggers.get_buffer(),
                  channels[i].url_keyword_triggers.get_buffer() + channels[i].url_keyword_triggers.length(),
                  std::back_inserter(channel->url_keyword_triggers));

                if(load_trigger_match_config)
                {
                  new_trigger_match_config->channels.insert(
                    std::make_pair(channels[i].channel_id, channel));
                }
              }
            }

            if(load_trigger_match_config)
            {
              const CampaignSvcs::DeletedIdSeq& delete_simple_channels =
                expression_channels_info->delete_simple_channels;

              for(CORBA::ULong del_i = 0; del_i < delete_simple_channels.length(); ++del_i)
              {
                new_trigger_match_config->channels.erase(
                  delete_simple_channels[del_i].id);
              }
            }

            if(logger()->log_level() >= Logging::Logger::TRACE)
            {
              logger()->stream(Logging::Logger::TRACE,
                Aspect::EXPRESSION_MATCHER) <<
                "Expression channels updated, expression channels count: " <<
                new_config->expression_channels.size() <<
                ", active channels count: " <<
                new_config->all_channels.size() <<
                ", channels with triggers: " <<
                (new_trigger_match_config.in() ? new_trigger_match_config->channels.size() : 0);
            }
          } // portions loop

          const CampaignSvcs::CampaignServer::ColocationPropInfo_var placement_colo_props =
            campaign_server->get_colocation_prop(expression_matcher_config_.colo_id());

          placement_colo_ = new PlacementColo(
            placement_colo_props->found ?
              CorbaAlgs::unpack_time(placement_colo_props->time_offset) :
              Generics::Time());

          break;
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't update expression channels. "
            "Caught CampaignServer::ImplementationException: " <<
            ex.description;
          campaign_server.release_bad(ostr.str());
          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::EXPRESSION_MATCHER,
            "ADS-IMPL-4009");
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
        {
          String::SubString descr("CampaignServer not ready.");
          campaign_server.release_bad(descr);
          logger()->log(descr,
            Logging::Logger::NOTICE,
            Aspect::EXPRESSION_MATCHER,
            "ADS-ICON-4001");
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't update expression channels. Caught CORBA::SystemException: " <<
            ex;
          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::EXPRESSION_MATCHER,
            "ADS-ICON-4001");
          campaign_server.release_bad(ostr.str());
        }
      } // for (;;)

      if(!interrupted)
      {
        // spread target flag in depth
        for(auto ch_it = new_config->expression_channels.begin();
          ch_it != new_config->expression_channels.end(); ++ch_it)
        {
          if(ch_it->second->has_params() &&
            ch_it->second->params().common_params.in() &&
             (ch_it->second->params().common_params->flags &
              AdServer::CampaignSvcs::ChannelFlags::TARGETED))
          {
            ChannelIdSet modify_channels;
            ch_it->second->channel->get_all_channels(modify_channels);
            for(ChannelIdSet::const_iterator mch_id_it = modify_channels.begin();
              mch_id_it != modify_channels.end(); ++mch_id_it)
            {
              auto mch_it = new_config->expression_channels.find(*mch_id_it);
              if(mch_it != new_config->expression_channels.end() &&
                mch_it->second->has_params() &&
                 mch_it->second->params().common_params.in())
              {
                mch_it->second->params().common_params->flags =
                  mch_it->second->params().common_params->flags |
                    AdServer::CampaignSvcs::ChannelFlags::TARGETED;
              }
            }
          }
        }

        if(load_trigger_match_config)
        {
          for(auto ch_it = new_trigger_match_config->channels.begin();
            ch_it != new_trigger_match_config->channels.end();
            ++ch_it)
          {
            auto mch_it = new_config->expression_channels.find(ch_it->first);
            if(mch_it == new_config->expression_channels.end() ||
              !mch_it->second->has_params() ||
              !mch_it->second->params().common_params.in() ||
              !(mch_it->second->params().common_params->flags &
                AdServer::CampaignSvcs::ChannelFlags::TARGETED))
            {
              auto channel = ch_it->second;
              channel->page_min_visits = std::min(channel->page_min_visits, 1ul);
              channel->search_min_visits = std::min(channel->search_min_visits, 1ul);
              channel->url_min_visits = std::min(channel->url_min_visits, 1ul);
              channel->url_keyword_min_visits = std::min(channel->url_keyword_min_visits, 1ul);
            }
          }
        }

        // fill used, but not loaded from CampaignServer channels
        // as simple channels (Geo or Platform)
        //
        for(ChannelMatcher::ChannelMap::iterator ch_it =
              new_config->expression_channels.begin();
            ch_it != new_config->expression_channels.end(); ++ch_it)
        {
          if(!ch_it->second->channel.in())
          {
            AdServer::CampaignSvcs::ChannelParams channel_params(ch_it->first);
            channel_params.type = 'G';
            ch_it->second->channel = new AdServer::CampaignSvcs::SimpleChannel(
              channel_params);
          }
        }

        if(load_trigger_match_config)
        {
          user_trigger_match_container_.get()->config(new_trigger_match_config);
          temp_user_trigger_match_container_.get()->config(new_trigger_match_config);
        }

        Generics::Time now = Generics::Time::get_time_of_day();
        Generics::Time now_date = now.get_gm_time().get_date();
        new_config->fill_time = now;
        channel_matcher_->config(new_config);

        if(logger()->log_level() >= Logging::Logger::TRACE)
        {
          logger()->stream(Logging::Logger::TRACE,
            Aspect::EXPRESSION_MATCHER) <<
            "Expression channels indexed.";
        }

        {
          // process only new appeared channels or all if date changed
          ChannelIdSet appear_channels_holder;
          const ChannelIdSet* use_appear_channels = &new_config->all_channels;

          if(old_config.in() &&
             now_date == old_config->fill_time.get_gm_time().get_date())
          {
            std::set_difference(
              new_config->all_channels.begin(),
              new_config->all_channels.end(),
              old_config->all_channels.begin(),
              old_config->all_channels.end(),
              std::inserter(appear_channels_holder, appear_channels_holder.begin()));

            use_appear_channels = &appear_channels_holder;
          }

          expression_matcher_out_logger_->process_channel_activity(
            now_date,
            expression_matcher_config_.colo_id(),
            *use_appear_channels);
        }

        if(old_config.in() == 0)
        {
          const xsd::AdServer::Configuration::InLogsType&
            lp_config = expression_matcher_config_.LogProcessing().InLogs();

          ExpressionMatcherLogLoader::LogReadTraitsList log_read_traits;

          log_read_traits.emplace_back(
            make_log_read_traits(
                ExpressionMatcherLogLoader::LogType::RequestBasicChannels,
              lp_config.log_root().c_str(),
              lp_config.RequestBasicChannels(),
              DEFAULT_REQUEST_BASIC_CHANNELS_BASE_NAME));
          log_read_traits.emplace_back(
            make_log_read_traits(
                ExpressionMatcherLogLoader::LogType::ConsiderClick,
              lp_config.log_root().c_str(),
              lp_config.ConsiderClick(),
              DEFAULT_CONSIDER_CLICK_BASE_NAME));
          log_read_traits.emplace_back(
            make_log_read_traits(
                ExpressionMatcherLogLoader::LogType::ConsiderImpression,
              lp_config.log_root().c_str(),
              lp_config.ConsiderImpression(),
              DEFAULT_CONSIDER_IMPRESSION_BASE_NAME));

          log_loader_ = new ExpressionMatcherLogLoader(
            this,
            this,
            task_runner_,
            scheduler_,
            logger(),
            callback_,
            expression_matcher_config_.LogProcessing().threads(),
            log_read_traits,
            expression_matcher_config_.LogProcessing().InLogs().check_logs_period());

          add_child_object(log_loader_);

          /* start processing loop */
          Commons::make_goal_task(
            std::bind(
              &ExpressionMatcherImpl::update_stats_,
              this),
            task_runner_,
            scheduler_,
            Generics::Time(expression_matcher_config_.LogProcessing().InLogs().check_logs_period())
            )->deliver();
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-ICON-4001") << FUN <<
        ": Can't update expression channels. Caught eh::Exception: " <<
        ex.what();
    }

    try
    {
      /* schedule next expressions updating task */
      Generics::Time tm = Generics::Time::get_time_of_day() +
        expression_matcher_config_.update_period();

      Generics::Goal_var msg(new UpdateExpressionChannelsTask(this, task_runner_));
      scheduler_->schedule(msg, tm);
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4010") << FUN <<
        ": Can't set task for expression channels updating. "
        "Caught eh::Exception: " << ex.what();
    }

    try_start_daily_processing_loop_();
  }

  void
  ExpressionMatcherImpl::consider_impression(
    const AdServer::Commons::UserId& user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& time,
    const ChannelIdSet& channels)
    noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::consider_impression()";

    try
    {
      UserTriggerMatchContainer_var user_trigger_match_container =
        user_trigger_match_container_.get();

      if(!channels.empty() && user_trigger_match_container.in())
      {
        UserTriggerMatchContainer::ImpressionInfo triggers_imp;
        triggers_imp.user_id = user_id;
        triggers_imp.request_id = request_id;
        triggers_imp.time = time + placement_colo_.get()->time_offset;
        triggers_imp.channels = channels;

        user_trigger_match_container->process_impression(triggers_imp);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER) << FUN <<
        ": Caught eh::Exception: " << ex.what();
    }
  }

  void
  ExpressionMatcherImpl::consider_click(
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& time)
    noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::consider_click()";

    try
    {
      UserTriggerMatchContainer_var user_trigger_match_container =
        user_trigger_match_container_.get();

      if(user_trigger_match_container.in())
      {
        user_trigger_match_container->process_click(
          request_id,
          time + placement_colo_.get()->time_offset);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER) << FUN <<
        ": Caught eh::Exception: " << ex.what();
    }
  }

  void
  ExpressionMatcherImpl::run_daily_processing(bool sync)
    /*throw(AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady,
      AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException)*/
  {
    static const char* FUN = "ExpressionMatcherImpl::run_daily_processing()";

    try
    {
      {
        AdServer::UserInfoSvcs::UserInfoManagerSession_var user_info_manager_session;
        ChannelMatcher::Config_var configuration;

        {
          SyncPolicy::WriteGuard lock(lock_);
          user_info_manager_session = user_info_manager_session_;
          configuration = channel_matcher_->config();
        }

        if(!(user_info_manager_session.in() && configuration.in()))
        {
          throw AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady();
        }
      }

      if(sync)
      {
        clear_expired_users_(false);
        daily_check_users_impl_(0);
      }
      else
      {
        Generics::Task_var clear_expired_msg(new ClearExpiredUsersTask(
          this, 0, false));
        task_runner_->enqueue_task(clear_expired_msg);

        Generics::Task_var msg(new DailyCheckTask(this, task_runner_, false, 0));
        task_runner_->enqueue_task(msg);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<
        RequestInfoSvcs::ExpressionMatcher::ImplementationException>(
          ostr.str());
    }
  }

  bool
  ExpressionMatcherImpl::process_requests(
    LogProcessing::FileReceiver::FileGuard* file_ptr,
    std::size_t& processed_lines_count)
    /*throw(eh::Exception)*/
  {
    LogProcessing::FileReceiver::FileGuard_var file(ReferenceCounting::add_ref(file_ptr));
    const std::string file_name = file->full_path();

    UserTriggerMatchContainer_var user_trigger_match_container =
      user_trigger_match_container_.get();
    UserTriggerMatchContainer_var temp_user_trigger_match_container =
      temp_user_trigger_match_container_.get();

    UserInventoryInfoContainer_var user_inventory_container =
      user_inventory_container_.get();
    UserColoReachContainer_var household_colo_reach_container =
      household_colo_reach_container_.get();

    AdServer::LogProcessing::RequestBasicChannelsCollector req_collector;

    /* load requests file */
    std::ifstream ifs(file_name.c_str());
    AdServer::LogProcessing::RequestBasicChannelsTraits::IoHelperType(
      req_collector).load(ifs);

    AdServer::LogProcessing::LogFileNameInfo name_info;
    parse_log_file_name(file->file_name().c_str(), name_info);

    /* process requests collector */
    if (!req_collector.empty())
    {
      for(AdServer::LogProcessing::RequestBasicChannelsCollector::const_iterator
            coll_it = req_collector.begin();
          coll_it != req_collector.end(); ++coll_it)
      {
        for(AdServer::LogProcessing::
              RequestBasicChannelsInnerCollector::const_iterator req_it =
                coll_it->second.begin();
            req_it != coll_it->second.end(); ++req_it, ++processed_lines_count)
        {
          if (processed_lines_count >= name_info.processed_lines_count)
          {
            if (!active())
            {
              return true;
            }

            // configuration pointer copy life time must be short
            process_request_basic_channels_record_(
              user_inventory_container,
              user_trigger_match_container,
              temp_user_trigger_match_container,
              household_colo_reach_container,
              coll_it->first,
              *req_it);
          }
        }
      }
    }

    stats_.set_last_processed_timestamp(name_info.timestamp);
    return false;
  }

  bool
  ExpressionMatcherImpl::check_sampling_(const UserId& user_id) const noexcept
  {
    return (user_id.is_null() ||
      user_id.hash() % 100 < expression_matcher_config_.inventory_users_percentage());
  }

  void
  ExpressionMatcherImpl::process_request_basic_channels_record_(
    UserInventoryInfoContainer* user_inventory_container,
    UserTriggerMatchContainer* user_trigger_match_container,
    UserTriggerMatchContainer* temp_user_trigger_match_container,
    UserColoReachContainer* household_colo_reach_container,
    const AdServer::LogProcessing::
      RequestBasicChannelsCollector::KeyT& key,
    const AdServer::LogProcessing::
      RequestBasicChannelsCollector::DataT::DataT& record)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpressionMatcherImpl::process_request_basic_channels_record_()";

    typedef AdServer::LogProcessing::
      RequestBasicChannelsCollector::DataT::DataT RBCRecord;

    try
    {
      if(record.user_type() != 'H')
      {
        /* process one request */
        MatchRequestProcessor::MatchInfo match_info;
        const bool sampling_flag = check_sampling_(record.user_id());

        if(record.match_request().present() && sampling_flag)
        {
          const CampaignSvcs::ChannelIdSet history_channels(
            record.match_request().get().history_channels().begin(),
            record.match_request().get().history_channels().end());
          CampaignSvcs::ChannelIdSet result_channels;

          channel_matcher_->process_request(
            history_channels,
            result_channels,
            &match_info.triggered_cpm_expression_channels,
            &match_info.channel_actions);

          match_info.triggered_expression_channels.fill();
          match_info.triggered_expression_channels->assign(
            result_channels.begin(), result_channels.end());
        }

        match_info.user_id = record.user_id();
        match_info.merge_request = record.user_type() == 'P' ||
          !record.temporary_user_id().is_null();
        match_info.time = key.time();
        match_info.isp_time = key.isp_time();
        match_info.placement_colo_time = match_info.time + placement_colo_.get()->time_offset;
        match_info.colo_id = key.colo_id();
        match_info.max_text_ads = 0;

        if(record.ad_request().present())
        {
          match_info.ad_request = true;
          const RBCRecord::AdRequestProps& ad_request_props = record.ad_request().get();

          if (!ad_request_props.sizes().empty())
          {
            match_info.tag_size = ad_request_props.sizes().front();
            for (auto it = ad_request_props.sizes().begin();
              it != ad_request_props.sizes().end(); ++it)
            {
              match_info.sizes.insert(Commons::ImmutableString(*it));
            }
          }

          std::string country_code(ad_request_props.country_code());
          String::AsciiStringManip::to_upper(country_code);
          match_info.country_code = std::move(country_code);
          match_info.max_text_ads = ad_request_props.max_text_ads();
          match_info.cost_threshold = ad_request_props.text_ad_cost_threshold();
          match_info.auction_type = ad_request_props.auction_type();

          if (sampling_flag)
          {
            if(ad_request_props.display_ad_shown().present())
            {
              MatchRequestProcessor::MatchInfo::AdSlot display_ad;
              display_ad.avg_revenue = ad_request_props.display_ad_shown().get().revenue();
              std::copy(
                ad_request_props.display_ad_shown().get().impression_channels().begin(),
                ad_request_props.display_ad_shown().get().impression_channels().end(),
                std::inserter(display_ad.imp_channels, display_ad.imp_channels.begin()));
              match_info.display_ad = display_ad;
            }

            for(RBCRecord::AdBidSlotImpressionList::const_iterator text_imp_it =
                  ad_request_props.text_ad_shown().begin();
                text_imp_it != ad_request_props.text_ad_shown().end();
                ++text_imp_it)
            {
              MatchRequestProcessor::MatchInfo::AdBidSlot text_ad;
              text_ad.avg_revenue = text_imp_it->revenue();
              text_ad.max_avg_revenue = text_imp_it->revenue_bid();
              std::copy(
                text_imp_it->impression_channels().begin(),
                text_imp_it->impression_channels().end(),
                std::inserter(text_ad.imp_channels, text_ad.imp_channels.begin()));
              match_info.text_ads.push_back(text_ad);
            }
          }
        }

        user_inventory_container->process_match_request(match_info);

        expression_matcher_out_logger_->process_match_request(match_info);

        if((!record.user_id().is_null() && user_trigger_match_container) ||
           (!record.temporary_user_id().is_null() && temp_user_trigger_match_container))
        {
          UserTriggerMatchContainer::RequestInfo request_info;
          request_info.time = match_info.placement_colo_time;

          if(record.match_request().present())
          {
            const AdServer::LogProcessing::RequestBasicChannelsInnerData::
              Match& match_request = record.match_request().get();

            for(AdServer::LogProcessing::
                  RequestBasicChannelsInnerData::TriggerMatchList::
                    const_iterator cht_it = match_request.page_trigger_channels().begin();
                cht_it != match_request.page_trigger_channels().end(); ++cht_it)
            {
              request_info.page_matches[cht_it->channel_id].push_back(
                cht_it->channel_trigger_id);
            }

            for(AdServer::LogProcessing::
                  RequestBasicChannelsInnerData::TriggerMatchList::
                    const_iterator cht_it = match_request.search_trigger_channels().begin();
                cht_it != match_request.search_trigger_channels().end(); ++cht_it)
            {
              request_info.search_matches[cht_it->channel_id].push_back(
                cht_it->channel_trigger_id);
            }

            for(AdServer::LogProcessing::
                  RequestBasicChannelsInnerData::TriggerMatchList::
                    const_iterator cht_it = match_request.url_trigger_channels().begin();
                cht_it != match_request.url_trigger_channels().end(); ++cht_it)
            {
              request_info.url_matches[cht_it->channel_id].push_back(
                cht_it->channel_trigger_id);
            }

            for(AdServer::LogProcessing::
                  RequestBasicChannelsInnerData::TriggerMatchList::
                    const_iterator cht_it = match_request.url_keyword_trigger_channels().begin();
                cht_it != match_request.url_keyword_trigger_channels().end(); ++cht_it)
            {
              request_info.url_keyword_matches[cht_it->channel_id].push_back(
                cht_it->channel_trigger_id);
            }
          }

          if(!record.user_id().is_null())
          {
            request_info.user_id = record.user_id();
            request_info.merge_user_id = record.temporary_user_id();
            user_trigger_match_container->process_request(request_info);
            stats_.inc_persistent_user_processed();
          }
          else
          {
            request_info.user_id = record.temporary_user_id();
            temp_user_trigger_match_container->process_request(request_info);
            stats_.inc_temporary_user_processed();
          }
        }
        else
        {
          stats_.inc_not_optedin_user_processed();
        }
      }
      else if(household_colo_reach_container &&
         !record.user_id().is_null())
      {
        UserColoReachContainer::RequestInfo request_info;
        request_info.user_id = record.user_id();
        request_info.time = key.time();
        request_info.isp_time = key.isp_time();
        request_info.colo_id = key.colo_id();

        household_colo_reach_container->process_request(request_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ExpressionMatcherImpl::update_stats_() noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::update_stats_()";

    try
    {
      if (proc_stat_impl_.in())
      {
        proc_stat_impl_->fill_values(stats_.get_stats());
      }
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4014") << FUN <<
        "Can't updated stats. Caught eh::Exception: " << ex.what();
    }
  }

  void
  ExpressionMatcherImpl::flush_logs_() noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::flush_logs_()";

    logger()->log(String::SubString("flush logs"),
      TraceLevel::MIDDLE,
      Aspect::EXPRESSION_MATCHER);

    try
    {
      expression_matcher_out_logger_->flush_if_required();
    }
    catch (const eh::Exception &ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4017") << FUN <<
        ": eh::Exception caught: " << ex.what();
    }

    try
    {
      Generics::Goal_var task(new FlushLogsTask(this, task_runner_));

      Generics::Time tm = Generics::Time::get_time_of_day() +
        check_loggers_period_;

      scheduler_->schedule(task, tm);
    }
    catch (const eh::Exception &ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER,
        "ADS-IMPL-4018") << FUN <<
        ": Can't schedule next flush task. eh::Exception caught: " << ex.what();
    }
  }

  void
  ExpressionMatcherImpl::clear_expired_users_(bool reschedule) noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::clear_expired_users_()";

    logger()->sstream(Logging::Logger::INFO,
      Aspect::EXPRESSION_MATCHER) << FUN <<
      ": Cleanup of expired users started";

    UserInventoryInfoContainer_var user_inventory_container =
      user_inventory_container_.get();
    UserTriggerMatchContainer_var user_trigger_match_container =
      user_trigger_match_container_.get();
    UserTriggerMatchContainer_var temp_user_trigger_match_container =
      temp_user_trigger_match_container_.get();
    UserColoReachContainer_var household_colo_reach_container =
      household_colo_reach_container_.get();

    if(user_inventory_container.in())
    {
      try
      {
        user_inventory_container->clear_expired_users();
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4020") << FUN <<
          ": Can't clear expired users from UserInventoryContainer, "
          "eh::Exception caught: " << ex.what();
      }
    }

    if(user_trigger_match_container.in())
    {
      try
      {
        user_trigger_match_container->clear_expired();
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4020") << FUN <<
          ": Can't clear expired users from UserTriggerMatchContainer, "
          "eh::Exception caught: " << ex.what();
      }
    }

    if(temp_user_trigger_match_container.in())
    {
      try
      {
        temp_user_trigger_match_container->clear_expired();
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4020") << FUN <<
          ": Can't clear expired temporary users from UserTriggerMatchContainer, "
          "eh::Exception caught: " << ex.what();
      }
    }

    if(household_colo_reach_container.in())
    {
      try
      {
        household_colo_reach_container->clear_expired();
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4020") << FUN <<
          ": Can't clear expired household users from UserColoReachContainer, "
          "eh::Exception caught: " << ex.what();
      }
    }

    logger()->sstream(Logging::Logger::INFO,
      Aspect::EXPRESSION_MATCHER) << FUN <<
      ": Cleanup of expired users finished";

    if(reschedule)
    {
      try
      {
        Generics::Goal_var task(new ClearExpiredUsersTask(
          this,
          task_runner_,
          true // reschedule
          ));

        Generics::Time cleanup_period;

        if(expression_matcher_config_.TriggerImpsConfig().present())
        {
          cleanup_period = std::min(
            Generics::Time(std::max(
              std::min(
                expression_matcher_config_.TriggerImpsConfig()->UserChunksConfig().expire_time() / 4, // minutes
                expression_matcher_config_.TriggerImpsConfig()->TempUserChunksConfig().expire_time() / 4 // minutes
                ),
              1ull)), // minumum one cleanup per minute
            Generics::Time::ONE_DAY);
        }
        else
        {
          cleanup_period = Generics::Time::ONE_DAY;
        }

        scheduler_->schedule(task, Generics::Time::get_time_of_day() + cleanup_period);
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4021") << FUN <<
          ": Can't schedule next users cleanup task. eh::Exception caught: " << ex.what();
      }
    }
  }

  ExpressionMatcherImpl::DailyCheckState_var
  ExpressionMatcherImpl::daily_check_users_impl_(
    DailyCheckState* initial_state) noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::daily_check_users_impl_()";

    Generics::CPUTimer timer;
    timer.start();

    logger()->log(String::SubString("Daily check started"),
      Logging::Logger::INFO,
      Aspect::EXPRESSION_MATCHER_DAILY_CHECK);

    DailyCheckState_var state = ReferenceCounting::add_ref(initial_state);
    size_t users_count = 0;

    UserInventoryInfoContainer_var user_inventory_container =
      user_inventory_container_.get();

    if(!user_inventory_container.in())
    {
      return state;
    }

    try
    {
      // clear excess users for relieve users processing
      user_inventory_container->clear_expired_users();

      SyncPolicy::WriteGuard lock(daily_processing_lock_);

      const Generics::Time now(Generics::Time::get_time_of_day());

      if(!state.in())
      {
        state = new DailyCheckState;
        state->start_time = Generics::Time::get_time_of_day();
        user_inventory_container->all_users(state->users);
      }

      size_t processed_count = 0;
      size_t tasks_count = 0;
      auto thread_pool_size = expression_matcher_config_.DailyProcessing().thread_pool_size();
      UsersChunkArray chunk_array(thread_pool_size);
      users_count = split_users(*state, chunk_array, thread_pool_size);

      for (auto ch = chunk_array.begin();
        ch != chunk_array.end() && active(); ++ch, ++tasks_count)
      {
        daily_processing_task_runner_->enqueue_task(
          AdServer::Commons::make_delegate_task(
            std::bind(
              &ExpressionMatcherImpl::daily_check_users_thread_,
              this,
              ch - chunk_array.begin() + 1,
              now,
              user_inventory_container,
              std::ref(*ch),
              std::ref(processed_count))));
      }

      {
        Sync::ConditionalGuard lock(cond_);

        while (processed_count < tasks_count)
        {
          lock.wait();
        }
      }

      merge_users(chunk_array, *state);
    }
    catch (const eh::Exception &ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER_DAILY_CHECK,
        "ADS-IMPL-4020") << FUN <<
        ": Can't finish daily processing - " <<
        state->users.size() << " users left"
        ": eh::Exception caught: " << ex.what();
    }

    if(!state->unprocessed_users.empty())
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        Aspect::EXPRESSION_MATCHER_DAILY_CHECK,
        "ADS-IMPL-4020") << FUN <<
        ": Can't finish daily processing for " <<
        state->unprocessed_users.size() <<
        " users (lost connection to some UIM's)";
    }

    if(logger()->log_level() >= Logging::Logger::INFO)
    {
      size_t users_left = state.in() ? state->users.size() : 0;
      Stream::Error ostr;
      ostr << "Daily check " <<
        (state.in() && state->users.empty() ? "full" : "partly") <<
        " finished: " <<
        users_count - users_left << " users processed, ";

      if(state.in())
      {
        ostr << users_left;
      }
      else
      {
        ostr << "all";
      }

      ostr << " users left";

      timer.stop();
      ostr << ". Elapsed time: " << timer.elapsed_time();

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::EXPRESSION_MATCHER_DAILY_CHECK);
    }

    return state;
  }

  void
  ExpressionMatcherImpl::daily_check_users_thread_(
    int thread_number,
    const Generics::Time& now,
    UserInventoryInfoContainer* user_inventory_container,
    UsersChunk& users,
    size_t& processed_count) noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::daily_check_users_thread_()";

    Generics::CPUTimer timer;
    timer.start();

    const Generics::Time now_date(now.get_gm_time().get_date());
    const Generics::Time placement_colo_now_date(
      (now + placement_colo_.get()->time_offset).get_gm_time().get_date());

    unsigned long processed_user_count = 0;

    for(UserIdList::iterator user_it = users.users.begin();
        user_it != users.users.end() && this->active();
        ++processed_user_count)
    {
      try
      {
        if((processed_user_count + 1) % 1000 == 0 &&
           logger()->log_level() >= Logging::Logger::INFO)
        {
          logger()->stream(Logging::Logger::INFO,
            Aspect::EXPRESSION_MATCHER_DAILY_CHECK) << FUN <<
            ": processed " << processed_user_count + 1 << " users by thread " << thread_number;
        }

        Generics::Time last_process_time;

        // check_sampling_ process case when sampling changed and
        // user that sampled present in container
        bool need_process = check_sampling_(*user_it) &&
          user_inventory_container->get_last_daily_processing_time(
            *user_it, last_process_time) &&
          (last_process_time.get_gm_time().get_date() != now_date);

        if(need_process)
        {
          try
          {
            daily_check_user_impl_(*user_it, now, placement_colo_now_date, user_inventory_container);
          }
          catch(const AdServer::UserInfoSvcs::
            UserInfoManager::ImplementationException& ex)
          {
            // user must be ignored after implement
            // native ImplementationException catch (from UIM)
            /*
            logger()->sstream(Logging::Logger::EMERGENCY,
              Aspect::EXPRESSION_MATCHER_DAILY_CHECK,
              "ADS-IMPL-4019") << FUN <<
              ": Caught UserInfoManager::ImplementationException: " <<
              ex.description;
            */
            users.unprocessed_users.push_back(*user_it);
          }
          catch(const AdServer::UserInfoSvcs::
            UserInfoManager::NotReady& )
          {
            users.unprocessed_users.push_back(*user_it);
          }
          catch(const CORBA::SystemException& ex)
          {
            users.unprocessed_users.push_back(*user_it);
          }
        } // need_process
      }
      catch (const eh::Exception& ex)
      {
        // user will be ignored
        logger()->sstream(Logging::Logger::ERROR,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4020") << FUN << ": Can't process user '" << *user_it <<
          "': caught eh::Exception:" << ex.what();
      }

      users.users.erase(user_it++);
    } // users loop

    Sync::ConditionalGuard lock(cond_);
    ++processed_count;
    cond_.broadcast();

    if (logger()->log_level() >= Logging::Logger::INFO)
    {
      timer.stop();
      logger()->stream(Logging::Logger::INFO,
        Aspect::EXPRESSION_MATCHER_DAILY_CHECK) << FUN <<
        ": thread " << thread_number <<
        " has finished. " << processed_user_count <<
        " users have been processed. Elapsed time: " << timer.elapsed_time();
    }
  }

  void
  ExpressionMatcherImpl::daily_check_user_impl_(
    const UserId& user_id,
    const Generics::Time& now,
    const Generics::Time& placement_colo_now_date,
    UserInventoryInfoContainer* user_inventory_container)
  {
    static const char* FUN = "ExpressionMatcherImpl::daily_check_user_impl_()";

    AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
    match_params.use_empty_profile = false;
    match_params.silent_match = true;
    match_params.no_match = false;
    match_params.no_result = false;
    match_params.ret_freq_caps = false;
    match_params.provide_channel_count = false;
    match_params.provide_persistent_channels = true;
    match_params.filter_contextual_triggers = false;
    match_params.publishers_optin_timeout =
      CorbaAlgs::pack_time(Generics::Time::ZERO);

    AdServer::UserInfoSvcs::UserInfo user_info;

    user_info.user_id = CorbaAlgs::pack_user_id(user_id);
    user_info.last_colo_id = -1;
    user_info.current_colo_id = -1;
    user_info.request_colo_id = -1;
    user_info.temporary = false;
    user_info.time = now.tv_sec;

    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var match_result;
    CORBA::String_var hostname;

    user_info_manager_session_->match(
      user_info,
      match_params,
      match_result);

    ChannelIdSet history_channels;

    UserInventoryInfoContainer::InventoryDailyMatchInfo daily_match_info;

    for (CORBA::ULong hi = 0; hi < match_result->channels.length(); ++hi)
    {
      history_channels.insert(match_result->channels[hi].channel_id);
    }

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Result of history match for '" << user_id << "': ";
      Algs::print(ostr, history_channels.begin(), history_channels.end());

      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::EXPRESSION_MATCHER);
    }

    daily_match_info.user_id = user_id;
    daily_match_info.time = placement_colo_now_date;
    daily_match_info.colo_id = expression_matcher_config_.colo_id();

    CampaignSvcs::ChannelIdSet tr_channels;
    channel_matcher_->process_request(
      history_channels, tr_channels);

    std::copy(tr_channels.begin(),
      tr_channels.end(),
      std::back_inserter(daily_match_info.triggered_expression_channels));

    user_inventory_container->process_user(daily_match_info);
  }

  void
  ExpressionMatcherImpl::daily_check_users_(
    bool set_next_task,
    DailyCheckState* prev_state)
    noexcept
  {
    static const char* FUN = "ExpressionMatcherImpl::daily_check_users_()";

    DailyCheckState_var state = daily_check_users_impl_(prev_state);

    if(set_next_task)
    {
      try
      {
        const Generics::Time placement_colo_time_offset = placement_colo_.get()->time_offset;
        // all logic in placement colo time
        const Generics::Time now = Generics::Time::get_time_of_day() + placement_colo_time_offset;

        Generics::Time tm;
        DailyCheckState_var next_state;

        if(!state.in() || !state->users.empty())
        {
          tm = now + Generics::Time::ONE_MINUTE;
          next_state = state;
        }
        // some users isn't processed due to errors
        // process only these users after hour if day will not be changed
        else if(!state->unprocessed_users.empty() &&
          state->start_time.get_gm_time().get_date() ==
            (now + Generics::Time::ONE_HOUR).get_gm_time().get_date())
        {
          tm = now + Generics::Time::ONE_HOUR;
          next_state = new DailyCheckState();
          next_state->users.swap(state->unprocessed_users);
          next_state->start_time = tm;
        }
        else
        {
          const Generics::Time daily_time(
            expression_matcher_config_.DailyProcessing().processing_time(),
            "%H:%M");

          Generics::ExtendedTime start_date(state->start_time.get_gm_time().get_date());
          Generics::ExtendedTime now_date(now.get_gm_time().get_date());

          if(start_date == now_date)
          {
            tm = start_date + daily_time + Generics::Time::ONE_DAY;
          }
          else if(now < now_date + daily_time)
          {
            tm = now_date + daily_time;
          }
          else
          {
            tm = now + Generics::Time::ONE_MINUTE;
          }
        }

        Generics::Goal_var task(new DailyCheckTask(
          this,
          task_runner_,
          true, // reschedule
          next_state));

        tm -= placement_colo_time_offset; // back to UTC time
        scheduler_->schedule(task, tm);

        logger()->sstream(Logging::Logger::INFO,
          Aspect::EXPRESSION_MATCHER) << FUN <<
          ": Daily task scheduled for '" <<
          tm.get_gm_time() << "' start time of prev task '" <<
          (state ? state->start_time :
           Generics::Time::ZERO).get_gm_time() << "'";
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER,
          "ADS-IMPL-4021") << FUN <<
          ": Can't schedule next daily task. eh::Exception caught: " << ex.what();
      }
    }
  }

  AdServer::ProfilingCommons::LevelMapTraits
  ExpressionMatcherImpl::fill_level_map_traits_(
    const xsd::AdServer::Configuration::LevelChunksConfigType& chunks_config)
    noexcept
  {
    return AdServer::ProfilingCommons::LevelMapTraits(
      AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
      chunks_config.rw_buffer_size(),
      chunks_config.rwlevel_max_size(),
      chunks_config.max_undumped_size(),
      chunks_config.max_levels0(),
      Generics::Time(chunks_config.expire_time()),
      file_controller_);
  }

  /*
   * This function splits list to chunks.
   * Complexity: O(n), function iterates source list twice (state.users.size(), std::advance()).
   * Size of all chunks are equal or first chunk is biggest and other chunks are equal.
   */
  size_t
  ExpressionMatcherImpl::split_users(
    ExpressionMatcherImpl::DailyCheckState& state,
    ExpressionMatcherImpl::UsersChunkArray& chunk_array,
    size_t blocks)
  {
    size_t length = state.users.size();
    size_t block_size = length / blocks;
    for (size_t i = 0; i < blocks; ++i)
    {
      auto b = state.users.begin();
      auto e = b;
      std::advance(e, i ? block_size : block_size + length % blocks);
      chunk_array[i].users.splice(chunk_array[i].users.begin(), state.users, b, e);
    }
    return length;
  }

  void
  ExpressionMatcherImpl::merge_users(
    ExpressionMatcherImpl::UsersChunkArray& chunk_array,
    ExpressionMatcherImpl::DailyCheckState& state)
  {
    for (auto it = chunk_array.begin(); it != chunk_array.end(); ++it)
    {
      state.users.splice(state.users.end(), it->users);
      state.unprocessed_users.splice(state.unprocessed_users.end(), it->unprocessed_users);
    }
  }
} /*LogProcessing*/
} /*AdServer*/
