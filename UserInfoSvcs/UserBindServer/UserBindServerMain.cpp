#include <csignal>
#include <iostream>
#include <pthread.h>
#include <eh/Exception.hpp>

#include <Generics/DirSelector.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Logger/FileLogger.hpp>
#include <Logger/SimpleLogger.hpp>
#include <Logger/Syslog.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/PidFileGuard.hpp>

#include "UserBindServerMain.hpp"

namespace
{
  const char ASPECT[] = "UserBindServer";

  void fill_shutdown_signals_(sigset_t& signals)
  {
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
  }

  void block_shutdown_signals_()
  {
    sigset_t signals;
    fill_shutdown_signals_(signals);
    pthread_sigmask(SIG_BLOCK, &signals, nullptr);
  }

  void wait_for_shutdown_signal_()
  {
    sigset_t signals;
    fill_shutdown_signals_(signals);
    int signal_number = 0;
    sigwait(&signals, &signal_number);
  }

  Logging::Logger_var create_logger_(
    const xsd::AdServer::Configuration::ErrorLoggerType& xml_logger_config,
    const char* argv0)
  {
    static const char SYSLOG_PREFIX[] = "FOROS.";

    if (xml_logger_config.filename().empty())
    {
      throw UserBindServerApp_::Exception(
        "create_logger_(): empty file name");
    }

    const std::string& filename = xml_logger_config.filename();

    try
    {
      ReferenceCounting::Deque<Logging::QLogger_var> loggers;
      for (xsd::AdServer::Configuration::ErrorLoggerType::Suffix_sequence::
        const_iterator it = xml_logger_config.Suffix().begin();
        it != xml_logger_config.Suffix().end(); ++it)
      {
        Logging::File::Policies::PolicyList log_policies;

        std::string log_file_name = filename + it->name();
        if (it->size_span().present())
        {
          log_policies.push_back(
            new Logging::File::Policies::SizeSpanPolicy(
              it->size_span().get()));
        }

        if (it->time_span().present())
        {
          log_policies.push_back(
            new Logging::File::Policies::TimeSpanPolicy(
              Generics::Time(it->time_span().get())));
        }

        Logging::File::Config config(
          log_file_name.c_str(),
          log_policies,
          xml_logger_config.log_level() > it->max_log_level() ?
            it->max_log_level() : xml_logger_config.log_level());

        Logging::QLogger_var file_logger(
          new Logging::File::Logger(std::move(config)));

        if (it->min_log_level().present())
        {
          loggers.push_back(Logging::QLogger_var(
            new Logging::SeveritySelectorLogger(
              file_logger,
              it->min_log_level().get())));
        }
        else
        {
          loggers.push_back(Logging::QLogger_var(
            new Logging::SeveritySelectorLogger(
              it->max_log_level(),
              file_logger)));
        }
      }

      if (xml_logger_config.SysLog().present())
      {
        Logging::Logger_var sys_logger(
          new Logging::Syslog::Logger(Logging::Syslog::Config(
            xml_logger_config.SysLog().get().log_level(),
            argv0 ? (std::string(SYSLOG_PREFIX) +
              Generics::DirSelect::file_name(argv0)).c_str() : "")));

        if (loggers.empty())
        {
          return sys_logger;
        }

        loggers.push_back(Logging::QLogger_var(
          new Logging::SeveritySelectorLogger(
            sys_logger,
            Logging::Logger::EMERGENCY,
            Logging::Logger::NOTICE)));
      }

      if (loggers.size() == 1)
      {
        return loggers[0];
      }

      return Logging::Logger_var(
        new Logging::DistributorLogger(loggers.begin(), loggers.end()));
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "create_logger_(): Can't init logger. "
        "Caught eh::Exception. : " << ex.what();
      throw UserBindServerApp_::Exception(ostr);
    }
  }
}

