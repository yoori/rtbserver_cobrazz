#ifndef FRONTENDCOMMONS_GRPCCONTAINER
#define FRONTENDCOMMONS_GRPCCONTAINER

// STD
#include <memory>

// THIS
#include <ChannelSvcs/ChannelServer/GrpcChannelOperationPool.hpp>
#include <Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp>
#include <UserInfoSvcs/UserBindController/GrpcUserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserInfoManagerController/GrpcUserInfoOperationDistributor.hpp>

namespace FrontendCommons
{

struct GrpcContainer final
{
  using GrpcChannelOperationPool = AdServer::ChannelSvcs::GrpcChannelOperationPool;
  using GrpcChannelOperationPoolPtr = std::shared_ptr<GrpcChannelOperationPool>;
  using GrpcCampaignManagerPool = FrontendCommons::GrpcCampaignManagerPool;
  using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;
  using GrpcUserBindOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var;
  using GrpcUserInfoOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var;

  GrpcChannelOperationPoolPtr grpc_channel_operation_pool;
  GrpcCampaignManagerPoolPtr grpc_campaign_manager_pool;
  GrpcUserBindOperationDistributor_var grpc_user_bind_operation_distributor;
  GrpcUserInfoOperationDistributor_var grpc_user_info_operation_distributor;
};

using GrpcContainerPtr = std::shared_ptr<GrpcContainer>;

} // namespace FrontendCommons

#endif //FRONTENDCOMMONS_GRPCCONTAINER
