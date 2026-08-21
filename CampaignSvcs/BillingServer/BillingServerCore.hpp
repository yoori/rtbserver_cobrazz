#pragma once

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <vector>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/AccessActiveObject.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/CampaignSvcs/BillingServerConfig.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServerPool.hpp>
#include <CampaignSvcs/CampaignServer/BillStatSource.hpp>

#include "BillingContainer.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  /**
   * Implementation of BillingServer.
   */
  class BillingServerCore:
    public virtual Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(ImplementationException, Exception);

    typedef xsd::AdServer::Configuration::BillingServerConfigType
      BillingServerConfig;

    struct CheckBidInfo
    {
      BillingProcessor::Bid bid;
    };

    struct ReserveBidInfo
    {
      BillingProcessor::Bid bid;
      RevenueDecimal reserve_budget;
    };

    struct ConfirmBidInfo
    {
      BillingProcessor::Bid bid;
      RevenueDecimal account_spent_budget;
      RevenueDecimal spent_budget;
      RevenueDecimal reserved_budget;
      ImpRevenueDecimal imps;
      ImpRevenueDecimal clicks;
      bool forced = false;
    };

    using ConfirmBidSeq = std::vector<ConfirmBidInfo>;

    struct ConfirmBidRefInfo
    {
      std::size_t index = 0;
      ConfirmBidInfo confirm_bid;
    };

    using ConfirmBidRefSeq = std::vector<ConfirmBidRefInfo>;

    struct BidResultInfo
    {
      bool available = false;
      RevenueDecimal goal_ctr = RevenueDecimal::ZERO;
      BillingProcessor::BidUnavailableReason unavailable_reason =
        BillingProcessor::BidUnavailableReason::UNSPECIFIED;
    };

    BillingServerCore(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const BillingServerConfig& billing_server_config)
      /*throw(Exception)*/;

    BidResultInfo
    check_available_bid(
      const CheckBidInfo& request_info);

    BidResultInfo
    confirm_bid(
      ConfirmBidInfo& request_info);

    bool
    reserve_bid(
      const ReserveBidInfo& request_info);

    ConfirmBidRefSeq
    add_amount(
      const ConfirmBidSeq& request_seq);

    virtual void
    wait_object()
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

    Logging::Logger*
    logger() noexcept;

  protected:
    typedef Sync::Policy::PosixThreadRW SyncPolicy;

    typedef AdServer::Commons::AccessActiveObject<
      BillingProcessor_var>
      BillingProcessorHolder;

    typedef ReferenceCounting::SmartPtr<BillingProcessorHolder>
      BillingProcessorHolder_var;

  protected:
    virtual
    ~BillingServerCore() noexcept {};

    BillingProcessorHolder::Accessor
    get_accessor_();

    void
    apply_delivery_limitation_config_update_(
      BillingContainer::Config& res_config,
      const AdServer::CampaignSvcs::CampaignServer::
        DeliveryLimitConfigInfo& config)
      /*throw(Exception)*/;

    // tasks
    Generics::Time
    load_() noexcept;

    Generics::Time
    update_config_() noexcept;

    Generics::Time
    update_stat_() noexcept;

    void
    clear_expired_reservation_()
      noexcept;

    void
    dump_() noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;

    const BillingServerConfig config_;

    CampaignServerPoolPtr campaign_servers_;
    BillStatSource_var bill_stat_source_;
    BillingProcessorHolder_var billing_processor_;
    ReferenceCounting::PtrHolder<BillingContainer_var> billing_container_;
  };

  typedef ReferenceCounting::SmartPtr<BillingServerCore>
    BillingServerCore_var;

} /* CampaignSvcs */
} /* AdServer */

namespace AdServer
{
namespace CampaignSvcs
{
  inline
  Logging::Logger*
  BillingServerCore::logger() noexcept
  {
    return logger_;
  }
}
}
