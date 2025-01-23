#ifndef CAMPAIGNMANAGER_GRPCBILLINGSTATECONTAINER_HPP
#define CAMPAIGNMANAGER_GRPCBILLINGSTATECONTAINER_HPP

// BOOST
#include <boost/functional/hash.hpp>

// STD
#include <deque>
#include <shared_mutex>
#include <unordered_map>

// UNIXCOMMONS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Scheduler.hpp>
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/PoolClientFactory.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// USERVER
#include <engine/task/task_processor.hpp>

// THIS
#include <CampaignSvcs/BillingServer/proto/BillingServer_client.cobrazz.pb.hpp>
#include <Commons/GrpcAlgs.hpp>

namespace AdServer::CampaignSvcs
{

class GrpcBillingStateContainer final:
  public virtual Generics::CompositeActiveObject,
  public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  struct AvailableAndMinCTRSetter: public virtual ReferenceCounting::Interface
  {
    friend class GrpcBillingStateContainer;

  protected:
    virtual void set_available(
      const bool available_val,
      const RevenueDecimal& goal_ctr) const noexcept = 0;
  };
  using CAvailableAndMinCTRSetter_var =
    ReferenceCounting::ConstPtr<AvailableAndMinCTRSetter>;

  struct BidCheckResult final
  {
    BidCheckResult() = default;

    bool deactivate_account = false;
    bool deactivate_advertiser = false;
    bool deactivate_campaign = false;
    bool deactivate_ccg = false;
    bool available = false;
    RevenueDecimal goal_ctr = RevenueDecimal::ZERO;
  };

  struct ConfirmBidInfo final
  {
    ConfirmBidInfo() = default;

    Generics::Time time = Generics::Time::ZERO;
    std::uint32_t account_id = 0;
    std::uint32_t advertiser_id = 0;
    std::uint32_t campaign_id = 0;
    std::uint32_t ccg_id = 0;
    CTRDecimal ctr = CTRDecimal::ZERO;
    RevenueDecimal account_spent_budget = RevenueDecimal::ZERO;
    RevenueDecimal spent_budget = RevenueDecimal::ZERO;
    RevenueDecimal reserved_budget = RevenueDecimal::ZERO;
    RevenueDecimal imps = RevenueDecimal::ZERO;
    RevenueDecimal clicks = RevenueDecimal::ZERO;
    bool forced = false;
  };

  struct Endpoint final
  {
    using Host = std::string;
    using Port = std::size_t;

    Endpoint(
      const Host& host,
      const Port port)
      : host(host),
        port(port)
    {
    }

    Host host;
    Port port = 0;
  };
  using Endpoints = std::vector<Endpoint>;

  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ConfigPoolClient = UServerUtils::Grpc::Client::ConfigPoolCoro;
  using TaskProcessor = userver::engine::TaskProcessor;
  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
  using DisabledIndexMap = std::map<std::size_t, Generics::Time>;

  struct CCGState final
  {
    using NonAvailableIndexMap = std::map<std::size_t, Generics::Time>;

    CCGState() = default;

    std::size_t active_index = 0;
    int use_count = -1;
    DisabledIndexMap disabled_indexes;
    std::deque<Generics::Time> server_use_times;
    Generics::Time last_server_switch_time;
    Generics::Time planned_server_switch_time;
    CAvailableAndMinCTRSetter_var ccg_setter;
  };

private:
  class RecheckCCGTask;
  class ClientHolder;
  class FactoryClientHolder;

  using Mutex = std::mutex;
  using CCGStates = std::map<std::uint32_t, CCGState>;
  using CCGCheckTimes = std::map<std::uint32_t, Generics::Time>;
  using ClientHolderPtr = std::shared_ptr<ClientHolder>;
  using ClientHolders = std::vector<ClientHolderPtr>;
  using FactoryClientHolderPtr = std::unique_ptr<FactoryClientHolder>;

