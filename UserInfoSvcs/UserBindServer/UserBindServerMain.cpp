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
#include <Commons/ScopeGuard.hpp>

#include "UserBindServerMain.hpp"

namespace
{
  const char ASPECT[] = "UserBindServer";

  void
  append_json_stat(
    std::string& body,
    bool& first,
    const char* name,
    std::uint64_t value)
  {
    if(first)
    {
      first = false;
    }
    else
    {
      body += ',';
    }

    body += '"';
    body += name;
    body += "\":";
    body += std::to_string(value);
  }

  void
  append_grpc_lifecycle_stats(
    std::string& body,
    bool& first,
    const AdServer::Grpc::GrpcServiceBase::LifecycleStatsSnapshot& stats)
  {
    append_json_stat(
      body,
      first,
      "grpc_unary_call_created_total",
      stats.unary_call_created_total);
    append_json_stat(
      body,
      first,
      "grpc_unary_call_deleted_total",
      stats.unary_call_deleted_total);
    append_json_stat(
      body,
      first,
      "grpc_unary_call_live",
      stats.unary_call_live);
    append_json_stat(
      body,
      first,
      "grpc_coro_unary_call_created_total",
      stats.coro_unary_call_created_total);
    append_json_stat(
      body,
      first,
      "grpc_coro_unary_call_deleted_total",
      stats.coro_unary_call_deleted_total);
    append_json_stat(
      body,
      first,
      "grpc_coro_unary_call_live",
      stats.coro_unary_call_live);
    append_json_stat(
      body,
      first,
      "grpc_batch_stream_call_created_total",
      stats.batch_stream_call_created_total);
    append_json_stat(
      body,
      first,
      "grpc_batch_stream_call_deleted_total",
      stats.batch_stream_call_deleted_total);
    append_json_stat(
      body,
      first,
      "grpc_batch_stream_call_live",
      stats.batch_stream_call_live);
    append_json_stat(
      body,
      first,
      "grpc_debug_watchdog_scheduled_total",
      stats.debug_watchdog_scheduled_total);
    append_json_stat(
      body,
      first,
      "grpc_debug_watchdog_finished_total",
      stats.debug_watchdog_finished_total);
    append_json_stat(
      body,
      first,
      "grpc_debug_watchdog_live",
      stats.debug_watchdog_live);
  }

  void
  append_rocksdb_stats(
    std::string& body,
    bool& first,
    const AdServer::ProfilingCommons::RocksDBProfileMapProcessor::Stats& stats)
  {
    append_json_stat(body, first, "rdb_check_total", stats.check_total);
    append_json_stat(body, first, "rdb_get_total", stats.get_total);
    append_json_stat(body, first, "rdb_touch_total", stats.touch_total);
    append_json_stat(body, first, "rdb_save_total", stats.save_total);
    append_json_stat(body, first, "rdb_remove_total", stats.remove_total);
    append_json_stat(body, first, "rdb_read_batch_total", stats.read_batch_total);
    append_json_stat(
      body,
      first,
      "rdb_read_batch_total_time",
      stats.read_batch_total_time);
    append_json_stat(body, first, "rdb_write_batch_total", stats.write_batch_total);
    append_json_stat(
      body,
      first,
      "rdb_write_batch_total_time",
      stats.write_batch_total_time);
  }

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

  AdServer::UserInfoSvcs::UserBindServerCore::Config
  make_core_config_(
    const xsd::AdServer::Configuration::UserBindServerConfigType& config)
  {
    AdServer::UserInfoSvcs::UserBindServerCore::Config core_config;

    core_config.storage.chunks_root = config.Storage().chunks_root();
    core_config.storage.prefix = config.Storage().prefix();
    core_config.storage.bound_prefix = config.Storage().bound_prefix();
    core_config.storage.common_chunks_number =
      config.Storage().common_chunks_number();
    core_config.storage.expire_time =
      Generics::Time(config.Storage().expire_time());
    core_config.storage.bound_expire_time =
      Generics::Time(config.Storage().bound_expire_time());
    if(config.Storage().dump_period().present())
    {
      core_config.storage.dump_period =
        Generics::Time(*config.Storage().dump_period());
    }
    core_config.storage.portions = config.Storage().portions();
    core_config.storage.rocksdb_batching_threads =
      config.Storage().rocksdb_batching_threads();
    core_config.storage.load_slave =
      config.Storage().user_bind_keep_mode() == "keep slave";

    core_config.bind_request_storage.prefix =
      config.BindRequestStorage().prefix();
    core_config.bind_request_storage.common_chunks_number =
      config.BindRequestStorage().common_chunks_number();
    core_config.bind_request_storage.expire_time =
      Generics::Time(config.BindRequestStorage().expire_time());
    core_config.bind_request_storage.portions =
      config.BindRequestStorage().portions();

    if(config.OperationBackup().present())
    {
      AdServer::UserInfoSvcs::UserBindServerCore::OperationBackupConfig
        operation_backup;
      operation_backup.dir = config.OperationBackup()->dir();
      operation_backup.file_prefix = config.OperationBackup()->file_prefix();
      operation_backup.rotate_period =
        Generics::Time(config.OperationBackup()->rotate_period());
      operation_backup.threads = config.OperationBackup()->threads();
      core_config.operation_backup = std::move(operation_backup);
    }

    if(config.OperationLoad().present())
    {
      AdServer::UserInfoSvcs::UserBindServerCore::OperationLoadConfig
        operation_load;
      operation_load.dir = config.OperationLoad()->dir();
      operation_load.unprocessed_dir = config.OperationLoad()->unprocessed_dir();
      operation_load.file_prefix = config.OperationLoad()->file_prefix();
      operation_load.check_period =
        Generics::Time(config.OperationLoad()->check_period());
      operation_load.threads = config.OperationLoad()->threads();
      core_config.operation_load = std::move(operation_load);
    }

    if(config.UserIdBlackList().present())
    {
      core_config.user_id_black_list = config.UserIdBlackList().get();
    }

    if(config.bind_on_min_age())
    {
      core_config.bind_min_age = Generics::Time(config.min_age());
    }
    core_config.max_bad_event = config.max_bad_event();
    core_config.partition_index = config.partition_index();
    core_config.partitions_number = config.partitions_number();

    return core_config;
  }

