#include "BillingServerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <utility>
#include <vector>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/ExecutorPool.hpp>
#include <CampaignSvcs/BillingServer/BillingServerGrpc.grpc.pb.h>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    constexpr const char billing_server_grpc_aspect[] = "BillingServerGrpc";

    namespace Proto = adserver::campaign_svcs::billing_server;

    RevenueDecimal
    unpack_revenue_decimal(const std::string& value)
    {
      return value.empty() ?
        RevenueDecimal::ZERO :
        GrpcAlgs::unpack_decimal<RevenueDecimal>(value);
    }

    ImpRevenueDecimal
    unpack_imp_revenue_decimal(const std::string& value)
    {
      return value.empty() ?
        ImpRevenueDecimal::ZERO :
        GrpcAlgs::unpack_decimal<ImpRevenueDecimal>(value);
    }

    Generics::Time
    unpack_time(const std::string& value)
    {
      return value.empty() ? Generics::Time::ZERO : GrpcAlgs::unpack_time(value);
    }

    BillingProcessor::Bid
    adapt_bid(const Proto::BidInfo& source)
    {
      BillingProcessor::Bid result;
      result.time = unpack_time(source.time());
      result.account_id = source.account_id();
      result.advertiser_id = source.advertiser_id();
      result.campaign_id = source.campaign_id();
      result.ccg_id = source.ccg_id();
      result.ctr = unpack_revenue_decimal(source.ctr());
      result.optimize_campaign_ctr = source.optimize_campaign_ctr();
      return result;
    }

    void
    fill_bid(Proto::BidInfo& target, const BillingProcessor::Bid& source)
    {
      target.set_time(GrpcAlgs::pack_time(source.time));
      target.set_account_id(source.account_id);
      target.set_advertiser_id(source.advertiser_id);
      target.set_campaign_id(source.campaign_id);
      target.set_ccg_id(source.ccg_id);
      target.set_ctr(GrpcAlgs::pack_decimal(source.ctr));
      target.set_optimize_campaign_ctr(source.optimize_campaign_ctr);
    }

    BillingServerCore::ConfirmBidInfo
    adapt_confirm_bid(const Proto::ConfirmBidInfo& source)
    {
      BillingServerCore::ConfirmBidInfo result;
      result.bid = adapt_bid(source.bid());
      result.account_spent_budget =
        unpack_revenue_decimal(source.account_spent_budget());
      result.spent_budget = unpack_revenue_decimal(source.spent_budget());
      result.reserved_budget = unpack_revenue_decimal(source.reserved_budget());
      result.imps = unpack_imp_revenue_decimal(source.imps());
      result.clicks = unpack_imp_revenue_decimal(source.clicks());
      result.forced = source.forced();
      return result;
    }

    void
    fill_confirm_bid(
      Proto::ConfirmBidInfo& target,
      const BillingServerCore::ConfirmBidInfo& source)
    {
      fill_bid(*target.mutable_bid(), source.bid);
      target.set_account_spent_budget(
        GrpcAlgs::pack_decimal(source.account_spent_budget));
      target.set_spent_budget(GrpcAlgs::pack_decimal(source.spent_budget));
      target.set_reserved_budget(GrpcAlgs::pack_decimal(source.reserved_budget));
      target.set_imps(GrpcAlgs::pack_decimal(source.imps));
      target.set_clicks(GrpcAlgs::pack_decimal(source.clicks));
      target.set_forced(source.forced);
    }

    void
    fill_bid_result(
      Proto::BidResultResponse& target,
      const BillingServerCore::BidResultInfo& source)
    {
      target.set_available(source.available);
      target.set_goal_ctr(GrpcAlgs::pack_decimal(source.goal_ctr));
    }

    ::grpc::Status
    error_status(::grpc::StatusCode code, const char* fun, const char* what)
    {
      Stream::Error ostr;
      ostr << fun << ": " << what;
      const std::string message = ostr.str().str();
      return AdServer::Grpc::error_status(code, message.c_str());
    }

    std::size_t
    resolve_max_batch_split(
      std::size_t configured,
      std::size_t process_threads)
    {
      return std::max<std::size_t>(
        1,
        configured != 0 ? configured : process_threads);
    }
  }

  class BillingServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      BillingServerGrpc::ServiceImpl,
      Proto::BillingServerGrpc,
      Proto::BillingServerGrpc::AsyncService>
  {
    using AsyncService = Proto::BillingServerGrpc::AsyncService;

  public:
    ServiceImpl(
      BillingServerCore* core,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      std::size_t max_batch_split);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          Proto::CheckBidRequest,
          Proto::BidResultResponse,
          check_available_bid,
          co_check_available_bid,
          &ServiceImpl::hash_check_available_bid),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          Proto::ReserveBidRequest,
          Proto::ReserveBidResponse,
          reserve_bid,
          co_reserve_bid,
          &ServiceImpl::hash_reserve_bid),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          Proto::ConfirmBidRequest,
          Proto::ConfirmBidResponse,
          confirm_bid,
          co_confirm_bid,
          &ServiceImpl::hash_confirm_bid),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          Proto::AddAmountRequest,
          Proto::AddAmountResponse,
          add_amount,
          co_add_amount));
    }

    std::size_t distributed_batch_max_split() const noexcept override;

    AdServer::Grpc::GrpcCoroutine co_handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const override;

    AdServer::Grpc::GrpcCoroutine co_check_available_bid(
      const Proto::CheckBidRequest& request,
      Proto::BidResultResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_reserve_bid(
      const Proto::ReserveBidRequest& request,
      Proto::ReserveBidResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_confirm_bid(
      const Proto::ConfirmBidRequest& request,
      Proto::ConfirmBidResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_add_amount(
      const Proto::AddAmountRequest& request,
      Proto::AddAmountResponse& response,
      ::grpc::Status& result_status) const;

    static std::size_t hash_check_available_bid(
      const Proto::CheckBidRequest& request);

    static std::size_t hash_reserve_bid(
      const Proto::ReserveBidRequest& request);

    static std::size_t hash_confirm_bid(
      const Proto::ConfirmBidRequest& request);

  private:
    static std::size_t hash_bid_(const Proto::BidInfo& bid);

    static void
    translate_exception_(const char* fun, ::grpc::Status& result_status);

  private:
    const BillingServerCore_var core_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::size_t max_batch_split_;
  };

  BillingServerGrpc::ServiceImpl::ServiceImpl(
    BillingServerCore* core,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    std::size_t max_batch_split)
    : core_(ReferenceCounting::add_ref(core)),
      executor_pool_(std::move(executor_pool)),
      max_batch_split_(max_batch_split)
  {}

  std::size_t
  BillingServerGrpc::ServiceImpl::distributed_batch_max_split()
    const noexcept
  {
    return max_batch_split_;
  }

  AdServer::Grpc::GrpcCoroutine
  BillingServerGrpc::ServiceImpl::co_handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    co_await AdServer::Grpc::GrpcServiceBase::co_handle_batch_request(
      batch_request,
      batch_response);
  }

  AdServer::Grpc::GrpcCoroutine
  BillingServerGrpc::ServiceImpl::co_check_available_bid(
    const Proto::CheckBidRequest& request,
    Proto::BidResultResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN =
      "BillingServerGrpc::ServiceImpl::check_available_bid()";

    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    try
    {
      BillingServerCore::CheckBidInfo core_request;
      core_request.bid = adapt_bid(request.bid());
      fill_bid_result(
        response,
        core_->check_available_bid(core_request));
      result_status = ::grpc::Status::OK;
    }
    catch(...)
    {
      translate_exception_(FUN, result_status);
    }
  }

  AdServer::Grpc::GrpcCoroutine
  BillingServerGrpc::ServiceImpl::co_reserve_bid(
    const Proto::ReserveBidRequest& request,
    Proto::ReserveBidResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN = "BillingServerGrpc::ServiceImpl::reserve_bid()";

    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    try
    {
      BillingServerCore::ReserveBidInfo core_request;
      core_request.bid = adapt_bid(request.bid());
      core_request.reserve_budget =
        unpack_revenue_decimal(request.reserve_budget());
      response.set_reserved(core_->reserve_bid(core_request));
      result_status = ::grpc::Status::OK;
    }
    catch(...)
    {
      translate_exception_(FUN, result_status);
    }
  }

  AdServer::Grpc::GrpcCoroutine
  BillingServerGrpc::ServiceImpl::co_confirm_bid(
    const Proto::ConfirmBidRequest& request,
    Proto::ConfirmBidResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN = "BillingServerGrpc::ServiceImpl::confirm_bid()";

    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    try
    {
      BillingServerCore::ConfirmBidInfo core_request =
        adapt_confirm_bid(request.bid());
      const BillingServerCore::BidResultInfo result =
        core_->confirm_bid(core_request);
      fill_bid_result(*response.mutable_result(), result);
      fill_confirm_bid(*response.mutable_bid(), core_request);
      result_status = ::grpc::Status::OK;
    }
    catch(...)
    {
      translate_exception_(FUN, result_status);
    }
  }

  AdServer::Grpc::GrpcCoroutine
  BillingServerGrpc::ServiceImpl::co_add_amount(
    const Proto::AddAmountRequest& request,
    Proto::AddAmountResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN = "BillingServerGrpc::ServiceImpl::add_amount()";

    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    try
    {
      BillingServerCore::ConfirmBidSeq core_requests;
      core_requests.reserve(request.requests_size());

      for(int i = 0; i < request.requests_size(); ++i)
      {
        core_requests.emplace_back(adapt_confirm_bid(request.requests(i)));
      }

      const BillingServerCore::ConfirmBidRefSeq core_remainders =
        core_->add_amount(core_requests);

      for(const BillingServerCore::ConfirmBidRefInfo& source : core_remainders)
      {
        Proto::ConfirmBidRefInfo* target =
          response.add_remainder_requests();
        target->set_index(source.index);
        fill_confirm_bid(*target->mutable_confirm_bid(), source.confirm_bid);
      }

      result_status = ::grpc::Status::OK;
    }
    catch(...)
    {
      translate_exception_(FUN, result_status);
    }
  }

  std::size_t
  BillingServerGrpc::ServiceImpl::hash_check_available_bid(
    const Proto::CheckBidRequest& request)
  {
    return hash_bid_(request.bid());
  }

  std::size_t
  BillingServerGrpc::ServiceImpl::hash_reserve_bid(
    const Proto::ReserveBidRequest& request)
  {
    return hash_bid_(request.bid());
  }

  std::size_t
  BillingServerGrpc::ServiceImpl::hash_confirm_bid(
    const Proto::ConfirmBidRequest& request)
  {
    return hash_bid_(request.bid().bid());
  }

  std::size_t
  BillingServerGrpc::ServiceImpl::hash_bid_(const Proto::BidInfo& bid)
  {
    std::size_t result = bid.account_id();
    result = result * 31 + bid.advertiser_id();
    result = result * 31 + bid.campaign_id();
    result = result * 31 + bid.ccg_id();
    return result;
  }

  void
  BillingServerGrpc::ServiceImpl::translate_exception_(
    const char* fun,
    ::grpc::Status& result_status)
  {
    try
    {
      throw;
    }
    catch(const BillingServerCore::NotReady& ex)
    {
      result_status = error_status(::grpc::StatusCode::UNAVAILABLE, fun, ex.what());
    }
    catch(const BillingServerCore::ImplementationException& ex)
    {
      result_status = error_status(::grpc::StatusCode::INTERNAL, fun, ex.what());
    }
    catch(const BillingServerCore::Exception& ex)
    {
      result_status = error_status(::grpc::StatusCode::INTERNAL, fun, ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = error_status(::grpc::StatusCode::INTERNAL, fun, ex.what());
    }
  }

  BillingServerGrpc::BillingServerGrpc(
    BillingServerCore* core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t process_threads,
    std::size_t cq_threads,
    std::size_t max_split)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      max_batch_split_(resolve_max_batch_split(max_split, process_threads)),
      executor_pool_(std::make_shared<AdServer::Commons::ExecutorPool>(
        Generics::ActiveObjectCallback_var(
          new Logging::ActiveObjectCallbackImpl(
            logger,
            "",
            billing_server_grpc_aspect)),
        std::max<std::size_t>(1, process_threads))),
      impl_(std::make_shared<Impl>(
        logger,
        billing_server_grpc_aspect,
        bind_address_,
        cq_threads != 0 ? cq_threads : process_threads,
        std::make_unique<ServiceImpl>(
          core,
          executor_pool_,
          max_batch_split_)))
  {
    add_child_object(executor_pool_);
    add_child_object(impl_);
  }

  BillingServerGrpc::~BillingServerGrpc() noexcept
  {}
}
