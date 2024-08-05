#ifndef USERBINDCONTROLLER_GRPCUSERBINDOPERATIONDISTRIBUTOR_HPP
#define USERBINDCONTROLLER_GRPCUSERBINDOPERATIONDISTRIBUTOR_HPP

// STD
#include <shared_mutex>
#include <unordered_map>

// BOOST
#include <boost/functional/hash.hpp>

// PROTO
#include "UserInfoSvcs/UserBindServer/proto/UserBindServer_client.cobrazz.pb.hpp"

// UNIX_COMMONS
#include <GrpcAlgs.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/UserInfoManip.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <UServerUtils/Grpc/Core/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>
#include <UServerUtils/Grpc/CobrazzClientFactory.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>
namespace AdServer::UserInfoSvcs
{

extern const char* ASPECT_GRPC_USER_BIND_DISTRIBUTOR;

class GrpcUserBindOperationDistributor final :
  public Generics::CompositeActiveObject,
  public ReferenceCounting::AtomicImpl
{
private:
  using PartitionNumber = unsigned int;
  using TryCount = PartitionNumber;

  class ClientContainer;
  class FactoryClientContainer;
  class Partition;
  struct PartitionHolder;
  class ResolvePartitionTask;

  using SchedulerPtr = UServerUtils::Grpc::Core::Common::SchedulerPtr;
  using TaskProcessor = userver::engine::TaskProcessor;
  using ClientContainerPtr = std::shared_ptr<ClientContainer>;
  using FactoryClientContainerPtr = std::unique_ptr<FactoryClientContainer>;
  using PartitionPtr = std::shared_ptr<Partition>;
  using PartitionHolderPtr = std::shared_ptr<PartitionHolder>;
  using PartitionHolderArray = std::vector<PartitionHolderPtr>;
  using CorbaClientAdapter = CORBACommons::CorbaClientAdapter;
  using CorbaClientAdapter_var = CORBACommons::CorbaClientAdapter_var;
  using FixedTaskRunner_var = Generics::FixedTaskRunner_var;

public:
  using ChunkId = unsigned int;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ConfigPoolClient = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;
  using ControllerRef = CORBACommons::CorbaObjectRefList;
  using ControllerRefList = std::list<ControllerRef>;

  using GetBindRequestPtr = std::unique_ptr<GetBindRequest>;
  using GetBindResponsePtr = std::unique_ptr<GetBindResponse>;
  using AddBindRequestPtr = std::unique_ptr<AddBindRequest>;
  using AddBindResponsePtr = std::unique_ptr<AddBindResponse>;
  using GetUserIdRequestPtr = std::unique_ptr<GetUserIdRequest>;
  using GetUserIdResponsePtr = std::unique_ptr<GetUserIdResponse>;
  using AddUserIdRequestPtr = std::unique_ptr<AddUserIdRequest>;
  using AddUserIdResponsePtr = std::unique_ptr<AddUserIdResponse>;
  using GetSourceRequestPtr = std::unique_ptr<GetSourceRequest>;
  using GetSourceResponsePtr = std::unique_ptr<GetSourceResponse>;

public:
  GrpcUserBindOperationDistributor(
    Logger* logger,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const ControllerRefList& controller_refs,
    const CorbaClientAdapter* corba_client_adapter,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout = 1500,
    const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND);

  GetBindResponsePtr
  get_bind_request(
    const String::SubString& request_id,
    const Generics::Time& timestamp) noexcept;

  AddBindResponsePtr
  add_bind_request(
    const String::SubString& request_id,
    const AdServer::Commons::ExternalUserIdArray& user_ids,
    const Generics::Time& timestamp) noexcept;

  GetUserIdResponsePtr
  get_user_id(
    const String::SubString& external_id,
    const String::SubString& current_user_id,
    const Generics::Time& timestamp,
    const Generics::Time& create_timestamp,
    const bool silent,
    const bool generate_user_id,
    const bool for_set_cookie) noexcept;

  AddUserIdResponsePtr
  add_user_id(
    const String::SubString& external_id,
    const Generics::Time& timestamp,
    const String::SubString& user_id) noexcept;

protected:
  ~GrpcUserBindOperationDistributor() override = default;

private:
  void resolve_partition(
    const PartitionNumber partition_number) noexcept;

  void try_to_reresolve_partition(
    const PartitionNumber partition_number) noexcept;

  PartitionNumber get_partition_number(
    const String::SubString& id) const noexcept;

  PartitionPtr get_partition(
    const PartitionNumber partition_number) noexcept;

  GetBindRequestPtr create_get_bind_request(
    const String::SubString& request_id,
    const Generics::Time& time);

  AddBindRequestPtr create_add_bind_request(
    const String::SubString& request_id,
    const AdServer::Commons::ExternalUserIdArray& user_ids,
    const Generics::Time& time);

  GetUserIdRequestPtr create_get_user_id_request(
    const String::SubString& external_id,
    const String::SubString& current_user_id,
    const Generics::Time& timestamp,
    const Generics::Time& create_timestamp,
    const bool silent,
    const bool generate_user_id,
    const bool for_set_cookie);

  AddUserIdRequestPtr create_add_user_id_request(
    const String::SubString& external_id,
    const Generics::Time& timestamp,
    const String::SubString& user_id);

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(
    const String::SubString& id,
    Args&& ...args) noexcept;

private:
  const Logger_var logger_;

  Generics::ActiveObjectCallback_var callback_;

  const TryCount try_count_;

  const ConfigPoolClient config_pool_client_;

  const std::size_t grpc_client_timeout_;

  const Generics::Time pool_timeout_;

  const ControllerRefList controller_refs_;

  const SchedulerPtr scheduler_;

  FactoryClientContainerPtr factory_client_container_;

  CorbaClientAdapter_var corba_client_adapter_;

  FixedTaskRunner_var task_runner_;

  PartitionHolderArray partition_holders_;
};

class GrpcUserBindOperationDistributor::ResolvePartitionTask final :
  public Generics::Task,
  public ReferenceCounting::AtomicImpl
{
public:
  explicit ResolvePartitionTask(
    GrpcUserBindOperationDistributor* distributor,
    const PartitionNumber partition_number) noexcept
    : distributor_(distributor),
      partition_number_(partition_number)
  {
  }

  void execute() noexcept override
  {
    distributor_->resolve_partition(partition_number_);
  }

protected:
  ~ResolvePartitionTask() noexcept override = default;

private:
  GrpcUserBindOperationDistributor* distributor_;

  const PartitionNumber partition_number_;
};

class GrpcUserBindOperationDistributor::ClientContainer final
  : private Generics::Uncopyable
{
public:
  using GetBindRequestClient =
    UserBindService_get_bind_request_ClientPool;
  using GetBindRequestClientPtr =
    UserBindService_get_bind_request_ClientPoolPtr;
  using AddBindRequestClient =
    UserBindService_add_bind_request_ClientPool;
  using AddBindRequestClientPtr =
    UserBindService_add_bind_request_ClientPoolPtr;
  using GetUserIdClient =
    UserBindService_get_user_id_ClientPool;
  using GetUserIdClientPtr =
    UserBindService_get_user_id_ClientPoolPtr;
  using AddUserIdClient =
    UserBindService_add_user_id_ClientPool;
  using AddUserIdClientPtr =
    UserBindService_add_user_id_ClientPoolPtr;

  struct Clients final
  {
    Clients() = default;
    ~Clients() = default;

    GetBindRequestClientPtr get_bind_request_client;
    AddBindRequestClientPtr add_bind_request_client;
    GetUserIdClientPtr get_user_id_client;
    AddUserIdClientPtr add_user_id_client;
  };
  using ClientsPtr = std::shared_ptr<Clients>;

private:
  using Status = UServerUtils::Grpc::Core::Client::Status;
  using Mutex = std::shared_mutex;

public:
  explicit ClientContainer(
    const ClientsPtr& clients)
    : clients_(clients)
  {
  }

  ~ClientContainer() = default;

  bool is_bad(const Generics::Time& timeout) const
  {
    {
      std::shared_lock lock(mutex_);
      if (!marked_as_bad_)
        return false;
    }

    Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(mutex_);
    if(marked_as_bad_ && now >= marked_as_bad_time_ + timeout)
    {
      marked_as_bad_ = false;
    }

    return marked_as_bad_;
  }

  void set_bad()
  {
    Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(mutex_);
    marked_as_bad_ = true;
    marked_as_bad_time_ = now;
  }

  template<class Client, class Request, class Response>
  std::unique_ptr<Response>
  do_request(
    std::unique_ptr<Request>&& request,
    const std::size_t timeout) const noexcept
  {
    using ClientPtr = std::shared_ptr<Client>;

    ClientPtr client;
    if constexpr (std::is_same_v<Client, GetBindRequestClient>)
    {
      client = clients_->get_bind_request_client;
    }
    else if constexpr (std::is_same_v<Client, AddBindRequestClient>)
    {
      client = clients_->add_bind_request_client;
    }
    else if constexpr (std::is_same_v<Client, GetUserIdClient>)
    {
      client = clients_->get_user_id_client;
    }
    else if constexpr (std::is_same_v<Client, AddUserIdClient>)
    {
      client = clients_->add_user_id_client;
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Client>);
    }

    auto result = client->write(std::move(request), timeout);
    if (result.status == Status::Ok)
    {
      return std::move(result.response);
    }
    else
    {
      return {};
    }
  }

private:
  ClientsPtr clients_;

