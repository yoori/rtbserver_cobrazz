#include "ChannelClientUtils.hpp"

namespace AdServer::ChannelSvcs
{
  DistributedChannelClientObjects
  create_distributed_channel_client(
    const ChannelDistributedGrpcClient::ChannelControllerRefs&
      channel_controller_refs,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor)
  {
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
