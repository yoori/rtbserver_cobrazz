#ifndef FRONTENDCOMMONS_GRPCCAMPAIGNMANAGERPOOL_HPP
#define FRONTENDCOMMONS_GRPCCAMPAIGNMANAGERPOOL_HPP

// STD
#include <chrono>
#include <shared_mutex>
#include <unordered_map>

// BOOST
#include <boost/functional/hash.hpp>

// PROTO
#include "CampaignSvcs/CampaignManager/proto/CampaignManager_client.cobrazz.pb.hpp"

// UNIXCOMMONS
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/PoolClientFactory.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// USERVER
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <Commons/GrpcAlgs.hpp>

namespace FrontendCommons
{

inline constexpr char ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL[] = "GRPC_CAMPAIGN_MANAGERS_POOL";

class GrpcCampaignManagerPool final : Generics::Uncopyable
{
private:
  class ClientHolder;
  class FactoryClientHolder;

  using ClientHolderPtr = std::shared_ptr<ClientHolder>;
  using ClientHolders = std::vector<ClientHolderPtr>;
  using FactoryClientHolderPtr = std::unique_ptr<FactoryClientHolder>;

public:
  using Host = std::string;
  using Port = std::size_t;
  using Endpoint = std::pair<Host, Port>;
  using Endpoints = std::vector<Endpoint>;
  using ConfigPoolClient = UServerUtils::Grpc::Client::ConfigPoolCoro;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Time = Generics::Time;
  using TaskProcessor = userver::engine::TaskProcessor;
  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;

