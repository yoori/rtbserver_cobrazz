#include <eh/Exception.hpp>

#include <memory>
#include <string>
#include <Logger/StreamLogger.hpp>

#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/AsyncMutex.hpp>

#include "UserInfoManagerMain.hpp"

namespace
{
  const char ASPECT[] = "UserInfoManager";

  void
  append_json_stat(
    std::string& body,
    const char* name,
    const std::uint64_t value)
  {
    body += ",\"";
    body += name;
    body += "\":";
    body += std::to_string(value);
  }

  void
  append_grpc_lifecycle_stats(
    std::string& body,
    const AdServer::Grpc::GrpcServiceBase::LifecycleStatsSnapshot& stats)
  {
    append_json_stat(
      body,
      "grpc_unary_call_created_total",
      stats.unary_call_created_total);
    append_json_stat(
      body,
      "grpc_unary_call_deleted_total",
      stats.unary_call_deleted_total);
    append_json_stat(
      body,
      "grpc_unary_call_live",
      stats.unary_call_live);
    append_json_stat(
      body,
      "grpc_coro_unary_call_created_total",
      stats.coro_unary_call_created_total);
    append_json_stat(
      body,
      "grpc_coro_unary_call_deleted_total",
      stats.coro_unary_call_deleted_total);
    append_json_stat(
      body,
      "grpc_coro_unary_call_live",
      stats.coro_unary_call_live);
    append_json_stat(
      body,
      "grpc_batch_stream_call_created_total",
      stats.batch_stream_call_created_total);
    append_json_stat(
      body,
      "grpc_batch_stream_call_deleted_total",
      stats.batch_stream_call_deleted_total);
    append_json_stat(
      body,
      "grpc_batch_stream_call_live",
      stats.batch_stream_call_live);
    append_json_stat(
      body,
      "grpc_debug_watchdog_scheduled_total",
      stats.debug_watchdog_scheduled_total);
    append_json_stat(
      body,
      "grpc_debug_watchdog_finished_total",
      stats.debug_watchdog_finished_total);
    append_json_stat(
      body,
      "grpc_debug_watchdog_live",
      stats.debug_watchdog_live);
  }

  void
  append_rocksdb_stats(
    std::string& body,
    const AdServer::ProfilingCommons::RocksDBProfileMapProcessor::Stats& stats)
  {
    append_json_stat(body, "rdb_check_total", stats.check_total);
    append_json_stat(body, "rdb_get_total", stats.get_total);
    append_json_stat(body, "rdb_touch_total", stats.touch_total);
    append_json_stat(body, "rdb_save_total", stats.save_total);
    append_json_stat(body, "rdb_remove_total", stats.remove_total);
    append_json_stat(body, "rdb_read_batch_total", stats.read_batch_total);
    append_json_stat(
      body,
      "rdb_read_batch_total_time",
      stats.read_batch_total_time);
    append_json_stat(body, "rdb_write_batch_total", stats.write_batch_total);
    append_json_stat(
      body,
      "rdb_write_batch_total_time",
      stats.write_batch_total_time);
  }
}

UserInfoManagerApp_::UserInfoManagerApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr))),
      "UserInfoManagerApp_", ASPECT, 0)
{}

