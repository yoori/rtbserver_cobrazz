#include "CampaignManagerDistributedGrpcClient.hpp"

#include <algorithm>
#include <set>
#include <sstream>
#include <utility>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    namespace pb = adserver::campaign_svcs::campaign_manager;

    const Generics::Time DEFAULT_POOL_TIMEOUT = Generics::Time::ONE_SECOND;
    const char NO_AVAILABLE_DESCRIPTION[] =
      "no available CampaignManager grpc client";

    grpc::Status unavailable_status(const char* description)
    {
      return grpc::Status(
        grpc::StatusCode::UNAVAILABLE,
        description ? description : "");
    }

    grpc::Status unavailable_status(const std::string& description)
    {
      return grpc::Status(
        grpc::StatusCode::UNAVAILABLE,
        description);
    }

    std::string status_description(const grpc::Status& status)
    {
      std::ostringstream ostr;
      ostr << "code=" << static_cast<int>(status.error_code()) <<
        ", message=" << status.error_message();
      if (!status.error_details().empty())
      {
        ostr << ", details=" << status.error_details();
      }
      return ostr.str();
    }

    bool should_mark_as_bad(const grpc::Status& status)
    {
      return !AdServer::Grpc::is_transport_timeout(status);
    }

    class NullActiveObjectCallback final:
      public virtual Generics::ActiveObjectCallback,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      void
      report_error(
        Severity,
        const String::SubString&,
        const char* = nullptr) throw () override
      {}

    protected:
      ~NullActiveObjectCallback() throw () override = default;
    };
  }

  struct CampaignManagerDistributedGrpcClient::ClientHolder
  {
    ClientHolder(
      std::string endpoint_val,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner)
      : endpoint(std::move(endpoint_val)),
        name(endpoint),
        client(std::make_shared<Client>(
          endpoint,
          std::move(grpc_executor),
          std::move(coalesce_runner),
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
    const std::string name;
    ClientPtr client;
  };

  CampaignManagerDistributedGrpcClient::CampaignManagerDistributedGrpcClient(
    const CampaignManagerRefs& campaign_manager_refs,
    const AdServer::Grpc::BatchingOptions& batching_options,
    const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner)
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

    if (!coalesce_runner)
    {
      coalesce_runner =
        std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
          Generics::ActiveObjectCallback_var(new NullActiveObjectCallback()),
          std::make_shared<boost::asio::io_service>(),
          std::max<unsigned long>(1, batching_options.workers_number));
      add_child_object(coalesce_runner);
    }

    std::vector<ClientHolderPtr> default_refs;
    default_refs.reserve(campaign_manager_refs.size());

    for (const auto& ref : campaign_manager_refs)
    {
      auto client_holder = std::make_shared<ClientHolder>(
        ref.object_ref,
        batching_options,
        effective_grpc_executor,
        coalesce_runner);

      default_refs.emplace_back(client_holder);
      if (!ref.service_index.empty())
      {
        auto pool = std::make_shared<Pool>(
          std::vector<ClientHolderPtr>{client_holder},
          coalesce_runner);
        service_index_pools_.emplace(ref.service_index, std::move(pool));
      }
      client_holders_.emplace_back(std::move(client_holder));
    }

    default_pool_ = std::make_shared<Pool>(
      std::move(default_refs),
      coalesce_runner);
  }

  CampaignManagerDistributedGrpcClient::CampaignManagerDistributedGrpcClient(
    const CampaignManagerObjectRefs& campaign_manager_refs,
    const AdServer::Grpc::BatchingOptions& batching_options,
    const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner)
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
        grpc_executor,
        std::move(coalesce_runner))
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

    for (const auto& client_holder : client_holders_)
    {
      client_holder->client->deactivate_object();
    }

    for (auto& service_index_pool : service_index_pools_)
    {
      service_index_pool.second->wait_object();
    }
    default_pool_->wait_object();

    for (const auto& client_holder : client_holders_)
    {
      client_holder->client->wait_object();
    }
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

  CampaignManagerDistributedGrpcClient::PoolPtr
  CampaignManagerDistributedGrpcClient::get_pool_(
    const std::string& service_index) const
  {
    if (!service_index.empty())
    {
      const auto it = service_index_pools_.find(service_index);
      if (it != service_index_pools_.end())
      {
        return it->second;
      }
    }

    return default_pool_;
  }

  std::optional<CampaignManagerDistributedGrpcClient::Pool::Ref>
  CampaignManagerDistributedGrpcClient::get_ref_(
    const std::string& service_index) const
  {
    auto pool = get_pool_(service_index);
    return pool ? pool->get_object() : std::nullopt;
  }

  std::string
  CampaignManagerDistributedGrpcClient::unavailable_description_(
    const PoolPtr& pool)
  {
    if (!pool)
    {
      return NO_AVAILABLE_DESCRIPTION;
    }

    const auto details = pool->unavailable_description();
    if (details.empty())
    {
      return NO_AVAILABLE_DESCRIPTION;
    }

    return std::string(NO_AVAILABLE_DESCRIPTION) + ": " + details;
  }

  template<typename Request, typename Response, typename Callback, typename Call>
  void
  CampaignManagerDistributedGrpcClient::call_(
    const Request& request,
    Callback callback,
    Call call,
    const std::string& service_index)
  {
    if (!active())
    {
      callback(unavailable_status("inactive"), Response());
      return;
    }

    auto pool = get_pool_(service_index);
    auto ref = pool ? pool->get_object() : std::nullopt;
    if (!ref)
    {
      callback(
        unavailable_status(unavailable_description_(pool)),
        Response());
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
          if (should_mark_as_bad(status))
          {
            ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout,
              status_description(status));
          }
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
      });
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
      });
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
      });
  }

  void
  CampaignManagerDistributedGrpcClient::get_file(
    const pb::GetFileRequest& request,
    GetFileCallback callback)
  {
    auto pool = get_pool_(request.service_index());
    auto ref = pool ? pool->get_object() : std::nullopt;
    if (!ref)
    {
      callback(
        unavailable_status(unavailable_description_(pool)),
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
            if (should_mark_as_bad(status))
            {
              ref.mark_as_bad(
                Generics::Time::get_time_of_day() + pool_timeout,
                status_description(status));
            }
          }
          callback(status, response);
          return;
        }

        if (should_mark_as_bad(status))
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout,
            status_description(status));
        }

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
              if (should_mark_as_bad(fallback_status))
              {
                fallback_ref.mark_as_bad(
                  Generics::Time::get_time_of_day() + pool_timeout,
                  status_description(fallback_status));
              }
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
      });
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
