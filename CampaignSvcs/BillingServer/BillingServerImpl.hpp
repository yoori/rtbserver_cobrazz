#ifndef CAMPAIGNSVCS_BILLINGSERVERIMPL_HPP
#define CAMPAIGNSVCS_BILLINGSERVERIMPL_HPP

// UNIXCOMMONS
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

// THIS
#include <Commons/CorbaConfig.hpp>
#include <Commons/AccessActiveObject.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/CampaignSvcs/BillingServerConfig.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServerPool.hpp>
#include <CampaignSvcs/CampaignServer/BillStatSource.hpp>
#include <CampaignSvcs/BillingServer/BillingServer_s.hpp>
#include <CampaignSvcs/BillingServer/BillingContainer.hpp>
#include <CampaignSvcs/BillingServer/proto/BillingServer_service.cobrazz.pb.hpp>

namespace AdServer
{
namespace CampaignSvcs
{
  /**
   * Implementation of BillingServer.
   */
  class BillingServerImpl final:
    public virtual CORBACommons::ReferenceCounting::ServantImpl<
      POA_AdServer::CampaignSvcs::BillingServer>,
    public virtual Generics::CompositeActiveObject
  {
  public:
    using BillingServerConfig = xsd::AdServer::Configuration::BillingServerConfigType;
    using CheckAvailableBidRequestPtr = std::unique_ptr<Billing::Proto::CheckAvailableBidRequest>;
    using CheckAvailableBidResponsePtr = std::unique_ptr<Billing::Proto::CheckAvailableBidResponse>;
    using ReserveBidRequestPtr = std::unique_ptr<Billing::Proto::ReserveBidRequest>;
    using ReserveBidResponsePtr = std::unique_ptr<Billing::Proto::ReserveBidResponse>;
    using ConfirmBidRequestPtr = std::unique_ptr<Billing::Proto::ConfirmBidRequest>;
    using ConfirmBidResponsePtr = std::unique_ptr<Billing::Proto::ConfirmBidResponse>;
    using AddAmountRequestPtr = std::unique_ptr<Billing::Proto::AddAmountRequest>;
    using AddAmountResponsePtr = std::unique_ptr<Billing::Proto::AddAmountResponse>;

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    BillingServerImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const BillingServerConfig& billing_server_config)
      /*throw(Exception)*/;

    virtual AdServer::CampaignSvcs::BillingServer::BidResultInfo*
    check_available_bid(
      const AdServer::CampaignSvcs::BillingServer::CheckBidInfo& request_info)
      /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
        AdServer::CampaignSvcs::BillingServer::ImplementationException)*/;

    CheckAvailableBidResponsePtr check_available_bid(
      CheckAvailableBidRequestPtr&& request);

    virtual AdServer::CampaignSvcs::BillingServer::BidResultInfo*
    confirm_bid(
      AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& request_info)
      /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
        AdServer::CampaignSvcs::BillingServer::ImplementationException)*/;

    ConfirmBidResponsePtr confirm_bid(ConfirmBidRequestPtr&& request);

    virtual bool
    reserve_bid(
      const AdServer::CampaignSvcs::BillingServer::ReserveBidInfo& request_info)
      /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
        AdServer::CampaignSvcs::BillingServer::ImplementationException)*/;

    ReserveBidResponsePtr
    reserve_bid(ReserveBidRequestPtr&& request);

    virtual void
    add_amount(
      AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq_out remainder_request_seq,
      const AdServer::CampaignSvcs::BillingServer::ConfirmBidSeq& request_seq)
      /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
        AdServer::CampaignSvcs::BillingServer::ImplementationException)*/;

    AddAmountResponsePtr
    add_amount(AddAmountRequestPtr&& request);

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
    ~BillingServerImpl() noexcept {};

    BillingProcessorHolder::Accessor
    get_accessor_()
      /*throw(AdServer::CampaignSvcs::BillingServer::NotReady)*/;

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

  typedef ReferenceCounting::SmartPtr<BillingServerImpl>
    BillingServerImpl_var;

} /* CampaignSvcs */
} /* AdServer */

namespace AdServer
{
namespace CampaignSvcs
{
  inline
  Logging::Logger*
  BillingServerImpl::logger() noexcept
  {
    return logger_;
  }
}
}

#endif /*CAMPAIGNSVCS_BILLINGSERVERIMPL_HPP*/
