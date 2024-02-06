// USERVER
#include <userver/engine/future.hpp>

// THIS
#include <Frontends/HttpServer/Handler.hpp>
#include <Frontends/HttpServer/HttpRequest.hpp>
#include <Frontends/HttpServer/HttpResponse.hpp>

namespace AdServer::Frontends::Http
{

namespace Aspect
{

const char HTTP_HANDLER[] = "HTTP_HANDLER";

} // namespace Aspect

class HttpResponseWriterImpl final : public FrontendCommons::HttpResponseWriter
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using HttpResponseContainerPtr = internal::HttpResponseContainerPtr;
  using Promise = userver::engine::Promise<HttpResponseContainerPtr>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  HttpResponseWriterImpl(
    Logger* logger,
    Promise&& promise)
    : logger_(ReferenceCounting::add_ref(logger)),
      promise_(std::move(promise))
  {
  }

  void write(int result, FrontendCommons::HttpResponse* http_response) override
  {
    try
    {
      bool expected = false;
      if (is_writed_.compare_exchange_strong(expected, true))
      {
        HttpResponse* response = dynamic_cast<HttpResponse*>(http_response);
        if (response)
        {
          auto response_container = response->transfer();
          response_container->status_code = result;
          promise_.set_value(std::move(response_container));
        }
        else
        {
          Stream::Error stream;
          stream << FNS
                 << "Logic error... Not correct type of response";
          logger_->error(stream.str(), Aspect::HTTP_HANDLER);
          promise_.set_exception(std::make_exception_ptr(Exception(stream)));
        }
      }
    }
    catch (const Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger_->error(stream.str(), Aspect::HTTP_HANDLER);
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
        logger_->error(stream.str(), Aspect::HTTP_HANDLER);
      }
      catch (...)
      {
      }
    }
  }

private:
  ~HttpResponseWriterImpl() override = default;

private:
  Logger_var logger_;

  Promise promise_;

  std::atomic<bool> is_writed_{false};
};

HttpHandler::HttpHandler(
  const std::string& handler_name,
  const HandlerConfig& handler_config,
  const std::optional<Level> log_level,
  Logger* logger,
  FrontendCommons::FrontendInterface* frontend)
  : UServerUtils::Http::Server::HttpHandler(
      handler_name,
      handler_config,
      log_level),
    logger_(ReferenceCounting::add_ref(logger)),
    frontend_(ReferenceCounting::add_ref(frontend))
{
}

void HttpHandler::handle_stream_request(
  const HttpRequest& request,
  RequestContext& /*context*/,
  ResponseBodyStream& response_body_stream)
{
  FrontendCommons::HttpRequest::Method method =
    FrontendCommons::HttpRequest::Method::RM_GET;
  bool header_only = false;

  const auto& request_method = request.GetMethod();
  switch (request_method)
  {
    case userver::server::http::HttpMethod::kGet:
      method = FrontendCommons::HttpRequest::Method::RM_GET;
      break;
    case userver::server::http::HttpMethod::kHead:
      method = FrontendCommons::HttpRequest::Method::RM_GET;
      header_only = true;
      break;
    case userver::server::http::HttpMethod::kPost:
      method = FrontendCommons::HttpRequest::Method::RM_POST;
      break;
    default:
      Stream::Error stream;
      stream << FNS
             << "Unsupported method '"
             << userver::server::http::ToString(request_method)
             << "'";
      logger_->error(stream.str(), Aspect::HTTP_HANDLER);

      throw userver::server::handlers::ClientError{
        userver::server::handlers::ExternalBody{
        fmt::format("Unsupported method {}", request_method)}};
  }

  const auto& body = request.RequestBody();
  const auto& uri = request.GetRequestPath();
  const std::string server_name;

  std::string_view query_string;
  const auto& url = request.GetUrl();
  const auto pos = url.find('?', url.size());
  if (pos != std::string::npos)
  {
    query_string = std::string_view(
      url.data() + pos + 1,
      url.size() - pos - 1);
  }

  HTTP::HeaderList headers;
  const auto& header_names = request.GetHeaderNames();
  for (const auto& header_name : header_names)
  {
    const auto& header_value = request.GetHeader(header_name);
    headers.emplace_back(
      header_name,
      header_value);
  }

  const bool secure = request.HasHeader("https");

  Http::HttpRequest_var request(
    new Http::HttpRequest(
      method,
      body,
      uri,
      server_name,
      std::string(query_string.data(), query_string.size()),
      std::move(headers),
      header_only,
      secure));

  FrontendCommons::HttpRequestHolder_var request_holder(
    new Http::HttpRequestHolder(
      request.in()));

  userver::engine::Promise<internal::HttpResponseContainerPtr> promise;
  auto future = promise.get_future();

  FrontendCommons::HttpResponseWriter_var writer(
    new HttpResponseWriterImpl(
      logger_.in(),
      std::move(promise)));

  frontend_->handle_request_noparams(
    std::move(request_holder),
    std::move(writer));

  auto container = future.get();

  response_body_stream.SetStatusCode(container->status_code);

  auto& container_headers = container->headers;
  for (auto& header : container_headers)
  {
    response_body_stream.SetHeader(
      header.first,
      header.second);
  }

  auto& body_chunks = container->body_chunks;
  for (auto& chunk : body_chunks)
  {
    response_body_stream.PushBodyChunk(std::move(chunk), {});
  }
}

} // namespace AdServer::Frontends::Http