#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <grpcpp/support/status.h>

#include <Commons/ConfigUtils.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <UserInfoSvcs/UserBindClient/UserBindDistributedGrpcClient.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer::UserInfoSvcs
{
  namespace UserBindClient
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);
    DECLARE_EXCEPTION(ImplementationException, Exception);
  }

  struct DistributedUserBindClientObjects
  {
    std::shared_ptr<UserBindServerGrpcAsyncClient> client;
    std::shared_ptr<Generics::ActiveObject> active_object;
  };

  inline void
  throw_user_bind_exception(const grpc::Status& status)
  {
    const std::string message = status.error_message();
    switch(status.error_code())
    {
    case grpc::StatusCode::UNAVAILABLE:
      throw UserBindClient::NotReady(message.c_str());
    case grpc::StatusCode::NOT_FOUND:
      throw UserBindClient::ChunkNotFound(message.c_str());
    default:
      throw UserBindClient::ImplementationException(message.c_str());
    }
  }

  template<typename Response, typename Start>
  Response
  wait_user_bind_grpc_call(Start&& start)
  {
    return AdServer::Grpc::sync_call<Response>(
      std::forward<Start>(start),
      [](const grpc::Status& status)
      {
        throw_user_bind_exception(status);
      });
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
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    Logging::Logger* logger)
  {
    AdServer::Grpc::BatchingOptions batching_options;
    std::vector<std::string> user_bind_controller_refs;

    if(common_config.UserBind().present())
    {
      const auto& user_bind_config = *common_config.UserBind();
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
      auto client = std::make_shared<UserBindDistributedGrpcClient>(
        user_bind_controller_refs,
        batching_options,
        std::move(grpc_executor),
        logger);
      result.client = client;
      result.active_object = client;
    }

    return result;
  }
}
