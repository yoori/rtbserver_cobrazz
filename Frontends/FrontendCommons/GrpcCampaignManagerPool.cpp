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
  const std::size_t time_duration_client_bad_sec)
  : logger_(ReferenceCounting::add_ref(logger)),
    task_processor_(task_processor),
    scheduler_(scheduler),
    factory_client_holder_(new FactoryClientHolder(
      logger_.in(),
      scheduler_,
      config_pool_client,
      task_processor,
      Time{static_cast<time_t>(time_duration_client_bad_sec)})),
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

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcCampaignManagerPool::do_request(Args&& ...args) noexcept
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
      if constexpr (std::is_same_v<Request, GetPubPixelsRequest>)
      {
        request = create_get_pub_pixels_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, ConsiderWebOperationRequest>)
      {
        request = create_consider_web_operation_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, ConsiderPassbackTrackRequest>)
      {
        request = create_consider_passback_track_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, ConsiderPassbackRequest>)
      {
        request = create_consider_passback_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, ActionTakenRequest>)
      {
        request = create_action_taken_request(std::forward<Args>(args)...);
      }
      else if constexpr (std::is_same_v<Request, ProcessMatchRequestRequest>)
      {
        request = create_process_match_request_request(std::forward<Args>(args)...);
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
            break;
          }
          case AdServer::CampaignSvcs::Proto::Error_Type::Error_Type_NotReady:
          {
            stream << "NotReady";
            break;
          }
          case AdServer::CampaignSvcs::Proto::Error_Type::Error_Type_IncorrectArgument:
          {
            stream << "IncorrectArgument";
            break;
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

GrpcCampaignManagerPool::GetPubPixelsResponsePtr
GrpcCampaignManagerPool::get_pub_pixels(
  const std::string& country,
  const std::uint32_t user_status,
  const std::vector<std::uint32_t>& publisher_account_ids) noexcept
{
  using GetPubPixelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPool;

  return do_request<
    GetPubPixelsClient,
    GetPubPixelsRequest,
    GetPubPixelsResponse>(country, user_status, publisher_account_ids);
}

GrpcCampaignManagerPool::ConsiderWebOperationResponsePtr
GrpcCampaignManagerPool::consider_web_operation(
  const Generics::Time& time,
  const std::uint32_t colo_id,
  const std::uint32_t tag_id,
  const std::uint32_t cc_id,
  const std::string& ct,
  const std::string& curct,
  const std::string& browser,
  const std::string& os,
  const std::string& app,
  const std::string& source,
  const std::string& operation,
  const std::string& user_bind_src,
  const std::uint32_t result,
  const std::uint32_t user_status,
  const bool test_request,
  const std::vector<std::string>& request_ids,
  const std::string& global_request_id,
  const std::string& referer,
  const std::string& ip_address,
  const std::string& external_user_id,
  const std::string& user_agent) noexcept
{
  using ConsiderWebOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPool;

  return do_request<
    ConsiderWebOperationClient,
    ConsiderWebOperationRequest,
    ConsiderWebOperationResponse>(
      time,
      colo_id,
      tag_id,
      cc_id,
      ct,
      curct,
      browser,
      os,
      app,
      source,
      operation,
      user_bind_src,
      result,
      user_status,
      test_request,
      request_ids,
      global_request_id,
      referer,
      ip_address,
      external_user_id,
      user_agent);
}

GrpcCampaignManagerPool::ConsiderPassbackTrackResponsePtr
GrpcCampaignManagerPool::consider_passback_track(
  const Generics::Time& time,
  const std::string& country,
  const std::uint32_t colo_id,
  const std::uint32_t tag_id,
  const std::uint32_t user_status) noexcept
{
  using ConsiderPassbackTrackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPool;

  return do_request<
    ConsiderPassbackTrackClient,
    ConsiderPassbackTrackRequest,
    ConsiderPassbackTrackResponse>(
      time,
      country,
      colo_id,
      tag_id,
      user_status);
}

GrpcCampaignManagerPool::ConsiderPassbackResponsePtr
GrpcCampaignManagerPool::consider_passback(
  const Generics::Uuid& request_id,
  const UserIdHashModInfo& user_id_hash_mod,
  const std::string& passback,
  const Generics::Time& time) noexcept
{
  using ConsiderPassbackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPool;

  return do_request<
    ConsiderPassbackClient,
    ConsiderPassbackRequest,
    ConsiderPassbackResponse>(
      request_id,
      user_id_hash_mod,
      passback,
      time);
}

GrpcCampaignManagerPool::ActionTakenResponsePtr
GrpcCampaignManagerPool::action_taken(
  const Generics::Time& time,
  const bool test_request,
  const bool log_as_test,
  const bool campaign_id_defined,
  const std::uint32_t campaign_id,
  const bool action_id_defined,
  const std::uint32_t action_id,
  const std::string& order_id,
  const bool action_value_defined,
  const std::string& action_value,
  const std::string& referer,
  const std::uint32_t user_status,
  const Generics::Uuid& user_id,
  const std::string& ip_hash,
  const std::vector<std::uint32_t>& platform_ids,
  const std::string& peer_ip,
  const std::vector<GeoInfo>& location) noexcept
{
  using ActionTakenClient = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPool;

  return do_request<
    ActionTakenClient,
    ActionTakenRequest,
    ActionTakenResponse>(
      time,
      test_request,
      log_as_test,
      campaign_id_defined,
      campaign_id,
      action_id_defined,
      action_id,
      order_id,
      action_value_defined,
      action_value,
      referer,
      user_status,
      user_id,
      ip_hash,
      platform_ids,
      peer_ip,
      location);
}

GrpcCampaignManagerPool::ProcessMatchRequestResponsePtr
GrpcCampaignManagerPool::process_match_request(
  const Generics::Uuid& user_id,
  const std::string& household_id,
  const Generics::Time& request_time,
  const std::string& source,
  const std::vector<std::uint32_t>& channels,
  const std::vector<ChannelTriggerMatchInfo>& pkw_channels,
  const std::vector<std::uint32_t>& hid_channels,
  const std::uint32_t colo_id,
  const std::vector<GeoInfo>& location,
  const std::vector<GeoCoordInfo>& coord_location,
  const std::string& full_referer) noexcept
{
  using ProcessMatchRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPool;

  return do_request<
    ProcessMatchRequestClient,
    ProcessMatchRequestRequest,
    ProcessMatchRequestResponse>(
      user_id,
      household_id,
      request_time,
      source,
      channels,
      pkw_channels,
      hid_channels,
      colo_id,
      location,
      coord_location,
      full_referer);
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

GrpcCampaignManagerPool::ConsiderWebOperationRequestPtr
GrpcCampaignManagerPool::create_consider_web_operation_request(
  const Generics::Time& time,
  const std::uint32_t colo_id,
  const std::uint32_t tag_id,
  const std::uint32_t cc_id,
  const std::string& ct,
  const std::string& curct,
  const std::string& browser,
  const std::string& os,
  const std::string& app,
  const std::string& source,
  const std::string& operation,
  const std::string& user_bind_src,
  const std::uint32_t result,
  const std::uint32_t user_status,
  const bool test_request,
  const std::vector<std::string>& request_ids,
  const std::string& global_request_id,
  const std::string& referer,
  const std::string& ip_address,
  const std::string& external_user_id,
  const std::string& user_agent)
{
  auto request = std::make_unique<ConsiderWebOperationRequest>();
  auto* const web_op_info = request->mutable_web_op_info();
  web_op_info->set_time(GrpcAlgs::pack_time(time));
  web_op_info->set_colo_id(colo_id);
  web_op_info->set_tag_id(tag_id);
  web_op_info->set_cc_id(cc_id);
  web_op_info->set_ct(ct);
  web_op_info->set_curct(curct);
  web_op_info->set_browser(browser);
  web_op_info->set_os(os);
  web_op_info->set_app(app);
  web_op_info->set_source(source);
  web_op_info->set_operation(operation);
  web_op_info->set_user_bind_src(user_bind_src);
  web_op_info->set_result(result);
  web_op_info->set_user_status(user_status);
  web_op_info->set_test_request(test_request);
  auto* request_ids_proto = web_op_info->mutable_request_ids();
  request_ids_proto->Add(
    std::begin(request_ids),
    std::end(request_ids));
  web_op_info->set_global_request_id(global_request_id);
  web_op_info->set_referer(referer);
  web_op_info->set_ip_address(ip_address);
  web_op_info->set_external_user_id(external_user_id);
  web_op_info->set_user_agent(user_agent);

  return request;
}

GrpcCampaignManagerPool::ConsiderPassbackTrackRequestPtr
GrpcCampaignManagerPool::create_consider_passback_track_request(
  const Generics::Time& time,
  const std::string& country,
  const std::uint32_t colo_id,
  const std::uint32_t tag_id,
  const std::uint32_t user_status)
{
  auto request = std::make_unique<ConsiderPassbackTrackRequest>();
  auto* const pass_info = request->mutable_pass_info();
  pass_info->set_time(GrpcAlgs::pack_time(time));
  pass_info->set_country(country);
  pass_info->set_colo_id(colo_id);
  pass_info->set_tag_id(tag_id);
  pass_info->set_user_status(user_status);

  return request;
}

GrpcCampaignManagerPool::ConsiderPassbackRequestPtr
GrpcCampaignManagerPool::create_consider_passback_request(
    const Generics::Uuid& request_id,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::string& passback,
    const Generics::Time& time)
{
  auto request = std::make_unique<ConsiderPassbackRequest>();
  auto* const pass_info = request->mutable_pass_info();
  pass_info->set_request_id(GrpcAlgs::pack_request_id(request_id));
  pass_info->set_passback(passback);
  pass_info->set_time(GrpcAlgs::pack_time(time));

  auto* const user_id_hash_proto = pass_info->mutable_user_id_hash_mod();
  user_id_hash_proto->set_value(user_id_hash_mod.value.value_or(0));
  user_id_hash_proto->set_defined(user_id_hash_mod.value.has_value());

  return request;
}

GrpcCampaignManagerPool::ActionTakenRequestPtr
GrpcCampaignManagerPool::create_action_taken_request(
    const Generics::Time& time,
    const bool test_request,
    const bool log_as_test,
    const bool campaign_id_defined,
    const std::uint32_t campaign_id,
    const bool action_id_defined,
    const std::uint32_t action_id,
    const std::string& order_id,
    const bool action_value_defined,
    const std::string& action_value,
    const std::string& referer,
    const std::uint32_t user_status,
    const Generics::Uuid& user_id,
    const std::string& ip_hash,
    const std::vector<std::uint32_t>& platform_ids,
    const std::string& peer_ip,
    const std::vector<GeoInfo>& location)
{
  auto request = std::make_unique<ActionTakenRequest>();
  auto* action_info = request->mutable_action_info();

  action_info->set_time(GrpcAlgs::pack_time(time));
  action_info->set_test_request(test_request);
  action_info->set_log_as_test(log_as_test);
  action_info->set_campaign_id_defined(campaign_id_defined);
  action_info->set_campaign_id(campaign_id);
  action_info->set_action_id_defined(action_id_defined);
  action_info->set_action_id(action_id);
  action_info->set_order_id(order_id);
  action_info->set_action_value_defined(action_value_defined);
  action_info->set_action_value(action_value);
  action_info->set_referer(referer);
  action_info->set_user_status(user_status);
  action_info->set_user_id(GrpcAlgs::pack_user_id(user_id));
  action_info->set_ip_hash(ip_hash);
  action_info->set_peer_ip(peer_ip);

  auto* platform_ids_proto = action_info->mutable_platform_ids();
  platform_ids_proto->Add(std::begin(platform_ids), std::end(platform_ids));

  auto* location_proto = action_info->mutable_location();
  location_proto->Reserve(location.size());
  for (const auto& [country, region, city] : location)
  {
    auto* geo_info = location_proto->Add();
    geo_info->set_country(country);
    geo_info->set_region(region);
    geo_info->set_city(city);
  }

  return request;
}

GrpcCampaignManagerPool::ProcessMatchRequestRequestPtr
GrpcCampaignManagerPool::create_process_match_request_request(
  const Generics::Uuid& user_id,
  const std::string& household_id,
  const Generics::Time& request_time,
  const std::string& source,
  const std::vector<std::uint32_t>& channels,
  const std::vector<ChannelTriggerMatchInfo>& pkw_channels,
  const std::vector<std::uint32_t>& hid_channels,
  const std::uint32_t colo_id,
  const std::vector<GeoInfo>& location,
  const std::vector<GeoCoordInfo>& coord_location,
  const std::string& full_referer)
{
  auto request = std::make_unique<ProcessMatchRequestRequest>();
  auto* match_request_info = request->mutable_match_request_info();

  match_request_info->set_user_id(GrpcAlgs::pack_user_id(user_id));
  match_request_info->set_household_id(household_id);
  match_request_info->set_request_time(GrpcAlgs::pack_time(request_time));
  match_request_info->set_source(source);

  auto* match_info = match_request_info->mutable_match_info();
  match_info->set_colo_id(colo_id);
  match_info->set_full_referer(full_referer);
  auto* channels_proto = match_info->mutable_channels();
  channels_proto->Add(std::begin(channels), std::end(channels));
  auto* hid_channels_proto = match_info->mutable_hid_channels();
  hid_channels_proto->Add(std::begin(hid_channels), std::end(hid_channels));

  auto* pkw_channels_proto = match_info->mutable_pkw_channels();
  pkw_channels_proto->Reserve(pkw_channels.size());
  for (const auto& [channel_trigger_id, channel_id] : pkw_channels)
  {
    auto* element = pkw_channels_proto->Add();
    element->set_channel_trigger_id(channel_trigger_id);
    element->set_channel_id(channel_id);
  }

  auto* location_proto = match_info->mutable_location();
  location_proto->Reserve(location.size());
  for (const auto& [country, region, city] : location)
  {
    auto* element = location_proto->Add();
    element->set_country(country);
    element->set_region(region);
    element->set_city(city);
  }

  auto* coord_location_proto = match_info->mutable_coord_location();
  coord_location_proto->Reserve(coord_location.size());
  for (const auto& [longitude, latitude, accuracy] : coord_location)
  {
    auto* element = coord_location_proto->Add();
    element->set_longitude(longitude);
    element->set_latitude(latitude);
    element->set_accuracy(accuracy);
  }

  return request;
}

} // namespace FrontendCommons