  mutable Mutex mutex_;

  mutable bool marked_as_bad_ = false;

  Generics::Time marked_as_bad_time_;
};

class GrpcUserBindOperationDistributor::FactoryClientContainer final
  : private Generics::Uncopyable
{
public:
  using GetBindRequestClientPtr =
    typename ClientContainer::GetBindRequestClientPtr;
  using AddBindRequestClientPtr =
    typename ClientContainer::AddBindRequestClientPtr;
  using GetUserIdClientPtr =
    typename ClientContainer::GetUserIdClientPtr;
  using AddUserIdClientPtr =
    typename ClientContainer::AddUserIdClientPtr;
  using GrpcCobrazzPoolClientFactory =
    UServerUtils::Grpc::GrpcCobrazzPoolClientFactory;
  using TaskProcessor =
    GrpcCobrazzPoolClientFactory::TaskProcessor;

private:
  using Mutex = std::shared_mutex;
  using Host = std::string;
  using Port = std::size_t;
  using Key = std::pair<Host, Port>;
  using Value = ClientContainer::ClientsPtr;
  using Cache = std::unordered_map<Key, Value, boost::hash<Key>>;

public:
  FactoryClientContainer(
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

  ClientContainerPtr create(
    const Host& host, const Port port)
  {
    Key key(host, port);
    {
      std::shared_lock lock(mutex_);
      auto it = cache_.find(key);
      if (it != std::end(cache_))
      {
        ClientContainer::ClientsPtr clients = it->second;
        lock.unlock();
        return std::make_shared<ClientContainer>(clients);
      }
    }

    auto clients = std::make_shared<ClientContainer::Clients>();
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

      clients->get_bind_request_client =
        factory.create<UserBindService_get_bind_request_ClientPool>(task_processor_);
      clients->add_bind_request_client =
        factory.create<UserBindService_add_bind_request_ClientPool>(task_processor_);
      clients->add_user_id_client =
        factory.create<UserBindService_add_user_id_ClientPool>(task_processor_);
      clients->get_user_id_client =
        factory.create<UserBindService_get_user_id_ClientPool>(task_processor_);

      it = cache_.try_emplace(
        key,
        std::move(clients)).first;
    }

    clients = it->second;
    lock.unlock();

    return std::make_shared<ClientContainer>(clients);
  }

private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const ConfigPoolClient config_;

  TaskProcessor& task_processor_;

  Mutex mutex_;

  Cache cache_;
};