  using GetCampaignCreativeRequest = AdServer::CampaignSvcs::Proto::GetCampaignCreativeRequest;
  using GetCampaignCreativeRequestPtr = std::unique_ptr<GetCampaignCreativeRequest>;
  using GetCampaignCreativeResponse = AdServer::CampaignSvcs::Proto::GetCampaignCreativeResponse;
  using GetCampaignCreativeResponsePtr = std::unique_ptr<GetCampaignCreativeResponse>;
  using ProcessMatchRequestRequest = AdServer::CampaignSvcs::Proto::ProcessMatchRequestRequest;
  using ProcessMatchRequestRequestPtr = std::unique_ptr<ProcessMatchRequestRequest>;
  using ProcessMatchRequestResponse = AdServer::CampaignSvcs::Proto::ProcessMatchRequestResponse;
  using ProcessMatchRequestResponsePtr = std::unique_ptr<ProcessMatchRequestResponse>;
  using MatchGeoChannelsRequest = AdServer::CampaignSvcs::Proto::MatchGeoChannelsRequest;
  using MatchGeoChannelsRequestPtr = std::unique_ptr<MatchGeoChannelsRequest>;
  using MatchGeoChannelsResponse = AdServer::CampaignSvcs::Proto::MatchGeoChannelsResponse;
  using MatchGeoChannelsResponsePtr = std::unique_ptr<MatchGeoChannelsResponse>;
  using InstantiateAdRequest = AdServer::CampaignSvcs::Proto::InstantiateAdRequest;
  using InstantiateAdRequestPtr = std::unique_ptr<InstantiateAdRequest>;
  using InstantiateAdResponse = AdServer::CampaignSvcs::Proto::InstantiateAdResponse;
  using InstantiateAdResponsePtr = std::unique_ptr<InstantiateAdResponse>;
  using GetChannelLinksRequest = AdServer::CampaignSvcs::Proto::GetChannelLinksRequest;
  using GetChannelLinksRequestPtr = std::unique_ptr<GetChannelLinksRequest>;
  using GetChannelLinksResponse = AdServer::CampaignSvcs::Proto::GetChannelLinksResponse;
  using GetChannelLinksResponsePtr = std::unique_ptr<GetChannelLinksResponse>;
  using GetDiscoverChannelsRequest = AdServer::CampaignSvcs::Proto::GetDiscoverChannelsRequest;
  using GetDiscoverChannelsRequestPtr = std::unique_ptr<GetDiscoverChannelsRequest>;
  using GetDiscoverChannelsResponse = AdServer::CampaignSvcs::Proto::GetDiscoverChannelsResponse;
  using GetDiscoverChannelsResponsePtr = std::unique_ptr<GetDiscoverChannelsResponse>;
  using GetCategoryChannelsRequest = AdServer::CampaignSvcs::Proto::GetCategoryChannelsRequest;
  using GetCategoryChannelsRequestPtr = std::unique_ptr<GetCategoryChannelsRequest>;
  using GetCategoryChannelsResponse = AdServer::CampaignSvcs::Proto::GetCategoryChannelsResponse;
  using GetCategoryChannelsResponsePtr = std::unique_ptr<GetCategoryChannelsResponse>;
  using ConsiderPassbackRequest = AdServer::CampaignSvcs::Proto::ConsiderPassbackRequest;
  using ConsiderPassbackRequestPtr = std::unique_ptr<ConsiderPassbackRequest>;
  using ConsiderPassbackResponse = AdServer::CampaignSvcs::Proto::ConsiderPassbackResponse;
  using ConsiderPassbackResponsePtr = std::unique_ptr<ConsiderPassbackResponse>;
  using ConsiderPassbackTrackRequest = AdServer::CampaignSvcs::Proto::ConsiderPassbackTrackRequest;
  using ConsiderPassbackTrackRequestPtr = std::unique_ptr<ConsiderPassbackTrackRequest>;
  using ConsiderPassbackTrackResponse = AdServer::CampaignSvcs::Proto::ConsiderPassbackTrackResponse;
  using ConsiderPassbackTrackResponsePtr = std::unique_ptr<ConsiderPassbackTrackResponse>;
  using GetClickUrlRequest = AdServer::CampaignSvcs::Proto::GetClickUrlRequest;
  using GetClickUrlRequestPtr = std::unique_ptr<GetClickUrlRequest>;
  using GetClickUrlResponse = AdServer::CampaignSvcs::Proto::GetClickUrlResponse;
  using GetClickUrlResponsePtr = std::unique_ptr<GetClickUrlResponse>;
  using VerifyImpressionRequest = AdServer::CampaignSvcs::Proto::VerifyImpressionRequest;
  using VerifyImpressionRequestPtr = std::unique_ptr<VerifyImpressionRequest>;
  using VerifyImpressionResponse = AdServer::CampaignSvcs::Proto::VerifyImpressionResponse;
  using VerifyImpressionResponsePtr = std::unique_ptr<VerifyImpressionResponse>;
  using ActionTakenRequest = AdServer::CampaignSvcs::Proto::ActionTakenRequest;
  using ActionTakenRequestPtr = std::unique_ptr<ActionTakenRequest>;
  using ActionTakenResponse = AdServer::CampaignSvcs::Proto::ActionTakenResponse;
  using ActionTakenResponsePtr = std::unique_ptr<ActionTakenResponse>;
  using VerifyOptOperationRequest = AdServer::CampaignSvcs::Proto::VerifyOptOperationRequest;
  using VerifyOptOperationRequestPtr = std::unique_ptr<VerifyOptOperationRequest>;
  using VerifyOptOperationResponse = AdServer::CampaignSvcs::Proto::VerifyOptOperationResponse;
  using VerifyOptOperationResponsePtr = std::unique_ptr<VerifyOptOperationResponse>;
  using ConsiderWebOperationRequest = AdServer::CampaignSvcs::Proto::ConsiderWebOperationRequest;
  using ConsiderWebOperationRequestPtr = std::unique_ptr<ConsiderWebOperationRequest>;
  using ConsiderWebOperationResponse = AdServer::CampaignSvcs::Proto::ConsiderWebOperationResponse;
  using ConsiderWebOperationResponsePtr = std::unique_ptr<ConsiderWebOperationResponse>;
  using GetConfigRequest = AdServer::CampaignSvcs::Proto::GetConfigRequest;
  using GetConfigRequestPtr = std::unique_ptr<GetConfigRequest>;
  using GetConfigResponse = AdServer::CampaignSvcs::Proto::GetConfigResponse;
  using GetConfigResponsePtr = std::unique_ptr<GetConfigResponse>;
  using TraceCampaignSelectionIndexRequest = AdServer::CampaignSvcs::Proto::TraceCampaignSelectionIndexRequest;
  using TraceCampaignSelectionIndexRequestPtr = std::unique_ptr<TraceCampaignSelectionIndexRequest>;
  using TraceCampaignSelectionIndexResponse = AdServer::CampaignSvcs::Proto::TraceCampaignSelectionIndexResponse;
  using TraceCampaignSelectionIndexResponsePtr = std::unique_ptr<TraceCampaignSelectionIndexResponse>;
  using GetCampaignCreativeByCcidRequest = AdServer::CampaignSvcs::Proto::GetCampaignCreativeByCcidRequest;
  using GetCampaignCreativeByCcidRequestPtr = std::unique_ptr<GetCampaignCreativeByCcidRequest>;
  using GetCampaignCreativeByCcidResponse = AdServer::CampaignSvcs::Proto::GetCampaignCreativeByCcidResponse;
  using GetCampaignCreativeByCcidResponsePtr = std::unique_ptr<GetCampaignCreativeByCcidResponse>;
  using GetColocationFlagsRequest = AdServer::CampaignSvcs::Proto::GetColocationFlagsRequest;
  using GetColocationFlagsRequestPtr = std::unique_ptr<GetColocationFlagsRequest>;
  using GetColocationFlagsResponse = AdServer::CampaignSvcs::Proto::GetColocationFlagsResponse;
  using GetColocationFlagsResponsePtr = std::unique_ptr<GetColocationFlagsResponse>;
  using GetPubPixelsRequest = AdServer::CampaignSvcs::Proto::GetPubPixelsRequest;
  using GetPubPixelsRequestPtr = std::unique_ptr<GetPubPixelsRequest>;
  using GetPubPixelsResponse = AdServer::CampaignSvcs::Proto::GetPubPixelsResponse;
  using GetPubPixelsResponsePtr = std::unique_ptr<GetPubPixelsResponse>;
  using ProcessAnonymousRequestRequest = AdServer::CampaignSvcs::Proto::ProcessAnonymousRequestRequest;
  using ProcessAnonymousRequestRequestPtr = std::unique_ptr<ProcessAnonymousRequestRequest>;
  using ProcessAnonymousRequestResponse = AdServer::CampaignSvcs::Proto::ProcessAnonymousRequestResponse;
  using ProcessAnonymousRequestResponsePtr = std::unique_ptr<ProcessAnonymousRequestResponse>;
  using GetFileRequest = AdServer::CampaignSvcs::Proto::GetFileRequest;
  using GetFileRequestPtr = std::unique_ptr<GetFileRequest>;
  using GetFileResponse = AdServer::CampaignSvcs::Proto::GetFileResponse;
  using GetFileResponsePtr = std::unique_ptr<GetFileResponse>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit GrpcCampaignManagerPool(
    Logger* logger,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const Endpoints& endpoints,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout_ms = 1000,
    const std::size_t time_duration_client_bad_ms = 30000);

