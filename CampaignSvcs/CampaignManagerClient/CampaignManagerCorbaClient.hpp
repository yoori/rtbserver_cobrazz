#pragma once

#include <memory>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>

namespace AdServer::CampaignSvcs
{
  class CampaignManagerCorbaClient:
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

    explicit CampaignManagerCorbaClient(
      const CampaignManagerRefs& campaign_manager_refs)
      /*throw(Exception)*/;

    explicit CampaignManagerCorbaClient(
      const CampaignManagerObjectRefs& campaign_manager_refs)
      /*throw(Exception)*/;

    ~CampaignManagerCorbaClient() noexcept override;

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

    void trace_campaign_selection_index(
      const adserver::campaign_svcs::campaign_manager::TraceCampaignSelectionIndexRequest& request,
      TraceCampaignSelectionIndexCallback callback) override;

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

    void verify_opt_operation(
      const adserver::campaign_svcs::campaign_manager::VerifyOptOperationRequest& request,
      VerifyOptOperationCallback callback) override;

    void consider_web_operation(
      const adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest& request,
      ConsiderWebOperationCallback callback) override;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };

  using CampaignManagerCorbaClientPtr =
    std::shared_ptr<CampaignManagerCorbaClient>;
}
