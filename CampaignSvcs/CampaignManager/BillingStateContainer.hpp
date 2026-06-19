#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Generics/CompositeActiveObject.hpp>

#include <Commons/Coro.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace Generics
{
  class ActiveObjectCallback;
  class Time;
}

namespace Logging
{
  class Logger;
}

namespace AdServer::CampaignSvcs
{
  class AvailableAndMinCTRSetter;

  // BillingStateContainer
  // wrapper for delivery limits checking (BillingServer)
  //
  // TODO: background deactivated campaigns checking (CompositeActiveObject for this)
  //
  class BillingStateContainer:
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct BidCheckResult
    {
      bool deactivate_account;
      bool deactivate_advertiser;
      bool deactivate_campaign;
      bool deactivate_ccg;
      bool available;
      RevenueDecimal goal_ctr;
    };

    /*
     * max_use_count : number of calls after that need to switch BillingServer
     */
    BillingStateContainer(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      std::vector<std::string> billing_server_refs,
      unsigned long max_use_count,
      bool optimize_campaign_ctr);

    BidCheckResult
    check_available_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      const AvailableAndMinCTRSetter* ccg_setter)
      noexcept;

    AdServer::Commons::Task<BidCheckResult>
    co_check_available_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      const AvailableAndMinCTRSetter* ccg_setter);

    BidCheckResult
    confirm_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& account_spent_amount,
      const RevenueDecimal& spent_amount,
      const RevenueDecimal& ctr,
      const RevenueDecimal& imps,
      const RevenueDecimal& clicks,
      const AvailableAndMinCTRSetter* ccg_setter)
      noexcept;

    AdServer::Commons::Task<BidCheckResult>
    co_confirm_bid(
      const Generics::Time& now,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& account_spent_amount,
      const RevenueDecimal& spent_amount,
      const RevenueDecimal& ctr,
      const RevenueDecimal& imps,
      const RevenueDecimal& clicks,
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

    void clear_cache() noexcept;

  protected:
    virtual
    ~BillingStateContainer() noexcept override;

  private:
    class Impl;

  private:
    const std::unique_ptr<Impl> impl_;
  };

  typedef ReferenceCounting::QualPtr<BillingStateContainer>
    BillingStateContainer_var;
}