  ~GrpcCampaignManagerPool() = default;

  GetPubPixelsResponsePtr get_pub_pixels(
    const std::string& country,
    const std::uint32_t user_status,
    const std::vector<std::uint32_t>& publisher_account_ids);

private:
  GetPubPixelsRequestPtr create_get_pub_pixels_request(
    const std::string& country,
    const std::uint32_t user_status,
    const std::vector<std::uint32_t>& publisher_account_ids);

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(Args&& ...args) noexcept
  {
    if (client_holders_.empty())
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << " client_holders is empty";
        logger_->error(
          stream.str(),
          ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
      }
      catch (...)
      {
      }
      return {};
    }

    const std::size_t size = client_holders_.size();
    for (std::size_t i = 0; i < size; ++i)
    {
      try
      {
        const std::size_t number = counter_.fetch_add(
          1,
          std::memory_order_relaxed);
        const auto& client_holder = client_holders_[number % size];
        if (client_holder->is_bad())
        {
          continue;
        }

        std::unique_ptr<Request> request;
        if constexpr(std::is_same_v<Request, GetPubPixelsRequest>)
        {
          request = create_get_pub_pixels_request(std::forward<Args>(args)...);
        }
        else
        {
          static_assert(GrpcAlgs::AlwaysFalseV<Request>);
        }

        auto response = client_holder->template do_request<Client, Request, Response>(
          std::move(request),
          grpc_client_timeout_ms_);
        if (!response)
        {
          Stream::Error stream;
          stream << FNS
                 << "Internal grpc error";
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);

          continue;
        }

        const auto data_case = response->data_case();
        if (data_case == Response::DataCase::kInfo)
        {
          return response;
        }
        else if (data_case == Response::DataCase::kError)
        {
          std::ostringstream stream;
          stream << FNS
                 << "Error type=";

          const auto& error = response->error();
          const auto error_type = error.type();
          switch (error_type)
          {
            case AdServer::CampaignSvcs::Proto::Error_Type::Error_Type_Implementation:
            {
              stream << "Implementation";
            }
            case AdServer::CampaignSvcs::Proto::Error_Type::Error_Type_NotReady:
            {
              stream << "NotReady";
            }
            case AdServer::CampaignSvcs::Proto::Error_Type::Error_Type_IncorrectArgument:
            {
              stream << "IncorrectArgument";
            }
            default:
            {
              Stream::Error stream;
              stream << FNS
                     << "Unknown error type";
              throw  Exception(stream);
            }
          }

          stream << ", description="
                 << error.description();
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
          return response;
        }
        else
        {
          Stream::Error stream;
          stream << FNS
                 << "Unknown response type";
          throw  Exception(stream);
        }
      }
      catch (const eh::Exception& exc)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << exc.what();
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
        }
        catch (...)
        {
        }
      }
      catch (...)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << "Unknown error";
          logger_->error(
            stream.str(),
            ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
        }
        catch (...)
        {
        }
      }
    }

    try
    {
      Stream::Error stream;
      stream << FNS
             << "max tries is reached";
      logger_->error(
        stream.str(),
        ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
    }
    catch (...)
    {
    }

    return {};
  }

