#ifndef FRONTENDCOMMONS_GRPCCONTAINER
#define FRONTENDCOMMONS_GRPCCONTAINER

// STD
#include <memory>

// THIS
#include <ChannelSvcs/ChannelServer/GrpcChannelOperationPool.hpp>
#include <Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp>

namespace FrontendCommons
{

struct GrpcContainer final
{
  using GrpcChannelOperationPool = AdServer::ChannelSvcs::GrpcChannelOperationPool;
  using GrpcChannelOperationPoolPtr = std::shared_ptr<GrpcChannelOperationPool>;
  using GrpcCampaignManagerPool = FrontendCommons::GrpcCampaignManagerPool;
  using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;

  GrpcChannelOperationPoolPtr grpc_channel_operation_pool;
  GrpcCampaignManagerPoolPtr grpc_campaign_manager_pool;
};

using GrpcContainerPtr = std::shared_ptr<GrpcContainer>;

} // namespace FrontendCommons

#endif //FRONTENDCOMMONS_GRPCCONTAINER
