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

} // namespace FrontendCommons