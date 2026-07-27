#include <boost/asio.hpp>

#include <algorithm>
#include <deque>
#include <iostream>
#include <map>
#include <stdexcept>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Scheduler.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Generics/Rand.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <BillingServerGrpc.grpc-client.hpp>
#include <google/protobuf/arena.h>

#include "AvailableAndMinCTRSetter.hpp"
#include "BillingStateContainer.hpp"

namespace AdServer::CampaignSvcs
{
  namespace Aspect
  {
    const char BILLING_STATE_CONTAINER[] = "BillingStateContainer";
  };

  namespace
  {
    const unsigned long MAX_SIZE_SERVER_USE_TIMES = 10;
    const Generics::Time MAX_SERVER_USE_TIME = Generics::Time(10);
    const Generics::Time MIN_SERVER_USE_TIME = Generics::Time(1) / 100; // 10 ms
    const Generics::Time REENABLE_INDEX_TIME = Generics::Time(10);

    const bool DEBUG_BILLING_SERVER_CALL_ = false;

    namespace Proto = adserver::campaign_svcs::billing_server;

    void
    fill_bid_(
      Proto::BidInfo& target,
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      bool optimize_campaign_ctr)
    {
      target.set_time(GrpcAlgs::pack_time(now));
      target.set_account_id(account_id);
      target.set_advertiser_id(advertiser_id);
      target.set_campaign_id(campaign_id);
      target.set_ccg_id(ccg_id);
      target.set_ctr(GrpcAlgs::pack_decimal(ctr));
      target.set_optimize_campaign_ctr(optimize_campaign_ctr);
    }

    RevenueDecimal
    unpack_revenue_decimal_(const std::string& value)
    {
      return value.empty() ?
        RevenueDecimal::ZERO :
        GrpcAlgs::unpack_decimal<RevenueDecimal>(value);
    }

    void
    log_billing_server_grpc_error_(
      Logging::Logger* logger,
      const char* fun,
      unsigned long service_index,
      const std::string& endpoint,
      const grpc::Status& status)
    {
      Stream::Error ostr;
      ostr << fun << ": BillingServer gRPC call failed by service index #" <<
        service_index << ", endpoint=" << endpoint <<
        ", code=" << status.error_code() <<
        ", message=" << status.error_message();
      logger->log(
        ostr.str(),
        status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          Logging::Logger::NOTICE :
          Logging::Logger::EMERGENCY,
        Aspect::BILLING_STATE_CONTAINER,
        "ADS-ICON-4003");
    }

    void
    log_billing_server_exception_(
      Logging::Logger* logger,
      const char* fun,
      unsigned long service_index,
      const std::string& endpoint,
      const char* what)
    {
      Stream::Error ostr;
      ostr << fun << ": BillingServer gRPC call failed by service index #" <<
        service_index << ", endpoint=" << endpoint << ": " << what;
      logger->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BILLING_STATE_CONTAINER,
        "ADS-ICON-4003");
    }

    void
    merge_grpc_stats_(
      AdServer::Grpc::Stats& result,
      const AdServer::Grpc::Stats& source) noexcept
    {
      result.write_batches += source.write_batches;
      result.write_items += source.write_items;
      result.read_batches += source.read_batches;
      result.read_items += source.read_items;
      result.input_items += source.input_items;
      result.completed_items += source.completed_items;
      result.completed_error_items += source.completed_error_items;
      result.queue_wait_count += source.queue_wait_count;
      result.queue_wait_sum_us += source.queue_wait_sum_us;
      result.queue_wait_max_us =
        std::max(result.queue_wait_max_us, source.queue_wait_max_us);
      result.queue_timeout_count += source.queue_timeout_count;
      result.response_wait_count += source.response_wait_count;
      result.response_wait_sum_us += source.response_wait_sum_us;
      result.response_wait_max_us =
        std::max(result.response_wait_max_us, source.response_wait_max_us);
      result.timing_coalesce_items += source.timing_coalesce_items;
      result.max_streams = std::max(result.max_streams, source.max_streams);
      result.inflight_items += source.inflight_items;
      result.stream_inflight_items += source.stream_inflight_items;
      result.queue_items += source.queue_items;
      result.pending_batches += source.pending_batches;
      result.pending_batch_items += source.pending_batch_items;
      result.active_streams += source.active_streams;
      result.available_streams += source.available_streams;
      result.connecting_streams += source.connecting_streams;
      result.draining_streams += source.draining_streams;
      result.deferred_streams += source.deferred_streams;

      if (source.consumer_stream_write.has_value())
      {
        if (!result.consumer_stream_write.has_value())
        {
          result.consumer_stream_write =
            AdServer::Grpc::Stats::ConsumerStreamWrite();
        }

        result.consumer_stream_write->count +=
          source.consumer_stream_write->count;
        result.consumer_stream_write->sum_us +=
          source.consumer_stream_write->sum_us;
        result.consumer_stream_write->max_us = std::max(
          result.consumer_stream_write->max_us,
          source.consumer_stream_write->max_us);
      }

      AdServer::Grpc::merge_last_error(result, source);
    }

