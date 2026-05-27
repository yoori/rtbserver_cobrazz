#include "ChannelClientUtils.hpp"

namespace AdServer::ChannelSvcs
{
  std::shared_ptr<ChannelDistributedGrpcClient>
  create_distributed_channel_client(
    const ChannelDistributedGrpcClient::ChannelControllerRefs&
      channel_controller_refs,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    Logging::Logger* logger,
    AdServer::Grpc::BatchingOptions batching_options)
  {
    return std::make_shared<ChannelDistributedGrpcClient>(
      channel_controller_refs,
      std::move(batching_options),
      std::move(grpc_executor),
      std::move(coalesce_runner),
      logger);
  }
}
