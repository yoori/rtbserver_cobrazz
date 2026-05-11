#include "CampaignManagerDistributedGrpcClient.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    namespace pb = adserver::campaign_svcs::campaign_manager;

    const Generics::Time DEFAULT_POOL_TIMEOUT = Generics::Time::ONE_SECOND;

    grpc::Status unavailable_status(const char* description)
    {
      return grpc::Status(
        grpc::StatusCode::UNAVAILABLE,
        description ? description : "");
    }
  }

  struct CampaignManagerDistributedGrpcClient::ClientHolder
  {
    ClientHolder(
      std::string endpoint_val,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor)
      : endpoint(std::move(endpoint_val)),
        client(std::make_shared<Client>(
          endpoint,
          std::move(grpc_executor),
          std::move(batching_options)))
    {
      client->activate_object();
    }

    ~ClientHolder() noexcept
    {
      try
      {
        if (client->active())
        {
          client->deactivate_object();
          client->wait_object();
        }
      }
      catch (...)
      {
      }
    }

    const std::string endpoint;
    ClientPtr client;
  };

  CampaignManagerDistributedGrpcClient::CampaignManagerDistributedGrpcClient(
    const CampaignManagerRefs& campaign_manager_refs,
    const AdServer::Grpc::BatchingOptions& batching_options,
    const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& grpc_executor)
    : pool_timeout_(DEFAULT_POOL_TIMEOUT)
  {
    if (campaign_manager_refs.empty())
    {
      throw Exception("CampaignManager grpc refs list is empty");
    }

    auto effective_grpc_executor = grpc_executor;
    if (!effective_grpc_executor)
    {
      effective_grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(
        batching_options.workers_number);
      add_child_object(effective_grpc_executor);
    }

    std::vector<ClientHolderPtr> default_refs;
    default_refs.reserve(campaign_manager_refs.size());

    for (const auto& ref : campaign_manager_refs)
    {
      auto client_holder = std::make_shared<ClientHolder>(
        ref.object_ref,
        batching_options,
        effective_grpc_executor);

      default_refs.emplace_back(client_holder);
      if (!ref.service_index.empty())
      {
        auto pool = std::make_shared<Pool>();
        pool->set_refs({client_holder});
        service_index_pools_.emplace(ref.service_index, std::move(pool));
      }
      client_holders_.emplace_back(std::move(client_holder));
    }

    default_pool_ = std::make_shared<Pool>();
    default_pool_->set_refs(default_refs);
  }

  CampaignManagerDistributedGrpcClient::CampaignManagerDistributedGrpcClient(
    const CampaignManagerObjectRefs& campaign_manager_refs,
    const AdServer::Grpc::BatchingOptions& batching_options,
    const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& grpc_executor)
    : CampaignManagerDistributedGrpcClient(
        [&campaign_manager_refs]()
        {
          CampaignManagerRefs result;
          result.reserve(campaign_manager_refs.size());
          for (const auto& object_ref : campaign_manager_refs)
          {
            result.push_back({object_ref, std::string()});
          }
          return result;
        }(),
        batching_options,
        grpc_executor)
  {}

  CampaignManagerDistributedGrpcClient::~CampaignManagerDistributedGrpcClient()
    noexcept = default;

  void
  CampaignManagerDistributedGrpcClient::activate_object_()
  {
    Generics::CompositeActiveObject::activate_object_();
    default_pool_->activate_object();
    for (auto& service_index_pool : service_index_pools_)
    {
      service_index_pool.second->activate_object();
    }
  }

  void
  CampaignManagerDistributedGrpcClient::deactivate_object_()
  {
    for (auto& service_index_pool : service_index_pools_)
    {
      service_index_pool.second->deactivate_object();
    }
    default_pool_->deactivate_object();

    Generics::CompositeActiveObject::deactivate_object_();

    for (auto& service_index_pool : service_index_pools_)
    {
      service_index_pool.second->wait_object();
    }
    default_pool_->wait_object();
  }

  AdServer::Grpc::Stats
  CampaignManagerDistributedGrpcClient::stats() const noexcept
  {
    AdServer::Grpc::Stats result;
    std::set<const ClientHolder*> seen_clients;
    for (const auto& client_holder : client_holders_)
    {
      if (!client_holder || !seen_clients.insert(client_holder.get()).second)
      {
        continue;
      }

      merge_stats_(
        result,
        static_cast<CampaignManagerGrpcAsyncClient*>(
          client_holder->client.get())->stats());
    }
    return result;
  }

  std::optional<CampaignManagerDistributedGrpcClient::Pool::Ref>
  CampaignManagerDistributedGrpcClient::get_ref_(
    const std::string& service_index) const
  {
    if (!service_index.empty())
    {
      const auto it = service_index_pools_.find(service_index);
      if (it != service_index_pools_.end())
      {
        return it->second->get_object();
      }
    }

    return default_pool_->get_object();
  }

  template<typename Request, typename Response, typename Callback, typename Call>
  void
  CampaignManagerDistributedGrpcClient::call_(
    const Request& request,
    Callback callback,
    Call call,
    const char* unavailable_description,
    const std::string& service_index)
  {
    auto ref = get_ref_(service_index);
    if (!ref)
    {
      callback(unavailable_status(unavailable_description), Response());
      return;
    }

    auto pool_ref = std::move(*ref);
    auto client = pool_ref->client;

    call(
      client,
      request,
      [
        ref = std::move(pool_ref),
        callback = std::move(callback),
        pool_timeout = pool_timeout_
      ](
        const grpc::Status& status,
        const Response& response)
      mutable
      {
        if (!status.ok())
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, response);
      });
  }

  void
  CampaignManagerDistributedGrpcClient::ready(
    const pb::ReadyRequest& request,
    ReadyCallback callback)
  {
    call_<pb::ReadyRequest, pb::ReadyResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->ready(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::progress_comment(
    const pb::ProgressCommentRequest& request,
    ProgressCommentCallback callback)
  {
    call_<pb::ProgressCommentRequest, pb::ProgressCommentResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->progress_comment(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::match_geo_channels(
    const pb::MatchGeoChannelsRequest& request,
    MatchGeoChannelsCallback callback)
  {
    call_<pb::MatchGeoChannelsRequest, pb::MatchGeoChannelsResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->match_geo_channels(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_file(
    const pb::GetFileRequest& request,
    GetFileCallback callback)
  {
    auto ref = get_ref_(request.service_index());
    if (!ref)
    {
      callback(
        unavailable_status("no available CampaignManager grpc client"),
        pb::GetFileResponse());
      return;
    }

    auto fallback_ref = request.service_index().empty() ?
      std::optional<Pool::Ref>() :
      get_ref_();

    (*ref)->client->get_file(
      request,
      [
        ref = std::move(*ref),
        fallback_ref = std::move(fallback_ref),
        callback = std::move(callback),
        request,
        pool_timeout = pool_timeout_
      ](
        const grpc::Status& status,
        const pb::GetFileResponse& response)
      mutable
      {
        if (status.ok() || !fallback_ref)
        {
          if (!status.ok())
          {
            ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout);
          }
          callback(status, response);
          return;
        }

        ref.mark_as_bad(
          Generics::Time::get_time_of_day() + pool_timeout);

        pb::GetFileRequest fallback_request(request);
        fallback_request.clear_service_index();
        (*fallback_ref)->client->get_file(
          fallback_request,
          [
            fallback_ref = std::move(*fallback_ref),
            callback = std::move(callback),
            pool_timeout
          ](
            const grpc::Status& fallback_status,
            const pb::GetFileResponse& fallback_response)
          mutable
          {
            if (!fallback_status.ok())
            {
              fallback_ref.mark_as_bad(
                Generics::Time::get_time_of_day() + pool_timeout);
            }
            callback(fallback_status, fallback_response);
          });
      });
  }

  void
  CampaignManagerDistributedGrpcClient::get_campaign_creative(
    const pb::GetCampaignCreativeRequest& request,
    GetCampaignCreativeCallback callback)
  {
    call_<pb::GetCampaignCreativeRequest, pb::GetCampaignCreativeResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_campaign_creative(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::process_match_request(
    const pb::ProcessMatchRequestRequest& request,
    ProcessMatchRequestCallback callback)
  {
    call_<pb::ProcessMatchRequestRequest, pb::ProcessMatchRequestResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->process_match_request(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::process_anonymous_request(
    const pb::ProcessAnonymousRequestRequest& request,
    ProcessAnonymousRequestCallback callback)
  {
    call_<
      pb::ProcessAnonymousRequestRequest,
      pb::ProcessAnonymousRequestResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->process_anonymous_request(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::instantiate_ad(
    const pb::InstantiateAdRequest& request,
    InstantiateAdCallback callback)
  {
    call_<pb::InstantiateAdRequest, pb::InstantiateAdResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->instantiate_ad(request, std::move(callback));
      },
      "no available CampaignManager grpc client",
      request.service_index());
  }

  void
  CampaignManagerDistributedGrpcClient::trace_campaign_selection_index(
    const pb::TraceCampaignSelectionIndexRequest& request,
    TraceCampaignSelectionIndexCallback callback)
  {
    call_<
      pb::TraceCampaignSelectionIndexRequest,
      pb::TraceCampaignSelectionIndexResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->trace_campaign_selection_index(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::trace_campaign_selection(
    const pb::TraceCampaignSelectionRequest& request,
    TraceCampaignSelectionCallback callback)
  {
    call_<
      pb::TraceCampaignSelectionRequest,
      pb::TraceCampaignSelectionResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->trace_campaign_selection(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_campaign_creative_by_ccid(
    const pb::GetCampaignCreativeByCcidRequest& request,
    GetCampaignCreativeByCcidCallback callback)
  {
    call_<
      pb::GetCampaignCreativeByCcidRequest,
      pb::GetCampaignCreativeByCcidResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_campaign_creative_by_ccid(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_channel_links(
    const pb::GetChannelLinksRequest& request,
    GetChannelLinksCallback callback)
  {
    call_<pb::GetChannelLinksRequest, pb::GetChannelLinksResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_channel_links(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_discover_channels(
    const pb::GetDiscoverChannelsRequest& request,
    GetDiscoverChannelsCallback callback)
  {
    call_<pb::GetDiscoverChannelsRequest, pb::GetDiscoverChannelsResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_discover_channels(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_category_channels(
    const pb::GetCategoryChannelsRequest& request,
    GetCategoryChannelsCallback callback)
  {
    call_<pb::GetCategoryChannelsRequest, pb::GetCategoryChannelsResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_category_channels(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_colocation_flags(
    const pb::GetColocationFlagsRequest& request,
    GetColocationFlagsCallback callback)
  {
    call_<pb::GetColocationFlagsRequest, pb::GetColocationFlagsResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_colocation_flags(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_pub_pixels(
    const pb::GetPubPixelsRequest& request,
    GetPubPixelsCallback callback)
  {
    call_<pb::GetPubPixelsRequest, pb::GetPubPixelsResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_pub_pixels(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::consider_passback(
    const pb::ConsiderPassbackRequest& request,
    ConsiderPassbackCallback callback)
  {
    call_<pb::ConsiderPassbackRequest, pb::ConsiderPassbackResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->consider_passback(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::consider_passback_track(
    const pb::ConsiderPassbackTrackRequest& request,
    ConsiderPassbackTrackCallback callback)
  {
    call_<
      pb::ConsiderPassbackTrackRequest,
      pb::ConsiderPassbackTrackResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->consider_passback_track(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_click_url(
    const pb::GetClickUrlRequest& request,
    GetClickUrlCallback callback)
  {
    call_<pb::GetClickUrlRequest, pb::GetClickUrlResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_click_url(request, std::move(callback));
      },
      "no available CampaignManager grpc client",
      request.service_index());
  }

  void
  CampaignManagerDistributedGrpcClient::verify_impression(
    const pb::VerifyImpressionRequest& request,
    VerifyImpressionCallback callback)
  {
    call_<pb::VerifyImpressionRequest, pb::VerifyImpressionResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->verify_impression(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::action_taken(
    const pb::ActionTakenRequest& request,
    ActionTakenCallback callback)
  {
    call_<pb::ActionTakenRequest, pb::ActionTakenResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->action_taken(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::verify_opt_operation(
    const pb::VerifyOptOperationRequest& request,
    VerifyOptOperationCallback callback)
  {
    call_<pb::VerifyOptOperationRequest, pb::VerifyOptOperationResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->verify_opt_operation(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::consider_web_operation(
    const pb::ConsiderWebOperationRequest& request,
    ConsiderWebOperationCallback callback)
  {
    call_<pb::ConsiderWebOperationRequest, pb::ConsiderWebOperationResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->consider_web_operation(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::get_config(
    const pb::GetConfigRequest& request,
    GetConfigCallback callback)
  {
    call_<pb::GetConfigRequest, pb::GetConfigResponse>(
      request,
      std::move(callback),
      [](const ClientPtr& client, const auto& request, auto callback) {
        client->get_config(request, std::move(callback));
      },
      "no available CampaignManager grpc client");
  }

  void
  CampaignManagerDistributedGrpcClient::merge_stats_(
    AdServer::Grpc::Stats& result,
    const AdServer::Grpc::Stats& source) noexcept
  {
    result.write_batches += source.write_batches;
    result.write_items += source.write_items;
    result.queue_wait_count += source.queue_wait_count;
    result.queue_wait_sum_us += source.queue_wait_sum_us;
    result.queue_wait_max_us =
      std::max(result.queue_wait_max_us, source.queue_wait_max_us);
    result.response_wait_count += source.response_wait_count;
    result.response_wait_sum_us += source.response_wait_sum_us;
    result.response_wait_max_us =
      std::max(result.response_wait_max_us, source.response_wait_max_us);
    result.max_streams = std::max(result.max_streams, source.max_streams);
    if (source.consumer_stream_write.has_value())
    {
      if (!result.consumer_stream_write.has_value())
      {
        result.consumer_stream_write =
          AdServer::Grpc::Stats::ConsumerStreamWrite();
      }
      result.consumer_stream_write->count += source.consumer_stream_write->count;
      result.consumer_stream_write->sum_us += source.consumer_stream_write->sum_us;
      result.consumer_stream_write->max_us = std::max(
        result.consumer_stream_write->max_us,
        source.consumer_stream_write->max_us);
    }
  }
}
