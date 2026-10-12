#include <iostream>
#include <grpcpp/grpcpp.h>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ScopeGuard.hpp>
#include <Commons/SignalActiveObject.hpp>
#include "DictionaryProviderImpl.hpp"

int main(int argc, char** argv)
{
  AdServer::Commons::PidFileGuard::enable_process_exit_cleanup();
  try
  {
    AdServer::Commons::SignalActiveObject signals;
    if (argc != 2)
    {
      std::cerr << "usage: DictionaryProvider <config_file>\n";
      return 1;
    }

    Config::ErrorHandler errors;
    auto configuration = xsd::AdServer::Configuration::AdConfiguration(argv[1], errors);
    if (errors.has_errors())
    {
      std::string message;
      throw std::runtime_error(errors.text(message));
    }

    const auto& config = configuration->DictionaryProviderConfig();
    auto logger = Config::LoggerConfigReader::create(config.Logger(), argv[0]);
    AdServer::Commons::PidFileGuard pid_file(std::string(config.pid_file()));
    AdServer::ChannelSvcs::DictionaryProviderImpl_var service =
      new AdServer::ChannelSvcs::DictionaryProviderImpl(logger, &config);
    auto stop_service = AdServer::Commons::make_scope_guard([&service]()
    {
      service->deactivate_object();
      service->wait_object();
    });
    service->activate_object();

    const auto& grpc_config = config.GrpcConfig();
    const auto& endpoint = grpc_config.Endpoint();
    const std::string host = endpoint.host().present() ? endpoint.host().get() : "0.0.0.0";
    const auto address = host + ":" + std::to_string(endpoint.port());
    grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    builder.SetSyncServerOption(
      grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, grpc_config.process_threads());
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.in());
    auto server = builder.BuildAndStart();
    if (!server)
    {
      throw std::runtime_error("Cannot start DictionaryProvider gRPC listener at " + address);
    }

    auto stop_server = AdServer::Commons::make_scope_guard([&server]()
    {
      server->Shutdown();
      server->Wait();
    });
    logger->sstream(Logging::Logger::NOTICE, "DictionaryProvider") << "service started.";
    signals.wait_object();
    server->Shutdown();
    server->Wait();
    stop_server.release();
    service->deactivate_object();
    service->wait_object();
    stop_service.release();
    logger->sstream(Logging::Logger::NOTICE, "DictionaryProvider") << "service stopped.";
  }
  catch (const xml_schema::exception& error)
  {
    std::cerr << "DictionaryProvider configuration: " << error << '\n';
    return 1;
  }
  catch (const std::exception& error)
  {
    std::cerr << "DictionaryProvider: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
