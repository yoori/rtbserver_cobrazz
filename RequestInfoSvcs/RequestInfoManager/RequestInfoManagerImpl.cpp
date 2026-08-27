#include <memory>
#include <utility>
#include <vector>

#include <PrivacyFilter/Filter.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/GrpcAlgs.hpp>

#include <xsd/RequestInfoSvcs/RequestInfoManagerConfig.hpp>

#include <UserInfoSvcs/UserInfoClient/UserInfoDistributedGrpcClient.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoGrpcAlgs.hpp>

#include <Commons/LogReferrerUtils.hpp>

#include "RequestInfoContainer.hpp"
#include "TagRequestProfiler.hpp"
#include "CompositeTagRequestProcessor.hpp"
#include "CompositeAdvActionProcessor.hpp"
#include "BillingProcessor.hpp"
#include "RequestInfoManagerStats.hpp"
#include "RequestInfoManagerImpl.hpp"

/**
 * At input CampaignManager logs:
 *   Request,
 *   Impression,
 *   Click,
 *   AdvertiserAction,
 *   TagRequest
 */
namespace
{
  const unsigned long UIS_RESOLVE_PERIOD = 5; // 5 sec
  const unsigned long CHUNKS_RELOAD_PERIOD = 30; // 30 sec
  const unsigned long UPDATE_FRAUD_RULES_PERIOD = 60;

  const char CREATIVE_STAT_OUT_DIR[] = "CreativeStat";
  const char USER_PROPERTIES_OUT_DIR[] = "UserProperties";
  const char CHANNEL_PERFORMANCE_OUT_DIR[] = "ChannelPerformance";
  const char SITE_CHANNEL_STAT_OUT_DIR[] = "SiteChannelStat";
  const char EXPRESSION_PERFORMANCE_OUT_DIR[] = "ExpressionPerformance";
  const char CCG_KEYWORD_STAT_OUT_DIR[] = "CCGKeywordStat";
  const char CMP_STAT_OUT_DIR[] = "CMPStat";
  const char ACTION_STAT_OUT_DIR[] = "ActionStat";
  const char CHANNEL_IMP_INVENTORY_OUT_DIR[] = "ChannelImpInventory";
  const char TAG_POSITION_STAT_OUT_DIR[] = "TagPositionStat";
  const char CAMPAIGN_REFERRER_STAT_OUT_DIR[] = "CampaignReferrerStat";
  const char BID_COST_STAT_OUT_DIR[] = "BidCostStat";

  const char CCG_USER_STAT_OUT_DIR[] = "CCGUserStat";
  const char CC_USER_STAT_OUT_DIR[] = "CCUserStat";
  const char CAMPAIGN_USER_STAT_OUT_DIR[] = "CampaignUserStat";
  const char ADVERTISER_USER_OUT_DIR[] = "AdvertiserUserStat";

  const char PASSBACK_STAT_OUT_DIR[] = "PassbackStat";

  const char SITE_USER_STAT_OUT_DIR[] = "SiteUserStat";
  const char SITE_REFERRER_OUT_DIR[] = "SiteReferrerStat";
  const char PAGE_LOADS_DAILY_STAT_OUT_DIR[] = "PageLoadsDailyStat";

  const char REQUEST_OPERATION_OUT_DIR[] = "RequestOperation";

  const char RESEARCH_ACTION_OUT_DIR[] = "ResearchAction";
  const char RESEARCH_BID_OUT_DIR[] = "ResearchBid";
  const char RESEARCH_IMPRESSION_OUT_DIR[] = "ResearchImpression";
  const char RESEARCH_CLICK_OUT_DIR[] = "ResearchClick";

  const char CONSIDER_ACTION_OUT_DIR[] = "ConsiderAction";
  const char CONSIDER_CLICK_OUT_DIR[] = "ConsiderClick";
  const char CONSIDER_IMPRESSION_OUT_DIR[] = "ConsiderImpression";
  const char CONSIDER_REQUEST_OUT_DIR[] = "ConsiderRequest";
}

namespace Aspect
{
  char REQUEST_INFO_MANAGER[] = "RequestInfoManager";
  char CLEAR_EXPIRED_DATA[] = "RequestInfoManager:ClearExpiredData";
}

namespace AdServer::RequestInfoSvcs
{
  LogProcessing::LogFlushTraits
  read_flush_policy(
    const xsd::AdServer::Configuration::LogFlushPolicyType& log_flush_policy,
    const char* default_path)
  {
    AdServer::LogProcessing::LogFlushTraits res;
    res.period = Generics::Time(log_flush_policy.period());
    res.out_dir = log_flush_policy.path().present() ?
      log_flush_policy.path()->c_str() : default_path;
    return res;
  }


  LogProcessing::LogFlushTraits*
  read_flush_policy(
    const xsd::cxx::tree::optional<
      xsd::AdServer::Configuration::LogFlushPolicyType>& log_flush_policy,
    const char* default_path,
    LogProcessing::LogFlushTraits& res)
  {
    if (log_flush_policy.present())
    {
      res = read_flush_policy(log_flush_policy.get(), default_path);
      return &res;
    }
    return 0;
  }

  /** UserFraudDeactivator */
  struct UserFraudDeactivator:
    public virtual UserFraudProtectionContainer::Callback,
    public virtual Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    UserFraudDeactivator(
      Logging::Logger* logger,
      const xsd::AdServer::Configuration::RequestInfoManagerUserInfoType& user_info_config)
      /*throw(Exception)*/;

    virtual void
    detected_fraud_user(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& deactivate_time)
      noexcept;

  protected:
    virtual ~UserFraudDeactivator() noexcept {}