private:
  const Logger_var logger_;

  TaskProcessor& task_processor_;

  const SchedulerPtr scheduler_;

  const FactoryClientHolderPtr factory_client_holder_;

  const std::size_t grpc_client_timeout_ms_;

  ClientHolders client_holders_;

  std::atomic<std::size_t> counter_{0};
};

class GrpcCampaignManagerPool::ClientHolder final : private Generics::Uncopyable
{
public:
  using GetCampaignCreativeClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPool;
  using GetCampaignCreativeClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPoolPtr;
  using ProcessMatchRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPool;
  using ProcessMatchRequestClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPoolPtr;
  using MatchGeoChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_ClientPool;
  using MatchGeoChannelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_ClientPoolPtr;
  using InstantiateAdClient = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPool;
  using InstantiateAdClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPoolPtr;
  using GetChannelLinksClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_ClientPool;
  using GetChannelLinksClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_ClientPoolPtr;
  using GetDiscoverChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_ClientPool;
  using GetDiscoverChannelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_ClientPoolPtr;
  using GetCategoryChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_ClientPool;
  using GetCategoryChannelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_ClientPoolPtr;
  using ConsiderPassbackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPool;
  using ConsiderPassbackClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPoolPtr;
  using ConsiderPassbackTrackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPool;
  using ConsiderPassbackTrackClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPoolPtr;
  using GetClickUrlClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPool;
  using GetClickUrlClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPoolPtr;
  using VerifyImpressionClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_ClientPool;
  using VerifyImpressionClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_ClientPoolPtr;
  using ActionTakenClient = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPool;
  using ActionTakenClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPoolPtr;
  using VerifyOptOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPool;
  using VerifyOptOperationClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPoolPtr;
  using ConsiderWebOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPool;
  using ConsiderWebOperationClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPoolPtr;
  using GetConfigClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_config_ClientPool;
  using GetConfigClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_config_ClientPoolPtr;
  using TraceCampaignSelectionIndexClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_ClientPool;
  using TraceCampaignSelectionIndexClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_ClientPoolPtr;
  using TraceCampaignSelectionClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_ClientPool;
  using TraceCampaignSelectionClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_ClientPoolPtr;
  using GetCampaignCreativeByCcidClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_ClientPool;
  using GetCampaignCreativeByCcidClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_ClientPoolPtr;
  using GetColocationFlagsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPool;
  using GetColocationFlagsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPoolPtr;
  using GetPubPixelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPool;
  using GetPubPixelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPoolPtr;
  using ProcessAnonymousRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_ClientPool;
  using ProcessAnonymousRequestClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_ClientPoolPtr;
  using GetFileClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_file_ClientPool;
  using GetFileClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_file_ClientPoolPtr;

