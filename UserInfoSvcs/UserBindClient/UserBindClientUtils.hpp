#pragma once

#include <future>
#include <string>
#include <utility>
#include <vector>

#include <grpcpp/support/status.h>

#include <Commons/ConfigUtils.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServer.hpp>
#include <UserInfoSvcs/UserBindClient/UserBindDistributedGrpcClient.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer::UserInfoSvcs
{
  struct DistributedUserBindClientObjects
  {
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor;
    UserBindDistributedGrpcClient_var client;
  };

  inline void
  throw_user_bind_exception(const grpc::Status& status)
  {
    const std::string message = status.error_message();
    switch(status.error_code())
    {
    case grpc::StatusCode::UNAVAILABLE:
      throw UserBindMapper::NotReady(message.c_str());
    case grpc::StatusCode::NOT_FOUND:
      throw UserBindMapper::ChunkNotFound(message.c_str());
    default:
      throw UserBindMapper::ImplementationException(message.c_str());
    }
  }

  template<typename Response, typename Start>
  Response
  wait_user_bind_grpc_call(Start&& start)
  {
    std::promise<std::pair<grpc::Status, Response>> promise;
    auto future = promise.get_future();

    start([&](
      const grpc::Status& call_status,
      const Response& call_response)
    {
      promise.set_value(std::make_pair(call_status, call_response));
    });

    auto result = future.get();

    if(!result.first.ok())
    {
      throw_user_bind_exception(result.first);
    }

    return std::move(result.second);
  }

  inline adserver::user_info_svcs::user_bind::GetUserIdResponse
  sync_get_user_id(
    UserBindServerGrpcAsyncClient* client,
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request)
  {
    return wait_user_bind_grpc_call<
      adserver::user_info_svcs::user_bind::GetUserIdResponse>(
        [&](auto callback)
        {
          client->get_user_id(request, std::move(callback));
        });
  }

  inline adserver::user_info_svcs::user_bind::AddUserIdResponse
  sync_add_user_id(
    UserBindServerGrpcAsyncClient* client,
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request)
  {
    return wait_user_bind_grpc_call<
      adserver::user_info_svcs::user_bind::AddUserIdResponse>(
        [&](auto callback)
        {
          client->add_user_id(request, std::move(callback));
        });
  }

  inline DistributedUserBindClientObjects
  create_distributed_user_bind_client(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config,
    Logging::Logger* logger)
  {
    AdServer::Grpc::BatchingOptions batching_options;
    std::size_t grpc_executor_threads = 16;
    std::vector<std::string> user_bind_controller_refs;

    if(common_config.UserBind().present())
    {
      const auto& user_bind_config = *common_config.UserBind();
      grpc_executor_threads = user_bind_config.grpc_executor_threads();
      if(user_bind_config.BatchingOptions().present())
      {
        batching_options =
          Config::read_xsd_grpc_options(*user_bind_config.BatchingOptions());
      }

      for(const auto& group : user_bind_config.UserBindController2Group())
      {
        for(const auto& endpoint : group.Endpoint())
        {
          user_bind_controller_refs.emplace_back(endpoint);
        }
      }
    }

    DistributedUserBindClientObjects result;
    if(!user_bind_controller_refs.empty())
    {
      result.grpc_executor =
        std::make_shared<AdServer::Grpc::GrpcExecutor>(grpc_executor_threads);
      result.client = new UserBindDistributedGrpcClient(
        user_bind_controller_refs,
        batching_options,
        result.grpc_executor,
        logger);
    }

    return result;
  }
}
