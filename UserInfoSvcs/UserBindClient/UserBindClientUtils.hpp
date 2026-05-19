#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <eh/Exception.hpp>
#include <grpcpp/support/status.h>

#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <UserInfoSvcs/UserBindClient/UserBindDistributedGrpcClient.hpp>

namespace AdServer::UserInfoSvcs
{
  namespace UserBindClient
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);
    DECLARE_EXCEPTION(ImplementationException, Exception);
  }

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

  inline std::shared_ptr<UserBindDistributedGrpcClient>
  create_distributed_user_bind_client(
    const UserBindDistributedGrpcClient::UserBindControllerRefs&
      user_bind_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    Logging::Logger* logger)
  {
    if(!user_bind_controller_refs.empty())
    {
      return std::make_shared<UserBindDistributedGrpcClient>(
        user_bind_controller_refs,
        batching_options,
        std::move(grpc_executor),
        logger,
        std::move(coalesce_runner));
    }

    return {};
  }
}