  using CheckAvailableBidRequest = AdServer::CampaignSvcs::Billing::Proto::CheckAvailableBidRequest;
  using CheckAvailableBidRequestPtr = std::unique_ptr<CheckAvailableBidRequest>;
  using CheckAvailableBidResponse = AdServer::CampaignSvcs::Billing::Proto::CheckAvailableBidResponse;
  using CheckAvailableBidResponsePtr = std::unique_ptr<CheckAvailableBidResponse>;
  using ReserveBidRequest = AdServer::CampaignSvcs::Billing::Proto::ReserveBidRequest;
  using ReserveBidRequestPtr = std::unique_ptr<ReserveBidRequest>;
  using ReserveBidResponse = AdServer::CampaignSvcs::Billing::Proto::ReserveBidResponse;
  using ReserveBidResponsePtr = std::unique_ptr<ReserveBidResponse>;
  using ConfirmBidRequest = AdServer::CampaignSvcs::Billing::Proto::ConfirmBidRequest;
  using ConfirmBidRequestPtr = std::unique_ptr<ConfirmBidRequest>;
  using ConfirmBidResponse = AdServer::CampaignSvcs::Billing::Proto::ConfirmBidResponse;
  using ConfirmBidResponsePtr = std::unique_ptr<ConfirmBidResponse>;
  using AddAmountRequest = AdServer::CampaignSvcs::Billing::Proto::AddAmountRequest;
  using AddAmountRequestPtr = std::unique_ptr<AddAmountRequest>;
  using AddAmountResponse = AdServer::CampaignSvcs::Billing::Proto::AddAmountResponse;
  using AddAmountResponsePtr = std::unique_ptr<AddAmountResponse>;

public:
  explicit GrpcBillingStateContainer(
    Generics::ActiveObjectCallback* callback,
    Logger* logger,
    const std::size_t max_use_count,
    const bool optimize_campaign_ctr,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const Endpoints& endpoints,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout_ms = 1000);

  BidCheckResult reserve_bid(
    const Generics::Time& time,
    std::uint32_t account_id,
    std::uint32_t advertiser_id,
    std::uint32_t campaign_id,
    std::uint32_t ccg_id,
    const RevenueDecimal& amount) noexcept;

  BidCheckResult check_available_bid(
    const Generics::Time& time,
    const std::uint32_t account_id,
    const std::uint32_t advertiser_id,
    const std::uint32_t campaign_id,
    const std::uint32_t ccg_id,
    const CTRDecimal& ctr,
    const AvailableAndMinCTRSetter* const ccg_setter) noexcept;

  BidCheckResult confirm_bid(
    const Generics::Time& time,
    const std::uint32_t account_id,
    const std::uint32_t advertiser_id,
    const std::uint32_t campaign_id,
    const std::uint32_t ccg_id,
    const RevenueDecimal& account_spent_amount,
    const RevenueDecimal& spent_amount,
    const RevenueDecimal& ctr,
    const RevenueDecimal& imps,
    const RevenueDecimal& clicks,
    const AvailableAndMinCTRSetter* ccg_setter) noexcept;

protected:
  ~GrpcBillingStateContainer() override = default;

private:
  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request_service(
    const ClientHolderPtr& client_holder,
    Args&& ...args) noexcept;

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(
    const std::size_t index,
    Args&& ...args) noexcept;

  CheckAvailableBidRequestPtr create_check_available_bid_request(
    const Generics::Time& time,
    const std::uint32_t account_id,
    const std::uint32_t advertiser_id,
    const std::uint32_t campaign_id,
    const std::uint32_t ccg_id,
    const CTRDecimal& ctr,
    const bool optimize_campaign_ctr);

  ReserveBidRequestPtr create_reserve_bid_request(
    const Generics::Time& time,
    const std::uint32_t account_id,
    const std::uint32_t advertiser_id,
    const std::uint32_t campaign_id,
    const std::uint32_t ccg_id,
    const RevenueDecimal& reserve_budget);