  void
  append_user_bind_server_stats(
    std::string& body,
    bool& first,
    const AdServer::UserInfoSvcs::UserBindServerCore* user_bind_server_core,
    const AdServer::UserInfoSvcs::UserBindServerGrpc* grpc_adapter)
  {
    const auto stats = user_bind_server_core->stats();
    append_json_stat(
      body,
      first,
      "get_user_id_total",
      stats.get_user_id_total_requests);
    append_json_stat(
      body,
      first,
      "add_user_id_request_total",
      stats.add_user_id_requests);
    append_rocksdb_stats(body, first, user_bind_server_core->rocksdb_stats());

    if(grpc_adapter != 0)
    {
      const auto grpc_stats = grpc_adapter->stats();
      append_json_stat(body, first, "call_total", grpc_stats.call_total);
      append_json_stat(
        body,
        first,
        "call_total_time",
        grpc_stats.call_total_time);
      append_json_stat(
        body,
        first,
        "call_in_progress",
        grpc_stats.call_in_progress);
      append_json_stat(
        body,
        first,
        "get_bind_request_total",
        grpc_stats.get_bind_request_total);
      append_json_stat(
        body,
        first,
        "get_bind_request_total_time",
        grpc_stats.get_bind_request_total_time);
      append_json_stat(
        body,
        first,
        "get_bind_request_in_progress",
        grpc_stats.get_bind_request_in_progress);
      append_json_stat(
        body,
        first,
        "add_bind_request_total",
        grpc_stats.add_bind_request_total);
      append_json_stat(
        body,
        first,
        "add_bind_request_total_time",
        grpc_stats.add_bind_request_total_time);
      append_json_stat(
        body,
        first,
        "add_bind_request_in_progress",
        grpc_stats.add_bind_request_in_progress);
      append_json_stat(
        body,
        first,
        "get_user_id_total_time",
        grpc_stats.get_user_id_total_time);
      append_json_stat(
        body,
        first,
        "get_user_id_in_progress",
        grpc_stats.get_user_id_in_progress);
      append_json_stat(
        body,
        first,
        "add_user_id_total",
        grpc_stats.add_user_id_total);
      append_json_stat(
        body,
        first,
        "add_user_id_total_time",
        grpc_stats.add_user_id_total_time);
      append_json_stat(
        body,
        first,
        "add_user_id_in_progress",
        grpc_stats.add_user_id_in_progress);
      append_json_stat(body, first, "batch_total", grpc_stats.batch_total);
      append_json_stat(
        body,
        first,
        "batch_total_time",
        grpc_stats.batch_total_time);
      append_json_stat(
        body,
        first,
        "batch_in_progress",
        grpc_stats.batch_in_progress);
      append_grpc_lifecycle_stats(
        body,
        first,
        grpc_stats.grpc_lifecycle_stats);
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
        make_core_config_(config()),
        logger());
    add_child_object(user_bind_server_core);

    AdServer::UserInfoSvcs::UserBindServerGrpc_var grpc_adapter;
    auto active_objects_shutdown_guard = AdServer::Commons::make_scope_guard(
      [&]() noexcept
      {
        if(active())
        {
          deactivate_object();
          wait_object();
        }
      });

    if(config().GrpcConfig().present())
    {
      grpc_adapter = new AdServer::UserInfoSvcs::UserBindServerGrpc(
        user_bind_server_core,
        logger(),
        config().GrpcConfig()->Endpoint().host().present() &&
          *(config().GrpcConfig()->Endpoint().host()) != "*" ?
          *config().GrpcConfig()->Endpoint().host() :
          "0.0.0.0",
        config().GrpcConfig()->Endpoint().port(),
        static_cast<std::size_t>(config().GrpcConfig()->process_threads()),
        static_cast<std::size_t>(config().GrpcConfig()->cq_threads()),
        static_cast<std::size_t>(config().GrpcConfig()->max_sequential_ops()));
      add_child_object(grpc_adapter);
    }

    if(config().HttpConfig().present())
    {
      AdServer::Commons::HttpServer::HttpServer_var http_server;
      http_server = new AdServer::Commons::HttpServer::HttpServer(
        config().HttpConfig()->Endpoint().host().present() &&
          *(config().HttpConfig()->Endpoint().host()) != "*" ?
          *config().HttpConfig()->Endpoint().host() :
          "0.0.0.0",
        config().HttpConfig()->Endpoint().port(),
        4);
      http_server->add_handler(
        "/stats",
        [user_bind_server_core, grpc_adapter](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          std::string body =
            "{";
          bool first = true;
          append_user_bind_server_stats(
            body,
            first,
            user_bind_server_core.in(),
            grpc_adapter.in());

          body += "}\n";
          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::move(body)
          };
        });
      add_child_object(http_server);
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
