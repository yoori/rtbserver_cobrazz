#pragma once

#include <utility>

#include <Commons/ConfigUtils.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoDistributedGrpcClient.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoGrpcAlgs.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer::UserInfoSvcs
{
  inline std::shared_ptr<UserInfoDistributedGrpcClient>
  create_distributed_user_info_client(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    Logging::Logger* logger)
  {
    AdServer::Grpc::BatchingOptions batching_options;
    UserInfoDistributedGrpcClient::UserInfoControllerRefs
      user_info_controller_refs;

    if(common_config.UserInfo().present())
    {
      const auto& user_info_config = *common_config.UserInfo();
      if(user_info_config.BatchingOptions().present())
      {
        batching_options =
          Config::read_xsd_grpc_options(*user_info_config.BatchingOptions());
      }

      for(const auto& group : user_info_config.UserInfoControllerGroup())
      {
        UserInfoDistributedGrpcClient::UserInfoControllerRefGroup
          user_info_controller_ref_group;
        for(const auto& endpoint : group.Endpoint())
        {
          user_info_controller_ref_group.emplace_back(endpoint);
        }
        if(!user_info_controller_ref_group.empty())
        {
          user_info_controller_refs.emplace_back(
            std::move(user_info_controller_ref_group));
        }
      }
    }

    return std::make_shared<UserInfoDistributedGrpcClient>(
      user_info_controller_refs,
      batching_options,
      std::move(grpc_executor),
      logger,
      std::move(coalesce_runner));
  }
}