void
UserBindServerApp_::main(int& argc, char** argv) noexcept
{
  static const char* FUN = "UserBindServerApp_::main()";
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    static const char* USAGE = "usage: UserBindServer <config_file>";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << USAGE;
      throw InvalidArgument(ostr);
    }

    Config::ErrorHandler error_handler;

    try
    {
      /* using xsd namespace */
      using namespace xsd::AdServer::Configuration;

      std::string file_name(argv[1]);

      std::unique_ptr<AdConfigurationType> ad_configuration =
        AdConfiguration(file_name.c_str(), error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ = ConfigPtr(new UserBindServerConfigType(
        ad_configuration->UserBindServerConfig()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '" << argv[1] << "': ";

      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "': " << e.what();
      throw Exception(ostr);
    }

    // Initializing logger
    try
    {
      logger_ = create_logger_(config().Logger(), argv[0]);
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got logger init exception: " << e.what();
      throw Exception(ostr);
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(config().pid_file()));
    block_shutdown_signals_();

    AdServer::UserInfoSvcs::UserBindServerCore_var user_bind_server_core =
      new AdServer::UserInfoSvcs::UserBindServerCore(
        config(),
        logger());
    add_child_object(user_bind_server_core);

    if(config().GrpcConfig().present())
    {
      grpc_adapter_ = new AdServer::UserInfoSvcs::UserBindServerGrpc(
        user_bind_server_core,
        logger(),
        config().GrpcConfig()->Endpoint().host().present() &&
          *(config().GrpcConfig()->Endpoint().host()) != "*" ?
          *config().GrpcConfig()->Endpoint().host() :
          "0.0.0.0",
        config().GrpcConfig()->Endpoint().port(),
        static_cast<std::size_t>(config().GrpcConfig()->process_threads()),
        config().GrpcConfig()->max_split().present() ?
          static_cast<std::size_t>(*config().GrpcConfig()->max_split()) :
          static_cast<std::size_t>(config().GrpcConfig()->process_threads()));
      add_child_object(grpc_adapter_);
    }

    if(config().HttpConfig().present())
    {
      AdServer::UserInfoSvcs::UserBindServerGrpc_var grpc_adapter =
        grpc_adapter_;
      http_server_ = new AdServer::Commons::HttpServer::HttpServer(
        config().HttpConfig()->Endpoint().host().present() &&
          *(config().HttpConfig()->Endpoint().host()) != "*" ?
          *config().HttpConfig()->Endpoint().host() :
          "0.0.0.0",
        config().HttpConfig()->Endpoint().port(),
        4);
      http_server_->add_handler(
        "/stats",
        [user_bind_server_core, grpc_adapter](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          const auto stats = user_bind_server_core->stats();
          std::string body =
            std::string("{\"get_user_id_total_requests\":") +
            std::to_string(stats.get_user_id_total_requests) +
            ",\"add_user_id_requests\":" +
            std::to_string(stats.add_user_id_requests);

          if(grpc_adapter.in())
          {
            const auto grpc_stats = grpc_adapter->stats();
            body +=
              ",\"call_in_progress\":" +
              std::to_string(grpc_stats.call_in_progress) +
              ",\"get_bind_request_in_progress\":" +
              std::to_string(grpc_stats.get_bind_request_in_progress) +
              ",\"add_bind_request_in_progress\":" +
              std::to_string(grpc_stats.add_bind_request_in_progress) +
              ",\"get_user_id_in_progress\":" +
              std::to_string(grpc_stats.get_user_id_in_progress) +
              ",\"add_user_id_in_progress\":" +
              std::to_string(grpc_stats.add_user_id_in_progress);
          }

          body += "}\n";
          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::move(body)
          };
        });
      add_child_object(http_server_);
    }

    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    wait_for_shutdown_signal_();

    deactivate_object();
    wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& e)
  {
    if (logger())
    {
      logger()->sstream(Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-IMPL-58") << FUN <<
        ": Got UserBindServerApp_::Exception: " << e.what();
    }
    else
    {
      std::cerr << FUN << ": Got UserBindServerApp_::Exception: " <<
        e.what() << std::endl;
    }
  }
  catch (const eh::Exception& e)
  {
    if (logger())
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-59") << FUN <<
        ": Got eh::Exception: " << e.what();
    }
    else
    {
      std::cerr << FUN << ": Got eh::Exception: " << e.what() << std::endl;
    }
  }
}

int
main(int argc, char** argv)
{
  UserBindServerApp_ app;
  app.main(argc, argv);
  return 0;
}
