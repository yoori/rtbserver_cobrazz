#ifndef CHANNELSVCS_CHANNELSERVER_GRPCCHANNELOPERATIONDISTRIBUTOR
#define CHANNELSVCS_CHANNELSERVER_GRPCCHANNELOPERATIONDISTRIBUTOR

// STD
#include <chrono>
#include <shared_mutex>
#include <unordered_map>

// BOOST
#include <boost/functional/hash.hpp>

// PROTO
#include "ChannelSvcs/ChannelCommons/proto/ChannelServer_client.cobrazz.pb.hpp"

// UNIXCOMMONS
#include <Generics/CompositeActiveObject.hpp>
#include <UServerUtils/Grpc/Core/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>
#include <UServerUtils/Grpc/CobrazzClientFactory.hpp>

// USERVER
#include <userver/engine/task/task_processor.hpp>

namespace AdServer::ChannelSvcs
{

namespace Internal
{

template<class T>
struct always_false : std::false_type
{
};

template<class T>
constexpr auto always_false_v = always_false<T>::value;

} // namespace Internal

extern const char* ASPECT_GRPC_CHANNEL_OPERATION_POOL;

class GrpcChannelOperationPool final : Generics::Uncopyable
{
private:
  class ClientHolder;
  class FactoryClientHolder;

  using ClientHolderPtr = std::shared_ptr<ClientHolder>;
  using ClientHolders = std::vector<ClientHolderPtr>;
  using FactoryClientHolderPtr = std::unique_ptr<FactoryClientHolder>;

public:
  using Host = std::string;
  using Port = std::size_t;
  using Endpoint = std::pair<Host, Port>;
  using Endpoints = std::vector<Endpoint>;
  using ConfigPoolClient = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Time = Generics::Time;

  using MatchRequest = Proto::MatchRequest;
  using MatchRequestPtr = std::unique_ptr<MatchRequest>;
  using MatchResponse = Proto::MatchResponse;
  using MatchResponsePtr = std::unique_ptr<MatchResponse>;
  using GetCcgTraitsRequest = Proto::GetCcgTraitsRequest;
  using GetCcgTraitsRequestPtr = std::unique_ptr<GetCcgTraitsRequest>;
  using GetCcgTraitsResponse = Proto::GetCcgTraitsResponse;
  using GetCcgTraitsResponsePtr = std::unique_ptr<GetCcgTraitsResponse>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using SchedulerPtr = UServerUtils::Grpc::Core::Common::SchedulerPtr;

public:
  explicit GrpcChannelOperationPool(
    Logger* logger,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const Endpoints& endpoints,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout_ms = 1000,
    const std::size_t time_duration_client_bad_ms = 30000);

  ~GrpcChannelOperationPool() = default;

  MatchResponsePtr match(
    const std::string& request_id,
    const std::string& first_url,
    const std::string& first_url_words,
    const std::string& urls,
    const std::string& urls_words,
    const std::string& pwords,
    const std::string& swords,
    const std::string& uid,
    const std::string& statuses,
    const bool non_strict_word_match,
    const bool non_strict_url_match,
    const bool return_negative,
    const bool simplify_page,
    const bool fill_content) noexcept;

  GetCcgTraitsResponsePtr get_ccg_traits(
    const std::vector<std::size_t>& ids) noexcept;

private:
  MatchRequestPtr create_match_request(
    const std::string& request_id,
    const std::string& first_url,
    const std::string& first_url_words,
    const std::string& urls,
    const std::string& urls_words,
    const std::string& pwords,
    const std::string& swords,
    const std::string& uid,
    const std::string& statuses,
    const bool non_strict_word_match,
    const bool non_strict_url_match,
    const bool return_negative,
    const bool simplify_page,
    const bool fill_content);

  GetCcgTraitsRequestPtr create_get_ccg_traits_request(
    const std::vector<std::size_t>& ids);

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(Args&& ...args) noexcept
  {
    if (client_holders_.empty())
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << " client_holders is empty";
        logger_->error(
          stream.str(),
          ASPECT_GRPC_CHANNEL_OPERATION_POOL);
      }
      catch (...)
      {
      }
      return {};
    }

