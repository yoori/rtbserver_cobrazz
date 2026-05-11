#include "ChannelClientUtils.hpp"

#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>

namespace AdServer::ChannelSvcs
{
  DistributedChannelClientObjects
  create_distributed_channel_client(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor)
  {
    ChannelDistributedGrpcClient::ChannelControllerRefs
      channel_controller_refs;

    for(const auto& group : common_config.ChannelController2Group())
    {
      for(const auto& endpoint : group.Endpoint())
      {
        channel_controller_refs.emplace_back(endpoint);
      }
    }

    DistributedChannelClientObjects result;
    auto client = std::make_shared<ChannelDistributedGrpcClient>(
      channel_controller_refs,
      AdServer::Grpc::BatchingOptions(),
      std::move(grpc_executor));

    result.client = client;
    result.active_object = client;

    return result;
  }
}
