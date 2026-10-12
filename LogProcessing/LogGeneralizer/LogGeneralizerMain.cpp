#include <eh/Exception.hpp>

#include <Logger/StreamLogger.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ScopeGuard.hpp>

#include "LogGeneralizerMain.hpp"
#include "LogGeneralizerHttp.hpp"
#include "ErrorCode.hpp"

namespace
{
  const char ASPECT[] = "LogGeneralizer";
}

LogGeneralizerApp_::LogGeneralizerApp_() /*throw(eh::Exception)*/
:
  Logging::LoggerCallbackHolder(
    Logging::Logger_var(new Logging::OStream::Logger(Logging::OStream::Config(std::cerr))),
    "LogGeneralizerApp_", ASPECT, 0)
{
}

void
LogGeneralizerApp_::shutdown()
{
  ShutdownGuard guard(shutdown_lock_);

  if (shutdown_requested_)
  {
    return;
  }

  if (db_access_server_)
  {
    db_access_server_->deactivate_object();
    db_access_server_->wait_object();
  }

  if (http_server_)
  {
    http_server_->deactivate_object();
    http_server_->wait_object();
  }

  if (log_generalizer_impl_.in() && log_generalizer_impl_->active())
  {
    log_generalizer_impl_->deactivate_object();
    log_generalizer_impl_->wait_object();
  }

  shutdown_requested_ = true;
}

void
LogGeneralizerApp_::main(int& argc, char** argv, AdServer::Commons::SignalActiveObject& signals)
  noexcept
{
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    const char *usage = "usage: LogGeneralizer <config_file>";

    if (argc < 2)
    {
      Stream::Error es;
      es << "config file is not specified\n" << usage;
      throw InvalidArgument(es);
    }

    Config::ErrorHandler error_handler;

    try
    {
      /* using xsd namespace */
      using namespace xsd::AdServer::Configuration;

      std::string file_name(argv[1]);

      std::unique_ptr<AdConfigurationType>
        ad_configuration = AdConfiguration(file_name.c_str(), error_handler);

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ =
        ConfigPtr(new LogGeneralizerConfigType(ad_configuration->LogGeneralizerConfig()));
    }
    catch (const xml_schema::parsing &ex)
    {
      Stream::Error es;

      es << "Can't parse config file '" << argv[1] << "'. : ";

      if (error_handler.has_errors())
      {
        std::string error_string;
        es << error_handler.text(error_string);
      }

      throw Exception(es);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error es;
      es << "Can't parse config file '" << argv[1] << "'. : " << ex.what();
      throw Exception(es);
    }

    // Initializing logger
    try
    {
      logger(Config::LoggerConfigReader::create(config().Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception &ex)
    {
      Stream::Error es;
      es << "got LoggerConfigReader::Exception: " << ex.what();
      throw Exception(es);
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(config().pid_file()));

    proc_stat_impl_ = new AdServer::LogProcessing::ProcStatImpl;

    if (config().HttpConfig().present())
    {
      const auto& http_config = config().HttpConfig().get();
      const auto& endpoint = http_config.Endpoint();
      const auto host = endpoint.host().present() && endpoint.host().get() != "*" ?
        endpoint.host().get() : "0.0.0.0";
      http_server_ = new AdServer::Commons::HttpServer::HttpServer(
        host, endpoint.port(), http_config.process_threads(), true);
      http_server_->add_handler("/stats",
        AdServer::LogProcessing::make_log_generalizer_stats_http_handler(proc_stat_impl_.in()));
    }

    log_generalizer_impl_ =
      new AdServer::LogProcessing::LogGeneralizerImpl(callback(), logger(),
        config(), proc_stat_impl_);

    {
      grpc_service_ = std::make_unique<AdServer::LogProcessing::LogGeneralizerGrpcService>(
        [this](auto id) { log_generalizer_impl_->stop_stat_upload(id); },
        [this](auto id, bool clear) { log_generalizer_impl_->start_stat_upload(id, clear); },
        [this](auto id, bool clear) { return log_generalizer_impl_->get_stat_info(id, clear); });
      const auto& grpc_config = config().GrpcConfig();
      const auto& endpoint = grpc_config.Endpoint();
      const std::string host = endpoint.host().present() && endpoint.host().get() != "*" ?
        endpoint.host().get() : "0.0.0.0";
      db_access_server_ = std::make_shared<AdServer::Grpc::DbAccessServer>(
        host + ":" + std::to_string(endpoint.port()), grpc_config.process_threads(),
        [this]() -> std::optional<bool>
        {
          try
          {
            return log_generalizer_impl_->get_db_state();
          }
          catch (const AdServer::Commons::DbStateChanger::NotSupported&)
          {
            return std::nullopt;
          }
        },
        [this](bool enabled) { log_generalizer_impl_->change_db_state(enabled); },
        [this](auto& response) { response.set_ready(log_generalizer_impl_->active()); },
        std::vector<grpc::Service*>{grpc_service_.get()});
    }

    if (config().SNMPConfig().present())
    {
      try
      {
        unsigned snmp_index = config().SNMPConfig().get().index().present() ?
                                config().SNMPConfig().get().index().get() :
                                  getpid();

        proc_stat_impl_->set("index", static_cast<unsigned long>(snmp_index));

        snmp_stat_impl_ =
          new SNMPAgentX::SNMPStatsImpl(
            proc_stat_impl_, snmp_index,
            Logging::Logger_var(new Logging::LoggerDefaultHolder(logger(), 0, "ADS-IMPL-1012")),
            "",
            "LogGeneralizer-MIB:logGeneralizer",
            config().SNMPConfig().get().mib_dirs().c_str()
  //          , "/home/alexandr_kashkirov/tmp/agentX"
          );
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(
          Logging::Logger::ERROR,
          ASPECT,
          AdServer::LogProcessing::LOG_GEN_IMPL_ERR_CODE_10
        ) << "LogGeneralizerApp_::main(): "
          "Got eh::Exception during initialization of SNMP stats. "
          ": " << ex.what();
      }
    }

    auto shutdown_guard = AdServer::Commons::make_scope_guard([this]() noexcept
    {
      try
      {
        shutdown();
      }
      catch (...)
      {
        logger()->log(
          String::SubString("Failed to stop LogGeneralizer"), Logging::Logger::EMERGENCY, ASPECT);
      }
    });

    log_generalizer_impl_->activate_object();

    if (http_server_)
    {
      http_server_->activate_object();
    }

    if (db_access_server_)
    {
      db_access_server_->activate_object();
    }

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    signals.wait_object();

    shutdown();
    shutdown_guard.release();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

  }
  catch (const Exception &ex)
  {
    logger()->sstream(
      Logging::Logger::CRITICAL,
      ASPECT,
      AdServer::LogProcessing::LOG_GEN_IMPL_ERR_CODE_0
) << "LogGeneralizerApp_::main(): Got LogGeneralizerApp_::Exception: " << ex.what();
  }
  catch (const eh::Exception &ex)
  {
    logger()->sstream(
      Logging::Logger::EMERGENCY,
      ASPECT,
      AdServer::LogProcessing::LOG_GEN_IMPL_ERR_CODE_0
) << "LogGeneralizerApp_::main(): Got eh::Exception: " << ex.what();
  }

}

int
main(int argc, char **argv) noexcept
{
  AdServer::Commons::PidFileGuard::enable_process_exit_cleanup();
  AdServer::Commons::SignalActiveObject signals;

  LogGeneralizerApp_ *app = 0;

  try
  {
    app = &LogGeneralizerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  app->main(argc, argv, signals);
}
