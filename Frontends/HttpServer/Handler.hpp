#ifndef FRONTENDS_HTTPSERVER_HANDLER
#define FRONTENDS_HTTPSERVER_HANDLER

// UNIXCOMMONS
#include <Generics/TaskPool.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Http/Server/HttpHandler.hpp>

// THIS
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

namespace AdServer::Frontends::Http
{

class HttpHandler final
  : public UServerUtils::Http::Server::HttpHandler,
    public ReferenceCounting::AtomicImpl
{
public:
  using HandlerConfig = UServerUtils::Http::Server::HandlerConfig;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using FrontendInterface = FrontendCommons::FrontendInterface;
  using FrontendInterface_var = ReferenceCounting::SmartPtr<FrontendInterface>;

public:
  HttpHandler(
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const std::optional<Level> log_level,
    Logger* logger,
    FrontendCommons::FrontendInterface* frontend);

  void handle_stream_request(
    const HttpRequest& request,
    RequestContext& context,
    ResponseBodyStream& response_body_stream);

private:
  ~HttpHandler() override = default;

private:
  const Logger_var logger_;

  const FrontendInterface_var frontend_;
};

using HttpHandler_var = ReferenceCounting::SmartPtr<HttpHandler>;

} // namespace AdServer::Frontends::Http

#endif //FRONTENDS_HTTPSERVER_HANDLER