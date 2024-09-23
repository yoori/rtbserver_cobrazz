// THIS
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <UserInfoSvcs/UserBindController/GrpcUserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserBindController/UserBindController.hpp>

// UNIX_COMMONS
#include <UServerUtils/Grpc/Common/Utils.hpp>

namespace AdServer::UserInfoSvcs
{

const char* ASPECT_GRPC_USER_BIND_DISTRIBUTOR =
  "GRPC_USER_BIND_OPERATION_DISTRIBUTOR";

GrpcUserBindOperationDistributor::GrpcUserBindOperationDistributor(
  Logging::Logger* logger,
  TaskProcessor& task_processor,
  const SchedulerPtr& scheduler,
  const ControllerRefList& controller_refs,
  const CORBACommons::CorbaClientAdapter* corba_client_adapter,
  const ConfigPoolClient& config_pool_client,
  const std::size_t grpc_client_timeout,
  const Generics::Time& pool_timeout)
  : logger_(ReferenceCounting::add_ref(logger)),
    callback_(Generics::ActiveObjectCallback_var(
      new Logging::ActiveObjectCallbackImpl(
        logger,
        "GrpcUserBindOperationDistributor",
        "UserInfo"))),
    try_count_(controller_refs.size()),
    config_pool_client_(config_pool_client),
    grpc_client_timeout_(grpc_client_timeout),
    pool_timeout_(pool_timeout),
    controller_refs_(controller_refs),
    scheduler_(scheduler),
    factory_client_container_(new FactoryClientContainer(
      logger_.in(),
      scheduler_,
      config_pool_client_,
      task_processor)),
    corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter)),
    task_runner_(new Generics::TaskRunner(callback_, try_count_))
{
  try
  {
    add_child_object(task_runner_);
  }
  catch(const Generics::CompositeActiveObject::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    throw Exception(stream);
  }

  const PartitionNumber size = controller_refs_.size();
  for (PartitionNumber partition_number = 0;
       partition_number < size;
       ++partition_number)
  {
    partition_holders_.emplace_back(
      std::make_shared<PartitionHolder>());
    resolve_partition(partition_number);
  }
}

