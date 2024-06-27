// THIS
#include "Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp"

namespace FrontendCommons
{

GrpcCampaignManagerPool::GrpcCampaignManagerPool(
  Logger* logger,
  TaskProcessor& task_processor,
  const SchedulerPtr& scheduler,
  const Endpoints& endpoints,
  const ConfigPoolClient& config_pool_client,
  const std::size_t grpc_client_timeout_ms,
  const std::size_t time_duration_client_bad_ms)
  : logger_(ReferenceCounting::add_ref(logger)),
    task_processor_(task_processor),
    scheduler_(scheduler),
    factory_client_holder_(new FactoryClientHolder(
      logger_.in(),
      scheduler_,
      config_pool_client,
      task_processor,
      Time{static_cast<time_t>(time_duration_client_bad_ms / 1000)})),
    grpc_client_timeout_ms_(grpc_client_timeout_ms)
{
  const std::size_t size = endpoints.size();
  client_holders_.reserve(size);
  for (const auto& [host, port] : endpoints)
  {
    client_holders_.emplace_back(
      factory_client_holder_->create(host, port));
  }
}

GrpcCampaignManagerPool::GetPubPixelsRequestPtr
GrpcCampaignManagerPool::create_get_pub_pixels_request(
  const std::string& country,
  const std::uint32_t user_status,
  const std::vector<std::uint32_t>& publisher_account_ids)
{
  auto request = std::make_unique<GetPubPixelsRequest>();
  request->set_country(country);
  request->set_user_status(user_status);
  auto* ids = request->mutable_publisher_account_ids();
  ids->Add(
    std::begin(publisher_account_ids),
    std::end(publisher_account_ids));

  return request;
}

GrpcCampaignManagerPool::GetPubPixelsResponsePtr
GrpcCampaignManagerPool::get_pub_pixels(
  const std::string& country,
  const std::uint32_t user_status,
  const std::vector<std::uint32_t>& publisher_account_ids)
{
  using GetPubPixelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPool;

  return do_request<
    GetPubPixelsClient,
    GetPubPixelsRequest,
    GetPubPixelsResponse>(country, user_status, publisher_account_ids);
}

} // namespace FrontendCommons