    std::string
    check_available_bid_request_params_(
      long service_index,
      const char* reason,
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      bool optimize_campaign_ctr)
    {
      Stream::Error ostr;
      ostr << "check_available_bid request params: service_index=" <<
        service_index;
      if (reason)
      {
        ostr << ", reason=" << reason;
      }
      ostr << ", time=" << now <<
        ", account_id=" << account_id <<
        ", advertiser_id=" << advertiser_id <<
        ", campaign_id=" << campaign_id <<
        ", ccg_id=" << ccg_id <<
        ", ctr=" << ctr <<
        ", optimize_campaign_ctr=" << optimize_campaign_ctr;
      return ostr.str().str();
    }

    std::string
    confirm_bid_request_params_(
      long service_index,
      const char* reason,
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      const Proto::ConfirmBidInfo& confirm_bid_info)
    {
      Stream::Error ostr;
      ostr << "confirm_bid request params: service_index=" << service_index;
      if (reason)
      {
        ostr << ", reason=" << reason;
      }
      ostr << ", time=" << now <<
        ", account_id=" << account_id <<
        ", advertiser_id=" << advertiser_id <<
        ", campaign_id=" << campaign_id <<
        ", ccg_id=" << ccg_id <<
        ", ctr=" << ctr <<
        ", account_spent_budget=" << unpack_revenue_decimal_(
          confirm_bid_info.account_spent_budget()) <<
        ", spent_budget=" << unpack_revenue_decimal_(
          confirm_bid_info.spent_budget()) <<
        ", reserved_budget=" << unpack_revenue_decimal_(
          confirm_bid_info.reserved_budget()) <<
        ", imps=" << GrpcAlgs::unpack_decimal<ImpRevenueDecimal>(
          confirm_bid_info.imps()).str() <<
        ", clicks=" << GrpcAlgs::unpack_decimal<ImpRevenueDecimal>(
          confirm_bid_info.clicks()).str() <<
        ", forced=" << confirm_bid_info.forced();
      return ostr.str().str();
    }
  };

  class BillingStateContainer::Impl
  {
  public:
    using BidCheckResult = BillingStateContainer::BidCheckResult;
    using SyncPolicy = Sync::Policy::PosixThreadRW;
    using DisabledIndexMap = std::map<unsigned long, Generics::Time>;

    struct CCGState
    {
      CCGState()
        : active_index(0),
          use_count(-1)
      {}

      unsigned long active_index;
      long use_count;
      DisabledIndexMap disabled_indexes;
      std::deque<Generics::Time> server_use_times;
      Generics::Time last_server_switch_time;
      Generics::Time planned_server_switch_time;
      CAvailableAndMinCTRSetter_var ccg_setter;
    };

    using CCGStateMap = std::map<unsigned long, CCGState>;

    struct CCGServerSwitchTime
    {
      unsigned long ccg_id;
      Generics::Time planned_server_switch_time;
    };

    using CCGServerSwitchTimeArray = std::deque<CCGServerSwitchTime>;

    class BillingServerClient final : public Generics::CompositeActiveObject
    {
    public:
      BillingServerClient(
        const std::string& endpoint,
        std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
        std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
          coalesce_runner,
        std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
        const AdServer::Grpc::BatchingOptions& batching_options)
        : async_client_(
            std::make_shared<BillingServerGrpcAsyncBatchingClient>(
              endpoint,
              std::move(grpc_executor),
              std::move(coalesce_runner),
              batching_options)),
          coro_client_(async_client_, std::move(executor_pool))
      {
        add_child_object(async_client_);
      }

      AdServer::Grpc::Stats
      stats() const noexcept
      {
        return static_cast<const AdServer::Grpc::Client&>(*async_client_).stats();
      }

      BillingServerGrpcCoroClient::CheckAvailableBidAwaiter
      co_check_available_bid(const Proto::CheckBidRequest& request)
      {
        return coro_client_.co_check_available_bid(request);
      }

      BillingServerGrpcCoroClient::ConfirmBidAwaiter
      co_confirm_bid(const Proto::ConfirmBidRequest& request)
      {
        return coro_client_.co_confirm_bid(request);
      }

    private:
      std::shared_ptr<BillingServerGrpcAsyncBatchingClient> async_client_;
      BillingServerGrpcCoroClient coro_client_;
    };

    struct BillingServerDescr
    {
      BillingServerDescr(
        std::string endpoint_val,
        std::shared_ptr<BillingServerClient> client_val)
        : endpoint(std::move(endpoint_val)),
          client(std::move(client_val))
      {}

      BillingServerDescr(BillingServerDescr&& init) noexcept = default;

      BillingServerDescr&
      operator=(BillingServerDescr&& init) noexcept = default;

      std::string endpoint;
      std::shared_ptr<BillingServerClient> client;
    };

    using BillingServerDescrArray = std::vector<BillingServerDescr>;
    using CCGCheckTimeMap = std::map<unsigned long, Generics::Time>;

    class RecheckCCGTask;

    Impl(
      BillingStateContainer& owner,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      std::vector<std::string> billing_server_refs,
      AdServer::Grpc::BatchingOptions billing_server_batching_options,
      unsigned long max_use_count,
      bool optimize_campaign_ctr);

    AdServer::Commons::Awaitable<BidCheckResult>
    co_check_available_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      const AvailableAndMinCTRSetter* ccg_setter);

    AdServer::Commons::Awaitable<BidCheckResult>
    co_confirm_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& account_spent_amount,
      const RevenueDecimal& spent_amount,
      const RevenueDecimal& ctr,
      const ImpRevenueDecimal& imps,
      const ImpRevenueDecimal& clicks,
      const AvailableAndMinCTRSetter* ccg_setter);

    BidCheckResult
    reserve_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& amount)
      noexcept;

    void
    clear_cache() noexcept;

    BillingStateContainer::Stats
    stats() const noexcept;

  private:
    unsigned long
    next_index_(unsigned long serv_index) const
      noexcept;

    long
    get_service_index_(
      bool* switched,
      const Generics::Time& now,
      unsigned long ccg_id,
      const DisabledIndexMap* disabled_indexes,
      const AvailableAndMinCTRSetter* ccg_setter)
      noexcept;

    void
    fill_planned_server_switch_time_(
      unsigned long ccg_id,
      CCGState& ccg_state,
      const Generics::Time& now)
      noexcept;

    void
    run_recheck_ccgs_() noexcept;

    void
    ccg_set_available_(
      const AvailableAndMinCTRSetter* ccg_setter,
      unsigned long ccg_id,
      bool available,
      const RevenueDecimal& goal_ctr,
      const Generics::Time& now,
      const std::string& billing_request_params)
      noexcept;

  private:
    BillingStateContainer& owner_;
    Logging::Logger_var logger_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner_;

    const long max_use_count_;
    const unsigned long max_try_count_;
    const bool optimize_campaign_ctr_;
    unsigned long billing_server_count_;
    BillingServerDescrArray billing_servers_;

    mutable SyncPolicy::Mutex lock_;
    CCGStateMap cache_;

    CCGCheckTimeMap recheck_ccgs_;

    mutable SyncPolicy::Mutex add_recheck_ccgs_lock_;
    CCGCheckTimeMap add_recheck_ccgs_;
  };

  class BillingStateContainer::Impl::RecheckCCGTask: public Generics::TaskGoal
  {
  public:
    RecheckCCGTask(
      Impl* billing_state_container,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        billing_state_container_(billing_state_container)
    {}

    virtual void
    execute() noexcept
    {
      billing_state_container_->run_recheck_ccgs_();
    }

  protected:
    Impl* billing_state_container_;
  };

  BillingStateContainer::Impl::Impl(
    BillingStateContainer& owner,
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    std::vector<std::string> billing_server_refs,
    AdServer::Grpc::BatchingOptions billing_server_batching_options,
    unsigned long max_use_count,
    bool optimize_campaign_ctr)
    : owner_(owner),
      logger_(ReferenceCounting::add_ref(logger)),
      task_runner_(new Generics::TaskRunner(callback, 1)),
      scheduler_(new Generics::Planner(callback)),
      grpc_executor_(std::make_shared<AdServer::Grpc::GrpcExecutor>(
        4,
        "bs-grpc")),
      coalesce_runner_(
        std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
          callback,
          std::make_shared<boost::asio::io_service>(),
          1,
          128 * 1024,
          "bs-grpc-c")),
      max_use_count_(static_cast<long>(max_use_count)),
      max_try_count_(10),
      optimize_campaign_ctr_(optimize_campaign_ctr),
      billing_server_count_(billing_server_refs.size())
  {
    static const char* FUN = "BillingStateContainer::BillingStateContainer()";

    assert(max_try_count_ > 0);
    assert(!billing_server_refs.empty());
    if (!executor_pool)
    {
      Stream::Error ostr;
      ostr << FUN << ": executor_pool is null";
      throw BillingStateContainer::Exception(ostr);
    }

    for(auto& endpoint : billing_server_refs)
    {
      auto client = std::make_shared<BillingServerClient>(
        endpoint,
        grpc_executor_,
        coalesce_runner_,
        executor_pool,
        billing_server_batching_options);
      billing_servers_.emplace_back(std::move(endpoint), std::move(client));
    }

    try
    {
      owner_.add_child_object(grpc_executor_);
      owner_.add_child_object(coalesce_runner_);
      for(const auto& billing_server : billing_servers_)
      {
        owner_.add_child_object(billing_server.client);
      }
      owner_.add_child_object(task_runner_.in());
      owner_.add_child_object(scheduler_.in());
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": CompositeActiveObject::Exception caught: " << ex.what();
      throw BillingStateContainer::Exception(ostr);
    }

    // push lopped RecheckCCGTask
    Generics::Task_var msg = new RecheckCCGTask(this, task_runner_);
    task_runner_->enqueue_task(msg);
  }

  BillingStateContainer::BillingStateContainer(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    std::vector<std::string> billing_server_refs,
    AdServer::Grpc::BatchingOptions billing_server_batching_options,
    unsigned long max_use_count,
    bool optimize_campaign_ctr)
    : impl_(std::make_unique<Impl>(
        *this,
        callback,
        logger,
        std::move(executor_pool),
        std::move(billing_server_refs),
        std::move(billing_server_batching_options),
        max_use_count,
        optimize_campaign_ctr))
  {}

  BillingStateContainer::~BillingStateContainer() noexcept = default;

  AdServer::Commons::Awaitable<BillingStateContainer::BidCheckResult>
  BillingStateContainer::co_check_available_bid(
    const Generics::Time& now,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& ctr,
    const AvailableAndMinCTRSetter* ccg_setter)
  {
    return impl_->co_check_available_bid(
      now,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      ccg_setter);
  }

  AdServer::Commons::Awaitable<BillingStateContainer::BidCheckResult>
  BillingStateContainer::co_confirm_bid(
    const Generics::Time& now,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& account_spent_amount,
    const RevenueDecimal& spent_amount,
    const RevenueDecimal& ctr,
    const ImpRevenueDecimal& imps,
    const ImpRevenueDecimal& clicks,
    const AvailableAndMinCTRSetter* ccg_setter)
  {
    return impl_->co_confirm_bid(
      now,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      account_spent_amount,
      spent_amount,
      ctr,
      imps,
      clicks,
      ccg_setter);
  }

  BillingStateContainer::BidCheckResult
  BillingStateContainer::reserve_bid(
    const Generics::Time& now,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& amount)
    noexcept
  {
    return impl_->reserve_bid(
      now,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      amount);
  }

  void
  BillingStateContainer::clear_cache() noexcept
  {
    impl_->clear_cache();
  }

  BillingStateContainer::Stats
  BillingStateContainer::stats() const noexcept
  {
    return impl_->stats();
  }

  void
  BillingStateContainer::Impl::clear_cache() noexcept
  {
    CCGStateMap old_cache;

    SyncPolicy::WriteGuard lock(lock_);
    cache_.swap(old_cache);
  }

  BillingStateContainer::Stats
  BillingStateContainer::Impl::stats() const noexcept
  {
    BillingStateContainer::Stats result;
    result.endpoints.reserve(billing_servers_.size());

    for(const auto& billing_server : billing_servers_)
    {
      const auto stats = billing_server.client->stats();
      merge_grpc_stats_(result.total, stats);
      result.endpoints.emplace_back(billing_server.endpoint, stats);
    }

    return result;
  }

  AdServer::Commons::Awaitable<BillingStateContainer::BidCheckResult>
  BillingStateContainer::Impl::co_check_available_bid(
    const Generics::Time& now,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& ctr,
    const AvailableAndMinCTRSetter* ccg_setter)
  {
    static const char* FUN = "BillingStateContainer::Impl::co_check_available_bid()";

    google::protobuf::Arena request_arena;
    auto* check_bid_info = google::protobuf::Arena::CreateMessage<
      Proto::CheckBidRequest>(&request_arena);
    fill_bid_(
      *check_bid_info->mutable_bid(),
      now,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      optimize_campaign_ctr_);

    unsigned long try_i = 0;
    bool full_deactivation_check = false;
    bool some_call_failed = false;
    bool available = false;
    RevenueDecimal goal_ctr = RevenueDecimal::ZERO;
    std::string billing_request_params = check_available_bid_request_params_(
      -1,
      "not called yet",
      now,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      optimize_campaign_ctr_);

    while (true)
    {
      long res_service_index = get_service_index_(
        nullptr,
        now,
        ccg_id,
        nullptr,
        ccg_setter);

      if (res_service_index == -1)
      {
        billing_request_params = check_available_bid_request_params_(
          res_service_index,
          "no available BillingServer index",
          now,
          account_id,
          advertiser_id,
          campaign_id,
          ccg_id,
          ctr,
          optimize_campaign_ctr_);

        full_deactivation_check = true;
        break;
      }

      if (try_i >= max_try_count_)
      {
        break;
      }

      assert(static_cast<unsigned long>(res_service_index) < billing_servers_.size());

      billing_request_params = check_available_bid_request_params_(
        res_service_index,
        nullptr,
        now,
        account_id,
        advertiser_id,
        campaign_id,
        ccg_id,
        ctr,
        optimize_campaign_ctr_);

      AdServer::Grpc::ResponseHolder<Proto::BidResultResponse> check_available_bid_result;
      const BillingServerDescr& billing_server = billing_servers_[res_service_index];
      try
      {
        auto call_result = co_await billing_server.client->co_check_available_bid(
          *check_bid_info);

        if (!call_result.status.ok())
        {
          log_billing_server_grpc_error_(
            logger_.in(),
            FUN,
            res_service_index,
            billing_server.endpoint,
            call_result.status);
        }
        else
        {
          check_available_bid_result = std::move(call_result.response_holder);
        }
      }
      catch(const eh::Exception& ex)
      {
        log_billing_server_exception_(
          logger_.in(),
          FUN,
          res_service_index,
          billing_server.endpoint,
          ex.what());
      }
      catch(...)
      {
        log_billing_server_exception_(
          logger_.in(),
          FUN,
          res_service_index,
          billing_server.endpoint,
          "unknown exception");
      }

      if (DEBUG_BILLING_SERVER_CALL_)
      {
        std::cerr << "check_available_bid call (server #" << res_service_index <<
          "): success_called = " << static_cast<bool>(check_available_bid_result) <<
          ", check_available_bid_result = " <<
          (check_available_bid_result ?
            check_available_bid_result->available() :
            false) << std::endl;
      }

      if (!check_available_bid_result || !check_available_bid_result->available())
      {
        SyncPolicy::WriteGuard lock(lock_);
        cache_[ccg_id].disabled_indexes.emplace(res_service_index, now);
        if (!check_available_bid_result)
        {
          some_call_failed = true;
        }
      }
      else
      {
        available = true;
        goal_ctr = unpack_revenue_decimal_(check_available_bid_result->goal_ctr());
        break;
      }

      ++try_i;
    }

    const bool deactivate_ccg = full_deactivation_check;
    if (deactivate_ccg)
    {
      (void)some_call_failed;
    }

    BillingStateContainer::BidCheckResult result;
    result.deactivate_account = false;
    result.deactivate_advertiser = false;
    result.deactivate_campaign = false;
    result.deactivate_ccg = deactivate_ccg;
    result.available = available;
    result.goal_ctr = goal_ctr;

    if (deactivate_ccg)
    {
      ccg_set_available_(
        ccg_setter,
        ccg_id,
        false,
        goal_ctr,
        now,
        billing_request_params);
    }

    co_return result;
  }

  AdServer::Commons::Awaitable<BillingStateContainer::BidCheckResult>
  BillingStateContainer::Impl::co_confirm_bid(
    const Generics::Time& now,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& account_spent_amount,
    const RevenueDecimal& spent_amount,
    const RevenueDecimal& ctr,
    const ImpRevenueDecimal& imps,
    const ImpRevenueDecimal& clicks,
    const AvailableAndMinCTRSetter* ccg_setter)
  {
    static const char* FUN = "BillingStateContainer::Impl::co_confirm_bid()";

    google::protobuf::Arena request_arena;
    auto* confirm_bid_info = google::protobuf::Arena::CreateMessage<
      Proto::ConfirmBidInfo>(&request_arena);
    fill_bid_(
      *confirm_bid_info->mutable_bid(),
      now,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      optimize_campaign_ctr_);

    confirm_bid_info->set_account_spent_budget(GrpcAlgs::pack_decimal(account_spent_amount));
    confirm_bid_info->set_spent_budget(GrpcAlgs::pack_decimal(spent_amount));
    confirm_bid_info->set_reserved_budget(GrpcAlgs::pack_decimal(RevenueDecimal::ZERO));
    confirm_bid_info->set_imps(GrpcAlgs::pack_decimal(imps));
    confirm_bid_info->set_clicks(GrpcAlgs::pack_decimal(clicks));
    confirm_bid_info->set_forced(false);

    DisabledIndexMap bad_indexes;
    bool available = false;
    RevenueDecimal goal_ctr = RevenueDecimal::ZERO;
    std::string billing_request_params;

    while (true)
    {
      long res_service_index = get_service_index_(
        nullptr,
        now,
        ccg_id,
        confirm_bid_info->forced() ? &bad_indexes : nullptr,
        nullptr);

      if (res_service_index == -1)
      {
        if (confirm_bid_info->forced())
        {
          billing_request_params = confirm_bid_request_params_(
            res_service_index,
            "no available BillingServer index",
            now,
            account_id,
            advertiser_id,
            campaign_id,
            ccg_id,
            ctr,
            *confirm_bid_info);
          break;
        }

        confirm_bid_info->set_forced(true);
        continue;
      }

      auto* confirm_bid_request = google::protobuf::Arena::CreateMessage<
        Proto::ConfirmBidRequest>(&request_arena);
      *confirm_bid_request->mutable_bid() = *confirm_bid_info;

      billing_request_params = confirm_bid_request_params_(
        res_service_index,
        nullptr,
        now,
        account_id,
        advertiser_id,
        campaign_id,
        ccg_id,
        ctr,
        *confirm_bid_info);

      AdServer::Grpc::ResponseHolder<Proto::ConfirmBidResponse> confirm_bid_response;
      const BillingServerDescr& billing_server = billing_servers_[res_service_index];
      try
      {
        auto call_result = co_await billing_server.client->co_confirm_bid(*confirm_bid_request);
        if (!call_result.status.ok())
        {
          log_billing_server_grpc_error_(
            logger_.in(),
            FUN,
            res_service_index,
            billing_server.endpoint,
            call_result.status);
        }
        else
        {
          confirm_bid_response = std::move(call_result.response_holder);
        }
      }
      catch(const eh::Exception& ex)
      {
        log_billing_server_exception_(
          logger_.in(),
          FUN,
          res_service_index,
          billing_server.endpoint,
          ex.what());
      }
      catch(...)
      {
        log_billing_server_exception_(
          logger_.in(),
          FUN,
          res_service_index,
          billing_server.endpoint,
          "unknown exception");
      }

      if (DEBUG_BILLING_SERVER_CALL_)
      {
        std::cerr << "confirm_bid call (server #" << res_service_index <<
          "): success_called = " << static_cast<bool>(confirm_bid_response) <<
          ", confirm_bid_result = " <<
          (confirm_bid_response ?
            confirm_bid_response->result().available() :
            false) <<
          ", forced = " << confirm_bid_info->forced() << std::endl;
      }

      if (!confirm_bid_response ||
        (!confirm_bid_info->forced() && !confirm_bid_response->has_result()))
      {
        if (confirm_bid_info->forced())
        {
          bad_indexes.emplace(res_service_index, now);
        }
        else
        {
          SyncPolicy::WriteGuard lock(lock_);
          cache_[ccg_id].disabled_indexes.emplace(res_service_index, now);
        }
      }
      else
      {
        if (confirm_bid_response->has_bid())
        {
          *confirm_bid_info = confirm_bid_response->bid();
        }

        available = confirm_bid_response->result().available();
        goal_ctr = unpack_revenue_decimal_(confirm_bid_response->result().goal_ctr());
        break;
      }
    }

    BillingStateContainer::BidCheckResult result;
    result.deactivate_account = false;
    result.deactivate_advertiser = false;
    result.deactivate_campaign = false;
    result.deactivate_ccg = !available;
    result.available = available;
    result.goal_ctr = goal_ctr;

    if (!available)
    {
      ccg_set_available_(
        ccg_setter,
        ccg_id,
        false,
        goal_ctr,
        now,
        billing_request_params);
    }

    co_return result;
  }

  BillingStateContainer::BidCheckResult
  BillingStateContainer::Impl::reserve_bid(
    const Generics::Time& /*now*/,
    unsigned long /*account_id*/,
    unsigned long /*advertiser_id*/,
    unsigned long /*campaign_id*/,
    unsigned long /*ccg_id*/,
    const RevenueDecimal& /*amount*/)
    noexcept
  {
    BillingStateContainer::BidCheckResult result;
    result.deactivate_account = false;
    result.deactivate_advertiser = false;
    result.deactivate_campaign = false;
    result.deactivate_ccg = false;
    result.available = true;
    return result;
  }

  long
  BillingStateContainer::Impl::get_service_index_(
    bool* switched,
    const Generics::Time& now,
    unsigned long ccg_id,
    const DisabledIndexMap* disabled_indexes_in,
    const AvailableAndMinCTRSetter* ccg_setter)
    noexcept
  {
    if (switched)
    {
      *switched = false;
    }

    CAvailableAndMinCTRSetter_var new_ccg_setter = ReferenceCounting::add_ref(ccg_setter);

    SyncPolicy::WriteGuard lock(lock_);
    CCGState& ccg_state = cache_[ccg_id];

    if (new_ccg_setter)
    {
      ccg_state.ccg_setter.swap(new_ccg_setter);
    }

    const DisabledIndexMap& disabled_indexes =
      disabled_indexes_in ?
      *disabled_indexes_in : ccg_state.disabled_indexes;

    if (ccg_state.use_count == -1)
    {
      // use random index for distribute loading between consumers
      ccg_state.active_index = Generics::unsafe_rand(billing_servers_.size());
      ccg_state.use_count = 0;
      ccg_state.last_server_switch_time = now;

      fill_planned_server_switch_time_(ccg_id, ccg_state, now);
    }

    bool active_index_is_bad =
      disabled_indexes.find(ccg_state.active_index) != disabled_indexes.end();

    if (active_index_is_bad ||
      ++ccg_state.use_count > max_use_count_ ||
      now > ccg_state.planned_server_switch_time)
    {
      unsigned long count_servers = billing_servers_.size();

      // check all indexes while don't find good, but no more than count servers
      for(size_t i = 0; i < count_servers; ++i)
      {
        ccg_state.active_index = next_index_(ccg_state.active_index);
        ccg_state.use_count = 1;

        if (disabled_indexes.find(ccg_state.active_index) == disabled_indexes.end())
        {
          if (switched)
          {
            *switched = true;
          }

          // service switched
          if (ccg_state.last_server_switch_time != Generics::Time::ZERO)
          {
            const Generics::Time server_use_time = now - ccg_state.last_server_switch_time;
            ccg_state.server_use_times.push_back(server_use_time);
            while (ccg_state.server_use_times.size() > MAX_SIZE_SERVER_USE_TIMES)
            {
              ccg_state.server_use_times.pop_front();
            }

            fill_planned_server_switch_time_(ccg_id, ccg_state, now);
          }

          return static_cast<long>(ccg_state.active_index);
        }
      }

      // not found good index
      return -1;
    }

    return static_cast<long>(ccg_state.active_index);
  }

  unsigned long
  BillingStateContainer::Impl::next_index_(unsigned long serv_index) const
    noexcept
  {
    return ++serv_index % billing_servers_.size();
  }

  void
  BillingStateContainer::Impl::fill_planned_server_switch_time_(
    unsigned long ccg_id,
    CCGState& ccg_state,
    const Generics::Time& now)
    noexcept
  {
    const Generics::Time last_server_switch_time = ccg_state.last_server_switch_time;

    if (!ccg_state.server_use_times.empty())
    {
      Generics::Time sum_time;
      for(auto time_it = ccg_state.server_use_times.begin();
        time_it != ccg_state.server_use_times.end(); ++time_it)
      {
        sum_time += *time_it;
      }

      const Generics::Time avg_time = sum_time / ccg_state.server_use_times.size();
      const Generics::Time use_time = std::max(
        std::min(avg_time, MAX_SERVER_USE_TIME), MIN_SERVER_USE_TIME);

      ccg_state.last_server_switch_time = now;
      ccg_state.planned_server_switch_time = now + use_time;
    }
    else
    {
      ccg_state.planned_server_switch_time = now + MAX_SERVER_USE_TIME;
    }

    if (last_server_switch_time != Generics::Time::ZERO)
    {
      ccg_state.server_use_times.push_back(now - last_server_switch_time);
      while (ccg_state.server_use_times.size() > MAX_SIZE_SERVER_USE_TIMES)
      {
        ccg_state.server_use_times.pop_front();
      }
    }

    SyncPolicy::WriteGuard lock(add_recheck_ccgs_lock_);
    add_recheck_ccgs_[ccg_id] = ccg_state.planned_server_switch_time;
  }

  void
  BillingStateContainer::Impl::run_recheck_ccgs_() noexcept
  {
    static const char* FUN = "BillingStateContainer::Impl::run_recheck_ccgs_()";

    {
      // recheck ccgs
      CCGCheckTimeMap add_recheck_ccgs;

      {
        SyncPolicy::WriteGuard lock(add_recheck_ccgs_lock_);
        add_recheck_ccgs_.swap(add_recheck_ccgs);
      }

      for(auto ccg_it = add_recheck_ccgs.begin(); ccg_it != add_recheck_ccgs.end(); ++ccg_it)
      {
        Generics::Time& check_time = recheck_ccgs_[ccg_it->first];
        check_time = std::max(check_time, ccg_it->second);
      }

      // apply add_recheck_ccgs_ to recheck_ccgs_
      for(auto it = add_recheck_ccgs.begin(); it != add_recheck_ccgs.end(); ++it)
      {
        auto prev_it = recheck_ccgs_.find(it->first);
        if (prev_it != recheck_ccgs_.end())
        {
          Generics::Time& target_time = recheck_ccgs_[it->first];
          target_time = std::min(target_time, it->second);
        }
        else
        {
          recheck_ccgs_.insert(*it);
        }
      }

      if (DEBUG_BILLING_SERVER_CALL_)
      {
        std::cerr << Generics::Time::get_time_of_day().gm_ft() <<
          ": run_recheck_ccgs_: recheck_ccgs_.size() = " <<
          recheck_ccgs_.size() <<
          std::endl;
      }

      // fetch recheck_ccgs_
      const Generics::Time now = Generics::Time::get_time_of_day();
      const Generics::Time reenable_time = now - REENABLE_INDEX_TIME;

      for(auto it = recheck_ccgs_.begin(); it != recheck_ccgs_.end(); )
      {
        if (DEBUG_BILLING_SERVER_CALL_)
        {
          std::cerr << "BillingStateContainer: pre recheck ccg #" << it->first <<
            ", now = " << now.gm_ft() <<
            ", planned check date = " << it->second.gm_ft() << std::endl;
        }

        if (now >= it->second)
        {
          const unsigned long ccg_id = it->first;

          CAvailableAndMinCTRSetter_var ccg_setter;

          {
            SyncPolicy::WriteGuard lock(lock_);
            CCGState& ccg_state = cache_[ccg_id];
            ccg_setter = ReferenceCounting::add_ref(ccg_state.ccg_setter);
            for(auto disable_index_it = ccg_state.disabled_indexes.begin();
              disable_index_it != ccg_state.disabled_indexes.end(); )
            {
              if (disable_index_it->second < reenable_time)
              {
                ccg_state.disabled_indexes.erase(disable_index_it++);
              }
              else
              {
                ++disable_index_it;
              }
            }
          }

          if (DEBUG_BILLING_SERVER_CALL_)
          {
            std::cerr << "BillingStateContainer: recheck ccg #" << ccg_id <<
              ", ccg_setter = " << ccg_setter << std::endl;
          }

          // get_service_index_ will switch index by planned_server_switch_time check
          get_service_index_(
            nullptr, // switched
            now,
            ccg_id,
            nullptr, // disabled_indexes
            nullptr // ccg_setter
            );

          if (DEBUG_BILLING_SERVER_CALL_)
          {
            std::cerr << "BillingStateContainer: ccg #" << ccg_id <<
              " set available" << std::endl;
          }

          ccg_set_available_(
            ccg_setter,
            ccg_id,
            true,
            RevenueDecimal::ZERO,
            now,
            "recheck_ccgs request params: action=set_available"
            ", reason=planned recheck time reached");

          recheck_ccgs_.erase(it++);
        }
        else
        {
          ++it;
        }
      }
    }

    try
    {
      Generics::Goal_var msg = new RecheckCCGTask(this, task_runner_);
      scheduler_->schedule(
        msg,
        Generics::Time::get_time_of_day() + Generics::Time::ONE_SECOND / 10); // 100 ms
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't schedule next recheck ccgs task. "
        "eh::Exception caught:" << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::BILLING_STATE_CONTAINER,
        "ADS-IMPL-?");
    }
  }

  void
  BillingStateContainer::Impl::ccg_set_available_(
    const AvailableAndMinCTRSetter* ccg_setter,
    unsigned long ccg_id,
    bool available,
    const RevenueDecimal& goal_ctr,
    const Generics::Time& now,
    const std::string& billing_request_params)
    noexcept
  {
    if (ccg_setter)
    {
      ccg_setter->set_available(available, goal_ctr);
    }

    if (!available)
    {
      bool new_deactivation = false;
      Generics::Time planned_recheck_time;
      {
        SyncPolicy::WriteGuard lock(add_recheck_ccgs_lock_);
        Generics::Time& planned_time = add_recheck_ccgs_[ccg_id];
        if (planned_time != Generics::Time::ZERO)
        {
          planned_time = std::min(planned_time, now + REENABLE_INDEX_TIME);
        }
        else
        {
          new_deactivation = true;
          planned_time = now + REENABLE_INDEX_TIME;
        }
        planned_recheck_time = planned_time;
      }

      if (new_deactivation)
      {
        Stream::Error ostr;
        ostr << "BillingStateContainer::Impl::ccg_set_available_(): CCG #" <<
          ccg_id << " deactivated by BillingServer response, goal_ctr = " <<
          goal_ctr << ", recheck planned at " << planned_recheck_time;
        if (!billing_request_params.empty())
        {
          ostr << ", " << billing_request_params;
        }
        else
        {
          ostr << ", billing request params: missing";
        }
        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          Aspect::BILLING_STATE_CONTAINER,
          "ADS-ICON-4004");
      }
    }
  }
} // namespace AdServer::CampaignSvcs
