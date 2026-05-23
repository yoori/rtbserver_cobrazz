#include "BillingServerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <utility>
#include <vector>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <CampaignSvcs/BillingServer/BillingServerGrpc.grpc.pb.h>
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
  }

  class BillingServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      BillingServerGrpc::ServiceImpl,
      Proto::BillingServerGrpc,
      Proto::BillingServerGrpc::AsyncService>
  {
    using AsyncService = Proto::BillingServerGrpc::AsyncService;

  public:
    explicit ServiceImpl(BillingServerCore* core);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CALL(
          Proto::CheckBidRequest,
          Proto::BidResultResponse,
          check_available_bid),
        MAKE_GRPC_CALL(
          Proto::ReserveBidRequest,
          Proto::ReserveBidResponse,
          reserve_bid),
        MAKE_GRPC_CALL(
          Proto::ConfirmBidRequest,
          Proto::ConfirmBidResponse,
          confirm_bid),
        MAKE_GRPC_CALL(
          Proto::AddAmountRequest,
          Proto::AddAmountResponse,
          add_amount));
    }

    void check_available_bid(
      const Proto::CheckBidRequest& request,
      Proto::BidResultResponse& response,
      ::grpc::Status& result_status) const;

    void reserve_bid(
      const Proto::ReserveBidRequest& request,
      Proto::ReserveBidResponse& response,
      ::grpc::Status& result_status) const;

    void confirm_bid(
      const Proto::ConfirmBidRequest& request,
      Proto::ConfirmBidResponse& response,
      ::grpc::Status& result_status) const;

    void add_amount(
      const Proto::AddAmountRequest& request,
      Proto::AddAmountResponse& response,
      ::grpc::Status& result_status) const;

  private:
    static void
    translate_exception_(const char* fun, ::grpc::Status& result_status);

  private:
    const BillingServerCore_var core_;
  };

  BillingServerGrpc::ServiceImpl::ServiceImpl(BillingServerCore* core)
    : core_(ReferenceCounting::add_ref(core))
  {}

  void
  BillingServerGrpc::ServiceImpl::check_available_bid(
    const Proto::CheckBidRequest& request,
    Proto::BidResultResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN =
      "BillingServerGrpc::ServiceImpl::check_available_bid()";

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

  void
  BillingServerGrpc::ServiceImpl::reserve_bid(
    const Proto::ReserveBidRequest& request,
    Proto::ReserveBidResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN = "BillingServerGrpc::ServiceImpl::reserve_bid()";

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

  void
  BillingServerGrpc::ServiceImpl::confirm_bid(
    const Proto::ConfirmBidRequest& request,
    Proto::ConfirmBidResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN = "BillingServerGrpc::ServiceImpl::confirm_bid()";

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

  void
  BillingServerGrpc::ServiceImpl::add_amount(
    const Proto::AddAmountRequest& request,
    Proto::AddAmountResponse& response,
    ::grpc::Status& result_status) const
  {
    static const char* FUN = "BillingServerGrpc::ServiceImpl::add_amount()";

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
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        billing_server_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(core)))
  {
    add_child_object(impl_);
  }

  BillingServerGrpc::~BillingServerGrpc() noexcept
  {}
}