  ConfirmBidRequestPtr create_confirm_bid_request(
    const ConfirmBidInfo& request_info);

  AddAmountRequestPtr create_add_amount_request(
    const std::vector<ConfirmBidInfo>& requests);

  CheckAvailableBidResponsePtr check_available_bid(
    const std::size_t index,
    const Generics::Time& time,
    const std::uint32_t account_id,
    const std::uint32_t advertiser_id,
    const std::uint32_t campaign_id,
    const std::uint32_t ccg_id,
    const CTRDecimal& ctr,
    const bool optimize_campaign_ctr) noexcept;

  ReserveBidResponsePtr reserve_bid(
    const std::size_t index,
    const Generics::Time& time,
    const std::uint32_t account_id,
    const std::uint32_t advertiser_id,
    const std::uint32_t campaign_id,
    const std::uint32_t ccg_id,
    const RevenueDecimal& reserve_budget) noexcept;

  ConfirmBidResponsePtr confirm_bid(
    const std::size_t index,
    const ConfirmBidInfo& request_info) noexcept;

  AddAmountResponsePtr add_amount(
    const std::size_t index,
    const std::vector<ConfirmBidInfo>& requests) noexcept;

  std::size_t next_index(const std::size_t server_index) const noexcept;

  void fill_planned_server_switch_time(
    const std::uint32_t ccg_id,
    const Generics::Time& now,
    CCGState& ccg_state) noexcept;

  std::optional<std::size_t>
  get_service_index(
    bool* switched,
    const Generics::Time& now,
    const std::uint32_t ccg_id,
    const DisabledIndexMap* disabled_indexes_in,
    const AvailableAndMinCTRSetter* ccg_setter);

  void ccg_set_available(
    const AvailableAndMinCTRSetter* ccg_setter,
    const std::uint32_t ccg_id,
    const bool available,
    const RevenueDecimal& goal_ctr,
    const Generics::Time& now) noexcept;

  void run_recheck_ccgs() noexcept;

  void clear_cache() noexcept;

private:
  const Logger_var logger_;

  Generics::TaskRunner_var task_runner_;

  Generics::Planner_var planner_;

  TaskProcessor& task_processor_;

  const FactoryClientHolderPtr factory_client_holder_;

  const std::size_t grpc_client_timeout_ms_;

  ClientHolders client_holders_;

  const int max_use_count_;

  const std::size_t max_try_count_ = 10;

  const bool optimize_campaign_ctr_;

  mutable Mutex cache_mutex_;

  CCGStates cache_;

  CCGCheckTimes recheck_ccgs_;   // thread unsafe - read and write only from RecheckCCGTask

  mutable Mutex add_recheck_ccgs_mutex_;

  CCGCheckTimes add_recheck_ccgs_;
};

class GrpcBillingStateContainer::ClientHolder final : private Generics::Uncopyable
{
public:
  using CheckAvailableBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_check_available_bid_ClientPool;
  using CheckAvailableBidClientPtr =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_check_available_bid_ClientPoolPtr;
  using ReserveBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_reserve_bid_ClientPool;
  using ReserveBidClientPtr =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_reserve_bid_ClientPoolPtr;
  using ConfirmBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_confirm_bid_ClientPool;
  using ConfirmBidClientPtr =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_confirm_bid_ClientPoolPtr;
  using AddAmountClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPool;
  using AddAmountClientPtr =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPoolPtr;

  struct Clients final
  {
    CheckAvailableBidClientPtr check_available_bid_client;
    ReserveBidClientPtr reserve_bid_client;
    ConfirmBidClientPtr confirm_bid_client;
    AddAmountClientPtr add_amount_client;
  };
  using ClientsPtr = std::shared_ptr<Clients>;

private:
  using Status = UServerUtils::Grpc::Client::Status;

public:
  explicit ClientHolder(
    const ClientsPtr& clients)
    : clients_(clients)
  {
  }

