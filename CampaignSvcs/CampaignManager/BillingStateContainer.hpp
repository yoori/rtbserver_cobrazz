#ifndef CAMPAIGNMANAGER_BILLINGSTATECONTAINER_HPP_
#define CAMPAIGNMANAGER_BILLINGSTATECONTAINER_HPP_

#include <deque>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Scheduler.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/CorbaObject.hpp>

#include <CampaignSvcs/BillingServer/BillingServer.hpp>
#include "AvailableAndMinCTRSetter.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  // BillingStateContainer
  // wrapper for delivery limits checking (BillingServer)
  //
  // TODO: background deactivated campaigns checking (CompositeActiveObject for this)
  //
  class BillingStateContainer:
    public virtual Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
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
      const CORBACommons::CorbaObjectRefList& billing_server_refs,
      unsigned long max_use_count,
      bool optimize_campaign_ctr)
      /*throw (eh::Exception)*/;

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
    typedef Sync::Policy::PosixThreadRW SyncPolicy;

    typedef std::map<unsigned long, Generics::Time>
      DisabledIndexMap;

    struct CCGState
    {
      typedef std::map<unsigned long, Generics::Time>
        NonAvailableIndexMap;

      CCGState()
        : active_index(0),
          use_count(-1)
      {}

      unsigned long active_index;
      long use_count;
      // service id => disable time
      DisabledIndexMap disabled_indexes;

      std::deque<Generics::Time> server_use_times;
      Generics::Time last_server_switch_time;
      Generics::Time planned_server_switch_time;

      CAvailableAndMinCTRSetter_var ccg_setter;
    };

    typedef std::map<unsigned long, CCGState> CCGStateMap;

    struct CCGServerSwitchTime
    {
      unsigned long ccg_id;
      Generics::Time planned_server_switch_time;
    };

    typedef std::deque<CCGServerSwitchTime> CCGServerSwitchTimeArray;

    // use pool for implement one get after bad (
    //   ObjectPoolConfiguration::object_once)
    typedef CORBACommons::ObjectPool<
      AdServer::CampaignSvcs::BillingServer,
      CORBACommons::ObjectPoolRefConfiguration>
      SingleBillingServerPool;

    struct BillingServerDescr
    {
      BillingServerDescr(SingleBillingServerPool* pool_val)
        : pool(pool_val)
      {}

      BillingServerDescr(BillingServerDescr&& init)
        : pool(std::move(init.pool))
      {
        //std::swap(pool, init.pool);
      }

      BillingServerDescr&
      operator=(BillingServerDescr&& init)
      {
        std::swap(pool, init.pool);
        return *this;
      }

      std::unique_ptr<SingleBillingServerPool> pool;
    };

    typedef std::vector<BillingServerDescr> BillingServerDescrArray;
    typedef std::map<unsigned long, Generics::Time> CCGCheckTimeMap;

    class RecheckCCGTask;

  protected:
    virtual
    ~BillingStateContainer() noexcept = default;

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

    template<typename CallType,
      typename CallResultType,
      typename CallArgType>
    bool
    billing_server_call_(
      CallResultType& call_result,
      unsigned long service_index,
      CallType call,
      CallArgType&& call_arg)
      noexcept;

    void
    fill_planned_server_switch_time_(
      unsigned long ccg_id,
      CCGState& ccg_state,
      const Generics::Time& now)
      noexcept;

    // periodically recheck campaigns
    // 1) by number of ad requests
    // 2) by fixed time period
    // 3) by avg time period used for previous billing servers
    // 4) by min(fixed time period, avg time period used for previous billing servers)
    //
    void
    run_recheck_ccgs_() noexcept;

    void
    ccg_set_available_(
      const AvailableAndMinCTRSetter* ccg_setter,
      unsigned long ccg_id,
      bool available,
      const RevenueDecimal& goal_ctr,
      const Generics::Time& now)
      noexcept;

  protected:
    Logging::Logger_var logger_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;

    const long max_use_count_;
    const unsigned long max_try_count_;
    const bool optimize_campaign_ctr_;
    unsigned long billing_server_count_;
    BillingServerDescrArray billing_servers_;

    mutable SyncPolicy::Mutex lock_;
    CCGStateMap cache_;

    // recheck_ccgs_: thread unsafe - read and write only from RecheckCCGTask
    CCGCheckTimeMap recheck_ccgs_;

    mutable SyncPolicy::Mutex add_recheck_ccgs_lock_;
    CCGCheckTimeMap add_recheck_ccgs_;
  };

  typedef ReferenceCounting::QualPtr<BillingStateContainer>
    BillingStateContainer_var;
}
}

namespace AdServer
{
namespace CampaignSvcs
{
  inline
  void BillingStateContainer::clear_cache() noexcept
  {
    SyncPolicy::WriteGuard lock(lock_);
    cache_.clear();
  }
}
}

#endif /*CAMPAIGNMANAGER_BILLINGSTATECONTAINER_HPP_*/
