// STD
#include <list>

// UNIXCOMMONS
#include <PrivacyFilter/Filter.hpp>

// THIS
#include <Commons/CorbaAlgs.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <CampaignSvcs/BillingServer/BillingContainer.hpp>
#include <CampaignSvcs/BillingServer/BillingServerImpl.hpp>
#include <CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp>
#include <CampaignSvcs/CampaignServer/BillStatServerSource.hpp>

namespace
{
  template<class Response>
  auto create_grpc_response(const std::uint32_t id_request_grpc)
  {
    auto response = std::make_unique<Response>();
    response->set_id_request_grpc(id_request_grpc);
    return response;
  }

  template<class Response>
  auto create_grpc_error_response(
    const AdServer::CampaignSvcs::Billing::Proto::Error_Type error_type,
    const String::SubString detail,
    const std::uint32_t id_request_grpc)
  {
    auto response = create_grpc_response<Response>(id_request_grpc);
    auto* error = response->mutable_error();
    error->set_type(error_type);
    error->set_description(detail.data(), detail.length());
    return response;
  }
} // namespace

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

  namespace
  {
    RevenueDecimal
    compatibility_cast(const ImpRevenueDecimal& val)
    {
      return RevenueDecimal(ImpRevenueDecimal(val).ceil(8).str());
    }
  }
  
  // BillingServerImpl
  BillingServerImpl::BillingServerImpl(
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

    add_child_object(task_runner_);
    add_child_object(scheduler_);
    add_child_object(billing_processor_);

    Commons::make_repeating_task(
      std::bind(&BillingServerImpl::load_, this),
      task_runner_,
      scheduler_)->deliver();
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "BillingServerImpl::BillingServerImpl(): eh::Exception caught: " << ex.what();
    throw Exception(ostr);
  }

  AdServer::CampaignSvcs::BillingServer::BidResultInfo*
  BillingServerImpl::check_available_bid(
    const AdServer::CampaignSvcs::BillingServer::CheckBidInfo& request_info)
    /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
      AdServer::CampaignSvcs::BillingServer::ImplementationException)*/
  {
    static const char* FUN = "BillingServerImpl::check_available_bid()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      BillingProcessor::Bid bid;
      bid.time = CorbaAlgs::unpack_time(request_info.time);
      bid.account_id = request_info.account_id;
      bid.advertiser_id = request_info.advertiser_id;
      bid.campaign_id = request_info.campaign_id;
      bid.ccg_id = request_info.ccg_id;
      bid.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.ctr);
      bid.optimize_campaign_ctr = request_info.optimize_campaign_ctr;

      const BillingProcessor::BidResult res = billing_accessor->check_available_bid(bid);

      AdServer::CampaignSvcs::BillingServer::BidResultInfo_var bid_result =
        new AdServer::CampaignSvcs::BillingServer::BidResultInfo();
      bid_result->available = res.available;
      bid_result->goal_ctr = CorbaAlgs::pack_decimal(res.goal_ctr);

      return bid_result._retn();
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      CORBACommons::throw_desc<AdServer::CampaignSvcs::
        BillingServer::ImplementationException>(
          ostr.str());
    }

    return nullptr; // unreachable
  }

  BillingServerImpl::CheckAvailableBidResponsePtr
  BillingServerImpl::check_available_bid(
    CheckAvailableBidRequestPtr&& request)
  {
    const auto id_request_grpc = request->id_request_grpc();
    try
    {
      auto billing_accessor = get_accessor_();
      const auto& request_info = request->request_info();

      BillingProcessor::Bid bid;
      bid.time = GrpcAlgs::unpack_time(request_info.time());
      bid.account_id = request_info.account_id();
      bid.advertiser_id = request_info.advertiser_id();
      bid.campaign_id = request_info.campaign_id();
      bid.ccg_id = request_info.ccg_id();
      bid.ctr = GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.ctr());
      bid.optimize_campaign_ctr = request_info.optimize_campaign_ctr();

      const BillingProcessor::BidResult res = billing_accessor->check_available_bid(bid);

      auto response = create_grpc_response<Billing::Proto::CheckAvailableBidResponse>(
        id_request_grpc);
      auto* info = response->mutable_info();
      auto* return_value = info->mutable_return_value();
      return_value->set_available(res.available);
      return_value->set_goal_ctr(GrpcAlgs::pack_decimal(res.goal_ctr));

      return response;
    }
    catch (AdServer::CampaignSvcs::BillingServer::NotReady& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.description;
      auto response = create_grpc_error_response<Billing::Proto::CheckAvailableBidResponse>(
        Billing::Proto::Error_Type::Error_Type_NotReady,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.what();
      auto response = create_grpc_error_response<Billing::Proto::CheckAvailableBidResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: Unknown error";
      auto response = create_grpc_error_response<Billing::Proto::CheckAvailableBidResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
  }

  bool
  BillingServerImpl::reserve_bid(
    const AdServer::CampaignSvcs::BillingServer::ReserveBidInfo& request_info)
    /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
      AdServer::CampaignSvcs::BillingServer::ImplementationException)*/
  {
    static const char* FUN = "BillingServerImpl::reserve_bid()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      BillingProcessor::Bid bid;
      bid.time = CorbaAlgs::unpack_time(request_info.time);
      bid.account_id = request_info.account_id;
      bid.advertiser_id = request_info.advertiser_id;
      bid.campaign_id = request_info.campaign_id;
      bid.ccg_id = request_info.ccg_id;

      return billing_accessor->reserve_bid(
        bid,
        CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.reserve_budget));
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      CORBACommons::throw_desc<AdServer::CampaignSvcs::
        BillingServer::ImplementationException>(
          ostr.str());
    }

    return false;
  }

  BillingServerImpl::ReserveBidResponsePtr
  BillingServerImpl::reserve_bid(
    ReserveBidRequestPtr&& request)
  {
    const auto id_request_grpc = request->id_request_grpc();
    try
    {
      auto billing_accessor = get_accessor_();
      const auto& request_info = request->request_info();

      BillingProcessor::Bid bid;
      bid.time = GrpcAlgs::unpack_time(request_info.time());
      bid.account_id = request_info.account_id();
      bid.advertiser_id = request_info.advertiser_id();
      bid.campaign_id = request_info.campaign_id();
      bid.ccg_id = request_info.ccg_id();

      const auto result = billing_accessor->reserve_bid(
        bid,
        GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.reserve_budget()));

      auto response = create_grpc_response<Billing::Proto::ReserveBidResponse>(
        id_request_grpc);
      auto* const info = response->mutable_info();
      info->set_return_value(result);

      return response;
    }
    catch (AdServer::CampaignSvcs::BillingServer::NotReady& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.description;
      auto response = create_grpc_error_response<Billing::Proto::ReserveBidResponse>(
        Billing::Proto::Error_Type::Error_Type_NotReady,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.what();
      auto response = create_grpc_error_response<Billing::Proto::ReserveBidResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: Unknown error";
      auto response = create_grpc_error_response<Billing::Proto::ReserveBidResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
  }

  AdServer::CampaignSvcs::BillingServer::BidResultInfo*
  BillingServerImpl::confirm_bid(
    AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& request_info)
    /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
      AdServer::CampaignSvcs::BillingServer::ImplementationException)*/
  {
    static const char* FUN = "BillingServerImpl::confirm_bid()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      BillingProcessor::Bid bid;
      bid.time = CorbaAlgs::unpack_time(request_info.time);
      bid.account_id = request_info.account_id;
      bid.advertiser_id = request_info.advertiser_id;
      bid.campaign_id = request_info.campaign_id;
      bid.ccg_id = request_info.ccg_id;
      bid.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.ctr);

      RevenueDecimal account_spent_budget =
        CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.account_spent_budget);
      RevenueDecimal spent_budget =
        CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.spent_budget);
      ImpRevenueDecimal imps = ImpRevenueDecimal(
        CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.imps));
      ImpRevenueDecimal clicks = ImpRevenueDecimal(
        CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.clicks));

      // reserved_budget
      const BillingProcessor::BidResult res = billing_accessor->confirm_bid(
        account_spent_budget,
        spent_budget,
        imps,
        clicks,
        bid,
        request_info.forced);

      // fill remind amounts
      request_info.account_spent_budget = CorbaAlgs::pack_decimal(account_spent_budget);
      request_info.spent_budget = CorbaAlgs::pack_decimal(spent_budget);
      request_info.imps = CorbaAlgs::pack_decimal(compatibility_cast(imps)); // XXX temprary shortify imps
      request_info.clicks = CorbaAlgs::pack_decimal(compatibility_cast(clicks));

      AdServer::CampaignSvcs::BillingServer::BidResultInfo_var bid_result =
        new AdServer::CampaignSvcs::BillingServer::BidResultInfo();
      bid_result->available = res.available;
      bid_result->goal_ctr = CorbaAlgs::pack_decimal(res.goal_ctr);

      return bid_result._retn();
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      CORBACommons::throw_desc<AdServer::CampaignSvcs::
        BillingServer::ImplementationException>(
          ostr.str());
    }

    return nullptr; // unreachable
  }

  BillingServerImpl::ConfirmBidResponsePtr
  BillingServerImpl::confirm_bid(
    ConfirmBidRequestPtr&& request)
  {
    const auto id_request_grpc = request->id_request_grpc();
    try
    {
      auto* request_info = request->mutable_request_info();

      BillingProcessor::Bid bid;
      bid.time = GrpcAlgs::unpack_time(request_info->time());
      bid.account_id = request_info->account_id();
      bid.advertiser_id = request_info->advertiser_id();
      bid.campaign_id = request_info->campaign_id();
      bid.ccg_id = request_info->ccg_id();
      bid.ctr = GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info->ctr());

      RevenueDecimal account_spent_budget =
        GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info->account_spent_budget());
      RevenueDecimal spent_budget =
        GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info->spent_budget());
      ImpRevenueDecimal imps = ImpRevenueDecimal(
        GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info->imps()));
      ImpRevenueDecimal clicks = ImpRevenueDecimal(
        GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info->clicks()));

      // reserved_budget
      auto billing_accessor = get_accessor_();
      const BillingProcessor::BidResult res = billing_accessor->confirm_bid(
        account_spent_budget,
        spent_budget,
        imps,
        clicks,
        bid,
        request_info->forced());

      // fill remind amounts
      request_info->set_account_spent_budget(GrpcAlgs::pack_decimal(account_spent_budget));
      request_info->set_spent_budget(GrpcAlgs::pack_decimal(spent_budget));
      request_info->set_imps(GrpcAlgs::pack_decimal(compatibility_cast(imps)));
      request_info->set_clicks(GrpcAlgs::pack_decimal(compatibility_cast(clicks)));

      auto response = create_grpc_response<Billing::Proto::ConfirmBidResponse>(
        id_request_grpc);
      auto* const info = response->mutable_info();

      auto* return_value = info->mutable_return_value();
      return_value->set_available(res.available);
      return_value->set_goal_ctr(GrpcAlgs::pack_decimal(res.goal_ctr));

      info->set_allocated_request_info(
        request->release_request_info());

      return response;
    }
    catch (AdServer::CampaignSvcs::BillingServer::NotReady& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.description;
      auto response = create_grpc_error_response<Billing::Proto::ConfirmBidResponse>(
        Billing::Proto::Error_Type::Error_Type_NotReady,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.what();
      auto response = create_grpc_error_response<Billing::Proto::ConfirmBidResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: Unknown error";
      auto response = create_grpc_error_response<Billing::Proto::ConfirmBidResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
  }

  void
  BillingServerImpl::add_amount(
    AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq_out remainder_request_seq,
    const AdServer::CampaignSvcs::BillingServer::ConfirmBidSeq& request_seq)
    /*throw(AdServer::CampaignSvcs::BillingServer::NotReady,
      AdServer::CampaignSvcs::BillingServer::ImplementationException)*/
  {
    static const char* FUN = "BillingServerImpl::add_amount()";

    BillingProcessorHolder::Accessor billing_accessor =
      get_accessor_();

    try
    {
      remainder_request_seq = new AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq();
      remainder_request_seq->length(request_seq.length());
      CORBA::ULong remainder_req_i = 0;

      for(CORBA::ULong req_i = 0; req_i < request_seq.length(); ++req_i)
      {
        const AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& request_info =
          request_seq[req_i];

        BillingProcessor::Bid bid;
        bid.time = CorbaAlgs::unpack_time(request_info.time);
        bid.account_id = request_info.account_id;
        bid.advertiser_id = request_info.advertiser_id;
        bid.campaign_id = request_info.campaign_id;
        bid.ccg_id = request_info.ccg_id;
        bid.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.ctr);

        RevenueDecimal account_spent_budget =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.account_spent_budget);
        RevenueDecimal spent_budget =
          CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.spent_budget);
        ImpRevenueDecimal imps = ImpRevenueDecimal(
          CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.imps));
        ImpRevenueDecimal clicks = ImpRevenueDecimal(
          CorbaAlgs::unpack_decimal<RevenueDecimal>(request_info.clicks));

        // reserved_budget
        if(!billing_accessor->confirm_bid(
          account_spent_budget,
          spent_budget,
          imps,
          clicks,
          bid,
          request_info.forced).available)
        {
          AdServer::CampaignSvcs::BillingServer::ConfirmBidRefInfo& remainder_request_info =
            remainder_request_seq[remainder_req_i];
          remainder_request_info.index = req_i;
          remainder_request_info.confirm_bid = request_info;
          remainder_request_info.confirm_bid.account_spent_budget =
            CorbaAlgs::pack_decimal(account_spent_budget);
          remainder_request_info.confirm_bid.spent_budget =
            CorbaAlgs::pack_decimal(spent_budget);
          remainder_request_info.confirm_bid.imps =
            CorbaAlgs::pack_decimal(compatibility_cast(imps));
          remainder_request_info.confirm_bid.clicks =
            CorbaAlgs::pack_decimal(compatibility_cast(clicks));
          ++remainder_req_i;
        }
      }

      remainder_request_seq->length(remainder_req_i);
    }
    catch(const BillingProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BillingProcessor::Exception: " << ex.what();
      CORBACommons::throw_desc<AdServer::CampaignSvcs::
        BillingServer::ImplementationException>(
          ostr.str());
    }
  }

  BillingServerImpl::AddAmountResponsePtr
  BillingServerImpl::add_amount(
    AddAmountRequestPtr&& request)
  {
    const auto id_request_grpc = request->id_request_grpc();
    try
    {
      const auto& request_seq = request->request_seq();
      auto billing_accessor = get_accessor_();

      auto response = create_grpc_response<Billing::Proto::AddAmountResponse>(
        id_request_grpc);
      auto* const info = response->mutable_info();
      auto* remainder_request_seq = info->mutable_remainder_request_seq();
      remainder_request_seq->Reserve(request_seq.size());

      std::uint32_t req_i = 0;
      for (const auto& request_info : request_seq)
      {
        BillingProcessor::Bid bid;
        bid.time = GrpcAlgs::unpack_time(request_info.time());
        bid.account_id = request_info.account_id();
        bid.advertiser_id = request_info.advertiser_id();
        bid.campaign_id = request_info.campaign_id();
        bid.ccg_id = request_info.ccg_id();
        bid.ctr = GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.ctr());

        RevenueDecimal account_spent_budget =
          GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.account_spent_budget());
        RevenueDecimal spent_budget =
          GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.spent_budget());
        ImpRevenueDecimal imps = ImpRevenueDecimal(
          GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.imps()));
        ImpRevenueDecimal clicks = ImpRevenueDecimal(
          GrpcAlgs::unpack_decimal<RevenueDecimal>(request_info.clicks()));

        // reserved_budget
        if (!billing_accessor->confirm_bid(
          account_spent_budget,
          spent_budget,
          imps,
          clicks,
          bid,
          request_info.forced()).available)
        {
          auto* const remainder_request_info = remainder_request_seq->Add();
          remainder_request_info->set_index(req_i);

          auto* confirm_bid = remainder_request_info->mutable_request_info();
          *confirm_bid = request_info;

          confirm_bid->set_time(request_info.time());
          confirm_bid->set_account_id(request_info.account_id());
          confirm_bid->set_advertiser_id(request_info.advertiser_id());
          confirm_bid->set_campaign_id(request_info.campaign_id());
          confirm_bid->set_ccg_id(request_info.ccg_id());
          confirm_bid->set_ctr(request_info.ctr());
          confirm_bid->set_reserved_budget(request_info.reserved_budget());
          confirm_bid->set_forced(request_info.forced());

          confirm_bid->set_account_spent_budget(
            GrpcAlgs::pack_decimal(account_spent_budget));
          confirm_bid->set_spent_budget(
            GrpcAlgs::pack_decimal(spent_budget));
          confirm_bid->set_imps(
            GrpcAlgs::pack_decimal(compatibility_cast(imps)));
          confirm_bid->set_clicks(
            GrpcAlgs::pack_decimal(compatibility_cast(clicks)));
        }

        req_i += 1;
      }

      return response;
    }
    catch (AdServer::CampaignSvcs::BillingServer::NotReady& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.description;
      auto response = create_grpc_error_response<Billing::Proto::AddAmountResponse>(
        Billing::Proto::Error_Type::Error_Type_NotReady,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: "
             << exc.what();
      auto response = create_grpc_error_response<Billing::Proto::AddAmountResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught BillingProcessor::Exception: Unknown error";
      auto response = create_grpc_error_response<Billing::Proto::AddAmountResponse>(
        Billing::Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);

      return response;
    }
  }

  void
  BillingServerImpl::wait_object()
   /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // dump container only when its usage stopped (AccessActiveObject)
    BillingContainer_var billing_container = billing_container_.get();
    if(billing_container)
    {
      billing_container->dump();
    }
  }

  BillingServerImpl::BillingProcessorHolder::Accessor
  BillingServerImpl::get_accessor_()
    /*throw(AdServer::CampaignSvcs::BillingServer::NotReady)*/
  {
    BillingProcessorHolder::Accessor billing_accessor =
      billing_processor_->get_accessor();

    if(!billing_accessor.get())
    {
      throw AdServer::CampaignSvcs::BillingServer::NotReady();
    }

    return billing_accessor;
  }

  Generics::Time
  BillingServerImpl::load_()
    noexcept
  {
    static const char* FUN = "BillingServerImpl::load_()";

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

    if(loaded)
    {
      // exception here can't be processed correctly - crash
      Commons::make_goal_task(
        std::bind(&BillingServerImpl::clear_expired_reservation_, this),
        task_runner_,
        scheduler_,
        CLEAR_EXPIRED_RESERVATION_PERIOD)->deliver();

      // TODO: short update period after fail
      Commons::make_repeating_task(
        std::bind(&BillingServerImpl::update_config_, this),
        task_runner_,
        scheduler_)->deliver();

      Commons::make_repeating_task(
        std::bind(&BillingServerImpl::update_stat_, this),
        task_runner_,
        scheduler_)->deliver();

      Commons::make_goal_task(
        std::bind(&BillingServerImpl::dump_, this),
        task_runner_,
        scheduler_,
        Generics::Time(config_.Storage().dump_period()))->deliver();
    }

    return !loaded ? Generics::Time::get_time_of_day() + RELOAD_PERIOD :
      Generics::Time::ZERO; // reschedule
  }

  Generics::Time
  BillingServerImpl::update_config_()
    noexcept
  {
    static const char* FUN = "BillingServerImpl::update_config_()";

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
          if(!billing_processor_->get_object().in())
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
  BillingServerImpl::update_stat_()
    noexcept
  {
    static const char* FUN = "BillingServerImpl::update_stat_()";

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
  BillingServerImpl::apply_delivery_limitation_config_update_(
    BillingContainer::Config& res_config,
    const AdServer::CampaignSvcs::CampaignServer::
      DeliveryLimitConfigInfo& config)
    /*throw(Exception)*/
  {
    for(CORBA::ULong acc_i = 0; acc_i < config.accounts.length();
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

    for(CORBA::ULong campaign_i = 0; campaign_i < config.campaigns.length();
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

    for(CORBA::ULong ccg_i = 0; ccg_i < config.ccgs.length();
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
  BillingServerImpl::clear_expired_reservation_()
    noexcept
  {
    static const char* FUN = "BillingServerImpl::clear_expired_reservation_()";

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
  BillingServerImpl::dump_()
    noexcept
  {
    static const char* FUN = "BillingServerImpl::dump_()";

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