    const std::size_t size = client_holders_.size();
    for (std::size_t i = 0; i < size; ++i)
    {
      try
      {
        const std::size_t number = counter_.fetch_add(
          1,
          std::memory_order_relaxed);
        const auto& client_holder = client_holders_[number % size];
        if (client_holder->is_bad())
        {
          continue;
        }

        std::unique_ptr<Request> request;
        if constexpr(std::is_same_v<Request, MatchRequest>)
        {
          request = create_match_request(std::forward<Args>(args)...);
        }
        else if constexpr(std::is_same_v<Request, GetCcgTraitsRequest>)
        {
          request = create_get_ccg_traits_request(std::forward<Args>(args)...);
        }
        else
        {
          static_assert(Internal::always_false_v<Request>);
        }

        auto response = client_holder->template do_request<Client, Request, Response>(
          std::move(request),
          grpc_client_timeout_ms_);
        if (!response)
        {
          Stream::Error stream;
          stream << FNS
                 << ": Internal grpc error";
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CHANNEL_OPERATION_POOL);

          continue;
        }

        const auto data_case = response->data_case();
        if (data_case == Response::DataCase::kInfo)
        {
          return response;
        }
        else if (data_case == Response::DataCase::kError)
        {
          const auto& error = response->error();
          const auto error_type = error.type();
          switch (error_type)
          {
            case Proto::Error_Type::Error_Type_Implementation:
            case Proto::Error_Type::Error_Type_NotConfigured:
            {
              Stream::Error stream;
              stream << FNS
                     << "Error type="
                     << (error_type == Proto::Error_Type::Error_Type_Implementation
                         ? "Implementation" : "NotConfigured")
                     << " description="
                     << error.description();
              logger_->error(
                stream.str(),
                ASPECT_GRPC_CHANNEL_OPERATION_POOL);
              return response;
            }
            default:
            {
              Stream::Error stream;
              stream << FNS
                     << ": "
                     << "Unknown error type";
              logger_->error(
                stream.str(),
                ASPECT_GRPC_CHANNEL_OPERATION_POOL);
              break;
            }
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << exc.what();
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CHANNEL_OPERATION_POOL);
        }
        catch (...)
        {
        }
      }
      catch (...)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << "Unknown error";
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CHANNEL_OPERATION_POOL);
        }
        catch (...)
        {
        }
      }
    }

    try
    {
      Stream::Error stream;
      stream << FNS
             << "max tries is reached";
      logger_->error(
        stream.str(),
        ASPECT_GRPC_CHANNEL_OPERATION_POOL);
    }
    catch (...)
    {
    }

    return {};
  }

private:
  const Logger_var logger_;

  TaskProcessor& task_processor_;

  const SchedulerPtr scheduler_;

  const FactoryClientHolderPtr factory_client_holder_;

  const std::size_t grpc_client_timeout_ms_;

  ClientHolders client_holders_;

  std::atomic<std::size_t> counter_{0};
};

using GrpcChannelOperationPoolPtr = std::unique_ptr<GrpcChannelOperationPool>;

class GrpcChannelOperationPool::ClientHolder final : private Generics::Uncopyable
{
public:
  using MatchClient = Proto::ChannelServer_match_ClientPool;
  using MatchClientPtr = Proto::ChannelServer_match_ClientPoolPtr;
  using GetCcgTraitsClient = Proto::ChannelServer_get_ccg_traits_ClientPool;
  using GetCcgTraitsClientPtr = Proto::ChannelServer_get_ccg_traits_ClientPoolPtr;

  struct Clients final
  {
    MatchClientPtr match_client;
    GetCcgTraitsClientPtr get_ccg_traits_client;
  };
  using ClientsPtr = std::shared_ptr<Clients>;

private:
  using Status = UServerUtils::Grpc::Core::Client::Status;
  using Mutex = std::shared_mutex;

public:
  explicit ClientHolder(
    const ClientsPtr& clients,
    const Generics::Time& time_duration_client_bad)
    : clients_(clients),
      time_duration_client_bad_(time_duration_client_bad)
  {
  }

