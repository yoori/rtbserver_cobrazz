#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Generics/ActiveObject.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelClient/ChannelGrpcAlgs.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>

namespace AdServer::ChannelSvcs
{
  struct DistributedChannelClientObjects
  {
    std::shared_ptr<ChannelServerGrpcAsyncClient> client;
    std::shared_ptr<Generics::ActiveObject> active_object;
  };

  DistributedChannelClientObjects
  create_distributed_channel_client(
    const ChannelDistributedGrpcClient::ChannelControllerRefs&
      channel_controller_refs,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor);
}
