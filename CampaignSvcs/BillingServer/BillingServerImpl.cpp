#include <Commons/CorbaAlgs.hpp>

#include "BillingServerImpl.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    RevenueDecimal
    compatibility_cast(const ImpRevenueDecimal& val)
    {
      return RevenueDecimal(ImpRevenueDecimal(val).ceil(8).str());
    }
  }

  BillingServerImpl::BillingServerImpl(BillingServerCore* core) noexcept
    : core_(ReferenceCounting::add_ref(core))
  {}

  AdServer::CampaignSvcs::BillingServer::BidResultInfo*
  BillingServerImpl::check_available_bid(
    const AdServer::CampaignSvcs::BillingServer::CheckBidInfo& request_info)
  {
    static const char* FUN = "BillingServerImpl::check_available_bid()";

    try
    {
      return adapt_bid_result_(
        core_->check_available_bid(adapt_check_bid_(request_info)));
    }
    catch(...)
    {
      translate_exception_(FUN);
    }

    return nullptr;
  }

  bool
  BillingServerImpl::reserve_bid(
    const AdServer::CampaignSvcs::BillingServer::ReserveBidInfo& request_info)
  {
    static const char* FUN = "BillingServerImpl::reserve_bid()";

    try
    {
      return core_->reserve_bid(adapt_reserve_bid_(request_info));
    }
    catch(...)
    {
      translate_exception_(FUN);
    }

    return false;
  }

  AdServer::CampaignSvcs::BillingServer::BidResultInfo*
  BillingServerImpl::confirm_bid(
    AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& request_info)
  {
    static const char* FUN = "BillingServerImpl::confirm_bid()";

    try
    {
      BillingServerCore::ConfirmBidInfo core_request =
        adapt_confirm_bid_(request_info);
      BillingServerCore::BidResultInfo result = core_->confirm_bid(core_request);
      fill_confirm_bid_(request_info, core_request);
      return adapt_bid_result_(result);
    }
    catch(...)
    {
      translate_exception_(FUN);
    }

    return nullptr;
  }

  void
  BillingServerImpl::add_amount(
    AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq_out
      remainder_request_seq,
    const AdServer::CampaignSvcs::BillingServer::ConfirmBidSeq& request_seq)
  {
    static const char* FUN = "BillingServerImpl::add_amount()";

    try
    {
      BillingServerCore::ConfirmBidSeq core_requests;
      core_requests.reserve(request_seq.length());
      for(CORBA::ULong i = 0; i < request_seq.length(); ++i)
      {
        core_requests.emplace_back(adapt_confirm_bid_(request_seq[i]));
      }

      const BillingServerCore::ConfirmBidRefSeq core_remainders =
        core_->add_amount(core_requests);

      remainder_request_seq =
        new AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq();
      remainder_request_seq->length(core_remainders.size());

      for(CORBA::ULong i = 0; i < core_remainders.size(); ++i)
      {
        const BillingServerCore::ConfirmBidRefInfo& source = core_remainders[i];
        AdServer::CampaignSvcs::BillingServer::ConfirmBidRefInfo& target =
          remainder_request_seq[i];
        target.index = source.index;
        fill_confirm_bid_(target.confirm_bid, source.confirm_bid);
      }
    }
    catch(...)
    {
      translate_exception_(FUN);
    }
  }

  BillingServerCore::CheckBidInfo
  BillingServerImpl::adapt_check_bid_(
    const AdServer::CampaignSvcs::BillingServer::CheckBidInfo& source)
  {
    BillingServerCore::CheckBidInfo result;
    result.bid.time = CorbaAlgs::unpack_time(source.time);
    result.bid.account_id = source.account_id;
    result.bid.advertiser_id = source.advertiser_id;
    result.bid.campaign_id = source.campaign_id;
    result.bid.ccg_id = source.ccg_id;
    result.bid.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(source.ctr);
    result.bid.optimize_campaign_ctr = source.optimize_campaign_ctr;
    return result;
  }

  BillingServerCore::ReserveBidInfo
  BillingServerImpl::adapt_reserve_bid_(
    const AdServer::CampaignSvcs::BillingServer::ReserveBidInfo& source)
  {
    BillingServerCore::ReserveBidInfo result;
    result.bid.time = CorbaAlgs::unpack_time(source.time);
    result.bid.account_id = source.account_id;
    result.bid.advertiser_id = source.advertiser_id;
    result.bid.campaign_id = source.campaign_id;
    result.bid.ccg_id = source.ccg_id;
    result.bid.ctr = RevenueDecimal::ZERO;
    result.bid.optimize_campaign_ctr = false;
    result.reserve_budget =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(source.reserve_budget);
    return result;
  }

  BillingServerCore::ConfirmBidInfo
  BillingServerImpl::adapt_confirm_bid_(
    const AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& source)
  {
    BillingServerCore::ConfirmBidInfo result;
    result.bid.time = CorbaAlgs::unpack_time(source.time);
    result.bid.account_id = source.account_id;
    result.bid.advertiser_id = source.advertiser_id;
    result.bid.campaign_id = source.campaign_id;
    result.bid.ccg_id = source.ccg_id;
    result.bid.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(source.ctr);
    result.bid.optimize_campaign_ctr = false;
    result.account_spent_budget =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(source.account_spent_budget);
    result.spent_budget =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(source.spent_budget);
    result.reserved_budget =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(source.reserved_budget);
    result.imps = ImpRevenueDecimal(
      CorbaAlgs::unpack_decimal<RevenueDecimal>(source.imps));
    result.clicks = ImpRevenueDecimal(
      CorbaAlgs::unpack_decimal<RevenueDecimal>(source.clicks));
    result.forced = source.forced;
    return result;
  }

  void
  BillingServerImpl::fill_confirm_bid_(
    AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& target,
    const BillingServerCore::ConfirmBidInfo& source)
  {
    target.time = CorbaAlgs::pack_time(source.bid.time);
    target.account_id = source.bid.account_id;
    target.advertiser_id = source.bid.advertiser_id;
    target.campaign_id = source.bid.campaign_id;
    target.ccg_id = source.bid.ccg_id;
    target.ctr = CorbaAlgs::pack_decimal(source.bid.ctr);
    target.account_spent_budget =
      CorbaAlgs::pack_decimal(source.account_spent_budget);
    target.spent_budget = CorbaAlgs::pack_decimal(source.spent_budget);
    target.reserved_budget = CorbaAlgs::pack_decimal(source.reserved_budget);
    target.imps = CorbaAlgs::pack_decimal(compatibility_cast(source.imps));
    target.clicks = CorbaAlgs::pack_decimal(compatibility_cast(source.clicks));
    target.forced = source.forced;
  }

  AdServer::CampaignSvcs::BillingServer::BidResultInfo*
  BillingServerImpl::adapt_bid_result_(
    const BillingServerCore::BidResultInfo& source)
  {
    AdServer::CampaignSvcs::BillingServer::BidResultInfo_var result =
      new AdServer::CampaignSvcs::BillingServer::BidResultInfo();
    result->available = source.available;
    result->goal_ctr = CorbaAlgs::pack_decimal(source.goal_ctr);
    return result._retn();
  }

  void
  BillingServerImpl::translate_exception_(const char* fun)
  {
    try
    {
      throw;
    }
    catch(const BillingServerCore::NotReady& ex)
    {
      CORBACommons::throw_desc<AdServer::CampaignSvcs::BillingServer::NotReady>(
        String::SubString(ex.what()));
    }
    catch(const BillingServerCore::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << fun << ": " << ex.what();
      CORBACommons::throw_desc<
        AdServer::CampaignSvcs::BillingServer::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << fun << ": caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<
        AdServer::CampaignSvcs::BillingServer::ImplementationException>(
          ostr.str());
    }
  }
}
}