  struct Clients final
  {
    GetCampaignCreativeClientPtr get_campaign_creative_client;
    ProcessMatchRequestClientPtr process_match_request_client;
    MatchGeoChannelsClientPtr match_geo_channels_client;
    InstantiateAdClientPtr instantiate_ad_client;
    GetChannelLinksClientPtr get_channel_links_client;
    GetDiscoverChannelsClientPtr get_discover_channels_client;
    GetCategoryChannelsClientPtr get_category_channels_client;
    ConsiderPassbackClientPtr consider_passback_client;
    ConsiderPassbackTrackClientPtr consider_passback_track_client;
    GetClickUrlClientPtr get_click_url_client;
    VerifyImpressionClientPtr verify_impression_client;
    ActionTakenClientPtr action_taken_client;
    VerifyOptOperationClientPtr verify_opt_operation_client;
    ConsiderWebOperationClientPtr consider_web_operation_client;
    GetConfigClientPtr get_config_client;
    TraceCampaignSelectionIndexClientPtr trace_campaign_selection_index_client;
    TraceCampaignSelectionClientPtr trace_campaign_selection_client;
    GetCampaignCreativeByCcidClientPtr get_campaign_creative_by_ccid_client;
    GetColocationFlagsClientPtr get_colocation_flags_client;
    GetPubPixelsClientPtr get_pub_pixels_client;
    ProcessAnonymousRequestClientPtr process_anonymous_request_client;
    GetFileClientPtr get_file_client;
  };
  using ClientsPtr = std::shared_ptr<Clients>;

private:
  using Status = UServerUtils::Grpc::Client::Status;
  using Mutex = std::shared_mutex;

public:
  explicit ClientHolder(
    const ClientsPtr& clients,
    const Generics::Time& time_duration_client_bad)
    : clients_(clients),
      time_duration_client_bad_(time_duration_client_bad)
  {
  }

  ~ClientHolder() = default;

  bool is_bad() const
  {
    {
      std::shared_lock lock(mutex_);
      if (!marked_as_bad_)
        return false;
    }

    const Generics::Time now = Generics::Time::get_time_of_day();
    {
      std::shared_lock lock(mutex_);
      if (marked_as_bad_ && now < marked_as_bad_time_ + time_duration_client_bad_)
      {
        return true;
      }
    }

    std::unique_lock lock(mutex_);
    if (marked_as_bad_ && now >= marked_as_bad_time_ + time_duration_client_bad_)
    {
      marked_as_bad_ = false;
    }

    return marked_as_bad_;
  }

