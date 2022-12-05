#include <Commons/CorbaAlgs.hpp>

#include "BillingStateContainer.hpp"

namespace AdServer
{
namespace CampaignSvcs
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
  };

  class BillingStateContainer::RecheckCCGTask: public Generics::TaskGoal
  {
  public:
    RecheckCCGTask(
      BillingStateContainer* billing_state_container,
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
    BillingStateContainer* billing_state_container_;
  };

  BillingStateContainer::BillingStateContainer(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const CORBACommons::CorbaObjectRefList& billing_server_refs,
    unsigned long max_use_count,
    bool optimize_campaign_ctr)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      task_runner_(new Generics::TaskRunner(callback, 1)),
      scheduler_(new Generics::Planner(callback)),
      max_use_count_(static_cast<long>(max_use_count)),
      max_try_count_(10),
      optimize_campaign_ctr_(optimize_campaign_ctr),
      billing_server_count_(billing_server_refs.size())
  {
    static const char* FUN = "BillingStateContainer::BillingStateContainer()";

    assert(max_try_count_ > 0);
    assert(!billing_server_refs.empty());

    CORBACommons::CorbaClientAdapter_var corba_client_adapter =
      new CORBACommons::CorbaClientAdapter();

    for(auto it = billing_server_refs.begin();
      it != billing_server_refs.end(); ++it)
    {
      CORBACommons::ObjectPoolRefConfiguration pool_config(corba_client_adapter);
      pool_config.timeout = Generics::Time(2);
      pool_config.object_once = true;
      pool_config.iors_list.push_back(*it);
      billing_servers_.push_back(std::move(BillingServerDescr(
        new SingleBillingServerPool(
          pool_config,
          CORBACommons::ChoosePolicyType::PT_LOOP))));
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": CompositeActiveObject::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    // push lopped RecheckCCGTask
    Generics::Task_var msg = new RecheckCCGTask(this, task_runner_);
    task_runner_->enqueue_task(msg);
  }

  BillingStateContainer::BidCheckResult
  BillingStateContainer::check_available_bid(
    const Generics::Time& now,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& ctr,
    const AvailableAndMinCTRSetter* ccg_setter)
    noexcept
  {
    //std::cerr << "check_available_bid: ccg_id = " << ccg_id << std::endl;

    AdServer::CampaignSvcs::BillingServer::CheckBidInfo check_bid_info;
    check_bid_info.time = CorbaAlgs::pack_time(now);
    check_bid_info.account_id = account_id;
    check_bid_info.advertiser_id = advertiser_id;
    check_bid_info.campaign_id = campaign_id;
    check_bid_info.ccg_id = ccg_id;
    check_bid_info.ctr = CorbaAlgs::pack_decimal(ctr);
    check_bid_info.optimize_campaign_ctr = optimize_campaign_ctr_;

    unsigned long try_i = 0;
    bool full_deactivation_check = false;
    bool some_call_failed = false;
    bool available = false;
    RevenueDecimal goal_ctr = RevenueDecimal::ZERO;

    while(true)
    {
      // Do no more than try max_count quieres to servers.
      // States of servers are shared beetwen quieres
      // if all instance returned "no available bid"
      // make ccg deactivation
      long res_service_index = get_service_index_(
        nullptr, // switched
        now,
        ccg_id,
        nullptr, // disabled_indexes 
        ccg_setter);

      if(res_service_index == -1)
      {
        full_deactivation_check = true;
        break;
      }

      if(try_i >= max_try_count_)
      {
        // much servers are exhausted, don't try more on this call
        break;
      }

      assert(static_cast<unsigned long>(res_service_index) < billing_servers_.size());

      AdServer::CampaignSvcs::BillingServer::BidResultInfo_var check_available_bid_result;

      bool success_called = billing_server_call_(
        check_available_bid_result,
        res_service_index,
        &BillingServer::check_available_bid,
        check_bid_info);

      if(DEBUG_BILLING_SERVER_CALL_)
      {
        std::cerr << "check_available_bid call (server #" << res_service_index <<
          "): success_called = " << success_called <<
          ", check_available_bid_result = " <<
          (check_available_bid_result.ptr() ? check_available_bid_result->available : false) << std::endl;
      }

      if(!success_called ||
        !check_available_bid_result ||
        !check_available_bid_result->available)
      {
        SyncPolicy::WriteGuard lock(lock_);
        cache_[ccg_id].disabled_indexes.insert(
          std::make_pair(res_service_index, now));
        if(!success_called)
        {
          some_call_failed = true;
        }
      }
      else
      {
        // index that allow bid found
        available = true;
        goal_ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          check_available_bid_result->goal_ctr);
        break;
      }

      ++try_i;
    }

    bool deactivate_ccg = full_deactivation_check;

    if(deactivate_ccg)
    {
      // ccg deactivated on all servers
      // TODO: control deactivate period (short period if some_call_failed is true)
      (void)some_call_failed;
    }

    BillingStateContainer::BidCheckResult result;
    result.deactivate_account = false;
    result.deactivate_advertiser = false;
    result.deactivate_campaign = false;
    result.deactivate_ccg = deactivate_ccg;
    result.available = available;
    result.goal_ctr = goal_ctr;

    if(deactivate_ccg)
    {
      ccg_set_available_(ccg_setter, ccg_id, false, goal_ctr, now);
    }

    return result;
  }

  BillingStateContainer::BidCheckResult
  BillingStateContainer::confirm_bid(
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
    noexcept
  {
    AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo confirm_bid_info;
    confirm_bid_info.time = CorbaAlgs::pack_time(now);
    confirm_bid_info.account_id = account_id;
    confirm_bid_info.advertiser_id = advertiser_id;
    confirm_bid_info.campaign_id = campaign_id;
    confirm_bid_info.ccg_id = ccg_id;
    confirm_bid_info.ctr = CorbaAlgs::pack_decimal(ctr);

    confirm_bid_info.account_spent_budget = CorbaAlgs::pack_decimal(account_spent_amount);
    confirm_bid_info.spent_budget = CorbaAlgs::pack_decimal(spent_amount);
    confirm_bid_info.reserved_budget = CorbaAlgs::pack_decimal(RevenueDecimal::ZERO);
    confirm_bid_info.imps = CorbaAlgs::pack_decimal(imps);
    confirm_bid_info.clicks = CorbaAlgs::pack_decimal(clicks);

    // one billing server short way: call with forced flag (amount should be saved) and
    // use call result for deactivate campaign
    confirm_bid_info.forced = false; // DEBUG (billing_servers_.size() == 1);

    DisabledIndexMap bad_indexes;
    //bool some_call_failed = false;
    bool available = false;
    RevenueDecimal goal_ctr = RevenueDecimal::ZERO;
    //unsigned long try_i = 0;

    // fetch all services before success confirm (confirm can't be rollbacked)
    // loop should be interrupted only on success or if no servers that can confirm
    while(true)
    {
      //std::cerr << __func__ << ": try #" << ++try_i << std::endl;

      long res_service_index = get_service_index_(
        nullptr, // switched
        now,
        ccg_id,
        confirm_bid_info.forced ? &bad_indexes : nullptr,
        nullptr);

      if(res_service_index == -1)
      {
        // all indexes disabled
        if(confirm_bid_info.forced)
        {
          break;
        }
        else
        {
          // repeat loop in forced mode (don't use bad ref's)
          confirm_bid_info.forced = true;
          continue;
        }
      }

      AdServer::CampaignSvcs::BillingServer::BidResultInfo_var confirm_bid_result;

      // confirm_bid_info is inout, can be confirmed part of amount
      bool success_called = billing_server_call_(
        confirm_bid_result,
        res_service_index,
        &BillingServer::confirm_bid,
        confirm_bid_info);

      if(DEBUG_BILLING_SERVER_CALL_)
      {
        std::cerr << "confirm_bid call (server #" << res_service_index <<
          "): success_called = " << success_called <<
          ", confirm_bid_result = " << (confirm_bid_result ? confirm_bid_result->available : false) <<
          ", forced = " << confirm_bid_info.forced << std::endl;
      }

      if(!success_called || (!confirm_bid_info.forced && !confirm_bid_result))
      {
        if(confirm_bid_info.forced)
        {
          bad_indexes.insert(
            std::make_pair(res_service_index, now));
        }
        else
        {
          SyncPolicy::WriteGuard lock(lock_);
          cache_[ccg_id].disabled_indexes.insert(
            std::make_pair(res_service_index, now));
        }

        /*
        if(!success_called)
        {
          some_call_failed = true;
        }
        */
      }
      else // success_called && (confirm_bid_result || confirm_bid_info.forced)
      {
        // index that saved bid found, but if confirm_bid_result is false
        // we got confirm failure on all available servers (in non forced loop)
        available = confirm_bid_result->available;
        goal_ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(confirm_bid_result->goal_ctr);
        break;
      }
    }

    if(!available) // deactivate_ccg
    {
      // ccg deactivated on all servers
      // TODO: control deactivate period (short period if some_call_failed is true)
    }

    BillingStateContainer::BidCheckResult result;
    result.deactivate_account = false;
    result.deactivate_campaign = false;
    result.deactivate_advertiser = false;
    result.deactivate_ccg = !available;
    result.available = available;
    result.goal_ctr = goal_ctr;

    if(!available)
    {
      ccg_set_available_(ccg_setter, ccg_id, false, goal_ctr, now);
    }

    return result;
  }

  BillingStateContainer::BidCheckResult
  BillingStateContainer::reserve_bid(
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
  BillingStateContainer::get_service_index_(
    bool* switched,
    const Generics::Time& now,
    unsigned long ccg_id,
    const DisabledIndexMap* disabled_indexes_in,
    const AvailableAndMinCTRSetter* ccg_setter)
    noexcept
  {
    if(switched)
    {
      *switched = false;
    }

    CAvailableAndMinCTRSetter_var new_ccg_setter = ReferenceCounting::add_ref(ccg_setter);

    SyncPolicy::WriteGuard lock(lock_);
    CCGState& ccg_state = cache_[ccg_id];

    if(new_ccg_setter)
    {
      ccg_state.ccg_setter.swap(new_ccg_setter);
    }

    const DisabledIndexMap& disabled_indexes =
      disabled_indexes_in ?
      *disabled_indexes_in : ccg_state.disabled_indexes;

    if(ccg_state.use_count == -1)
    {
      // use random index for distribute loading between consumers
      ccg_state.active_index = Generics::safe_rand(billing_servers_.size());
      ccg_state.use_count = 0;
      ccg_state.last_server_switch_time = now;

      fill_planned_server_switch_time_(ccg_id, ccg_state, now);
    }

    bool active_index_is_bad = 
      disabled_indexes.find(ccg_state.active_index) != disabled_indexes.end();

    if(active_index_is_bad ||
      ++ccg_state.use_count > max_use_count_ ||
      now > ccg_state.planned_server_switch_time)
    {
      unsigned long count_servers = billing_servers_.size();

      // check all indexes while don't find good, but no more than count servers
      for(size_t i = 0; i < count_servers; ++i)
      {
        ccg_state.active_index = next_index_(ccg_state.active_index);
        ccg_state.use_count = 1;

        if(disabled_indexes.find(ccg_state.active_index) == disabled_indexes.end())
        {
          if(switched)
          {
            *switched = true;
          }

          // service switched
          if(ccg_state.last_server_switch_time != Generics::Time::ZERO)
          {
            const Generics::Time server_use_time = now - ccg_state.last_server_switch_time;
            ccg_state.server_use_times.push_back(server_use_time);
            while(ccg_state.server_use_times.size() > MAX_SIZE_SERVER_USE_TIMES)
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
  BillingStateContainer::next_index_(unsigned long serv_index) const
    noexcept
  {
    return ++serv_index % billing_servers_.size();
  }

  template<typename CallType,
    typename CallResultType,
    typename CallArgType>
  bool
  BillingStateContainer::billing_server_call_(
    CallResultType& call_result,
    unsigned long service_index,
    CallType call,
    CallArgType&& call_arg)
    noexcept
  {
    static const char* FUN = "BillingStateContainer::billing_server_call_()";

    try
    {
      SingleBillingServerPool::ObjectHandlerType billing_server =
        billing_servers_[service_index].pool->get_object<Exception>(logger_);

      try
      {
        call_result = ((*billing_server).*call)(call_arg);
        return true;
      }
      catch(const AdServer::CampaignSvcs::BillingServer::NotReady&)
      {
        Stream::Error ostr;
        ostr << FUN << ": BillingServer::NotReady caught";
        billing_server.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          Aspect::BILLING_STATE_CONTAINER,
          "ADS-ICON-4003");
      }
      catch(const AdServer::CampaignSvcs::BillingServer::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": BillingServer::ImplementationException caught: " <<
          e.description;
        billing_server.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::BILLING_STATE_CONTAINER,
          "ADS-ICON-4003");
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": CORBA::SystemException caught: " << e;
        billing_server.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::BILLING_STATE_CONTAINER,
          "ADS-ICON-4003");
      }
    }
    catch(const eh::Exception&) // NoGoodReference,NoFreeObject,InvalidReference
    {}

    return false;
  }  

  void
  BillingStateContainer::fill_planned_server_switch_time_(
    unsigned long ccg_id,
    CCGState& ccg_state,
    const Generics::Time& now)
    noexcept
  {
    const Generics::Time last_server_switch_time = ccg_state.last_server_switch_time;

    if(!ccg_state.server_use_times.empty())
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

    if(last_server_switch_time != Generics::Time::ZERO)
    {
      ccg_state.server_use_times.push_back(now - last_server_switch_time);
      while(ccg_state.server_use_times.size() > MAX_SIZE_SERVER_USE_TIMES)
      {
        ccg_state.server_use_times.pop_front();
      }
    }

    SyncPolicy::WriteGuard lock(add_recheck_ccgs_lock_);
    add_recheck_ccgs_[ccg_id] = ccg_state.planned_server_switch_time;
  }

  void
  BillingStateContainer::run_recheck_ccgs_() noexcept
  {
    static const char* FUN = "BillingStateContainer::run_recheck_ccgs_()";

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
        if(prev_it != recheck_ccgs_.end())
        {
          Generics::Time& target_time = recheck_ccgs_[it->first];
          target_time = std::min(target_time, it->second);
        }
        else
        {
          recheck_ccgs_.insert(*it);
        }
      }

      if(DEBUG_BILLING_SERVER_CALL_)
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
        if(DEBUG_BILLING_SERVER_CALL_)
        {
          std::cerr << "BillingStateContainer: pre recheck ccg #" << it->first <<
            ", now = " << now.gm_ft() <<
            ", planned check date = " << it->second.gm_ft() << std::endl;
        }

        if(now >= it->second)
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
              if(disable_index_it->second < reenable_time)
              {
                ccg_state.disabled_indexes.erase(disable_index_it++);
              }
              else
              {
                ++disable_index_it;
              }
            }
          }

          if(DEBUG_BILLING_SERVER_CALL_)
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

          if(DEBUG_BILLING_SERVER_CALL_)
          {
            std::cerr << "BillingStateContainer: ccg #" << ccg_id <<
              " set available" << std::endl;
          }

          ccg_set_available_(ccg_setter, ccg_id, true, RevenueDecimal::ZERO, now);

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
  BillingStateContainer::ccg_set_available_(
    const AvailableAndMinCTRSetter* ccg_setter,
    unsigned long ccg_id,
    bool available,
    const RevenueDecimal& goal_ctr,
    const Generics::Time& now)
    noexcept
  {
    if(ccg_setter)
    {
      ccg_setter->set_available(available, goal_ctr);
    }

    if(!available)
    {
      SyncPolicy::WriteGuard lock(add_recheck_ccgs_lock_);
      Generics::Time& planned_time = add_recheck_ccgs_[ccg_id];
      if(planned_time != Generics::Time::ZERO)
      {
        planned_time = std::min(planned_time, now + REENABLE_INDEX_TIME);
      }
      else
      {
        planned_time = now + REENABLE_INDEX_TIME;
      }
    }
  }
}
}
