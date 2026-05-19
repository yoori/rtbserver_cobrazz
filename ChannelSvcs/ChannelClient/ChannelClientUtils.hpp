#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Commons/Grpc/GrpcExecutor.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelClient/ChannelGrpcAlgs.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>

namespace AdServer::ChannelSvcs
{
  std::shared_ptr<ChannelDistributedGrpcClient>
  create_distributed_channel_client(
    const ChannelDistributedGrpcClient::ChannelControllerRefs&
      channel_controller_refs,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner = {});
}
