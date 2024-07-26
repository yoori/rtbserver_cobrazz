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
  for (const auto& endpoint : endpoints)
  {
    const auto client_holder = factory_client_holder_->create(
      endpoint.host,
      endpoint.port);
    client_holders_.emplace_back(client_holder);
    service_ids_.emplace_back(endpoint.service_id);
    const auto result = service_id_to_client_holder_.try_emplace(
      service_ids_.back(),
      client_holder);
    if (!result.second)
    {
      Stream::Error stream;
      stream << FNS
             << "service_id="
             << endpoint.service_id
             << " already exist";
      throw Exception(stream);
    }
  }
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcCampaignManagerPool::do_request_service(
  const ClientHolderPtr& client_holder,
  Args&& ...args) noexcept
{
  try
  {
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
    else if constexpr (std::is_same_v<Request, GetColocationFlagsRequest>)
    {
      request = create_get_colocation_flags_request(std::forward<Args>(args)...);
    }
    else if constexpr (std::is_same_v<Request, VerifyOptOperationRequest>)
    {
      request = create_verify_opt_operation_request(std::forward<Args>(args)...);
    }
    else if constexpr (std::is_same_v<Request, GetCampaignCreativeRequest>)
    {
      request = create_get_campaign_creative_request(std::forward<Args>(args)...);
    }
    else if constexpr (std::is_same_v<Request, InstantiateAdRequest>)
    {
      request = create_instantiate_ad_request(std::forward<Args>(args)...);
    }
    else if constexpr (std::is_same_v<Request, GetClickUrlRequest>)
    {
      request = create_get_click_url_request(std::forward<Args>(args)...);
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
      return {};
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
          throw Exception(stream);
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
      throw Exception(stream);
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

  return {};
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcCampaignManagerPool::do_request(
  const std::optional<std::string_view> service_id,
  Args&& ...args) noexcept
{
  if (client_holders_.empty())
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << "client_holders is empty";
      logger_->error(
        stream.str(),
        ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
    }
    catch (...)
    {
    }

    return {};
  }

  if (service_id)
  {
    const auto it = service_id_to_client_holder_.find(*service_id);
    if (it != std::end(service_id_to_client_holder_))
    {
      const auto& client_holder = it->second;
      if (!client_holder->is_bad())
      {
        auto response = do_request_service<Client, Request, Response>(
          client_holder,
          std::forward<Args>(args)...);
        if (response)
        {
          return response;
        }
      }
    }
    else
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << "Not existing service_id="
               << *service_id;
        logger_->error(
          stream.str(),
          ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL);
      }
      catch (...)
      {
      }
    }
  }

  const std::size_t size = client_holders_.size();
  for (std::size_t i = 0; i < size; ++i)
  {
    const std::size_t number = counter_.fetch_add(
      1,
      std::memory_order_relaxed);
    const auto& client_holder = client_holders_[number % size];
    if (client_holder->is_bad())
    {
      continue;
    }

    auto response = do_request_service<Client, Request, Response>(
      client_holder,
      std::forward<Args>(args)...);
    if (response)
    {
      return response;
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
    GetPubPixelsResponse>(
      {},
      country,
      user_status,
      publisher_account_ids);
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
      {},
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
      {},
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
      {},
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
      {},
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
      {},
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

GrpcCampaignManagerPool::GetColocationFlagsResponsePtr
GrpcCampaignManagerPool::get_colocation_flags() noexcept
{
  using GetColocationFlagsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPool;

  return do_request<
    GetColocationFlagsClient,
    GetColocationFlagsRequest,
    GetColocationFlagsResponse>({});
}

GrpcCampaignManagerPool::VerifyOptOperationResponsePtr
GrpcCampaignManagerPool::verify_opt_operation(
  const std::uint32_t time,
  const std::int32_t colo_id,
  const std::string& referer,
  const OptOperation operation,
  const std::uint32_t status,
  const std::uint32_t user_status,
  const bool log_as_test,
  const std::string& browser,
  const std::string& os,
  const std::string& ct,
  const std::string& curct,
  const AdServer::Commons::UserId& user_id) noexcept
{
  using VerifyOptOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPool;

  return do_request<
    VerifyOptOperationClient,
    VerifyOptOperationRequest,
    VerifyOptOperationResponse>(
      {},
      time,
      colo_id,
      referer,
      operation,
      status,
      user_status,
      log_as_test,
      browser,
      os,
      ct,
      curct,
      user_id);
}

GrpcCampaignManagerPool::GetCampaignCreativeResponsePtr
GrpcCampaignManagerPool::get_campaign_creative(
  const RequestParams& request_params) noexcept
{
  using GetCampaignCreativeClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPool;

  return do_request<
    GetCampaignCreativeClient,
    GetCampaignCreativeRequest,
    GetCampaignCreativeResponse>(
      {},
      request_params);
}

GrpcCampaignManagerPool::InstantiateAdResponsePtr
GrpcCampaignManagerPool::instantiate_ad(
  const std::string_view service_id,
  const InstantiateAdInfo& instantiate_ad) noexcept
{
  using InstantiateAdClient = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPool;

  return do_request<
    InstantiateAdClient,
    InstantiateAdRequest,
    InstantiateAdResponse>(
      service_id,
      instantiate_ad);
}

GrpcCampaignManagerPool::GetClickUrlResponsePtr
GrpcCampaignManagerPool::get_click_url(
  const std::string_view service_id,
  const Generics::Time& time,
  const Generics::Time& bid_time,
  const std::uint32_t colo_id,
  const std::uint32_t tag_id,
  const std::uint32_t tag_size_id,
  const std::uint32_t ccid,
  const std::uint32_t ccg_keyword_id,
  const std::uint32_t creative_id,
  const AdServer::Commons::UserId& match_user_id,
  const AdServer::Commons::UserId& cookie_user_id,
  const AdServer::Commons::UserId& request_id,
  const UserIdHashModInfo& user_id_hash_mod,
  const std::string& relocate,
  const std::string& referer,
  const bool log_click,
  const CTRDecimal& ctr,
  const std::vector<TokenInfo>& tokens) noexcept
{
  using GetClickUrlClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPool;

  return do_request<
    GetClickUrlClient,
    GetClickUrlRequest,
    GetClickUrlResponse>(
      service_id,
      time,
      bid_time,
      colo_id,
      tag_id,
      tag_size_id,
      ccid,
      ccg_keyword_id,
      creative_id,
      match_user_id,
      cookie_user_id,
      request_id,
      user_id_hash_mod,
      relocate,
      referer,
      log_click,
      ctr,
      tokens);
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
  for (const auto& data : location)
  {
    auto* geo_info = location_proto->Add();
    geo_info->set_country(data.country);
    geo_info->set_region(data.region);
    geo_info->set_city(data.city);
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
  for (const auto& pkw_channel : pkw_channels)
  {
    auto* element = pkw_channels_proto->Add();
    element->set_channel_trigger_id(pkw_channel.channel_trigger_id);
    element->set_channel_id(pkw_channel.channel_id);
  }

  auto* location_proto = match_info->mutable_location();
  location_proto->Reserve(location.size());
  for (const auto& data : location)
  {
    auto* element = location_proto->Add();
    element->set_country(data.country);
    element->set_region(data.region);
    element->set_city(data.city);
  }

  auto* coord_location_proto = match_info->mutable_coord_location();
  coord_location_proto->Reserve(coord_location.size());
  for (const auto& data : coord_location)
  {
    auto* element = coord_location_proto->Add();
    element->set_longitude(GrpcAlgs::pack_decimal(data.longitude));
    element->set_latitude(GrpcAlgs::pack_decimal(data.latitude));
    element->set_accuracy(GrpcAlgs::pack_decimal(data.accuracy));
  }

  return request;
}

GrpcCampaignManagerPool::GetColocationFlagsRequestPtr
GrpcCampaignManagerPool::create_get_colocation_flags_request()
{
  return std::make_unique<GetColocationFlagsRequest>();
}

GrpcCampaignManagerPool::VerifyOptOperationRequestPtr
GrpcCampaignManagerPool::create_verify_opt_operation_request(
  const std::uint32_t time,
  const std::int32_t colo_id,
  const std::string& referer,
  const OptOperation operation,
  const std::uint32_t status,
  const std::uint32_t user_status,
  const bool log_as_test,
  const std::string& browser,
  const std::string& os,
  const std::string& ct,
  const std::string& curct,
  const AdServer::Commons::UserId& user_id)
{
  auto request = std::make_unique<VerifyOptOperationRequest>();
  request->set_time(time);
  request->set_colo_id(colo_id);
  request->set_referer(referer);
  request->set_operation(operation);
  request->set_status(status);
  request->set_user_status(user_status);
  request->set_log_as_test(log_as_test);
  request->set_browser(browser);
  request->set_os(os);
  request->set_ct(ct);
  request->set_curct(curct);
  request->set_user_id(GrpcAlgs::pack_user_id(user_id));

  return request;
}

void GrpcCampaignManagerPool::fill_common_ad_request_info(
  const CommonAdRequestInfo& common_info,
  AdServer::CampaignSvcs::Proto::CommonAdRequestInfo& common_info_proto)
{
  common_info_proto.set_time(GrpcAlgs::pack_time(common_info.time));
  common_info_proto.set_request_id(GrpcAlgs::pack_request_id(common_info.request_id));
  common_info_proto.set_creative_instantiate_type(common_info.creative_instantiate_type);
  common_info_proto.set_request_type(common_info.request_type);
  common_info_proto.set_random(common_info.random);
  common_info_proto.set_test_request(common_info.test_request);
  common_info_proto.set_log_as_test(common_info.log_as_test);
  common_info_proto.set_colo_id(common_info.colo_id);
  common_info_proto.set_external_user_id(common_info.external_user_id);
  common_info_proto.set_source_id(common_info.source_id);

  auto* const location_proto = common_info_proto.mutable_location();
  location_proto->Reserve(common_info.location.size());
  for (const auto& data : common_info.location)
  {
    auto* const location = location_proto->Add();
    location->set_country(data.country);
    location->set_region(data.region);
    location->set_city(data.city);
  }

  auto* const coord_location_proto = common_info_proto.mutable_coord_location();
  coord_location_proto->Reserve(common_info.coord_location.size());
  for (const auto& data : common_info.coord_location)
  {
    auto* const coord_location = coord_location_proto->Add();
    coord_location->set_longitude(GrpcAlgs::pack_decimal(data.longitude));
    coord_location->set_latitude(GrpcAlgs::pack_decimal(data.latitude));
    coord_location->set_accuracy(GrpcAlgs::pack_decimal(data.accuracy));
  }

  common_info_proto.set_full_referer(common_info.full_referer);
  common_info_proto.set_referer(common_info.referer);

  common_info_proto.mutable_urls()->Add(
    std::begin(common_info.urls),
    std::end(common_info.urls));

  common_info_proto.set_security_token(common_info.security_token);
  common_info_proto.set_pub_impr_track_url(common_info.pub_impr_track_url);
  common_info_proto.set_pub_param(common_info.pub_param);
  common_info_proto.set_preclick_url(common_info.preclick_url);
  common_info_proto.set_click_prefix_url(common_info.click_prefix_url);
  common_info_proto.set_original_url(common_info.original_url);
  common_info_proto.set_track_user_id(GrpcAlgs::pack_user_id(common_info.track_user_id));
  common_info_proto.set_user_id(GrpcAlgs::pack_user_id(common_info.user_id));
  common_info_proto.set_user_status(common_info.user_status);
  common_info_proto.set_signed_user_id(common_info.signed_user_id);
  common_info_proto.set_peer_ip(common_info.peer_ip);
  common_info_proto.set_user_agent(common_info.user_agent);
  common_info_proto.set_cohort(common_info.cohort);
  common_info_proto.set_hpos(common_info.hpos);
  common_info_proto.set_ext_track_params(common_info.ext_track_params);

  auto* const tokens_proto = common_info_proto.mutable_tokens();
  tokens_proto->Reserve(common_info.tokens.size());
  for (const auto& token : common_info.tokens)
  {
    auto* const token_proto = tokens_proto->Add();
    token_proto->set_name(token.name);
    token_proto->set_value(token.value);
  }

  common_info_proto.set_set_cookie(common_info.set_cookie);
  common_info_proto.set_passback_type(common_info.passback_type);
  common_info_proto.set_passback_url(common_info.passback_url);
}

void GrpcCampaignManagerPool::fill_context_ad_request_info(
  const ContextAdRequestInfo& context_info,
  AdServer::CampaignSvcs::Proto::ContextAdRequestInfo& context_info_proto)
{
  context_info_proto.set_enabled_notice(context_info.enabled_notice);
  context_info_proto.set_client(context_info.client);
  context_info_proto.set_client_version(context_info.client_version);
  context_info_proto.mutable_platform_ids()->Add(
    std::begin(context_info.platform_ids),
    std::end(context_info.platform_ids));
  context_info_proto.mutable_geo_channels()->Add(
    std::begin(context_info.geo_channels),
    std::end(context_info.geo_channels));
  context_info_proto.set_platform(context_info.platform);
  context_info_proto.set_full_platform(context_info.full_platform);
  context_info_proto.set_web_browser(context_info.web_browser);
  context_info_proto.set_ip_hash(context_info.ip_hash);
  context_info_proto.set_profile_referer(context_info.profile_referer);
  context_info_proto.set_page_load_id(context_info.page_load_id);
  context_info_proto.set_full_referer_hash(context_info.full_referer_hash);
  context_info_proto.set_short_referer_hash(context_info.short_referer_hash);
}

GrpcCampaignManagerPool::GetCampaignCreativeRequestPtr
GrpcCampaignManagerPool::create_get_campaign_creative_request(
  const RequestParams& request_params)
{
  auto request = std::make_unique<GetCampaignCreativeRequest>();
  auto* const request_params_proto = request->mutable_request_params();

  const auto& common_info = request_params.common_info;
  auto* const common_info_proto = request_params_proto->mutable_common_info();
  fill_common_ad_request_info(common_info, *common_info_proto);

  const auto& context_info = request_params.context_info;
  auto* const context_info_proto = request_params_proto->mutable_context_info();
  fill_context_ad_request_info(context_info, *context_info_proto);

  request_params_proto->set_publisher_site_id(request_params.publisher_site_id);
  request_params_proto->mutable_publisher_account_ids()->Add(
    std::begin(request_params.publisher_account_ids),
    std::end(request_params.publisher_account_ids));
  request_params_proto->set_fill_track_pixel(request_params.fill_track_pixel);
  request_params_proto->set_fill_iurl(request_params.fill_iurl);
  request_params_proto->set_ad_instantiate_type(request_params.ad_instantiate_type);
  request_params_proto->set_only_display_ad(request_params.only_display_ad);
  request_params_proto->mutable_full_freq_caps()->Add(
    std::begin(request_params.full_freq_caps),
    std::end(request_params.full_freq_caps));

  auto* const seq_orders_proto = request_params_proto->mutable_seq_orders();
  seq_orders_proto->Reserve(request_params.seq_orders.size());
  for (const auto& seq_order : request_params.seq_orders)
  {
    auto* const seq_order_proto = seq_orders_proto->Add();
    seq_order_proto->set_ccg_id(seq_order.ccg_id);
    seq_order_proto->set_set_id(seq_order.set_id);
    seq_order_proto->set_imps(seq_order.imps);
  }

  auto* const campaign_freqs_proto = request_params_proto->mutable_campaign_freqs();
  campaign_freqs_proto->Reserve(request_params.campaign_freqs.size());
  for (const auto& campaign_freq : request_params.campaign_freqs)
  {
    auto* const campaign_freq_proto = campaign_freqs_proto->Add();
    campaign_freq_proto->set_campaign_id(campaign_freq.campaign_id);
    campaign_freq_proto->set_imps(campaign_freq.imps);
  }

  request_params_proto->set_household_id(GrpcAlgs::pack_user_id(request_params.household_id));
  request_params_proto->set_merged_user_id(GrpcAlgs::pack_user_id(request_params.merged_user_id));
  request_params_proto->set_search_engine_id(request_params.search_engine_id);
  request_params_proto->set_search_words(request_params.search_words);
  request_params_proto->set_page_keywords_present(request_params.page_keywords_present);
  request_params_proto->set_profiling_available(request_params.profiling_available);
  request_params_proto->set_fraud(request_params.fraud);
  request_params_proto->mutable_channels()->Add(
    std::begin(request_params.channels),
    std::end(request_params.channels));
  request_params_proto->mutable_hid_channels()->Add(
    std::begin(request_params.hid_channels),
    std::end(request_params.hid_channels));

  auto* const ccg_keywords_proto = request_params_proto->mutable_ccg_keywords();
  ccg_keywords_proto->Reserve(request_params.ccg_keywords.size());
  for (const auto& data : request_params.ccg_keywords)
  {
    auto* const ccg_keyword_proto = ccg_keywords_proto->Add();
    ccg_keyword_proto->set_ccg_keyword_id(data.ccg_keyword_id);
    ccg_keyword_proto->set_ccg_id(data.ccg_id);
    ccg_keyword_proto->set_channel_id(data.channel_id);
    ccg_keyword_proto->set_max_cpc(GrpcAlgs::pack_decimal(data.max_cpc));
    ccg_keyword_proto->set_ctr(GrpcAlgs::pack_decimal(data.ctr));
    ccg_keyword_proto->set_click_url(data.click_url);
    ccg_keyword_proto->set_original_keyword(data.original_keyword);
  }

  auto* const hid_ccg_keywords_proto = request_params_proto->mutable_hid_ccg_keywords();
  hid_ccg_keywords_proto->Reserve(request_params.hid_ccg_keywords.size());
  for (const auto& hid_ccg_keyword : request_params.hid_ccg_keywords)
  {
    auto* const hid_ccg_keyword_proto = hid_ccg_keywords_proto->Add();
    hid_ccg_keyword_proto->set_ccg_keyword_id(hid_ccg_keyword.ccg_keyword_id);
    hid_ccg_keyword_proto->set_ccg_id(hid_ccg_keyword.ccg_id);
    hid_ccg_keyword_proto->set_channel_id(hid_ccg_keyword.channel_id);
    hid_ccg_keyword_proto->set_max_cpc(GrpcAlgs::pack_decimal(hid_ccg_keyword.max_cpc));
    hid_ccg_keyword_proto->set_ctr(GrpcAlgs::pack_decimal(hid_ccg_keyword.ctr));
    hid_ccg_keyword_proto->set_click_url(hid_ccg_keyword.click_url);
    hid_ccg_keyword_proto->set_original_keyword(hid_ccg_keyword.original_keyword);
  }

  auto* const trigger_match_result_proto = request_params_proto->mutable_trigger_match_result();
  auto* const url_channels_proto = trigger_match_result_proto->mutable_url_channels();
  url_channels_proto->Reserve(request_params.trigger_match_result.url_channels.size());
  for (const auto& url_channel : request_params.trigger_match_result.url_channels)
  {
    auto* const url_channel_proto = url_channels_proto->Add();
    url_channel_proto->set_channel_trigger_id(url_channel.channel_trigger_id);
    url_channel_proto->set_channel_id(url_channel.channel_id);
  }

  auto* const pkw_channels_proto = trigger_match_result_proto->mutable_pkw_channels();
  pkw_channels_proto->Reserve(request_params.trigger_match_result.pkw_channels.size());
  for (const auto& pkw_channel : request_params.trigger_match_result.pkw_channels)
  {
    auto* const pkw_channel_proto = pkw_channels_proto->Add();
    pkw_channel_proto->set_channel_trigger_id(pkw_channel.channel_trigger_id);
    pkw_channel_proto->set_channel_id(pkw_channel.channel_id);
  }

  auto* const skw_channels_proto = trigger_match_result_proto->mutable_skw_channels();
  skw_channels_proto->Reserve(request_params.trigger_match_result.skw_channels.size());
  for (const auto& skw_channel : request_params.trigger_match_result.skw_channels)
  {
    auto* const skw_channel_proto = skw_channels_proto->Add();
    skw_channel_proto->set_channel_trigger_id(skw_channel.channel_trigger_id);
    skw_channel_proto->set_channel_id(skw_channel.channel_id);
  }

  auto* const ukw_channels_proto = trigger_match_result_proto->mutable_ukw_channels();
  ukw_channels_proto->Reserve(request_params.trigger_match_result.ukw_channels.size());
  for (const auto& ukw_channel : request_params.trigger_match_result.ukw_channels)
  {
    auto* const ukw_channel_proto = ukw_channels_proto->Add();
    ukw_channel_proto->set_channel_trigger_id(ukw_channel.channel_trigger_id);
    ukw_channel_proto->set_channel_id(ukw_channel.channel_id);
  }

  trigger_match_result_proto->mutable_uid_channels()->Add(
    std::begin(request_params.trigger_match_result.uid_channels),
    std::end(request_params.trigger_match_result.uid_channels));

  request_params_proto->set_client_create_time(GrpcAlgs::pack_time(request_params.client_create_time));
  request_params_proto->set_session_start(GrpcAlgs::pack_time(request_params.session_start));
  request_params_proto->mutable_exclude_pubpixel_accounts()->Add(
    std::begin(request_params.exclude_pubpixel_accounts),
    std::end(request_params.exclude_pubpixel_accounts));
  request_params_proto->set_tag_delivery_factor(request_params.tag_delivery_factor);
  request_params_proto->set_ccg_delivery_factor(request_params.ccg_delivery_factor);
  request_params_proto->set_preview_ccid(request_params.preview_ccid);

  auto* const ad_slots_proto = request_params_proto->mutable_ad_slots();
  ad_slots_proto->Reserve(request_params.ad_slots.size());
  for (const auto& ad_slot : request_params.ad_slots)
  {
    auto* const ad_slot_proto = ad_slots_proto->Add();
    ad_slot_proto->set_ad_slot_id(ad_slot.ad_slot_id);
    ad_slot_proto->set_format(ad_slot.format);
    ad_slot_proto->set_tag_id(ad_slot.tag_id);
    ad_slot_proto->mutable_sizes()->Add(
      std::begin(ad_slot.sizes),
      std::end(ad_slot.sizes));
    ad_slot_proto->set_ext_tag_id(ad_slot.ext_tag_id);
    ad_slot_proto->set_min_ecpm(
      GrpcAlgs::pack_decimal<AdServer::CampaignSvcs::RevenueDecimal>(ad_slot.min_ecpm));
    ad_slot_proto->set_min_ecpm_currency_code(ad_slot.min_ecpm_currency_code);
    ad_slot_proto->mutable_currency_codes()->Add(
      std::begin(ad_slot.currency_codes),
      std::end(ad_slot.currency_codes));
    ad_slot_proto->set_passback(ad_slot.passback);
    ad_slot_proto->set_up_expand_space(ad_slot.up_expand_space);
    ad_slot_proto->set_right_expand_space(ad_slot.right_expand_space);
    ad_slot_proto->set_left_expand_space(ad_slot.left_expand_space);
    ad_slot_proto->set_tag_visibility(ad_slot.tag_visibility);
    ad_slot_proto->set_tag_predicted_viewability(ad_slot.tag_predicted_viewability);
    ad_slot_proto->set_down_expand_space(ad_slot.down_expand_space);
    ad_slot_proto->set_video_min_duration(ad_slot.video_min_duration);
    ad_slot_proto->set_video_max_duration(ad_slot.video_max_duration);
    ad_slot_proto->set_video_skippable_max_duration(ad_slot.video_skippable_max_duration);
    ad_slot_proto->set_video_allow_skippable(ad_slot.video_allow_skippable);
    ad_slot_proto->set_video_allow_unskippable(ad_slot.video_allow_unskippable);
    ad_slot_proto->set_video_width(ad_slot.video_width);
    ad_slot_proto->set_video_height(ad_slot.video_height);
    ad_slot_proto->mutable_exclude_categories()->Add(
      std::begin(ad_slot.exclude_categories),
      std::end(ad_slot.exclude_categories));
    ad_slot_proto->mutable_required_categories()->Add(
      std::begin(ad_slot.required_categories),
      std::end(ad_slot.required_categories));
    ad_slot_proto->set_debug_ccg(ad_slot.debug_ccg);
    ad_slot_proto->mutable_allowed_durations()->Add(
      std::begin(ad_slot.allowed_durations),
      std::end(ad_slot.allowed_durations));

    auto* const native_data_tokens_proto = ad_slot_proto->mutable_native_data_tokens();
    native_data_tokens_proto->Reserve(ad_slot.native_data_tokens.size());
    for (const auto& native_data_token : ad_slot.native_data_tokens)
    {
      auto* const native_data_token_proto = native_data_tokens_proto->Add();
      native_data_token_proto->set_name(native_data_token.name);
      native_data_token_proto->set_required(native_data_token.required);
    }

    auto* const native_image_tokens_proto = ad_slot_proto->mutable_native_image_tokens();
    native_image_tokens_proto->Reserve(ad_slot.native_image_tokens.size());
    for (const auto& native_image_token : ad_slot.native_image_tokens)
    {
      auto* const native_image_token_proto = native_image_tokens_proto->Add();
      native_image_token_proto->set_name(native_image_token.name);
      native_image_token_proto->set_required(native_image_token.required);
      native_image_token_proto->set_width(native_image_token.width);
      native_image_token_proto->set_height(native_image_token.height);
    }

    ad_slot_proto->set_native_ads_impression_tracker_type(ad_slot.native_ads_impression_tracker_type);
    ad_slot_proto->set_fill_track_html(ad_slot.fill_track_html);
  }

  request_params_proto->set_required_passback(request_params.required_passback);
  request_params_proto->set_profiling_type(request_params.profiling_type);
  request_params_proto->set_disable_fraud_detection(request_params.disable_fraud_detection);
  request_params_proto->set_need_debug_info(request_params.need_debug_info);
  request_params_proto->set_page_keywords(request_params.page_keywords);
  request_params_proto->set_url_keywords(request_params.url_keywords);
  request_params_proto->set_ssp_location(request_params.ssp_location);

  return request;
}

GrpcCampaignManagerPool::InstantiateAdRequestPtr
GrpcCampaignManagerPool::create_instantiate_ad_request(
  const InstantiateAdInfo& instantiate_ad)
{
  auto request = std::make_unique<InstantiateAdRequest>();
  auto* const instantiate_ad_info_proto = request->mutable_instantiate_ad_info();

  const auto& common_info = instantiate_ad.common_info;
  auto* const common_info_proto = instantiate_ad_info_proto->mutable_common_info();
  fill_common_ad_request_info(common_info, *common_info_proto);

  const auto& context_info = instantiate_ad.context_info;
  auto* const context_info_proto = instantiate_ad_info_proto->mutable_context_info();
  context_info_proto->Reserve(context_info.size());
  for (const auto& info : context_info)
  {
    auto* const info_proto = context_info_proto->Add();
    fill_context_ad_request_info(info, *info_proto);
  }

  instantiate_ad_info_proto->set_format(instantiate_ad.format);
  instantiate_ad_info_proto->set_publisher_site_id(instantiate_ad.publisher_site_id);
  instantiate_ad_info_proto->set_publisher_account_id(instantiate_ad.publisher_account_id);
  instantiate_ad_info_proto->set_tag_id(instantiate_ad.tag_id);
  instantiate_ad_info_proto->set_tag_size_id(instantiate_ad.tag_size_id);

  auto* const creatives_proto = instantiate_ad_info_proto->mutable_creatives();
  creatives_proto->Reserve(instantiate_ad.creatives.size());
  for (const auto& creative : instantiate_ad.creatives)
  {
    auto* const creative_proto = creatives_proto->Add();
    creative_proto->set_ccid(creative.ccid);
    creative_proto->set_ccg_keyword_id(creative.ccg_keyword_id);
    creative_proto->set_request_id(GrpcAlgs::pack_request_id(creative.request_id));
    creative_proto->set_ctr(GrpcAlgs::pack_decimal(creative.ctr));
  }

  instantiate_ad_info_proto->set_creative_id(instantiate_ad.creative_id);

  auto* const user_id_hash_mod_proto = instantiate_ad_info_proto->mutable_user_id_hash_mod();
  const auto& user_id_hash_mod = instantiate_ad.user_id_hash_mod;
  user_id_hash_mod_proto->set_defined(user_id_hash_mod.value.has_value());
  user_id_hash_mod_proto->set_value(user_id_hash_mod.value.value_or(0));

  instantiate_ad_info_proto->set_merged_user_id(GrpcAlgs::pack_user_id(instantiate_ad.merged_user_id));
  instantiate_ad_info_proto->mutable_pubpixel_accounts()->Add(
    std::begin(instantiate_ad.pubpixel_accounts),
    std::end(instantiate_ad.pubpixel_accounts));
  instantiate_ad_info_proto->set_open_price(instantiate_ad.open_price);
  instantiate_ad_info_proto->set_openx_price(instantiate_ad.openx_price);
  instantiate_ad_info_proto->set_liverail_price(instantiate_ad.liverail_price);
  instantiate_ad_info_proto->set_google_price(instantiate_ad.google_price);
  instantiate_ad_info_proto->set_ext_tag_id(instantiate_ad.ext_tag_id);
  instantiate_ad_info_proto->set_video_width(instantiate_ad.video_width);
  instantiate_ad_info_proto->set_video_height(instantiate_ad.video_height);
  instantiate_ad_info_proto->set_consider_request(instantiate_ad.consider_request);
  instantiate_ad_info_proto->set_enabled_notice(instantiate_ad.enabled_notice);
  instantiate_ad_info_proto->set_emulate_click(instantiate_ad.emulate_click);
  instantiate_ad_info_proto->set_pub_imp_revenue(
    GrpcAlgs::pack_decimal(instantiate_ad.pub_imp_revenue));
  instantiate_ad_info_proto->set_pub_imp_revenue_defined(
    instantiate_ad.pub_imp_revenue_defined);

  return request;
}

GrpcCampaignManagerPool::GetClickUrlRequestPtr
GrpcCampaignManagerPool::create_get_click_url_request(
  const Generics::Time& time,
  const Generics::Time& bid_time,
  const std::uint32_t colo_id,
  const std::uint32_t tag_id,
  const std::uint32_t tag_size_id,
  const std::uint32_t ccid,
  const std::uint32_t ccg_keyword_id,
  const std::uint32_t creative_id,
  const AdServer::Commons::UserId& match_user_id,
  const AdServer::Commons::UserId& cookie_user_id,
  const AdServer::Commons::RequestId& request_id,
  const UserIdHashModInfo& user_id_hash_mod,
  const std::string& relocate,
  const std::string& referer,
  const bool log_click,
  const CTRDecimal& ctr,
  const std::vector<TokenInfo>& tokens)
{
  auto request = std::make_unique<GetClickUrlRequest>();
  auto* const click_info_proto = request->mutable_click_info();

  click_info_proto->set_time(GrpcAlgs::pack_time(time));
  click_info_proto->set_bid_time(GrpcAlgs::pack_time(bid_time));
  click_info_proto->set_colo_id(colo_id);
  click_info_proto->set_tag_id(tag_id);
  click_info_proto->set_tag_size_id(tag_size_id);
  click_info_proto->set_ccid(ccid);
  click_info_proto->set_ccg_keyword_id(ccg_keyword_id);
  click_info_proto->set_creative_id(creative_id);
  click_info_proto->set_match_user_id(GrpcAlgs::pack_user_id(match_user_id));
  click_info_proto->set_cookie_user_id(GrpcAlgs::pack_user_id(cookie_user_id));
  click_info_proto->set_request_id(GrpcAlgs::pack_request_id(request_id));

  auto* const user_id_hash_mod_proto = click_info_proto->mutable_user_id_hash_mod();
  user_id_hash_mod_proto->set_defined(user_id_hash_mod.value.has_value());
  user_id_hash_mod_proto->set_value(user_id_hash_mod.value.value_or(0));

  click_info_proto->set_relocate(relocate);
  click_info_proto->set_referer(referer);
  click_info_proto->set_log_click(log_click);
  click_info_proto->set_ctr(GrpcAlgs::pack_decimal(ctr));

  auto* const tokens_proto = click_info_proto->mutable_tokens();
  tokens_proto->Reserve(tokens.size());
  for (const auto& token : tokens)
  {
    auto* const token_proto = tokens_proto->Add();
    token_proto->set_value(token.value);
    token_proto->set_name(token.name);
  }

  return request;
}

} // namespace FrontendCommons