  template<class Client, class Request, class Response>
  std::unique_ptr<Response> do_request(
    std::unique_ptr<Request>&& request,
    const std::size_t timeout) noexcept
  {
    using ClientPtr = std::shared_ptr<Client>;

    ClientPtr client;
    if constexpr (std::is_same_v<Client, GetPubPixelsClient>)
    {
      client = clients_->get_pub_pixels_client;
    }
    else if constexpr (std::is_same_v<Client, GetCampaignCreativeClient>)
    {
      client = clients_->get_campaign_creative_client;
    }
    else if constexpr (std::is_same_v<Client, ProcessMatchRequestClient>)
    {
      client = clients_->process_match_request_client;
    }
    else if constexpr (std::is_same_v<Client, MatchGeoChannelsClient>)
    {
      client = clients_->match_geo_channels_client;
    }
    else if constexpr (std::is_same_v<Client, InstantiateAdClient>)
    {
      client = clients_->instantiate_ad_client;
    }
    else if constexpr (std::is_same_v<Client, GetChannelLinksClient>)
    {
      client = clients_->get_channel_links_client;
    }
    else if constexpr (std::is_same_v<Client, GetDiscoverChannelsClient>)
    {
      client = clients_->get_discover_channels_client;
    }
    else if constexpr (std::is_same_v<Client, GetCategoryChannelsClient>)
    {
      client = clients_->get_category_channels_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderPassbackClient>)
    {
      client = clients_->consider_passback_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderPassbackTrackClient>)
    {
      client = clients_->consider_passback_track_client;
    }
    else if constexpr (std::is_same_v<Client, GetClickUrlClient>)
    {
      client = clients_->get_click_url_client;
    }
    else if constexpr (std::is_same_v<Client, VerifyImpressionClient>)
    {
      client = clients_->verify_impression_client;
    }
    else if constexpr (std::is_same_v<Client, ActionTakenClient>)
    {
      client = clients_->action_taken_client;
    }
    else if constexpr (std::is_same_v<Client, VerifyOptOperationClient>)
    {
      client = clients_->verify_opt_operation_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderWebOperationClient>)
    {
      client = clients_->consider_web_operation_client;
    }
    else if constexpr (std::is_same_v<Client, GetConfigClient>)
    {
      client = clients_->get_config_client;
    }
    else if constexpr (std::is_same_v<Client, TraceCampaignSelectionIndexClient>)
    {
      client = clients_->trace_campaign_selection_index_client;
    }
    else if constexpr (std::is_same_v<Client, TraceCampaignSelectionClient>)
    {
      client = clients_->trace_campaign_selection_client;
    }
    else if constexpr (std::is_same_v<Client, GetCampaignCreativeByCcidClient>)
    {
      client = clients_->get_campaign_creative_by_ccid_client;
    }
    else if constexpr (std::is_same_v<Client, GetColocationFlagsClient>)
    {
      client = clients_->get_colocation_flags_client;
    }
    else if constexpr (std::is_same_v<Client, ProcessAnonymousRequestClient>)
    {
      client = clients_->process_anonymous_request_client;
    }
    else if constexpr (std::is_same_v<Client, GetFileClient>)
    {
      client = clients_->get_file_client;
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Client>);
    }

    for (std::size_t i = 1; i <= 3; ++i)
    {
      auto result = client->write(std::move(request), timeout);
      if (result.status == Status::Ok)
      {
        return std::move(result.response);
      }
    }

    set_bad();
    return {};
  }

private:
  void set_bad() noexcept
  {
    try
    {
      const Generics::Time now = Generics::Time::get_time_of_day();

      std::unique_lock lock(mutex_);
      marked_as_bad_ = true;
      marked_as_bad_time_ = now;
    }
    catch (...)
    {
    }
  }

private:
  const ClientsPtr clients_;

  const Generics::Time time_duration_client_bad_;

  mutable Mutex mutex_;

  mutable bool marked_as_bad_ = false;

  Generics::Time marked_as_bad_time_;
};

class GrpcCampaignManagerPool::FactoryClientHolder final : private Generics::Uncopyable
{
public:
  using GetCampaignCreativeClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPool;
  using ProcessMatchRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPool;
  using MatchGeoChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_ClientPool;
  using InstantiateAdClient = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPool;
  using GetChannelLinksClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_ClientPool;
  using GetDiscoverChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_ClientPool;
  using GetCategoryChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_ClientPool;
  using ConsiderPassbackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPool;
  using ConsiderPassbackTrackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPool;
  using GetClickUrlClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPool;
  using VerifyImpressionClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_ClientPool;
  using ActionTakenClient = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPool;
  using VerifyOptOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPool;
  using ConsiderWebOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPool;
  using GetConfigClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_config_ClientPool;
  using TraceCampaignSelectionIndexClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_ClientPool;
  using TraceCampaignSelectionClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_ClientPool;
  using GetCampaignCreativeByCcidClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_ClientPool;
  using GetColocationFlagsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPool;
  using GetPubPixelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPool;
  using ProcessAnonymousRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_ClientPool;
  using GetFileClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_file_ClientPool;
  using GrpcPoolClientFactory = UServerUtils::Grpc::Client::PoolClientFactory;

private:
  using Mutex = std::shared_mutex;
  using Key = std::pair<Host, Port>;
  using Value = ClientHolder::ClientsPtr;
  using Cache = std::unordered_map<Key, Value, boost::hash<Key>>;

public:
  FactoryClientHolder(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolClient& config,
    TaskProcessor& task_processor,
    const Generics::Time& time_duration_client_bad)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      config_(config),
      task_processor_(task_processor),
      time_duration_client_bad_(time_duration_client_bad)
  {
  }

