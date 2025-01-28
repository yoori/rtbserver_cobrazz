// THIS
#include "ChannelSvcs/ChannelServer/GrpcChannelOperationPool.hpp"

namespace AdServer::ChannelSvcs
{

GrpcChannelOperationPool::GrpcChannelOperationPool(
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
std::unique_ptr<Response> GrpcChannelOperationPool::do_request_service(
  const ClientHolderPtr& client_holder,
  const Args& ...args) noexcept
{
  for (std::size_t i = 1; i <= 3; i += 1)
  {
    auto response = try_do_request_service<Client, Request, Response, Args...>(
      client_holder,
      args...);
    if (!response)
    {
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
        case Proto::Error_Type::Error_Type_Implementation:
        {
          stream << "Implementation";
          break;
        }
        case Proto::Error_Type::Error_Type_NotConfigured:
        {
          stream << "NotConfigured";
          break;
        }
        default:
        {
          Stream::Error stream;
          stream << FNS
                 << "Unknown error type";
          break;
        }
      }

      stream << ", description="
             << error.description();
      logger_->error(
        stream.str(),
        ASPECT_GRPC_CHANNEL_OPERATION_POOL);

      client_holder->set_bad();
      return response;
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown response type";
      logger_->error(
        stream.str(),
        ASPECT_GRPC_CHANNEL_OPERATION_POOL);

      client_holder->set_bad();
      return response;
    }
  }

  client_holder->set_bad();
  return {};
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcChannelOperationPool::try_do_request_service(
  const ClientHolderPtr& client_holder,
  const Args& ...args) noexcept
{
  try
  {
    std::unique_ptr<Request> request;
    if constexpr (std::is_same_v<Request, MatchRequest>)
    {
      request = create_match_request(args...);
    }
    else if constexpr (std::is_same_v<Request, GetCcgTraitsRequest>)
    {
      request = create_get_ccg_traits_request(args...);
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Request>);
    }

    auto response = client_holder->template do_request<Client, Request, Response>(
      std::move(request),
      grpc_client_timeout_ms_);

    return response;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger_->error(
      stream.str(),
      ASPECT_GRPC_CHANNEL_OPERATION_POOL);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger_->error(
      stream.str(),
      ASPECT_GRPC_CHANNEL_OPERATION_POOL);
  }

  return {};
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcChannelOperationPool::do_request(
  const Args& ...args) noexcept
{
  if (client_holders_.empty())
  {
    Stream::Error stream;
    stream << FNS
           << " client_holders is empty";
    logger_->error(
      stream.str(),
      ASPECT_GRPC_CHANNEL_OPERATION_POOL);

    return {};
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
      args...);
    if (response && response->has_info())
    {
      return response;
    }
  }

  Stream::Error stream;
  stream << FNS
         << "max tries is reached";
  logger_->error(
    stream.str(),
    ASPECT_GRPC_CHANNEL_OPERATION_POOL);

  return {};
}

GrpcChannelOperationPool::MatchResponsePtr
GrpcChannelOperationPool::match(
  const std::string& request_id,
  const std::string& first_url,
  const std::string& first_url_words,
  const std::string& urls,
  const std::string& urls_words,
  const std::string& pwords,
  const std::string& swords,
  const std::string& uid,
  const std::string& statuses,
  const bool non_strict_word_match,
  const bool non_strict_url_match,
  const bool return_negative,
  const bool simplify_page,
  const bool fill_content) noexcept
{
  return do_request<
    Proto::ChannelServer_match_ClientPool,
    MatchRequest,
    MatchResponse>(
      request_id,
      first_url,
      first_url_words,
      urls,
      urls_words,
      pwords,
      swords,
      uid,
      statuses,
      non_strict_word_match,
      non_strict_url_match,
      return_negative,
      simplify_page,
      fill_content);
}

GrpcChannelOperationPool::GetCcgTraitsResponsePtr
GrpcChannelOperationPool::get_ccg_traits(
  const std::vector<std::size_t>& ids) noexcept
{
  return do_request<
    Proto::ChannelServer_get_ccg_traits_ClientPool,
    GetCcgTraitsRequest,
    GetCcgTraitsResponse>(ids);
}

GrpcChannelOperationPool::MatchRequestPtr
GrpcChannelOperationPool::create_match_request(
  const std::string& request_id,
  const std::string& first_url,
  const std::string& first_url_words,
  const std::string& urls,
  const std::string& urls_words,
  const std::string& pwords,
  const std::string& swords,
  const std::string& uid,
  const std::string& statuses,
  const bool non_strict_word_match,
  const bool non_strict_url_match,
  const bool return_negative,
  const bool simplify_page,
  const bool fill_content)
{
  auto request = std::make_unique<MatchRequest>();
  auto* query = request->mutable_query();
  query->set_request_id(request_id);
  query->set_first_url(first_url);
  query->set_first_url_words(first_url_words);
  query->set_urls(urls);
  query->set_urls_words(urls_words);
  query->set_pwords(pwords);
  query->set_swords(swords);
  query->set_uid(uid);
  query->set_statuses(statuses);
  query->set_non_strict_word_match(non_strict_word_match);
  query->set_non_strict_url_match(non_strict_url_match);
  query->set_return_negative(return_negative);
  query->set_simplify_page(simplify_page);
  query->set_fill_content(fill_content);
  return request;
}

GrpcChannelOperationPool::GetCcgTraitsRequestPtr
GrpcChannelOperationPool::create_get_ccg_traits_request(
  const std::vector<std::size_t>& ids)
{
  auto request = std::make_unique<GetCcgTraitsRequest>();
  auto* ids_proto = request->mutable_ids();
  ids_proto->Reserve(ids.size());
  ids_proto->Add(std::begin(ids), std::end(ids));
  return request;
}

} // namespace AdServer::ChannelSvcs