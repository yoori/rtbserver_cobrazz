#pragma once

#include <utility>

#include <Commons/ConfigUtils.hpp>
#include <UserInfoSvcs/UserBindClient/UserBindClientUtils.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer::UserInfoSvcs
{
  inline std::shared_ptr<UserBindDistributedGrpcClient>
  create_distributed_user_bind_client(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    Logging::Logger* logger)
  {
    AdServer::Grpc::BatchingOptions batching_options;
    UserBindDistributedGrpcClient::UserBindControllerRefs
      user_bind_controller_refs;

    if(common_config.UserBind().present())
    {
      const auto& user_bind_config = *common_config.UserBind();
      if(user_bind_config.BatchingOptions().present())
      {
        batching_options =
          Config::read_xsd_grpc_options(*user_bind_config.BatchingOptions());
      }

      for(const auto& group : user_bind_config.UserBindControllerGroup())
      {
        UserBindDistributedGrpcClient::UserBindControllerRefGroup
          user_bind_controller_ref_group;
        for(const auto& endpoint : group.Endpoint())
        {
          user_bind_controller_ref_group.emplace_back(endpoint);
        }
        if(!user_bind_controller_ref_group.empty())
        {
          user_bind_controller_refs.emplace_back(
            std::move(user_bind_controller_ref_group));
        }
      }
    }

    return create_distributed_user_bind_client(
      user_bind_controller_refs,
      batching_options,
      std::move(grpc_executor),
      std::move(coalesce_runner),
      logger);
  }
}
