#ifndef REQUESTINFOMANAGER_GRPCBILLINGMANAGERPOOL_HPP
#define REQUESTINFOMANAGER_GRPCBILLINGMANAGERPOOL_HPP

// BOOST
#include <boost/functional/hash.hpp>

// UNIXCOMMONS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/PoolClientFactory.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// THIS
#include <CampaignSvcs/BillingServer/proto/BillingServer_client.cobrazz.pb.hpp>
#include <Commons/GrpcAlgs.hpp>

namespace AdServer::RequestInfoSvcs
{

class GrpcBillingManagerPool final : public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  using CTRDecimal = AdServer::CampaignSvcs::CTRDecimal;
  using RevenueDecimal = AdServer::CampaignSvcs::RevenueDecimal;

  struct ConfirmBidInfo final
  {
    ConfirmBidInfo() = default;

    ConfirmBidInfo(
      const Generics::Time& time,
      const std::uint32_t account_id,
      const std::uint32_t advertiser_id,
      const std::uint32_t campaign_id,
      const std::uint32_t ccg_id,
      const CTRDecimal& ctr,
      const RevenueDecimal& account_spent_budget,
      const RevenueDecimal& spent_budget,
      const RevenueDecimal& reserved_budget,
      const RevenueDecimal& imps,
      const RevenueDecimal& clicks,
      const bool forced)
      : time(time),
        account_id(account_id),
        advertiser_id(advertiser_id),
        campaign_id(campaign_id),
        ccg_id(ccg_id),
        ctr(ctr),
        account_spent_budget(account_spent_budget),
        spent_budget(spent_budget),
        reserved_budget(reserved_budget),
        imps(imps),
        clicks(clicks),
        forced(forced)
    {
    }

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

private:
  class ClientHolder;
  class FactoryClientHolder;

  using Mutex = std::mutex;
  using ClientHolderPtr = std::shared_ptr<ClientHolder>;
  using ClientHolders = std::vector<ClientHolderPtr>;
  using FactoryClientHolderPtr = std::unique_ptr<FactoryClientHolder>;

  using AddAmountRequest = AdServer::CampaignSvcs::Billing::Proto::AddAmountRequest;
  using AddAmountRequestPtr = std::unique_ptr<AddAmountRequest>;
  using AddAmountResponse = AdServer::CampaignSvcs::Billing::Proto::AddAmountResponse;
  using AddAmountResponsePtr = std::unique_ptr<AddAmountResponse>;

public:
  explicit GrpcBillingManagerPool(
    Logger* logger,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const Endpoints& endpoints,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout_ms = 1000);

  std::size_t size() const noexcept;

  AddAmountResponsePtr add_amount(
    const std::size_t index,
    const std::vector<ConfirmBidInfo>& requests) noexcept;

protected:
  ~GrpcBillingManagerPool() override = default;

private:
  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request_service(
    const ClientHolderPtr& client_holder,
    Args&& ...args) noexcept;

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(
    const std::size_t index,
    Args&& ...args) noexcept;

  AddAmountRequestPtr create_add_amount_request(
    const std::vector<ConfirmBidInfo>& requests);

private:
  const Logger_var logger_;

  TaskProcessor& task_processor_;

  const FactoryClientHolderPtr factory_client_holder_;

  const std::size_t grpc_client_timeout_ms_;

  ClientHolders client_holders_;
};

using GrpcBillingManagerPool_var =
  ReferenceCounting::QualPtr<GrpcBillingManagerPool>;

class GrpcBillingManagerPool::ClientHolder final : private Generics::Uncopyable
{
public:
  using AddAmountClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPool;
  using AddAmountClientPtr =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPoolPtr;

  struct Clients final
  {
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
    if constexpr (std::is_same_v<Client, AddAmountClient>)
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
  static constexpr std::size_t MaxNumberAttempts = 3;

  const ClientsPtr clients_;
};

class GrpcBillingManagerPool::FactoryClientHolder final
  : private Generics::Uncopyable
{
public:
  using GrpcPoolClientFactory =
    UServerUtils::Grpc::Client::PoolClientFactory;
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

} // namespace AdServer::RequestInfoSvcs

#endif //REQUESTINFOMANAGER_GRPCBILLINGMANAGERPOOL_HPP
