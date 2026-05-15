#include "UserBindDistributedGrpcClient.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/DistributedPartitionPool.hpp>
#include <Commons/UserInfoManip.hpp>
#include <String/StringManip.hpp>
#include <UserInfoSvcs/UserBindController2/UserBindControllerGrpc.grpc.pb.h>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    const Generics::Time DEFAULT_POOL_TIMEOUT = Generics::Time::ONE_SECOND;
    const Generics::Time DEFAULT_RESOLVE_PERIOD = Generics::Time(10);
    const std::chrono::seconds DEFAULT_RPC_TIMEOUT(5);

    void set_deadline_(grpc::ClientContext& context)
    {
      context.set_deadline(std::chrono::system_clock::now() + DEFAULT_RPC_TIMEOUT);
    }
  }

  class UserBindDistributedGrpcClient::Distributor:
    public Generics::CompositeActiveObject,
    public UserBindServerGrpcAsyncClient
  {
  public:
    using ControllerRefList = std::vector<std::string>;
    using ServerClient = UserBindServerGrpcAsyncBatchingClient;
    using Pool = AdServer::Grpc::DistributedPartitionPool<ServerClient>;
    using ControllerGrpc =
      adserver::user_info_svcs::user_bind_controller::UserBindControllerGrpc;

    Distributor(
      Logging::Logger* logger,
      ControllerRefList controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor)
      : pool_(std::make_shared<Pool>(
          "UserBindDistributedGrpcClient",
          "UserInfo",
          std::move(controller_refs),
          std::move(batching_options),
          std::move(grpc_executor),
          logger,
          &resolve_partition_,
          &partition_index_,
          &chunk_index_,
          DEFAULT_POOL_TIMEOUT,
          DEFAULT_RESOLVE_PERIOD))
    {
      add_child_object(pool_);
    }

    void get_bind_request(
      const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
      GetBindRequestCallback callback) override
    {
      route_request_<
        adserver::user_info_svcs::user_bind::GetBindRequestResponse>(
        request,
        std::move(callback),
        request.request_id(),
        [](const auto& client, const auto& req, auto cb)
        {
          client->get_bind_request(req, std::move(cb));
        });
    }

    void add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      AddBindRequestCallback callback) override
    {
      route_request_<
        adserver::user_info_svcs::user_bind::AddBindRequestResponse>(
        request,
        std::move(callback),
        request.request_id(),
        [](const auto& client, const auto& req, auto cb)
        {
          client->add_bind_request(req, std::move(cb));
        });
    }

    void get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      GetUserIdCallback callback) override
    {
      route_request_<
        adserver::user_info_svcs::user_bind::GetUserIdResponse>(
        request,
        std::move(callback),
        request.id(),
        [](const auto& client, const auto& req, auto cb)
        {
          client->get_user_id(req, std::move(cb));
        });
    }

    void add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      AddUserIdCallback callback) override
    {
      route_request_<
        adserver::user_info_svcs::user_bind::AddUserIdResponse>(
        request,
        std::move(callback),
        request.id(),
        [](const auto& client, const auto& req, auto cb)
        {
          client->add_user_id(req, std::move(cb));
        });
    }

    void get_source(
      const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
      GetSourceCallback callback) override
    {
      route_request_<
        adserver::user_info_svcs::user_bind::GetSourceResponse>(
        request,
        std::move(callback),
        std::string(),
        [](const auto& client, const auto& req, auto cb)
        {
          client->get_source(req, std::move(cb));
        });
    }

    AdServer::Grpc::Stats stats() const noexcept override
    {
      return pool_->stats();
    }

  private:
    template<typename Response, typename Request, typename Callback, typename Call>
    void route_request_(
      const Request& request,
      Callback callback,
      const std::string& key,
      Call call)
    {
      auto ref = pool_->get_ref(key);
      if (!ref)
      {
        finish_with_unavailable_<Response>(std::move(callback));
        return;
      }

      auto pool_ref = std::move(*ref);
      auto client = pool_ref->client;
      call(
        client,
        request,
        [
          pool_ref = std::move(pool_ref),
          callback = std::move(callback)
        ](const grpc::Status& status, const Response& response) mutable
        {
          if (!status.ok())
          {
            pool_ref.mark_as_bad(
              Generics::Time::get_time_of_day() + DEFAULT_POOL_TIMEOUT);
          }
          callback(status, response);
        });
    }

    template<typename Response, typename Callback>
    static void finish_with_unavailable_(Callback callback)
    {
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available UserBindServer grpc client"),
        Response());
    }

    static std::optional<Pool::EndpointChunksList> resolve_partition_(
      const std::string& endpoint)
    {
      auto channel = grpc::CreateChannel(
        endpoint,
        grpc::InsecureChannelCredentials());
      auto stub = ControllerGrpc::NewStub(channel);

      grpc::ClientContext context;
      set_deadline_(context);
      adserver::user_info_svcs::user_bind_controller::
        GetSessionDescriptionRequest request;
      adserver::user_info_svcs::user_bind_controller::
        GetSessionDescriptionResponse response;

      const auto status =
        stub->get_session_description(&context, request, &response);
      if (!status.ok())
      {
        return std::nullopt;
      }

      Pool::EndpointChunksList refs;
      refs.reserve(response.user_bind_servers_size());
      for (const auto& server : response.user_bind_servers())
      {
        Pool::EndpointChunks endpoint_chunks;
        endpoint_chunks.endpoint = server.user_bind_server_endpoint();
        endpoint_chunks.chunk_ids.reserve(server.chunk_ids_size());
        for (const auto chunk_id : server.chunk_ids())
        {
          endpoint_chunks.chunk_ids.emplace_back(chunk_id);
        }
        refs.emplace_back(std::move(endpoint_chunks));
      }
      return refs;
    }

    static unsigned long partition_index_(
      const std::string& user_id,
      unsigned long partitions_number)
    {
      return (
        AdServer::Commons::external_id_distribution_hash(
          String::SubString(user_id)) >> 8) % partitions_number;
    }

    static unsigned long chunk_index_(
      const std::string& user_id,
      unsigned long chunks_number)
    {
      return AdServer::Commons::external_id_distribution_hash(
        String::SubString(user_id)) % chunks_number;
    }

  private:
    std::shared_ptr<Pool> pool_;
  };

  UserBindDistributedGrpcClient::UserBindDistributedGrpcClient(
    const UserBindControllerRefs& user_bind_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    Logging::Logger* logger)
  {
    auto distributor = std::make_shared<Distributor>(
      logger,
      user_bind_controller_refs,
      std::move(batching_options),
      std::move(grpc_executor));
    user_bind_mapper_ = distributor;
    add_child_object(distributor);
  }

  UserBindDistributedGrpcClient::~UserBindDistributedGrpcClient() noexcept =
    default;

  AdServer::Grpc::Stats
  UserBindDistributedGrpcClient::stats() const noexcept
  {
    return user_bind_mapper_->stats();
  }

  void
  UserBindDistributedGrpcClient::get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    GetBindRequestCallback callback)
  {
    user_bind_mapper_->get_bind_request(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    AddBindRequestCallback callback)
  {
    user_bind_mapper_->add_bind_request(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    GetUserIdCallback callback)
  {
    user_bind_mapper_->get_user_id(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    AddUserIdCallback callback)
  {
    user_bind_mapper_->add_user_id(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
    GetSourceCallback callback)
  {
    user_bind_mapper_->get_source(request, std::move(callback));
  }
}