class GrpcUserBindOperationDistributor::Partition final
  : private Generics::Uncopyable
{
public:
  using ChunkToClientContainer = std::map<ChunkId, ClientContainerPtr>;

public:
  explicit Partition(
    ChunkToClientContainer&& chunk_to_client_container,
    const ChunkId max_chunk_number)
    : chunk_to_client_container_(std::move(chunk_to_client_container)),
      max_chunk_number_(max_chunk_number)
  {
  }

  ~Partition() = default;

  ChunkId chunk_id(
    const String::SubString& id) const noexcept
  {
    return AdServer::Commons::external_id_distribution_hash(id) % max_chunk_number_;
  }

  ClientContainerPtr get_client_container(
    const ChunkId chunk_id) noexcept
  {
    const auto it = chunk_to_client_container_.find(chunk_id);
    if (it == std::end(chunk_to_client_container_))
    {
      return {};
    }
    else
    {
      return it->second;
    }
  }

  ChunkId max_chunk_number() const noexcept
  {
    return max_chunk_number_;
  }

  bool empty() const noexcept
  {
    return chunk_to_client_container_.empty();
  }

private:
  const ChunkToClientContainer chunk_to_client_container_;

  const ChunkId max_chunk_number_ = 0;
};

struct GrpcUserBindOperationDistributor::PartitionHolder final
  : private Generics::Uncopyable
{
public:
  using Mutex = std::shared_mutex;

public:
  explicit PartitionHolder() noexcept = default;
  ~PartitionHolder() noexcept = default;

  PartitionPtr partition;
  Generics::Time last_try_to_resolve;
  bool resolve_in_progress = false;
  mutable Mutex mutex;
};