  ~ClientHolder() = default;

  bool is_bad() const
  {
    {
      std::shared_lock lock(mutex_);
      if (!marked_as_bad_)
        return false;
    }

    const Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(mutex_);
    if (marked_as_bad_ && now >= marked_as_bad_time_ + time_duration_client_bad_)
    {
      marked_as_bad_ = false;
    }

    return marked_as_bad_;
  }

  template<class Client, class Request, class Response>
  std::unique_ptr<Response> do_request(
    std::unique_ptr<Request>&& request,
    const std::size_t timeout) noexcept
  {
    using ClientPtr = std::shared_ptr<Client>;

    ClientPtr client;
    if constexpr (std::is_same_v<Client, MatchClient>)
    {
      client = clients_->match_client;
    }
    else if constexpr (std::is_same_v<Client, GetCcgTraitsClient>)
    {
      client = clients_->get_ccg_traits_client;
    }
    else
    {
      static_assert(Internal::always_false_v<Client>);
    }

    for (std::size_t i = 1; i <= 3; ++i)
    {
      auto result = client->write(std::move(request), timeout);
      if (result.status == Status::Ok)
      {
        return std::move(result.response);
      }
    }

    set_bad();
    return {};
  }

private:
  void set_bad() noexcept
  {
    try
    {
      const Generics::Time now = Generics::Time::get_time_of_day();

      std::unique_lock lock(mutex_);
      marked_as_bad_ = true;
      marked_as_bad_time_ = now;
    }
    catch (...)
    {
    }
  }

private:
  const ClientsPtr clients_;

  const Generics::Time time_duration_client_bad_;

  mutable Mutex mutex_;

  mutable bool marked_as_bad_ = false;

  Generics::Time marked_as_bad_time_;
};

class GrpcChannelOperationPool::FactoryClientHolder final : private Generics::Uncopyable
{
public:
  using MatchClient = Proto::ChannelServer_match_ClientPool;
  using GetCcgTraitsClient = Proto::ChannelServer_get_ccg_traits_ClientPool;
  using GrpcCobrazzPoolClientFactory = UServerUtils::Grpc::GrpcCobrazzPoolClientFactory;
  using TaskProcessor = GrpcCobrazzPoolClientFactory::TaskProcessor;

private:
  using Mutex = std::shared_mutex;
  using Host = std::string;
  using Port = std::size_t;
  using Key = std::pair<Host, Port>;
  using Value = ClientHolder::ClientsPtr;
  using Cache = std::unordered_map<Key, Value, boost::hash<Key>>;

public:
  FactoryClientHolder(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolClient& config,
    TaskProcessor& task_processor,
    const Generics::Time& time_duration_client_bad)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      config_(config),
      task_processor_(task_processor),
      time_duration_client_bad_(time_duration_client_bad)
  {
  }

  ClientHolderPtr create(
    const Host& host,
    const Port port)
  {
    Key key(host, port);
    {
      std::shared_lock lock(mutex_);
      auto it = cache_.find(key);
      if (it != std::end(cache_))
      {
        auto clients = it->second;
        lock.unlock();
        return std::make_shared<ClientHolder>(
          clients,
          time_duration_client_bad_);
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

      UServerUtils::Grpc::GrpcCobrazzPoolClientFactory factory(
        logger_.in(),
        scheduler_,
        config);

      clients->match_client = factory.create<MatchClient>(task_processor_);
      clients->get_ccg_traits_client = factory.create<GetCcgTraitsClient>(task_processor_);

      it = cache_.try_emplace(
        key,
        std::move(clients)).first;
    }

    clients = it->second;
    lock.unlock();

    return std::make_shared<ClientHolder>(
      clients,
      time_duration_client_bad_);
  }

private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const ConfigPoolClient config_;

  TaskProcessor& task_processor_;

  const Generics::Time time_duration_client_bad_;

  Mutex mutex_;

  Cache cache_;
};

} // namespace AdServer::ChannelSvcs

#endif //CHANNELSVCS_CHANNELSERVER_GRPCCHANNELOPERATIONDISTRIBUTOR