void GrpcUserBindOperationDistributor::resolve_partition(
  const PartitionNumber partition_number) noexcept
{
  using UserBindController =
    AdServer::UserInfoSvcs::UserBindController;
  using UserBindController_var =
    AdServer::UserInfoSvcs::UserBindController_var;

  PartitionPtr partition;

  auto it_controller_ref = controller_refs_.begin();
  std::advance(it_controller_ref, partition_number);
  auto& corba_object_ref_list = *it_controller_ref;

  for (const auto& ref : corba_object_ref_list)
  {
    try
    {
      CORBA::Object_var obj = corba_client_adapter_->resolve_object(ref);
      if(CORBA::is_nil(obj.in()))
      {
        continue;
      }

      UserBindController_var controller =
        UserBindController::_narrow(obj.in());

      if (CORBA::is_nil(controller.in()))
      {
        continue;
      }

      auto user_bind_servers = controller->get_session_description();
      if (user_bind_servers->length() == 0)
      {
        Stream::Error stream;
        stream << FNS
               << ": number of user_bind_servers is null";
        logger_->error(
          stream.str(),
          ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
        continue;
      }

      ChunkId max_chunk_number = 0;
      Partition::ChunkToClientContainer chunk_to_client_container;

      const auto length_user_bind_servers = user_bind_servers->length();
      for (std::size_t i = 0; i < length_user_bind_servers; ++i)
      {
        const auto& user_bind_description = (*user_bind_servers)[i];

        const auto& chunk_ids = user_bind_description.chunk_ids;
        const std::size_t grpc_port = user_bind_description.grpc_port;
        const std::string host(user_bind_description.host.in());
        if (host.empty())
        {
          Stream::Error stream;
          stream << FNS
                 << ": host is empty";
          logger_->error(
            stream.str(),
            ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
          throw Exception(stream.str());
        }

        const std::size_t length_chunk_ids = chunk_ids.length();
        for (std::size_t i = 0; i < length_chunk_ids; ++i)
        {
          const auto chunk_id = chunk_ids[i];
          auto client_container = factory_client_container_->create(
            host,
            grpc_port);
          auto result = chunk_to_client_container.try_emplace(
            chunk_id,
            std::move(client_container));
          if (!result.second)
          {
            Stream::Error stream;
            stream << FNS
                   << ": chunk_id="
                   << chunk_id
                   << " already exist";
            logger_->error(
              stream.str(),
              ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
            continue;
          }

          if (chunk_id >= max_chunk_number)
          {
            max_chunk_number = chunk_id + 1;
          }
        }
      }

      partition = PartitionPtr(
        new Partition(
          std::move(chunk_to_client_container),
          max_chunk_number));
      break;
    }
    catch (...)
    {
    }
  }

  try
  {
    const Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(partition_holders_[partition_number]->mutex);
    if (partition)
    {
      partition_holders_[partition_number]->partition = std::move(partition);
    }
    else
    {
      partition_holders_[partition_number]->last_try_to_resolve = now;
      partition_holders_[partition_number]->resolve_in_progress = false;
    }
  }
  catch (...)
  {
  }
}

void GrpcUserBindOperationDistributor::try_to_reresolve_partition(
  const PartitionNumber partition_num) noexcept
{
  try
  {
    const Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(partition_holders_[partition_num]->mutex);
    if(!partition_holders_[partition_num]->resolve_in_progress &&
       now >= partition_holders_[partition_num]->last_try_to_resolve + pool_timeout_)
    {
      partition_holders_[partition_num]->resolve_in_progress = true;
    }
    else
    {
      return;
    }
    lock.unlock();

    task_runner_->enqueue_task(
      Generics::Task_var(
        new ResolvePartitionTask(
          this,
          partition_num)));
  }
  catch (...)
  {
  }
}

GrpcUserBindOperationDistributor::PartitionNumber
GrpcUserBindOperationDistributor::get_partition_number(
  const String::SubString& id) const noexcept
{
  return (AdServer::Commons::external_id_distribution_hash(id) >> 8) % try_count_;
}

GrpcUserBindOperationDistributor::PartitionPtr
GrpcUserBindOperationDistributor::get_partition(
  const PartitionNumber partition_number) noexcept
{
  try
  {
    PartitionPtr partition;

    {
      std::shared_lock lock(partition_holders_[partition_number]->mutex);
      partition = partition_holders_[partition_number]->partition;
    }

    return partition;
  }
  catch (...)
  {
  }

  return {};
}

GrpcUserBindOperationDistributor::GetBindRequestPtr
GrpcUserBindOperationDistributor::create_get_bind_request(
  const String::SubString& request_id,
  const Generics::Time& time)
{
  auto request = std::make_unique<GetBindRequest>();
  request->set_request_id(request_id.data(), request_id.length());
  request->set_timestamp(GrpcAlgs::pack_time(time));
  return request;
}

GrpcUserBindOperationDistributor::GetBindResponsePtr
GrpcUserBindOperationDistributor::get_bind_request(
  const String::SubString& request_id,
  const Generics::Time& time) noexcept
{
  return do_request<
    UserBindService_get_bind_request_ClientPool,
    GetBindRequest,
    GetBindResponse>(
      request_id,
      request_id,
      time);
}

GrpcUserBindOperationDistributor::AddBindRequestPtr
GrpcUserBindOperationDistributor::create_add_bind_request(
  const String::SubString& request_id,
  const AdServer::Commons::ExternalUserIdArray& user_ids,
  const Generics::Time& time)
{
  auto request = std::make_unique<AddBindRequest>();
  request->set_request_id(request_id.data(), request_id.length());
  request->set_timestamp(GrpcAlgs::pack_time(time));

  for (const auto& user_id : user_ids)
  {
    request->add_bind_user_ids(user_id);
  }

  return request;
}

GrpcUserBindOperationDistributor::AddBindResponsePtr
GrpcUserBindOperationDistributor::add_bind_request(
  const String::SubString& request_id,
  const AdServer::Commons::ExternalUserIdArray& user_ids,
  const Generics::Time& timestamp) noexcept
{
  return do_request<
    UserBindService_add_bind_request_ClientPool,
    AddBindRequest,
    AddBindResponse>(
      request_id,
      request_id,
      user_ids,
      timestamp);
}

GrpcUserBindOperationDistributor::GetUserIdRequestPtr
GrpcUserBindOperationDistributor::create_get_user_id_request(
  const String::SubString& external_id,
  const AdServer::Commons::UserId& current_user_id,
  const Generics::Time& timestamp,
  const Generics::Time& create_timestamp,
  const bool silent,
  const bool generate_user_id,
  const bool for_set_cookie)
{
  auto request = std::make_unique<GetUserIdRequest>();
  request->set_id(external_id.data(), external_id.length());
  request->set_timestamp(GrpcAlgs::pack_time(timestamp));
  request->set_silent(silent);
  request->set_generate_user_id(generate_user_id);
  request->set_for_set_cookie(for_set_cookie);
  request->set_create_timestamp(GrpcAlgs::pack_time(create_timestamp));
  request->set_current_user_id(GrpcAlgs::pack_user_id(current_user_id));

  return request;
}

GrpcUserBindOperationDistributor::GetUserIdResponsePtr
GrpcUserBindOperationDistributor::get_user_id(
  const String::SubString& external_id,
  const AdServer::Commons::UserId& current_user_id,
  const Generics::Time& timestamp,
  const Generics::Time& create_timestamp,
  const bool silent,
  const bool generate_user_id,
  const bool for_set_cookie) noexcept
{
  return do_request<
    UserBindService_get_user_id_ClientPool,
    GetUserIdRequest,
    GetUserIdResponse>(
      external_id,
      external_id,
      current_user_id,
      timestamp,
      create_timestamp,
      silent,
      generate_user_id,
      for_set_cookie);
}

GrpcUserBindOperationDistributor::AddUserIdRequestPtr
GrpcUserBindOperationDistributor::create_add_user_id_request(
  const String::SubString& external_id,
  const Generics::Time& timestamp,
  const String::SubString& user_id)
{
  auto request = std::make_unique<AddUserIdRequest>();
  request->set_id(external_id.data(), external_id.length());
  request->set_user_id(user_id.data(), user_id.length());
  request->set_timestamp(GrpcAlgs::pack_time(timestamp));

  return request;
}

GrpcUserBindOperationDistributor::AddUserIdResponsePtr
GrpcUserBindOperationDistributor::add_user_id(
  const String::SubString& external_id,
  const Generics::Time& timestamp,
  const String::SubString& user_id) noexcept
{
  return do_request<
    UserBindService_add_user_id_ClientPool,
    AddUserIdRequest,
    AddUserIdResponse>(
      external_id,
      external_id,
      timestamp,
      user_id);
}

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
        std::ostringstream stream;
        stream << FNS
               << "Error type=";

        const auto& error = response->error();
        const auto error_type = error.type();
        switch (error_type)
        {
          case Error_Type::Error_Type_NotReady:
          {
            stream << "NotReady";
            client_container->set_bad();
            break;
          }
          case Error_Type::Error_Type_ChunkNotFound:
          {
            stream << "ChunkNotFound";
            client_container->set_bad();
            try_to_reresolve_partition(partition_number);
            break;
          }
          case Error_Type::Error_Type_Implementation:
          {
            stream << "Implementation";
            client_container->set_bad();
            try_to_reresolve_partition(partition_number);
            break;
          }
          default:
          {
            stream << "Unknown error type";
            client_container->set_bad();
            try_to_reresolve_partition(partition_number);
            break;
          }
        }

        stream << ", description="
               << error.description()
               << " [id="
               << id
               << ", chunk_id="
               << chunk_id
               << "]";
        logger_->error(stream.str(), ASPECT_GRPC_USER_BIND_DISTRIBUTOR);
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