using GrpcUserBindOperationDistributor_var =
  ReferenceCounting::SmartPtr<GrpcUserBindOperationDistributor>;

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcUserBindOperationDistributor::do_request(
  const String::SubString& id,
  Args&& ...args) noexcept
{
  ChunkId chunk_id = 0;
  for (std::size_t i = 0; i < try_count_; ++i)
  {
    const PartitionNumber partition_number =
      (get_partition_number(id) + i) % partition_holders_.size();

    try
    {
      PartitionPtr partition = get_partition(partition_number);
      if (!partition)
      {
        try_to_reresolve_partition(partition_number);
        continue;
      }

      chunk_id = partition->chunk_id(id);
      auto client_container = partition->get_client_container(chunk_id);
      if (!client_container)
      {
        try_to_reresolve_partition(partition_number);
        continue;
      }

      if (client_container->is_bad(pool_timeout_))
      {
        continue;
      }

      std::unique_ptr<Request> request;
      if constexpr (std::is_same_v<Request, GetBindRequest>)
      {
        request = create_get_bind_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, AddBindRequest>)
      {
        request = create_add_bind_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, GetUserIdRequest>)
      {
        request = create_get_user_id_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, AddUserIdRequest>)
      {
        request = create_add_user_id_request(std::forward<Args>(args)...);
      }
      else
      {
        static_assert(GrpcAlgs::AlwaysFalseV<Request>);
      }

      auto response = client_container->template do_request<Client, Request, Response>(
        std::move(request),
        grpc_client_timeout_);
      if (!response)
      {
        client_container->set_bad();
        try_to_reresolve_partition(partition_number);
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
          case Error_Type::Error_Type_NotReady:
          {
            client_container->set_bad();
            break;
          }
          case Error_Type::Error_Type_ChunkNotFound:
          {
            client_container->set_bad();
            try_to_reresolve_partition(partition_number);
            break;
          }
          case Error_Type::Error_Type_Implementation:
          {
            client_container->set_bad();
            try_to_reresolve_partition(partition_number);
            break;
          }
          default:
          {
            Stream::Error stream;
            stream << FNS
                   << ": "
                   << "Unknown error type";
            logger_->error(
              stream.str(),
              ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
            client_container->set_bad();
            try_to_reresolve_partition(partition_number);
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
               << ": "
               << exc.what();
        logger_->error(
          stream.str(),
          ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
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
           << ": max tries reached for id="
           << id
           << " and chunk="
           << chunk_id;
    logger_->error(
      stream.str(),
      ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
  }
  catch (...)
  {
  }

  return {};
}
} // namespace AdServer::UserInfoSvcs

#endif // USERBINDCONTROLLER_GRPCUSERBINDOPERATIONDISTRIBUTOR_HPP