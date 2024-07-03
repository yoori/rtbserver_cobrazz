// UNIX_COMMONS
#include <Logger/DistributorLogger.hpp>

// THIS
#include <FrontendCommons/HTTPUtils.hpp>
#include <Frontends/Modules/EchoFrontend/EchoFrontend.hpp>

namespace Aspect
{

const char ECHO_FRONTEND[] = "EchoFrontend";

} // namespace

namespace AdServer::Echo
{

Frontend::Frontend(
  const GrpcContainerPtr& grpc_container,
  Configuration* frontend_config,
  Logger* logger,
  CommonModule* /*common_module*/,
  HttpResponseFactory* response_factory)
  : FrontendCommons::FrontendInterface(response_factory),
    Logging::LoggerCallbackHolder(
      Logging::Logger_var(
        new Logging::SeveritySelectorLogger(
          logger,
          0,
          frontend_config->get().ActionFeConfiguration()->Logger().log_level())),
      "Echo::Frontend",
      Aspect::ECHO_FRONTEND,
      0),
    FrontendCommons::FrontendTaskPool(
      callback(),
      response_factory,
      frontend_config->get().ActionFeConfiguration()->threads(),
      0),
    grpc_container_(grpc_container),
    logger_(ReferenceCounting::add_ref(logger))
{
}

bool Frontend::will_handle(const String::SubString& uri) noexcept
{
  const static std::string resource = "/echo";
  if (uri.size() >= resource.size())
  {
    if (uri.size() == resource.size())
    {
      return uri.substr(0, 5) == "/echo";
    }
    else
    {
      return uri.substr(0, 5) == "/echo" && uri[5] == '?';
    }
  }
  else
  {
    return false;
  }
}

void Frontend::handle_request_(
  FrontendCommons::HttpRequestHolder_var request_holder,
  FrontendCommons::HttpResponseWriter_var response_writer) noexcept
{
  try
  {
    std::string stream_string;
    stream_string.reserve(1000);
    std::ostringstream stream(std::move(stream_string));

    const auto& request = request_holder->request();
    FrontendCommons::HttpResponse_var response = create_response();
    response->set_content_type(String::SubString("text"));

    const auto& headers = request.headers();
    if (!headers.empty())
    {
      stream << "\n\n\nRequest headers: {\n";
      for (auto& header : headers)
      {
        const auto& name = header.name;
        const auto& value = header.value;
        stream << name
               << " : "
               << value
               <<'\n';
      }
      stream << "}\n";
    }

    stream << "Request uri: "
           << request.uri()
           << '\n';

    const auto method = request.method();
    const bool header_only = request.header_only();
    stream << "Request method: ";
    switch (method)
    {
      case FrontendCommons::HttpRequest::Method::RM_GET:
        if (header_only)
        {
          stream << "HEAD\n";
        }
        else
        {
          stream << "GET\n";
        }
        break;
      case FrontendCommons::HttpRequest::Method::RM_POST:
        stream << "POST\n";
        break;
      default:
        stream << "UNKNOWN\n";
        break;
    }

    const auto& params = request.params();
    if (!params.empty())
    {
      stream << "Request params: ";
      for (const auto& param : params)
      {
        stream << param.name;
        if (!param.value.empty())
        {
          stream << "="
                 << param.value
                 << ", ";
        }
      }
      stream.seekp(-2, stream.cur);
      stream << '\n';
    }

    const auto& body = request.body();
    if (!body.empty())
    {
      stream << "Request body: "
             << body
             << '\n';
    }
    stream << "\n\n\n";

    response->write(stream.str());
    response_writer->write(200, response.in());
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::ECHO_FRONTEND);
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
      logger_->error(stream.str(), Aspect::ECHO_FRONTEND);
    }
    catch (...)
    {
    }
  }
}

void Frontend::init()
{
  std::ostringstream stream;
  stream << FNS
         << "Echo frontend is running";
  logger()->info(stream.str(), Aspect::ECHO_FRONTEND);

  activate_object();
}

void Frontend::shutdown() noexcept
{
  try
  {
    std::ostringstream stream;
    stream << FNS
           << "Echo frontend is sopped";
    logger()->info(stream.str(), Aspect::ECHO_FRONTEND);
  }
  catch (...)
  {
  }
}

} // namespace AdServer::Echo