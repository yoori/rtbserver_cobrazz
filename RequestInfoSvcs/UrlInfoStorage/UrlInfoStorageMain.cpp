#include <iostream>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ScopeGuard.hpp>
#include <Commons/SignalActiveObject.hpp>
#include <xsd/RequestInfoSvcs/UrlInfoStorageConfig.hpp>

#include "UrlInfoStorageGrpc.hpp"
#include "UrlInfoStorageHttp.hpp"

int main(int argc, char** argv)
{
  using namespace AdServer::RequestInfoSvcs;
  AdServer::Commons::PidFileGuard::enable_process_exit_cleanup();
  try
  {
    AdServer::Commons::SignalActiveObject signals;
    if (argc != 2)
    {
      std::cerr << "usage: UrlInfoStorage <config_file>\n";
      return 1;
    }

    Config::ErrorHandler errors;
    auto configuration = xsd::AdServer::Configuration::AdConfiguration(argv[1], errors);
    if (errors.has_errors())
    {
      std::string message;
      throw std::runtime_error(errors.text(message));
    }

    const auto& config = configuration->UrlInfoStorageConfig();
    auto logger = Config::LoggerConfigReader::create(config.Logger(), argv[0]);
    AdServer::Commons::PidFileGuard pid_file(std::string(config.pid_file()));
    UrlInfoStorage_var storage = new UrlInfoStorage(
      logger, config.storage_path(), config.rocksdb_workers(), config.rocksdb_batch_size());
    auto stop_storage = AdServer::Commons::make_scope_guard([&storage]()
    {
      storage->deactivate_object();
      storage->wait_object();
    });
    storage->activate_object();

    const auto& grpc_config = config.GrpcConfig();
    const auto& endpoint = grpc_config.Endpoint();
    const std::string host = endpoint.host().present() && endpoint.host().get() != "*" ?
      endpoint.host().get() : "0.0.0.0";
    UrlInfoStorageGrpc_var grpc_server = new UrlInfoStorageGrpc(
      storage, logger, host + ":" + std::to_string(endpoint.port()),
      grpc_config.cq_threads(), grpc_config.threads_per_cq());
    auto stop_grpc = AdServer::Commons::make_scope_guard([&grpc_server]()
    {
      grpc_server->deactivate_object();
      grpc_server->wait_object();
    });
    grpc_server->activate_object();

    const auto& http_config = config.HttpConfig();
    const auto& http_endpoint = http_config.Endpoint();
    const std::string http_host =
      http_endpoint.host().present() && http_endpoint.host().get() != "*" ?
      http_endpoint.host().get() : "0.0.0.0";
    AdServer::Commons::HttpServer::HttpServer_var http_server =
      new AdServer::Commons::HttpServer::HttpServer(
        http_host, http_endpoint.port(), http_config.process_threads(), true);
    add_url_info_storage_http_handlers(*http_server, storage);
    auto stop_http = AdServer::Commons::make_scope_guard([&http_server]()
    {
      http_server->deactivate_object();
      http_server->wait_object();
    });
    http_server->activate_object();

    logger->sstream(Logging::Logger::NOTICE, "UrlInfoStorage") << "service started.";
    signals.wait_object();
    logger->sstream(Logging::Logger::NOTICE, "UrlInfoStorage") << "service stopping.";
  }
  catch (const xml_schema::exception& error)
  {
    std::cerr << "UrlInfoStorage configuration: " << error << '\n';
    return 1;
  }
  catch (const std::exception& error)
  {
    std::cerr << "UrlInfoStorage: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