  protected:
    Logging::Logger_var logger_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient> user_info_matcher_;
  };

  /**
   * RequestInfoManagerImpl
   */
  RequestInfoManagerImpl::RequestInfoManagerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const RequestInfoManagerConfig& request_info_manager_config,
    const RequestInfoManagerStatsImpl_var& rim_stats_impl)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      SERVICE_INDEX_(request_info_manager_config.service_index()),
      check_logs_callback_(new Logging::ActiveObjectCallbackImpl(
        logger_,
        "RequestInfoManager::check_logs_()",
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3015")),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 7)),
      rocksdb_processor_(std::make_shared<ProfilingCommons::RocksDBProfileMapProcessor>(
        request_info_manager_config.rocksdb_batching_threads())),
      request_info_manager_config_(request_info_manager_config),
      rim_stats_impl_(rim_stats_impl)
  {
    static const char* FUN = "RequestInfoManagerImpl::RequestInfoManagerImpl()";

    const char* stage = "init CorbaClientAdapter";

    try
    {
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();
      stage = "resolve campaign servers";
      campaign_servers_ = resolve_campaign_servers_();
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't " << stage << ": " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      add_child_object(task_runner_.in());
      add_child_object(scheduler_.in());
      add_child_object(rocksdb_processor_);
    }
    catch (const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught CompositeActiveObject::Exception: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      // init fraud user processor: notify UIM's
      user_fraud_deactivator_ = new UserFraudDeactivator(
        logger_,
        request_info_manager_config_.UserInfo());
      add_child_object(user_fraud_deactivator_.in());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserFraudDeactivator: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      processing_distributor_ = new CompositeRequestActionProcessor();
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't instantiate processing distributor. "
        "Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    const xsd::AdServer::Configuration::OutLogsType&
      lp_config = request_info_manager_config_.LogProcessing().OutLogs();

    if (lp_config.RequestOperation().present())
    {
      try
      {
        const std::string log_root = lp_config.log_root();

        const xsd::AdServer::Configuration::RequestOperationFlushPolicyType&
          req_op_config = *lp_config.RequestOperation();

        LogProcessing::LogFlushTraits req_op_traits = read_flush_policy(
          req_op_config,
          (log_root + REQUEST_OPERATION_OUT_DIR).c_str());

        request_operation_saver_ = new RequestOperationSaver(
          logger_,
          req_op_traits.out_dir.c_str(),
          "RequestOperation",
          req_op_config.chunks_count(),
          req_op_traits.period,
          1);

        add_child_object(request_operation_saver_.in());

        request_operation_distributor_ = new RequestOperationDistributor(
          request_info_manager_config_.distrib_count(),
          SERVICE_INDEX_,
          request_info_manager_config_.services_count(),
          0,
          request_operation_saver_);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't instantiate request operation saver: " << ex.what();
        throw Exception(ostr);
      }
    }

    try
    {
      const std::string log_root = lp_config.log_root();

      const LogProcessing::LogFlushTraits consider_action_traits = read_flush_policy(
        lp_config.ConsiderAction(),
        (log_root + CONSIDER_ACTION_OUT_DIR).c_str());
      const LogProcessing::LogFlushTraits consider_click_traits = read_flush_policy(
        lp_config.ConsiderClick(),
        (log_root + CONSIDER_CLICK_OUT_DIR).c_str());
      const LogProcessing::LogFlushTraits consider_impression_traits = read_flush_policy(
        lp_config.ConsiderImpression(),
        (log_root + CONSIDER_IMPRESSION_OUT_DIR).c_str());
      const LogProcessing::LogFlushTraits consider_request_traits = read_flush_policy(
        lp_config.ConsiderRequest(),
        (log_root + CONSIDER_REQUEST_OUT_DIR).c_str());

      // init hook for notify ExpressionMatcher's about actions
      expression_matcher_notifier_ = new ExpressionMatcherNotifier(
        logger_,
        lp_config.notify_impressions(),
        lp_config.notify_revenue(),
        lp_config.distrib_count(),
        consider_action_traits,
        consider_click_traits,
        consider_impression_traits,
        consider_request_traits);
      add_child_object(expression_matcher_notifier_.in());

      processing_distributor_->add_child_processor(expression_matcher_notifier_);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init ExpressionMatcherNotifier: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      const xsd::AdServer::Configuration::OutLogsType&
        lp_config = request_info_manager_config_.LogProcessing().OutLogs();

      std::string log_root = lp_config.log_root();

      LogProcessing::LogFlushTraits research_action_flush;
      LogProcessing::LogFlushTraits research_bid_flush;
      LogProcessing::LogFlushTraits research_impression_flush;
      LogProcessing::LogFlushTraits research_click_flush;
      LogProcessing::LogFlushTraits bid_cost_stat_flush;

      request_out_logger_ =
        new RequestOutLogger(
          logger_,
          callback_,
          read_flush_policy(
            lp_config.CreativeStat(),
            (log_root + CREATIVE_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.UserProperties(),
            (log_root + USER_PROPERTIES_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.ChannelPerformance(),
            (log_root + CHANNEL_PERFORMANCE_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.ExpressionPerformance(),
            (log_root + EXPRESSION_PERFORMANCE_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.CcgKeywordStat(),
            (log_root + CCG_KEYWORD_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.CmpStat(),
            (log_root + CMP_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.ActionStat(),
            (log_root + ACTION_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.CcgUserStat(),
            (log_root + CCG_USER_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.CcUserStat(),
            (log_root + CC_USER_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.CampaignUserStat(),
            (log_root + CAMPAIGN_USER_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.PassbackStat(),
            (log_root + PASSBACK_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.ChannelImpInventory(),
            (log_root + CHANNEL_IMP_INVENTORY_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.SiteUserStat(),
            (log_root + SITE_USER_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.AdvertiserUserStat(),
            (log_root + ADVERTISER_USER_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.SiteReferrerStat(),
            (log_root + SITE_REFERRER_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.PageLoadsDailyStat(),
            (log_root + PAGE_LOADS_DAILY_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.TagPositionStat(),
            (log_root + TAG_POSITION_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.CampaignReferrerStat(),
            (log_root + CAMPAIGN_REFERRER_STAT_OUT_DIR).c_str()),
          read_flush_policy(
            lp_config.ResearchAction(),
            (log_root + RESEARCH_ACTION_OUT_DIR).c_str(),
            research_action_flush),
          read_flush_policy(
            lp_config.ResearchBid(),
            (log_root + RESEARCH_BID_OUT_DIR).c_str(),
            research_bid_flush),
          read_flush_policy(
            lp_config.ResearchImpression(),
            (log_root + RESEARCH_IMPRESSION_OUT_DIR).c_str(),
            research_impression_flush),
          read_flush_policy(
            lp_config.ResearchClick(),
            (log_root + RESEARCH_CLICK_OUT_DIR).c_str(),
            research_click_flush),
          read_flush_policy(
            lp_config.BidCostStat(),
            (log_root + BID_COST_STAT_OUT_DIR).c_str(),
            bid_cost_stat_flush),
          Commons::LogReferrer::read_log_referrer_settings(
            request_info_manager_config_.use_referrer_site_referrer_stats()),
	  request_info_manager_config_.colo_id());

      processing_distributor_->add_child_processor(
        request_out_logger_);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init RequestOutLogger: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      LoadDataState_var load_data_state(new LoadDataState());

      /// If we put scheduler task (TaskGoal) in task_runner we should avoid
      // cycle references and should NOT put task_runner in task to be
      // executed in task_runner.
      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::request_loaded,
        &RequestInfoManagerImpl::load_request_chunk_files_,
        0, // task runner
        this)));

      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::user_campaign_reach_loaded,
        &RequestInfoManagerImpl::load_user_campaign_reach_chunk_files_,
        0, // task runner
        this)));

      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::user_action_info_loaded,
        &RequestInfoManagerImpl::load_user_action_info_chunk_files_,
        0, // task runner
        this)));

      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::user_fraud_protection_loaded,
        &RequestInfoManagerImpl::load_user_fraud_protection_chunk_files_,
        0, // task runner
        this)));

      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::passback_loaded,
        &RequestInfoManagerImpl::init_passback_container_,
        0, // task runner
        this)));

      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::user_site_reach_loaded,
        &RequestInfoManagerImpl::init_user_site_reach_container_,
        0, // task runner
        this)));

      task_runner_->enqueue_task(Task_var(new LoadTask(
        load_data_state,
        &LoadDataState::user_tag_request_merge_loaded,
        &RequestInfoManagerImpl::init_user_tag_request_merge_container_,
        0, // task runner
        this)));

      if (request_info_manager_config_.Billing().present())
      {
        task_runner_->enqueue_task(Task_var(new LoadTask(
          load_data_state,
          &LoadDataState::billing_processor_loaded,
          &RequestInfoManagerImpl::init_billing_processor_,
          0, // task runner
          this)));
      }
      else
      {
        LoadDataState::SyncPolicy::WriteGuard guard(load_data_state->lock);
        load_data_state->billing_processor_loaded = true;
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE,
          Aspect::REQUEST_INFO_MANAGER) <<
          "LoadTask's was enqueued.";
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't instantiate object. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  RequestInfoManagerImpl::~RequestInfoManagerImpl() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
    }
    catch (...)
    {}
  }

  template<typename ContainerPtrHolderType, typename KeyType>
  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_profile_(
    const char* FUN,
    const ContainerPtrHolderType& container_ptr_holder,
    const KeyType& id)
  {
    try
    {
      auto container = container_ptr_holder.get();

      if (!container.in())
      {
        throw NotReady("Container is not ready");
      }

      co_return co_await container->co_get_profile(id);
    }
    catch (const NotReady&)
    {
      throw;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_profile(AdServer::Commons::RequestId request_id)
  {
    static const char* FUN = "RequestInfoManagerImpl::co_get_profile()";

    co_return co_await co_get_profile_(
      FUN,
      request_info_container_,
      request_id);
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_user_campaign_reach_profile(
    AdServer::Commons::UserId user_id)
  {
    static const char* FUN =
      "RequestInfoManagerImpl::co_get_user_campaign_reach_profile()";

    co_return co_await co_get_profile_(
      FUN,
      user_campaign_reach_container_,
      user_id);
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_user_action_profile(
    AdServer::Commons::UserId user_id)
  {
    static const char* FUN =
      "RequestInfoManagerImpl::co_get_user_action_profile()";

    co_return co_await co_get_profile_(FUN, user_action_info_container_, user_id);
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_user_fraud_protection_profile(
    AdServer::Commons::UserId user_id)
  {
    static const char* FUN =
      "RequestInfoManagerImpl::co_get_user_fraud_protection_profile()";

    co_return co_await co_get_profile_(
      FUN,
      user_fraud_protection_container_,
      user_id);
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_user_site_reach_profile(
    AdServer::Commons::UserId user_id)
  {
    static const char* FUN =
      "RequestInfoManagerImpl::co_get_user_site_reach_profile()";

    co_return co_await co_get_profile_(FUN, user_site_reach_container_, user_id);
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_user_tag_request_group_profile(
    AdServer::Commons::UserId user_id)
  {
    static const char* FUN =
      "RequestInfoManagerImpl::co_get_user_tag_request_group_profile()";

    co_return co_await co_get_profile_(
      FUN,
      user_tag_request_merge_container_,
      user_id);
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  RequestInfoManagerImpl::co_get_passback_profile(
    AdServer::Commons::RequestId request_id)
  {
    static const char* FUN = "RequestInfoManagerImpl::co_get_passback_profile()";

    co_return co_await co_get_profile_(FUN, passback_container_, request_id);
  }

  AdServer::Commons::Awaitable<void>
  RequestInfoManagerImpl::co_clear_expired(bool synchronous)
  {
    if (!synchronous)
    {
      Task_var clear_task = new ClearExpiredDataTask(*task_runner_, this, false);
      task_runner_->enqueue_task(clear_task);
      co_return;
    }

    logger_->log(
      String::SubString("Cleanup expired data task started"),
      Logging::Logger::INFO,
      Aspect::CLEAR_EXPIRED_DATA);

    co_await request_info_container_.get()->co_clear_expired_requests();
    co_await user_action_info_container_.get()->co_clear_expired_actions();
    co_await user_campaign_reach_container_.get()->co_clear_expired_users();
    co_await user_fraud_protection_container_.get()->co_clear_expired();
    co_await passback_container_.get()->co_clear_expired_requests();
    co_await user_site_reach_container_.get()->co_clear_expired_users();
    if (auto container = user_tag_request_merge_container_.get())
    {
      co_await container->co_clear_expired();
    }

    logger_->log(
      String::SubString("Cleanup expired data finished"),
      Logging::Logger::INFO,
      Aspect::CLEAR_EXPIRED_DATA);
  }

  void
  RequestInfoManagerImpl::start_logs_processing_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::start_logs_processing_()";

    try
    {
      user_fraud_protection_container_.get()->request_container_processor(
        request_info_container_.get()->proxy());

      user_action_info_container_.get()->request_container_processor(
        request_info_container_.get()->proxy());

      CompositeTagRequestProcessor_var tag_request_processor =
        new CompositeTagRequestProcessor();

      tag_request_processor->add_child_processor(user_site_reach_container_.get());
      tag_request_processor->add_child_processor(passback_container_.get());
      tag_request_processor->add_child_processor(request_out_logger_);
      if (user_tag_request_merge_container_.get())
      {
        tag_request_processor->add_child_processor(user_tag_request_merge_container_.get());
      }

      if (request_info_manager_config_.Profiling().present())
      {
        TagRequestProfiler::AddressList profiler_addresses;
        const xsd::AdServer::Configuration::ProfilingType&
          profiling_config = *request_info_manager_config_.Profiling();
        for(xsd::AdServer::Configuration::ProfilingType::Endpoint_sequence::
            const_iterator it = profiling_config.Endpoint().begin();
          it != profiling_config.Endpoint().end(); ++it)
        {
          profiler_addresses.push_back(it->url());
        }

        TagRequestProfiler_var tag_request_profiler =
          new TagRequestProfiler(
            logger_,
            callback_,
            profiling_config.threads(),
            Generics::Time(profiling_config.sending_window()),
            profiling_config.max_pool_size(),
            profiling_config.user_id_private_key().c_str(),
            profiler_addresses,
            Generics::Time(profiling_config.repeat_trigger_timeout()));

        tag_request_processor->add_child_processor(tag_request_profiler);

        add_child_object(tag_request_profiler.in());
      }

      CompositeAdvActionProcessor_var adv_action_processor =
        new CompositeAdvActionProcessor();

      adv_action_processor->add_child_processor(user_action_info_container_.get());
      adv_action_processor->add_child_processor(request_out_logger_);

      CompositeRequestContainerProcessor_var request_container_processor =
        new CompositeRequestContainerProcessor();

      request_container_processor->add_child_processor(request_info_container_.get());
      request_container_processor->add_child_processor(request_out_logger_);

      const xsd::AdServer::Configuration::InLogsType&
        lp_config = request_info_manager_config_.LogProcessing().InLogs();

      InLogs in_logs_config;
      init_(lp_config, in_logs_config);

      request_log_loader_ = new RequestLogLoader(
        check_logs_callback_,
        in_logs_config,
        request_out_logger_, // UnmergedClickProcessor
        request_container_processor,
        adv_action_processor,
        passback_container_.get(),
        tag_request_processor,
        request_operation_distributor_,
        Generics::Time(request_info_manager_config_.LogProcessing().
          InLogs().check_logs_period()),
        Generics::Time(1),
        lp_config.threads(),
        rim_stats_impl_);
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3008") << FUN <<
        ": Can't init logs loader. Caught eh::Exception: " << ex.what();
    }

    logger_->log(
      String::SubString("Request chunks loaded & Request logs loader inited."),
      Logging::Logger::TRACE,
      Aspect::REQUEST_INFO_MANAGER);

    // start check logs loop
    try
    {
      // activate object because main active object already active
      add_child_object(request_log_loader_.in());
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3008") << FUN <<
        ": Can't activate log loader. Caught eh::Exception: " << ex.what();
    }

    clear_expired_data_(true);

    logger_->log(String::SubString("First log checking finished."),
      Logging::Logger::TRACE,
      Aspect::REQUEST_INFO_MANAGER);

    /* start flush logs loop */
    flush_logs_();

    logger_->log(String::SubString("Test flush finished."),
      Logging::Logger::TRACE,
      Aspect::REQUEST_INFO_MANAGER);
  }

  bool
  RequestInfoManagerImpl::load_user_fraud_protection_chunk_files_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::load_user_fraud_protection_chunk_files_()";

    if (!user_fraud_protection_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of user fraud protection chunks started.";
        }

        {
          xsd::AdServer::Configuration::ChunksConfigType&
            chunks_config = request_info_manager_config_.UserFraudProtectionChunksConfig();

          UserFraudProtectionContainer_var user_fraud_protection_container =
            new UserFraudProtectionContainer(
              logger_,
              0, // will be linked to request_info_container_.get()->proxy()
              user_fraud_deactivator_,
              chunks_config.chunks_root().c_str(),
              chunks_config.expire_time().present() ?
                Generics::Time(*chunks_config.expire_time()) :
                DEFAULT_FRAUD_PROFILE_EXPIRE_TIME,
              rocksdb_processor_);

          UserFraudProtectionContainer::Config_var fraud_config(
            new UserFraudProtectionContainer::Config());

          add_child_object(user_fraud_protection_container.in());

          processing_distributor_->add_child_processor(user_fraud_protection_container);

          user_fraud_protection_container_ = user_fraud_protection_container;
        }
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3009") << FUN <<
          ": Can't load user fraud protection chunk files. Caught eh::Exception: " <<
          ex.what();
      }
    }

    return user_fraud_protection_container_.get().in();
  }

  bool
  RequestInfoManagerImpl::load_user_action_info_chunk_files_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::load_user_action_info_chunk_files_()";

    if (!user_action_info_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of user action chunks started.";
        }

        {
          xsd::AdServer::Configuration::ChunksConfigType&
            chunks_config = request_info_manager_config_.UserActionChunksConfig();

          UserActionInfoContainer_var user_action_info_container =
            new UserActionInfoContainer(
              logger_,
              0, // will be linked to request_info_container_.get()->proxy(),
              chunks_config.chunks_root().c_str(),
              request_info_manager_config_.action_ignore_time().present() ?
                Generics::Time(*request_info_manager_config_.action_ignore_time()) :
                DEFAULT_ACTION_IGNORE_TIME,
              chunks_config.expire_time().present() ?
                Generics::Time(*chunks_config.expire_time()) :
                DEFAULT_ACTION_PROFILE_EXPIRE_TIME,
              rocksdb_processor_);

          add_child_object(user_action_info_container.in());

          processing_distributor_->add_child_processor(
            user_action_info_container->request_processor());

          user_action_info_container_ = user_action_info_container;

          return true;
        }
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3009") << FUN <<
          ": Can't load user action chunk files. Caught eh::Exception: " <<
          ex.what();
      }

      return false;
    }

    return true;
  }

  bool
  RequestInfoManagerImpl::load_user_campaign_reach_chunk_files_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::load_user_campaign_reach_chunk_files_()";

    if (!user_campaign_reach_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of user campaign reach chunks started.";
        }

        {
          xsd::AdServer::Configuration::ChunksConfigType&
            chunks_config = request_info_manager_config_.UserCampaignReachChunksConfig();

          UserCampaignReachContainer_var user_campaign_reach_container =
            new UserCampaignReachContainer(
              logger_,
              request_out_logger_.in(),
              chunks_config.chunks_root().c_str(),
              chunks_config.expire_time().present() ?
                Generics::Time(*chunks_config.expire_time()) :
                USER_CAMPAIGN_REACH_DEFAULT_EXPIRE_TIME,
              rocksdb_processor_);

          add_child_object(user_campaign_reach_container.in());

          processing_distributor_->add_child_processor(user_campaign_reach_container);

          user_campaign_reach_container_ = user_campaign_reach_container;
        }
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3011") << FUN <<
          ": Can't load user campaign reach chunk files. "
            "Caught eh::Exception: " << ex.what();
      }
    }

    return user_campaign_reach_container_.get().in();
  }

  bool
  RequestInfoManagerImpl::load_request_chunk_files_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::load_request_chunk_files_()";

    if (!request_info_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of request chunks started.";
        }

        const xsd::AdServer::Configuration::ChunksConfigType&
          chunks_config = request_info_manager_config_.ChunksConfig();

        if (!request_info_manager_config_.BidChunksConfig().present())
        {
          throw Exception("BidChunksConfig is required for request profile storage");
        }

        const xsd::AdServer::Configuration::ChunksConfigType&
          bid_chunks_config = *request_info_manager_config_.BidChunksConfig();

        RequestInfoContainer_var request_info_container =
          new RequestInfoContainer(
            logger_,
            processing_distributor_.in(),
            request_operation_distributor_,
            String::SubString(bid_chunks_config.chunks_root()),
            bid_chunks_config.expire_time().present() ?
              Generics::Time(*bid_chunks_config.expire_time()) :
            chunks_config.expire_time().present() ?
              Generics::Time(*chunks_config.expire_time()) :
              RequestInfoContainer::DEFAULT_EXPIRE_TIME,
            rocksdb_processor_);

        add_child_object(request_info_container.in());

        request_operation_distributor_->request_operation_processor(
          request_info_container->request_operation_proxy());

        request_info_container_ = request_info_container;
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3013") << FUN <<
          ": Can't load chunk files. Caught eh::Exception: " << ex.what();
      }
    }

    return request_info_container_.get().in();
  }

  bool
  RequestInfoManagerImpl::init_passback_container_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::init_passback_container_()";

    if (!passback_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of passback chunks started.";
        }

        {
          xsd::AdServer::Configuration::ChunksConfigType&
            chunks_config = request_info_manager_config_.PassbackChunksConfig();

          PassbackContainer_var passback_container =
            new PassbackContainer(
              logger_,
              request_out_logger_.in(),
              chunks_config.chunks_root().c_str(),
              chunks_config.expire_time().present() ?
                Generics::Time(*chunks_config.expire_time()) :
                PassbackContainer::DEFAULT_EXPIRE_TIME,
              rocksdb_processor_);

          add_child_object(passback_container.in());

          passback_container_ = passback_container;

          return true;
        }
      }
      catch (const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": Can't load passback chunk files: " << ex.what();

        logger_->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3009");
      }

      return false;
    }

    return true;
  }

  bool
  RequestInfoManagerImpl::init_user_site_reach_container_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::init_user_site_reach_container_()";

    if (!user_site_reach_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of site reach chunks started.";
        }

        {
          xsd::AdServer::Configuration::ChunksConfigType&
            chunks_config = request_info_manager_config_.UserSiteReachChunksConfig();

          const std::string user_site_reach_rocksdb_path = chunks_config.chunks_root();

          UserSiteReachContainer_var user_site_reach_container =
            new UserSiteReachContainer(
              logger_,
              request_out_logger_.in(),
              user_site_reach_rocksdb_path.c_str(),
              chunks_config.expire_time().present() ?
                Generics::Time(*chunks_config.expire_time()) :
                USER_SITE_REACH_DEFAULT_EXPIRE_TIME,
              rocksdb_processor_);

          add_child_object(user_site_reach_container.in());

          user_site_reach_container_ = user_site_reach_container;

          return true;
        }
      }
      catch (const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": Can't load site reach chunk files: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3009");
      }

      return false;
    }

    return true;
  }

  bool
  RequestInfoManagerImpl::init_user_tag_request_merge_container_() noexcept
  {
    static const char* FUN =
      "RequestInfoManagerImpl::init_user_tag_request_merge_container_()";

    if (!request_info_manager_config_.TagRequestGroupingConfig().present())
    {
      return true;
    }

    if (!user_tag_request_merge_container_.get().in())
    {
      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::REQUEST_INFO_MANAGER) <<
            "Loading of tag request group chunks started.";
        }

        {
          xsd::AdServer::Configuration::TagRequestGroupingConfigType&
            config = *request_info_manager_config_.TagRequestGroupingConfig();

          UserTagRequestMergeContainer_var user_tag_request_merge_container =
            new UserTagRequestMergeContainer(
              logger_,
              request_out_logger_.in(),
              Generics::Time(config.merge_time_bound()),
              config.chunks_root().c_str(),
              config.expire_time().present() ?
                Generics::Time(*config.expire_time()) :
                UserTagRequestMergeContainer::DEFAULT_EXPIRE_TIME,
              rocksdb_processor_);

          add_child_object(user_tag_request_merge_container.in());

          user_tag_request_merge_container_ = user_tag_request_merge_container;

          return true;
        }
      }
      catch (const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": Can't load tag request group chunk files: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3009");
      }

      return false;
    }

    return true;
  }

  bool
  RequestInfoManagerImpl::init_billing_processor_() noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::init_billing_processor_()";

    try
    {
      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE,
          Aspect::REQUEST_INFO_MANAGER) <<
          "Init of billing processor started.";
      }

      const xsd::AdServer::Configuration::BillingType&
        bs_config = *request_info_manager_config_.Billing();

      std::vector<std::string> billing_server_refs;
      for(const auto& grpc_ref : bs_config.BillingServerGrpcRef())
      {
        const std::string host = grpc_ref.host().present() ?
          std::string(*grpc_ref.host()) :
          std::string("localhost");
        billing_server_refs.emplace_back(host + ":" + std::to_string(grpc_ref.port()));
      }

      BillingProcessor_var billing_processor = new BillingProcessor(
        logger_,
        callback_,
        bs_config.storage_root(),
        BillingProcessor::RequestSender_var(
          new BillingProcessor::BillingServerRequestSender(
            callback_,
            std::move(billing_server_refs))),
        bs_config.threads(), // thread_count
        Generics::Time(bs_config.dump_period()), // dump_period
        Generics::Time(bs_config.send_delayed_period()) // send_delayed_period
        );

      add_child_object(billing_processor.in());

      processing_distributor_->add_child_processor(billing_processor);

      return true;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init billing processor: " << ex.what();
      logger_->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3009");
    }

    return false;
  }

  void
  RequestInfoManagerImpl::load_data_(
    LoadDataState* load_data_state,
    bool LoadDataState::* data_loaded,
    LoadDataMethod load_data)
    noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::load_data_()";

    if ((this->*load_data)())
    {
      bool prev_fully_loaded;
      bool new_fully_loaded;

      {
        LoadDataState::SyncPolicy::WriteGuard guard(load_data_state->lock);
        prev_fully_loaded = load_data_state->fully_loaded_i();
        (load_data_state->*data_loaded) = true;
        new_fully_loaded = load_data_state->fully_loaded_i();
      }

      if (!prev_fully_loaded && new_fully_loaded)
      {
        // enqueue fraud rules update task
        task_runner_->enqueue_task(Task_var(new UpdateFraudRulesTask(
          0, // task runner
          this)));
      }
    }
    else
    {
      // reschedule data loading
      try
      {
        Task_var msg = new LoadTask(
          load_data_state,
          data_loaded,
          load_data,
          task_runner_,
          this);

        scheduler_->schedule(
          msg,
          Generics::Time::get_time_of_day() + CHUNKS_RELOAD_PERIOD);
      }
      catch (const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": Can't schedule chunks reloading task. "
          "Caught eh::Exception: " << ex.what();

        logger_->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3010");
      }
    }
  }

  void
  RequestInfoManagerImpl::flush_logs_() noexcept
  {
    static const char* FUN = "RequestInfoManager::flush_logs_()";

    const Generics::Time LOGS_DUMP_ERROR_RESCHEDULE_PERIOD(2);

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(Logging::Logger::TRACE,
        Aspect::REQUEST_INFO_MANAGER) << FUN << ": Flush logs.";
    }

    Generics::Time next_flush;

    try
    {
      next_flush = request_out_logger_->flush_if_required();
    }
    catch (const eh::Exception& ex)
    {
      next_flush = Generics::Time::get_time_of_day() + LOGS_DUMP_ERROR_RESCHEDULE_PERIOD;

      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3017") << FUN <<
        ": Can't flush logs. Caught eh::Exception: " << ex.what();
    }

    if (next_flush != Generics::Time::ZERO)
    {
      try
      {
        Task_var msg = new FlushLogsTask(*task_runner_, this);
        scheduler_->schedule(msg, next_flush);
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3018") << FUN <<
          ": Can't schedule next logs flush task. Caught eh::Exception: " <<
          ex.what();
      }
    }
  }

  RequestInfoManagerImpl::CampaignServerPoolPtr
  RequestInfoManagerImpl::resolve_campaign_servers_()
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "RequestInfoManagerImpl::resolve_campaign_servers_()";

    try
    {
      CORBACommons::CorbaObjectRefList campaign_server_refs;

      Config::CorbaConfigReader::read_multi_corba_ref(
        request_info_manager_config_.CampaignServerCorbaRef(),
        campaign_server_refs);

      CampaignServerPoolConfig pool_config(corba_client_adapter_.in());
      pool_config.timeout = Generics::Time(10); // 10 sec

      pool_config.iors_list.insert(
        pool_config.iors_list.end(),
        campaign_server_refs.begin(),
        campaign_server_refs.end());

      return CampaignServerPoolPtr(
        new CampaignServerPool(pool_config, CORBACommons::ChoosePolicyType::PT_PERSISTENT));
    }
    catch (const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": got CORBA::SystemException: " << e;
      throw Exception(ostr);
    }
  }

  bool
  RequestInfoManagerImpl::update_fraud_rules_(bool loop_update) noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::update_fraud_rules_()";

    bool fraud_rules_inited = false;
    bool fraud_rules_first_time_inited = false;

    try
    {
      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<CampaignServerPool::Exception>(
            logger_,
            Logging::Logger::EMERGENCY,
            Aspect::REQUEST_INFO_MANAGER,
            "",
            SERVICE_INDEX_,
            SERVICE_INDEX_);

        try
        {
          AdServer::CampaignSvcs::FraudConditionConfig_var fraud_config_info =
            campaign_server->fraud_conditions();

          const AdServer::CampaignSvcs::FraudConditionSeq& fraud_conds =
            fraud_config_info->rules;

          UserFraudProtectionContainer::Config_var config(
            new UserFraudProtectionContainer::Config());

          config->deactivate_period = CorbaAlgs::unpack_time(
            fraud_config_info->deactivate_period);

          for(CORBA::ULong fraud_i = 0; fraud_i < fraud_conds.length(); ++fraud_i)
          {
            UserFraudProtectionContainer::Config::FraudRule rule;
            rule.limit = fraud_conds[fraud_i].limit;
            rule.period = CorbaAlgs::unpack_time(fraud_conds[fraud_i].period);
            if (fraud_conds[fraud_i].type == 'I')
            {
              config->imp_rules.add_rule(rule);
            }
            else
            {
              config->click_rules.add_rule(rule);
            }
          }

          fraud_rules_first_time_inited =
            !user_fraud_protection_container_.get()->config_initialized();

          fraud_rules_inited = true;

          user_fraud_protection_container_.get()->config(config);

          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            logger_->stream(Logging::Logger::TRACE,
              Aspect::REQUEST_INFO_MANAGER) << "Fraud rules updated.";
          }
          break;
        }
        catch (const AdServer::CampaignSvcs::CampaignServer::NotReady&)
        {
          Stream::Error ostr;
          ostr << FUN << ": CampaignServer not ready.";
          campaign_server.release_bad(ostr.str());
          logger_->stream(Logging::Logger::NOTICE,
            Aspect::REQUEST_INFO_MANAGER) << ostr.str();
        }
        catch (const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't get session. Caught CampaignServer::ImplementationException: "
            << ex.description;
          campaign_server.release_bad(ostr.str());
          logger_->stream(Logging::Logger::EMERGENCY,
            Aspect::REQUEST_INFO_MANAGER) << ostr.str();
        }
        catch (const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't get session. Caught CORBA::SystemException: " << ex;
          campaign_server.release_bad(ostr.str());
          logger_->stream(Logging::Logger::EMERGENCY,
            Aspect::REQUEST_INFO_MANAGER,
            "ADS-IMPL-3019") << ostr.str();
        }
      } // for (;;)
    }
    catch (const eh::Exception& e)
    {
      logger_->stream(Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER) << FUN << ": eh::Exception caught: " << e.what();
    }

    if (loop_update)
    {
      try
      {
        scheduler_->schedule(
          Task_var(new UpdateFraudRulesTask(task_runner_, this)),
          Generics::Time::get_time_of_day() + UPDATE_FRAUD_RULES_PERIOD);
      }
      catch (const eh::Exception& ex)
      {
        logger_->stream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3020") << FUN <<
          ": Can't set task for update fraud rules. "
          "Caught eh::Exception: " << ex.what();
      }
    }

    if (fraud_rules_first_time_inited)
    {
      start_logs_processing_();
    }

    return fraud_rules_inited;
  }

  void RequestInfoManagerImpl::clear_expired_data_(bool reschedule)
    noexcept
  {
    static const char* FUN = "RequestInfoManagerImpl::clear_expired_data_()";

    Generics::Time start_time = Generics::Time::get_time_of_day();

    logger_->log(String::SubString("Cleanup expired data task started"),
      Logging::Logger::INFO,
      Aspect::CLEAR_EXPIRED_DATA);

    try
    {
      request_info_container_.get()->clear_expired_requests();
      user_action_info_container_.get()->clear_expired_actions();
      user_campaign_reach_container_.get()->clear_expired_users();
      user_fraud_protection_container_.get()->clear_expired();
      passback_container_.get()->clear_expired_requests();
      user_site_reach_container_.get()->clear_expired_users();
      if (user_tag_request_merge_container_.get())
      {
        user_tag_request_merge_container_.get()->clear_expired();
      }

      if (logger_->log_level() >= Logging::Logger::INFO)
      {
        Stream::Error ostr;
        ostr << "Cleanup expired data finished";

        logger_->log(ostr.str(),
          Logging::Logger::INFO,
          Aspect::CLEAR_EXPIRED_DATA);
      }
    }
    catch (const eh::Exception& e)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3021") << FUN <<
        ": eh::Exception caught: " << e.what();
    }

    if (reschedule)
    {
      try
      {
        Generics::Time tm = Generics::Time::get_time_of_day() + Generics::Time::ONE_MINUTE*30;

        Task_var msg = new ClearExpiredDataTask(*task_runner_, this, true);
        scheduler_->schedule(msg, tm);

        logger_->sstream(Logging::Logger::INFO, Aspect::REQUEST_INFO_MANAGER) <<
          FUN << ": Cleanup expired data task scheduled for '" <<
          tm.get_gm_time() <<
          "' start time of prev task '" <<
          start_time.get_gm_time() << "'";
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::REQUEST_INFO_MANAGER,
          "ADS-IMPL-3022") << FUN <<
          ": Can't schedule next clear expired data task. "
          "Caught eh::Exception: " <<
          ex.what();
      }
    }
  }

  void
  RequestInfoManagerImpl::init_(
    const xsd::AdServer::Configuration::InLogsType& lp_config,
    InLogs& in_logs)
    noexcept
  {
    const char REQUEST_IN_DIR[] = "Request";
    const char IMPRESSION_IN_DIR[] = "Impression";
    const char CLICK_IN_DIR[] = "Click";
    const char ADVERTISER_ACTION_IN_DIR[] = "AdvertiserAction";
    const char PASSBACK_IMPRESSION_IN_DIR[] = "PassbackImpression";
    const char TAG_REQUEST_IN_DIR[] = "TagRequest";
    const char REQUEST_OPERATION_IN_DIR[] = "RequestOperation";

    const std::string log_root = lp_config.log_root();

    in_logs.fetch_threads = lp_config.fetch_threads();

    init_(lp_config.Request(), log_root, REQUEST_IN_DIR, in_logs.request);
    init_(lp_config.Impression(), log_root, IMPRESSION_IN_DIR, in_logs.impression);
    init_(lp_config.Click(), log_root, CLICK_IN_DIR, in_logs.click);
    init_(lp_config.AdvertiserAction(), log_root, ADVERTISER_ACTION_IN_DIR, in_logs.advertiser_action);
    init_(lp_config.PassbackImpression(), log_root, PASSBACK_IMPRESSION_IN_DIR, in_logs.passback_impression);
    init_(lp_config.TagRequest(), log_root, TAG_REQUEST_IN_DIR, in_logs.tag_request);
    init_(lp_config.RequestOperation(), log_root, REQUEST_OPERATION_IN_DIR, in_logs.request_operation);
  }

  void
  RequestInfoManagerImpl::init_(
    const xsd::AdServer::Configuration::InLogType& config,
    const std::string& log_root,
    const char* default_in_dir,
    InLog& in_log)
    noexcept
  {
    in_log.dir = config.path().present() ? config.path()->c_str() : (log_root + default_in_dir);
    in_log.priority = config.priority();
  }

  /* UserFraudDeactivator implementation */
  UserFraudDeactivator::UserFraudDeactivator(
    Logging::Logger* logger,
    const xsd::AdServer::Configuration::RequestInfoManagerUserInfoType&
      user_info_config)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      grpc_executor_(std::make_shared<AdServer::Grpc::GrpcExecutor>(
        user_info_config.grpc_executor_threads()))
  {
    static const char* FUN = "UserFraudDeactivator::UserFraudDeactivator()";

    try
    {
      AdServer::Grpc::BatchingOptions batching_options;
      AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient::UserInfoControllerRefs
        user_info_controller_refs;

      if (user_info_config.BatchingOptions().present())
      {
        batching_options = Config::read_xsd_grpc_options(*user_info_config.BatchingOptions());
      }

      for(const auto& group : user_info_config.UserInfoControllerGroup())
      {
        AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient::
          UserInfoControllerRefGroup user_info_controller_ref_group;
        for(const auto& endpoint : group.Endpoint())
        {
          user_info_controller_ref_group.emplace_back(endpoint);
        }
        if (!user_info_controller_ref_group.empty())
        {
          user_info_controller_refs.emplace_back(
            std::move(user_info_controller_ref_group));
        }
      }

      add_child_object(grpc_executor_);

      auto client =
        std::make_shared<AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient>(
          user_info_controller_refs,
          batching_options,
          grpc_executor_,
          logger_,
          std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>());
      user_info_matcher_ = client;
      add_child_object(client);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't init UserInfoManager grpc client. eh::Exception caught: " <<
        ex.what();
      throw Exception(ostr, "ADS-IMPL-3024");
    }
  }

  void
  UserFraudDeactivator::detected_fraud_user(
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& deactivate_time)
    noexcept
  {
    static const char* FUN = "UserFraudDeactivator::detected_fraud_user()";

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(Logging::Logger::TRACE,
        Aspect::REQUEST_INFO_MANAGER) <<
        FUN << ": detected fraud user : '" << user_id <<
        "', will be disabled for " <<
        deactivate_time.get_gm_time();
    }

    try
    {
      AdServer::UserInfoSvcs::GrpcAlgs::fraud_user(
        *user_info_matcher_,
        ::GrpcAlgs::pack_user_id(user_id),
        ::GrpcAlgs::pack_time(deactivate_time));
    }
    catch (const AdServer::UserInfoSvcs::GrpcAlgs::NotReady&)
    {
      logger_->stream(Logging::Logger::NOTICE,
        Aspect::REQUEST_INFO_MANAGER) << FUN <<
        ": Can't mark user as fraud. "
        "Caught UserInfoSvcs::GrpcAlgs::NotReady.";
    }
    catch (const AdServer::UserInfoSvcs::GrpcAlgs::Exception& e)
    {
      logger_->stream(Logging::Logger::EMERGENCY,
        Aspect::REQUEST_INFO_MANAGER,
        "ADS-IMPL-3027") << FUN <<
        ": Can't mark user as fraud. "
        "Caught UserInfoSvcs::GrpcAlgs::Exception: " << e.what();
    }
  }
} /* AdServer::RequestInfoSvcs */