void
UserInfoManagerApp_::main(int& argc, char** argv)
  noexcept
{
  static const char* FUN = "UserInfoManagerApp_::main()";
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    static const char* USAGE = "usage: UserInfoManager <config_file>";

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

      std::unique_ptr<AdConfigurationType>
        ad_configuration = AdConfiguration(file_name.c_str(), error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ =
        ConfigPtr(new UserInfoManagerConfigType(
          ad_configuration->UserInfoManagerConfig()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '" << argv[1] << "'."
        ": ";

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
      logger(Config::LoggerConfigReader::create(
        config().Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr);
    }

    user_info_manager_core_ =
      std::make_shared<AdServer::UserInfoSvcs::UserInfoManagerCore>(
        callback(),
        logger(),
        config());

    add_child_object(user_info_manager_core_);

    AdServer::UserInfoSvcs::UserInfoManagerGrpc_var grpc_adapter;
    if(config().GrpcConfig().present())
    {
      grpc_adapter = new AdServer::UserInfoSvcs::UserInfoManagerGrpc(
        user_info_manager_core_,
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
      AdServer::Commons::HttpServer::HttpServer_var http_server =
        new AdServer::Commons::HttpServer::HttpServer(
        config().HttpConfig()->Endpoint().host().present() &&
          *(config().HttpConfig()->Endpoint().host()) != "*" ?
          *config().HttpConfig()->Endpoint().host() :
          "0.0.0.0",
        config().HttpConfig()->Endpoint().port(),
        4);
      http_server->add_handler(
        "/stats",
        [user_info_manager_core = user_info_manager_core_, grpc_adapter](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          const auto stats = grpc_adapter ?
            grpc_adapter->stats() :
            AdServer::UserInfoSvcs::UserInfoManagerGrpc::Stats{};
          const std::string min_time_of_request_in_progress =
            stats.min_time_of_request_in_progress ?
              "\"" + stats.min_time_of_request_in_progress->gm_ft() + "\"" :
              "null";
          const auto async_mutex_stats = AdServer::Commons::AsyncMutex::stats();
          std::string body =
            std::string("{\"call_total\":") + std::to_string(stats.call_total) +
              ",\"call_total_time\":" +
              std::to_string(stats.call_total_time) +
              ",\"call_in_progress\":" +
              std::to_string(stats.call_in_progress) +
              ",\"match_total\":" +
              std::to_string(stats.match_total) +
              ",\"match_total_time\":" +
              std::to_string(stats.match_total_time) +
              ",\"match_in_progress\":" +
              std::to_string(stats.match_in_progress) +
              ",\"update_user_freq_caps_total\":" +
              std::to_string(stats.update_user_freq_caps_total) +
              ",\"update_user_freq_caps_total_time\":" +
              std::to_string(stats.update_user_freq_caps_total_time) +
              ",\"update_user_freq_caps_in_progress\":" +
              std::to_string(stats.update_user_freq_caps_in_progress) +
              ",\"confirm_user_freq_caps_total\":" +
              std::to_string(stats.confirm_user_freq_caps_total) +
              ",\"confirm_user_freq_caps_total_time\":" +
              std::to_string(stats.confirm_user_freq_caps_total_time) +
              ",\"confirm_user_freq_caps_in_progress\":" +
              std::to_string(stats.confirm_user_freq_caps_in_progress) +
              ",\"fraud_user_total\":" +
              std::to_string(stats.fraud_user_total) +
              ",\"fraud_user_total_time\":" +
              std::to_string(stats.fraud_user_total_time) +
              ",\"fraud_user_in_progress\":" +
              std::to_string(stats.fraud_user_in_progress) +
              ",\"remove_user_profile_total\":" +
              std::to_string(stats.remove_user_profile_total) +
              ",\"remove_user_profile_total_time\":" +
              std::to_string(stats.remove_user_profile_total_time) +
              ",\"remove_user_profile_in_progress\":" +
              std::to_string(stats.remove_user_profile_in_progress) +
              ",\"merge_total\":" +
              std::to_string(stats.merge_total) +
              ",\"merge_total_time\":" +
              std::to_string(stats.merge_total_time) +
              ",\"merge_in_progress\":" +
              std::to_string(stats.merge_in_progress) +
              ",\"consider_publishers_optin_total\":" +
              std::to_string(stats.consider_publishers_optin_total) +
              ",\"consider_publishers_optin_total_time\":" +
              std::to_string(stats.consider_publishers_optin_total_time) +
              ",\"consider_publishers_optin_in_progress\":" +
              std::to_string(stats.consider_publishers_optin_in_progress) +
              ",\"batch_total\":" +
              std::to_string(stats.batch_total) +
              ",\"batch_total_time\":" +
              std::to_string(stats.batch_total_time) +
              ",\"batch_in_progress\":" +
              std::to_string(stats.batch_in_progress) +
              ",\"call_inflight\":" +
              std::to_string(stats.call_inflight) +
              ",\"min_time_of_request_in_progress\":" +
              min_time_of_request_in_progress;

          append_grpc_lifecycle_stats(body, stats.grpc_lifecycle_stats);
          append_rocksdb_stats(body, user_info_manager_core->rocksdb_stats());

          body +=
              ",\"async_mutex_lock_attempts\":" +
              std::to_string(async_mutex_stats.lock_attempts) +
              ",\"async_mutex_immediate_locks\":" +
              std::to_string(async_mutex_stats.immediate_locks) +
              ",\"async_mutex_contended_locks\":" +
              std::to_string(async_mutex_stats.contended_locks) +
              ",\"async_mutex_current_waiters\":" +
              std::to_string(async_mutex_stats.current_waiters) +
              ",\"async_mutex_max_waiters\":" +
              std::to_string(async_mutex_stats.max_waiters) +
              "}\n";

          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::move(body)
          };
        });
      add_child_object(http_server);
    }

    pid_file_guard =
      std::make_unique<AdServer::Commons::PidFileGuard>(
        std::string(config().pid_file()));

    AdServer::Commons::SignalActiveObject signal_active_object;

    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    signal_active_object.wait_object();

    deactivate_object();
    wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
      ASPECT,
      "ADS-IMPL-58") << FUN <<
      ": Got UserInfoManagerApp_::Exception: " << e.what();
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-59") << FUN <<
      ": Got eh::Exception: " << e.what();
  }
}

int
main(int argc, char** argv)
{
  UserInfoManagerApp_* app = 0;

  try
  {
    app = &UserInfoManagerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  app->main(argc, argv);
}