  ClientHolderPtr create(const Host& host, const Port port)
  {
    Key key(host, port);
    {
      std::shared_lock lock(mutex_);
      auto it = cache_.find(key);
      if (it != std::end(cache_))
      {
        auto clients = it->second;
        lock.unlock();
        return std::make_shared<ClientHolder>(
          clients,
          time_duration_client_bad_);
      }
    }

    auto clients = std::make_shared<ClientHolder::Clients>();
    auto config = config_;

    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it == std::end(cache_))
    {
      std::stringstream endpoint_stream;
      endpoint_stream << host
                      << ":"
                      << port;
      std::string endpoint = endpoint_stream.str();
      config.endpoint = endpoint;

      GrpcPoolClientFactory factory(
        logger_.in(),
        scheduler_,
        config);

      clients->get_campaign_creative_client = factory.create<GetCampaignCreativeClient>(task_processor_);
      clients->process_match_request_client = factory.create<ProcessMatchRequestClient>(task_processor_);
      clients->match_geo_channels_client = factory.create<MatchGeoChannelsClient>(task_processor_);
      clients->instantiate_ad_client = factory.create<InstantiateAdClient>(task_processor_);
      clients->get_channel_links_client = factory.create<GetChannelLinksClient>(task_processor_);
      clients->get_discover_channels_client = factory.create<GetDiscoverChannelsClient>(task_processor_);
      clients->get_category_channels_client = factory.create<GetCategoryChannelsClient>(task_processor_);
      clients->consider_passback_client = factory.create<ConsiderPassbackClient>(task_processor_);
      clients->consider_passback_track_client = factory.create<ConsiderPassbackTrackClient>(task_processor_);
      clients->get_click_url_client = factory.create<GetClickUrlClient>(task_processor_);
      clients->verify_impression_client = factory.create<VerifyImpressionClient>(task_processor_);
      clients->action_taken_client = factory.create<ActionTakenClient>(task_processor_);
      clients->verify_opt_operation_client = factory.create<VerifyOptOperationClient>(task_processor_);
      clients->consider_web_operation_client = factory.create<ConsiderWebOperationClient>(task_processor_);
      clients->get_config_client = factory.create<GetConfigClient>(task_processor_);
      clients->trace_campaign_selection_index_client = factory.create<TraceCampaignSelectionIndexClient>(task_processor_);
      clients->get_campaign_creative_by_ccid_client = factory.create<GetCampaignCreativeByCcidClient>(task_processor_);
      clients->get_colocation_flags_client = factory.create<GetColocationFlagsClient>(task_processor_);
      clients->get_pub_pixels_client = factory.create<GetPubPixelsClient>(task_processor_);
      clients->process_anonymous_request_client = factory.create<ProcessAnonymousRequestClient>(task_processor_);
      clients->get_file_client = factory.create<GetFileClient>(task_processor_);

      it = cache_.try_emplace(
        key,
        std::move(clients)).first;
    }

    clients = it->second;
    lock.unlock();

    return std::make_shared<ClientHolder>(
      clients,
      time_duration_client_bad_);
  }

private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const ConfigPoolClient config_;

  TaskProcessor& task_processor_;

  const Generics::Time time_duration_client_bad_;

  Mutex mutex_;

  Cache cache_;
};

using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;

} // namespace FrontendCommons

#endif // FRONTENDCOMMONS_GRPCCAMPAIGNMANAGERPOOL_HPP