  ~ClientHolder() = default;

  template<class Client, class Request, class Response>
  std::unique_ptr<Response> do_request(
    std::unique_ptr<Request>&& request,
    const std::size_t timeout) noexcept
  {
    using ClientPtr = std::shared_ptr<Client>;

    ClientPtr client;
    if constexpr (std::is_same_v<Client, CheckAvailableBidClient>)
    {
      client = clients_->check_available_bid_client;
    }
    else if constexpr (std::is_same_v<Client, ReserveBidClient>)
    {
      client = clients_->reserve_bid_client;
    }
    else if constexpr (std::is_same_v<Client, ConfirmBidClient>)
    {
      client = clients_->confirm_bid_client;
    }
    else if constexpr (std::is_same_v<Client, AddAmountClient>)
    {
      client = clients_->add_amount_client;
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Client>);
    }

    for (std::size_t i = 1; i <= MaxNumberAttempts; i += 1)
    {
      auto result = client->write(std::move(request), timeout);
      if (result.status == Status::Ok)
      {
        return std::move(result.response);
      }
    }

    return {};
  }

private:
  static constexpr std::size_t MaxNumberAttempts = 5;

  const ClientsPtr clients_;
};

class GrpcBillingStateContainer::FactoryClientHolder final : private Generics::Uncopyable
{
public:
  using GrpcPoolClientFactory = UServerUtils::Grpc::Client::PoolClientFactory;
  using CheckAvailableBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_check_available_bid_ClientPool;
  using ReserveBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_reserve_bid_ClientPool;
  using ConfirmBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_confirm_bid_ClientPool;
  using AddAmountClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPool;

private:
  using Mutex = std::shared_mutex;
  using Host = typename Endpoint::Host;
  using Port = typename Endpoint::Port;
  using Key = std::pair<Host, Port>;
  using Value = ClientHolder::ClientsPtr;
  using Cache = std::unordered_map<Key, Value, boost::hash<Key>>;

public:
  FactoryClientHolder(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolClient& config,
    TaskProcessor& task_processor)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      config_(config),
      task_processor_(task_processor)
  {
  }

  ClientHolderPtr create(const Host& host, const Port port)
  {
    Key key(host, port);
    {
      std::shared_lock lock(mutex_);
      auto it = cache_.find(key);
      if (it != std::end(cache_))
      {
        auto clients = it->second;
        lock.unlock();
        return std::make_shared<ClientHolder>(clients);
      }
    }

    auto clients = std::make_shared<ClientHolder::Clients>();
    auto config = config_;

    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it == std::end(cache_))
    {
      std::stringstream endpoint_stream;
      endpoint_stream << host
                      << ":"
                      << port;
      std::string endpoint = endpoint_stream.str();
      config.endpoint = endpoint;

      GrpcPoolClientFactory factory(
        logger_.in(),
        scheduler_,
        config_);

      clients->check_available_bid_client = factory.create<CheckAvailableBidClient>(task_processor_);
      clients->reserve_bid_client = factory.create<ReserveBidClient>(task_processor_);
      clients->confirm_bid_client = factory.create<ConfirmBidClient>(task_processor_);
      clients->add_amount_client = factory.create<AddAmountClient>(task_processor_);

      it = cache_.try_emplace(
        key,
        std::move(clients)).first;
    }

    clients = it->second;
    lock.unlock();

    return std::make_shared<ClientHolder>(clients);
  }


private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const ConfigPoolClient config_;

  TaskProcessor& task_processor_;

  Mutex mutex_;

  Cache cache_;
};

using GrpcBillingStateContainer_var =
  ReferenceCounting::QualPtr<GrpcBillingStateContainer>;

} // namespace AdServer::CampaignSvcs

#endif // CAMPAIGNMANAGER_GRPCBILLINGSTATECONTAINER_HPP