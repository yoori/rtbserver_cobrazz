#pragma once

#include <ChannelSvcs/ChannelClient/ChannelClientUtils.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer::ChannelSvcs
{
  inline std::shared_ptr<ChannelDistributedGrpcClient>
  create_distributed_channel_client(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    Logging::Logger* logger = nullptr)
  {
    ChannelDistributedGrpcClient::ChannelControllerRefs
      channel_controller_refs;

    for(const auto& group : common_config.ChannelControllerGroup())
    {
      for(const auto& endpoint : group.Endpoint())
      {
        channel_controller_refs.emplace_back(endpoint);
      }
    }

    return create_distributed_channel_client(
      channel_controller_refs,
      std::move(grpc_executor),
      std::move(coalesce_runner),
      logger);
  }
}
