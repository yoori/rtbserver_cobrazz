#include <list>
#include <vector>
#include <iterator>

#include <PrivacyFilter/Filter.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>
#include <CampaignSvcs/CampaignServer/BillStatServerSource.hpp>

#include "BillingContainer.hpp"
#include "BillingServerCore.hpp"

namespace Aspect
{
  const char BILLING_SERVER[] = "BillingServer";
}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const Generics::Time RELOAD_PERIOD(60);
    const Generics::Time CLEAR_EXPIRED_RESERVATION_PERIOD(1); // one second
    const Generics::Time CONFIG_UPDATE_AFTER_FAIL_PERIOD(10);
    const Generics::Time STAT_UPDATE_AFTER_FAIL_PERIOD(10);
  }

  // BillingServerCore
  BillingServerCore::BillingServerCore(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const BillingServerConfig& config)
    /*throw(Exception)*/
  try
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 5)),
      config_(config),
      billing_processor_(new BillingProcessorHolder())
  {
    CORBACommons::CorbaClientAdapter_var corba_client_adapter =
      new CORBACommons::CorbaClientAdapter();

    const CORBACommons::CorbaObjectRefList campaign_server_refs =
      Config::CorbaConfigReader::read_multi_corba_ref(
        config.CampaignServerCorbaRef());

    campaign_servers_.reset(
      new AdServer::CampaignSvcs::CampaignServerPool(
        campaign_server_refs,
        corba_client_adapter,
        CORBACommons::ChoosePolicyType::PT_PERSISTENT,
        Generics::Time(10) // bad period
        ));

    bill_stat_source_ = new BillStatServerSource(
      logger_,
      config_.service_index(),
      campaign_server_refs);

    add_child_object(task_runner_.in());
    add_child_object(scheduler_.in());
    add_child_object(billing_processor_.in());

    Commons::make_repeating_task(
      std::bind(&BillingServerCore::load_, this),
      task_runner_,
      scheduler_)->deliver();
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "BillingServerCore::BillingServerCore(): eh::Exception caught: " << ex.what();
    throw Exception(ostr);
  }

  BillingServerCore::BidResultInfo
  BillingServerCore::check_available_bid(
    const CheckBidInfo& request_info)
  {
    static const char* FUN = "BillingServerCore::check_available_bid()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      const BillingProcessor::BidResult res =
        billing_accessor->check_available_bid(request_info.bid);

      return BidResultInfo{
        res.available,
        res.goal_ctr,
        res.unavailable_reason};
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      throw ImplementationException(ostr);
    }

    return BidResultInfo{}; // unreachable
  }

  bool
  BillingServerCore::reserve_bid(
    const ReserveBidInfo& request_info)
  {
    static const char* FUN = "BillingServerCore::reserve_bid()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      return billing_accessor->reserve_bid(
        request_info.bid,
        request_info.reserve_budget);
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      throw ImplementationException(ostr);
    }

    return false;
  }

  BillingServerCore::BidResultInfo
  BillingServerCore::confirm_bid(
    ConfirmBidInfo& request_info)
  {
    static const char* FUN = "BillingServerCore::confirm_bid()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      RevenueDecimal account_spent_budget = request_info.account_spent_budget;
      RevenueDecimal spent_budget = request_info.spent_budget;
      ImpRevenueDecimal imps = request_info.imps;
      ImpRevenueDecimal clicks = request_info.clicks;

      // reserved_budget
      const BillingProcessor::BidResult res = billing_accessor->confirm_bid(
        account_spent_budget,
        spent_budget,
        imps,
        clicks,
        request_info.bid,
        request_info.forced);

      // fill remind amounts
      request_info.account_spent_budget = account_spent_budget;
      request_info.spent_budget = spent_budget;
      request_info.imps = imps;
      request_info.clicks = clicks;

      return BidResultInfo{
        res.available,
        res.goal_ctr,
        res.unavailable_reason};
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      throw ImplementationException(ostr);
    }

    return BidResultInfo{}; // unreachable
  }

  BillingServerCore::ConfirmBidRefSeq
  BillingServerCore::add_amount(
    const ConfirmBidSeq& request_seq)
  {
    static const char* FUN = "BillingServerCore::add_amount()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      ConfirmBidRefSeq remainder_request_seq;

      for (std::size_t req_i = 0; req_i < request_seq.size(); ++req_i)
      {
        const ConfirmBidInfo& request_info =
          request_seq[req_i];

        RevenueDecimal account_spent_budget = request_info.account_spent_budget;
        RevenueDecimal spent_budget = request_info.spent_budget;
        ImpRevenueDecimal imps = request_info.imps;
        ImpRevenueDecimal clicks = request_info.clicks;

        // reserved_budget
        if (!billing_accessor->confirm_bid(
          account_spent_budget,
          spent_budget,
          imps,
          clicks,
          request_info.bid,
          request_info.forced).available)
        {
          ConfirmBidRefInfo remainder_request_info;
          remainder_request_info.index = req_i;
          remainder_request_info.confirm_bid = request_info;
          remainder_request_info.confirm_bid.account_spent_budget = account_spent_budget;
          remainder_request_info.confirm_bid.spent_budget = spent_budget;
          remainder_request_info.confirm_bid.imps = imps;
          remainder_request_info.confirm_bid.clicks = clicks;
          remainder_request_seq.emplace_back(std::move(remainder_request_info));
        }
      }

      return remainder_request_seq;
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      throw ImplementationException(ostr);
    }

    return ConfirmBidRefSeq{}; // unreachable
  }

  void
  BillingServerCore::wait_object()
   /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // dump container only when its usage stopped (AccessActiveObject)
    BillingContainer_var billing_container = billing_container_.get();
    if (billing_container)
    {
      billing_container->dump();
    }
  }

  BillingServerCore::BillingProcessorHolder::Accessor
  BillingServerCore::get_accessor_()
  {
    BillingProcessorHolder::Accessor billing_accessor =
      billing_processor_->get_accessor();

    if (!billing_accessor.get())
    {
      throw NotReady("BillingServer is not ready");
    }

    return billing_accessor;
  }

  Generics::Time
  BillingServerCore::load_()
    noexcept
  {
    static const char* FUN = "BillingServerCore::load_()";

    bool loaded = false;

    try
    {
      assert(config_.service_count() > 0); // xsd should check this

      BillingContainer_var billing_container = new BillingContainer(
        logger_,
        config_.Storage().dir(),
        Generics::Time(config_.max_stat_delay()),
        config_.service_count());

      billing_container_ = billing_container;
      // initialize billing_processor_ only when will be updated budget config (update_config_)

      loaded = true;
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_SERVER) << FUN <<
        ": caught eh::Exception: " <<
        ex.what();
    }

    if (loaded)
    {
      // exception here can't be processed correctly - crash
      Commons::make_goal_task(
        std::bind(&BillingServerCore::clear_expired_reservation_, this),
        task_runner_,
        scheduler_,
        CLEAR_EXPIRED_RESERVATION_PERIOD)->deliver();

      // TODO: short update period after fail
      Commons::make_repeating_task(
        std::bind(&BillingServerCore::update_config_, this),
        task_runner_,
        scheduler_)->deliver();

      Commons::make_repeating_task(
        std::bind(&BillingServerCore::update_stat_, this),
        task_runner_,
        scheduler_)->deliver();

      Commons::make_goal_task(
        std::bind(&BillingServerCore::dump_, this),
        task_runner_,
        scheduler_,
        Generics::Time(config_.Storage().dump_period()))->deliver();
    }

    return !loaded ? Generics::Time::get_time_of_day() + RELOAD_PERIOD :
      Generics::Time::ZERO; // reschedule
  }

  Generics::Time
  BillingServerCore::update_config_()
    noexcept
  {
    static const char* FUN = "BillingServerCore::update_config_()";

    try
    {
      for (;;)
      {
        AdServer::CampaignSvcs::CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<AdServer::CampaignSvcs::CampaignServerPool::Exception>(
            logger(),
            Logging::Logger::EMERGENCY,
            Aspect::BILLING_SERVER,
            "ADS_ICON-4001",
            config_.service_index(),
            config_.service_index());

        try
        {
          AdServer::CampaignSvcs::CampaignServer::DeliveryLimitConfigInfo_var
            delivery_limitation_config =
              campaign_server->get_delivery_limit_config();

          BillingContainer::Config_var new_config = new BillingContainer::Config;

          apply_delivery_limitation_config_update_(
            *new_config,
            *delivery_limitation_config);

          BillingContainer_var billing_container = billing_container_.get();
          // this task can be started only when container initialized
          assert(billing_container.in());
          billing_container->config(new_config);

          // initialize billing processor only after first config load
          if (!billing_processor_->get_object().in())
          {
            *billing_processor_ = billing_container;
          }

          return Generics::Time::get_time_of_day() +
            Generics::Time(config_.config_update_period());
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't update config, "
            "caught CampaignServer::ImplementationException: " <<
            ex.description;
          campaign_server.release_bad(ostr.str());
          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::BILLING_SERVER,
            "ADS-IMPL-?");
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
        {
          String::SubString descr("CampaignServer not ready.");
          campaign_server.release_bad(descr);
          logger()->log(descr,
            Logging::Logger::NOTICE,
            Aspect::BILLING_SERVER,
            "ADS-ICON-4001");
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't update expression channels, "
            "caught CORBA::SystemException: " <<
            ex;
          campaign_server.release_bad(ostr.str());
          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::BILLING_SERVER,
            "ADS-ICON-4001");
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_SERVER) << FUN <<
        ": Caught eh::Exception: " <<
        ex.what();
    }

    return Generics::Time::get_time_of_day() + std::min(
      CONFIG_UPDATE_AFTER_FAIL_PERIOD, Generics::Time(config_.config_update_period()));
  }

  Generics::Time
  BillingServerCore::update_stat_()
    noexcept
  {
    static const char* FUN = "BillingServerCore::update_stat_()";

    try
    {
      BillStatSource::Stat_var bill_stat = bill_stat_source_->update(
        nullptr,
        Generics::Time::get_time_of_day());

      BillingContainer_var billing_container = billing_container_.get();
      // this task can be started only when container initialized
      assert(billing_container.in());
      billing_container->stat(bill_stat);

      //bill_stat->print(std::cout, "> ");
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_SERVER) << FUN <<
        ": Caught eh::Exception: " <<
        ex.what();

      return Generics::Time::get_time_of_day() + std::min(
        STAT_UPDATE_AFTER_FAIL_PERIOD, Generics::Time(config_.stat_update_period()));
    }

    return Generics::Time::get_time_of_day() + config_.stat_update_period();
  }

  void
  BillingServerCore::apply_delivery_limitation_config_update_(
    BillingContainer::Config& res_config,
    const AdServer::CampaignSvcs::CampaignServer::
      DeliveryLimitConfigInfo& config)
    /*throw(Exception)*/
  {
    for (CORBA::ULong acc_i = 0; acc_i < config.accounts.length();
        ++acc_i)
    {
      auto& acc_info = config.accounts[acc_i];

      BillingContainer::Config::Account res_account;
      res_account.active = acc_info.active;
      res_account.time_offset = CorbaAlgs::unpack_time(
        acc_info.time_offset);
      res_account.budget = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        acc_info.budget);

      res_config.accounts.insert(std::make_pair(
        acc_info.account_id,
        res_account));
    }

    for (CORBA::ULong campaign_i = 0; campaign_i < config.campaigns.length();
        ++campaign_i)
    {
      auto& campaign_info = config.campaigns[campaign_i];

      BillingContainer::Config::Campaign res_campaign;
      res_campaign.active = campaign_info.active;
      res_campaign.time_offset = CorbaAlgs::unpack_time(
        campaign_info.time_offset);
      unpack_delivery_limits(res_campaign, campaign_info.delivery_limits);

      res_config.campaigns.insert(std::make_pair(
        campaign_info.campaign_id,
        res_campaign));
    }

    for (CORBA::ULong ccg_i = 0; ccg_i < config.ccgs.length();
        ++ccg_i)
    {
      auto& ccg_info = config.ccgs[ccg_i];

      BillingContainer::Config::CCG res_ccg;
      res_ccg.active = ccg_info.active;
      res_ccg.time_offset = CorbaAlgs::unpack_time(
        ccg_info.time_offset);
      unpack_delivery_limits(res_ccg, ccg_info.delivery_limits);
      res_ccg.campaign_id = ccg_info.campaign_id;
      res_ccg.imp_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(ccg_info.imp_amount);
      res_ccg.click_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(ccg_info.click_amount);

      res_config.ccgs.insert(std::make_pair(
        ccg_info.ccg_id,
        res_ccg));
    }
  }

  void
  BillingServerCore::clear_expired_reservation_()
    noexcept
  {
    static const char* FUN = "BillingServerCore::clear_expired_reservation_()";

    try
    {
      BillingContainer_var billing_container = billing_container_.get();
      assert(billing_container.in());
      billing_container->clear_expired_reservation(
        Generics::Time::get_time_of_day() -
        Generics::Time(config_.reserve_timeout()));
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_SERVER) << FUN <<
        ": Can't clear reservations. Caught eh::Exception: " <<
        ex.what();
    }
  }

  void
  BillingServerCore::dump_()
    noexcept
  {
    static const char* FUN = "BillingServerCore::dump_()";

    try
    {
      BillingContainer_var billing_container = billing_container_.get();
      assert(billing_container.in());
      billing_container->dump();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_SERVER) << FUN <<
        ": Can't dump storage. Caught eh::Exception: " <<
        ex.what();
    }
  }
} /*CampaignSvcs*/
} /*AdServer*/
