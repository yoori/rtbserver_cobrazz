#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/RefPool.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>

namespace AdServer::CampaignSvcs
{
  class CampaignManagerDistributedGrpcClient:
    public Generics::CompositeActiveObject,
    public CampaignManagerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct CampaignManagerRef
    {
      std::string object_ref;
      std::string service_index;
    };

    using CampaignManagerRefs = std::vector<CampaignManagerRef>;
    using CampaignManagerObjectRefs = std::vector<std::string>;

    explicit CampaignManagerDistributedGrpcClient(
      const CampaignManagerRefs& campaign_manager_refs,
      const AdServer::Grpc::BatchingOptions& batching_options = {},
      const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& grpc_executor = {});

    explicit CampaignManagerDistributedGrpcClient(
      const CampaignManagerObjectRefs& campaign_manager_refs,
      const AdServer::Grpc::BatchingOptions& batching_options = {},
      const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& grpc_executor = {});

    ~CampaignManagerDistributedGrpcClient() noexcept override;

    AdServer::Grpc::Stats stats() const noexcept override;

    void ready(
      const adserver::campaign_svcs::campaign_manager::ReadyRequest& request,
      ReadyCallback callback) override;

    void progress_comment(
      const adserver::campaign_svcs::campaign_manager::ProgressCommentRequest& request,
      ProgressCommentCallback callback) override;

    void match_geo_channels(
      const adserver::campaign_svcs::campaign_manager::MatchGeoChannelsRequest& request,
      MatchGeoChannelsCallback callback) override;

    void get_file(
      const adserver::campaign_svcs::campaign_manager::GetFileRequest& request,
      GetFileCallback callback) override;

    void get_campaign_creative(
      const adserver::campaign_svcs::campaign_manager::GetCampaignCreativeRequest& request,
      GetCampaignCreativeCallback callback) override;

    void process_match_request(
      const adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest& request,
      ProcessMatchRequestCallback callback) override;

    void process_anonymous_request(
      const adserver::campaign_svcs::campaign_manager::ProcessAnonymousRequestRequest& request,
      ProcessAnonymousRequestCallback callback) override;

    void instantiate_ad(
      const adserver::campaign_svcs::campaign_manager::InstantiateAdRequest& request,
      InstantiateAdCallback callback) override;

    void trace_campaign_selection_index(
      const adserver::campaign_svcs::campaign_manager::TraceCampaignSelectionIndexRequest& request,
      TraceCampaignSelectionIndexCallback callback) override;

    void trace_campaign_selection(
      const adserver::campaign_svcs::campaign_manager::TraceCampaignSelectionRequest& request,
      TraceCampaignSelectionCallback callback) override;

    void get_campaign_creative_by_ccid(
      const adserver::campaign_svcs::campaign_manager::GetCampaignCreativeByCcidRequest& request,
      GetCampaignCreativeByCcidCallback callback) override;

    void get_channel_links(
      const adserver::campaign_svcs::campaign_manager::GetChannelLinksRequest& request,
      GetChannelLinksCallback callback) override;

    void get_discover_channels(
      const adserver::campaign_svcs::campaign_manager::GetDiscoverChannelsRequest& request,
      GetDiscoverChannelsCallback callback) override;

    void get_category_channels(
      const adserver::campaign_svcs::campaign_manager::GetCategoryChannelsRequest& request,
      GetCategoryChannelsCallback callback) override;

    void get_colocation_flags(
      const adserver::campaign_svcs::campaign_manager::GetColocationFlagsRequest& request,
      GetColocationFlagsCallback callback) override;

    void get_pub_pixels(
      const adserver::campaign_svcs::campaign_manager::GetPubPixelsRequest& request,
      GetPubPixelsCallback callback) override;

    void consider_passback(
      const adserver::campaign_svcs::campaign_manager::ConsiderPassbackRequest& request,
      ConsiderPassbackCallback callback) override;

    void consider_passback_track(
      const adserver::campaign_svcs::campaign_manager::ConsiderPassbackTrackRequest& request,
      ConsiderPassbackTrackCallback callback) override;

    void get_click_url(
      const adserver::campaign_svcs::campaign_manager::GetClickUrlRequest& request,
      GetClickUrlCallback callback) override;

    void verify_impression(
      const adserver::campaign_svcs::campaign_manager::VerifyImpressionRequest& request,
      VerifyImpressionCallback callback) override;

    void action_taken(
      const adserver::campaign_svcs::campaign_manager::ActionTakenRequest& request,
      ActionTakenCallback callback) override;

    void verify_opt_operation(
      const adserver::campaign_svcs::campaign_manager::VerifyOptOperationRequest& request,
      VerifyOptOperationCallback callback) override;

    void consider_web_operation(
      const adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest& request,
      ConsiderWebOperationCallback callback) override;

    void get_config(
      const adserver::campaign_svcs::campaign_manager::GetConfigRequest& request,
      GetConfigCallback callback) override;

  private:
    using Client = CampaignManagerGrpcAsyncBatchingClient;
    using ClientPtr = std::shared_ptr<Client>;

    struct ClientHolder;
    using ClientHolderPtr = std::shared_ptr<ClientHolder>;

    using Pool = AdServer::Grpc::RefPool<ClientHolder>;
    using PoolPtr = std::shared_ptr<Pool>;

    void activate_object_() override;
    void deactivate_object_() override;

    std::optional<Pool::Ref> get_ref_(
      const std::string& service_index = std::string()) const;

    template<typename Request, typename Response, typename Callback, typename Call>
    void call_(
      const Request& request,
      Callback callback,
      Call call,
      const char* unavailable_description,
      const std::string& service_index = std::string());

    static void merge_stats_(
      AdServer::Grpc::Stats& result,
      const AdServer::Grpc::Stats& source) noexcept;

  private:
    const Generics::Time pool_timeout_;
    std::vector<ClientHolderPtr> client_holders_;
    PoolPtr default_pool_;
    std::map<std::string, PoolPtr> service_index_pools_;
  };

  using CampaignManagerDistributedGrpcClientPtr =
    std::shared_ptr<CampaignManagerDistributedGrpcClient>;
}
