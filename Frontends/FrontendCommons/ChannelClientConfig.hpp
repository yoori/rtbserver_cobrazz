#pragma once

#include <utility>

#include <Commons/ConfigUtils.hpp>
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
    AdServer::Grpc::BatchingOptions batching_options;

    if(common_config.Channel().present())
    {
      const auto& channel_config = *common_config.Channel();
      if(channel_config.BatchingOptions().present())
      {
        batching_options =
          Config::read_xsd_grpc_options(*channel_config.BatchingOptions());
      }

      for(const auto& group : channel_config.ChannelControllerGroup())
      {
        ChannelDistributedGrpcClient::ChannelControllerRefGroup
          channel_controller_ref_group;
        for(const auto& endpoint : group.Endpoint())
        {
          channel_controller_ref_group.emplace_back(endpoint);
        }
        if(!channel_controller_ref_group.empty())
        {
          channel_controller_refs.emplace_back(
            std::move(channel_controller_ref_group));
        }
      }
    }

    return create_distributed_channel_client(
      channel_controller_refs,
      std::move(grpc_executor),
      std::move(coalesce_runner),
      logger,
      std::move(batching_options));
  }
}
