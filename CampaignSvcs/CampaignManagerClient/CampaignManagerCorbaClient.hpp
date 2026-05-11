#pragma once

#include <memory>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>

namespace AdServer::CampaignSvcs
{
  class CampaignManagerCorbaClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using ReadyCallback = CampaignManagerGrpcAsyncClient::ReadyCallback;
    using ProgressCommentCallback =
      CampaignManagerGrpcAsyncClient::ProgressCommentCallback;
    using MatchGeoChannelsCallback =
      CampaignManagerGrpcAsyncClient::MatchGeoChannelsCallback;
    using GetFileCallback = CampaignManagerGrpcAsyncClient::GetFileCallback;
    using TraceCampaignSelectionIndexCallback =
      CampaignManagerGrpcAsyncClient::TraceCampaignSelectionIndexCallback;
    using GetCampaignCreativeByCcidCallback =
      CampaignManagerGrpcAsyncClient::GetCampaignCreativeByCcidCallback;
    using GetChannelLinksCallback =
      CampaignManagerGrpcAsyncClient::GetChannelLinksCallback;
    using GetDiscoverChannelsCallback =
      CampaignManagerGrpcAsyncClient::GetDiscoverChannelsCallback;
    using GetCategoryChannelsCallback =
      CampaignManagerGrpcAsyncClient::GetCategoryChannelsCallback;
    using GetColocationFlagsCallback =
      CampaignManagerGrpcAsyncClient::GetColocationFlagsCallback;
    using GetPubPixelsCallback =
      CampaignManagerGrpcAsyncClient::GetPubPixelsCallback;
    using ConsiderPassbackCallback =
      CampaignManagerGrpcAsyncClient::ConsiderPassbackCallback;
    using ConsiderPassbackTrackCallback =
      CampaignManagerGrpcAsyncClient::ConsiderPassbackTrackCallback;
    using VerifyOptOperationCallback =
      CampaignManagerGrpcAsyncClient::VerifyOptOperationCallback;
    using ConsiderWebOperationCallback =
      CampaignManagerGrpcAsyncClient::ConsiderWebOperationCallback;

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

    ~CampaignManagerCorbaClient() noexcept;

    void ready(
      const adserver::campaign_svcs::campaign_manager::ReadyRequest& request,
      ReadyCallback callback);

    void progress_comment(
      const adserver::campaign_svcs::campaign_manager::ProgressCommentRequest& request,
      ProgressCommentCallback callback);

    void match_geo_channels(
      const adserver::campaign_svcs::campaign_manager::MatchGeoChannelsRequest& request,
      MatchGeoChannelsCallback callback);

    void get_file(
      const adserver::campaign_svcs::campaign_manager::GetFileRequest& request,
      GetFileCallback callback);

    void trace_campaign_selection_index(
      const adserver::campaign_svcs::campaign_manager::TraceCampaignSelectionIndexRequest& request,
      TraceCampaignSelectionIndexCallback callback);

    void get_campaign_creative_by_ccid(
      const adserver::campaign_svcs::campaign_manager::GetCampaignCreativeByCcidRequest& request,
      GetCampaignCreativeByCcidCallback callback);

    void get_channel_links(
      const adserver::campaign_svcs::campaign_manager::GetChannelLinksRequest& request,
      GetChannelLinksCallback callback);

    void get_discover_channels(
      const adserver::campaign_svcs::campaign_manager::GetDiscoverChannelsRequest& request,
      GetDiscoverChannelsCallback callback);

    void get_category_channels(
      const adserver::campaign_svcs::campaign_manager::GetCategoryChannelsRequest& request,
      GetCategoryChannelsCallback callback);

    void get_colocation_flags(
      const adserver::campaign_svcs::campaign_manager::GetColocationFlagsRequest& request,
      GetColocationFlagsCallback callback);

    void get_pub_pixels(
      const adserver::campaign_svcs::campaign_manager::GetPubPixelsRequest& request,
      GetPubPixelsCallback callback);

    void consider_passback(
      const adserver::campaign_svcs::campaign_manager::ConsiderPassbackRequest& request,
      ConsiderPassbackCallback callback);

    void consider_passback_track(
      const adserver::campaign_svcs::campaign_manager::ConsiderPassbackTrackRequest& request,
      ConsiderPassbackTrackCallback callback);

    void verify_opt_operation(
      const adserver::campaign_svcs::campaign_manager::VerifyOptOperationRequest& request,
      VerifyOptOperationCallback callback);

    void consider_web_operation(
      const adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest& request,
      ConsiderWebOperationCallback callback);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };

  using CampaignManagerCorbaClientPtr =
    std::shared_ptr<CampaignManagerCorbaClient>;
}
