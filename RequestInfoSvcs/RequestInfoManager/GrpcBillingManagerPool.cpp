// THIS
#include <RequestInfoSvcs/RequestInfoManager/GrpcBillingManagerPool.hpp>

namespace AdServer::RequestInfoSvcs
{

namespace Aspect
{

const char BILLING_MANAGER_POOL[] = "GrpcBillingManagerPool";

} // namespace Aspect

GrpcBillingManagerPool::GrpcBillingManagerPool(
  Logger* logger,
  TaskProcessor& task_processor,
  const SchedulerPtr& scheduler,
  const Endpoints& endpoints,
  const ConfigPoolClient& config_pool_client,
  const std::size_t grpc_client_timeout_ms)
  : logger_(ReferenceCounting::add_ref(logger)),
    task_processor_(task_processor),
    factory_client_holder_(new FactoryClientHolder(
      logger_.in(),
      scheduler,
      config_pool_client,
      task_processor)),
    grpc_client_timeout_ms_(grpc_client_timeout_ms)
{
  if (endpoints.empty())
  {
    Stream::Error stream;
    stream << FNS
           << "endpoints is empty";
    throw Exception(stream);
  }

  const std::size_t size = endpoints.size();
  client_holders_.reserve(size);
  for (const auto& endpoint : endpoints)
  {
    const auto client_holder = factory_client_holder_->create(
      endpoint.host,
      endpoint.port);
    client_holders_.emplace_back(client_holder);
  }
}

std::size_t GrpcBillingManagerPool::size() const noexcept
{
  return client_holders_.size();
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response>
  GrpcBillingManagerPool::do_request_service(
  const ClientHolderPtr& client_holder,
  Args&& ...args) noexcept
{
  try
  {
    std::unique_ptr<Request> request;
    if constexpr (std::is_same_v<Request, AddAmountRequest>)
    {
      request = create_add_amount_request(std::forward<Args>(args)...);
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
        case AdServer::CampaignSvcs::Billing::Proto::Error_Type::Error_Type_Implementation:
        {
          stream << "Implementation";
          break;
        }
        case AdServer::CampaignSvcs::Billing::Proto::Error_Type::Error_Type_NotReady:
        {
          stream << "NotReady";
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
        Aspect::BILLING_MANAGER_POOL);

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
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger_->error(
      stream.str(),
      Aspect::BILLING_MANAGER_POOL);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger_->error(
      stream.str(),
      Aspect::BILLING_MANAGER_POOL);
  }

  return {};
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response>
GrpcBillingManagerPool::do_request(
  const std::size_t index,
  Args&& ...args) noexcept
{
  if (client_holders_.empty())
  {
    Stream::Error stream;
    stream << FNS
           << "client_holders is empty";
    logger_->emergency(
      stream.str(),
      Aspect::BILLING_MANAGER_POOL);

    return {};
  }

  if (index >= client_holders_.size())
  {
    Stream::Error stream;
    stream << FNS
           << "index must be less then number services";
    logger_->emergency(
      stream.str(),
      Aspect::BILLING_MANAGER_POOL);

    return {};
  }

  const auto& client_holder = client_holders_[index];
  auto response = do_request_service<Client, Request, Response>(
    client_holder,
    std::forward<Args>(args)...);

  return response;
}

GrpcBillingManagerPool::AddAmountRequestPtr
GrpcBillingManagerPool::create_add_amount_request(
  const std::vector<ConfirmBidInfo>& requests)
{
  auto request = std::make_unique<AddAmountRequest>();
  auto* const request_seq = request->mutable_request_seq();
  request_seq->Reserve(requests.size());
  for (const auto& request : requests)
  {
    auto* const proto_request_info = request_seq->Add();
    proto_request_info->set_time(GrpcAlgs::pack_time(request.time));
    proto_request_info->set_account_id(request.account_id);
    proto_request_info->set_advertiser_id(request.advertiser_id);
    proto_request_info->set_campaign_id(request.campaign_id);
    proto_request_info->set_ccg_id(request.ccg_id);
    proto_request_info->set_ctr(GrpcAlgs::pack_decimal(request.ctr));
    proto_request_info->set_account_spent_budget(
      GrpcAlgs::pack_decimal(request.account_spent_budget));
    proto_request_info->set_spent_budget(
      GrpcAlgs::pack_decimal(request.spent_budget));
    proto_request_info->set_reserved_budget(
      GrpcAlgs::pack_decimal(request.reserved_budget));
    proto_request_info->set_imps(
      GrpcAlgs::pack_decimal(request.imps));
    proto_request_info->set_clicks(
      GrpcAlgs::pack_decimal(request.clicks));
    proto_request_info->set_forced(request.forced);
  }

  return request;
}

GrpcBillingManagerPool::AddAmountResponsePtr
GrpcBillingManagerPool::add_amount(
  const std::size_t index,
  const std::vector<ConfirmBidInfo>& requests) noexcept
{
  using AddAmountClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPool;

  if (index >= client_holders_.size())
  {
    Stream::Error stream;
    stream << FNS
           << "index must be larger then size of client_holders";
    logger_->emergency(
      stream.str(),
      Aspect::BILLING_MANAGER_POOL);

    return {};
  }

  return do_request<
    AddAmountClient,
    AddAmountRequest,
    AddAmountResponse>(
      index,
      requests);
}

} // namespace AdServer::RequestInfoSvcs