#pragma once

#include <CORBACommons/ServantImpl.hpp>
#include <CampaignSvcs/BillingServer/BillingServer_s.hpp>

#include "BillingServerCore.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  class BillingServerImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl<
      POA_AdServer::CampaignSvcs::BillingServer>
  {
  public:
    explicit BillingServerImpl(BillingServerCore* core) noexcept;

    AdServer::CampaignSvcs::BillingServer::BidResultInfo*
    check_available_bid(
      const AdServer::CampaignSvcs::BillingServer::CheckBidInfo& request_info)
      override;

    AdServer::CampaignSvcs::BillingServer::BidResultInfo*
    confirm_bid(
      AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& request_info)
      override;

    bool
    reserve_bid(
      const AdServer::CampaignSvcs::BillingServer::ReserveBidInfo& request_info)
      override;

    void
    add_amount(
      AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq_out
        remainder_request_seq,
      const AdServer::CampaignSvcs::BillingServer::ConfirmBidSeq& request_seq)
      override;

  protected:
    ~BillingServerImpl() noexcept override = default;

  private:
    static BillingServerCore::CheckBidInfo
    adapt_check_bid_(
      const AdServer::CampaignSvcs::BillingServer::CheckBidInfo& source);

    static BillingServerCore::ReserveBidInfo
    adapt_reserve_bid_(
      const AdServer::CampaignSvcs::BillingServer::ReserveBidInfo& source);

    static BillingServerCore::ConfirmBidInfo
    adapt_confirm_bid_(
      const AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& source);

    static void
    fill_confirm_bid_(
      AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& target,
      const BillingServerCore::ConfirmBidInfo& source);

    static AdServer::CampaignSvcs::BillingServer::BidResultInfo*
    adapt_bid_result_(const BillingServerCore::BidResultInfo& source);

    static void
    translate_exception_(const char* fun);

  private:
    BillingServerCore_var core_;
  };

  typedef ReferenceCounting::SmartPtr<BillingServerImpl>
    BillingServerImpl_var;
}
}
