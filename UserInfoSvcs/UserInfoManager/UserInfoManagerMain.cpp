#include <eh/Exception.hpp>

#include <memory>
#include <string>
#include <unistd.h>

#include <Logger/StreamLogger.hpp>

#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>

#include "UserInfoManagerMain.hpp"

namespace
{
  const char ASPECT[] = "UserInfoManager";

  void
  lifecycle_debug_(
    const char* step,
    const std::string& pid_file = std::string()) noexcept
  {
    std::cerr << "TEMP UserInfoManager lifecycle: pid=" << ::getpid() <<
      " step=" << step;
    if (!pid_file.empty())
    {
      std::cerr << " pid_file=" << pid_file;
    }
    std::cerr << std::endl;
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

    if(config().GrpcConfig().present())
    {
      grpc_adapter_ = new AdServer::UserInfoSvcs::UserInfoManagerGrpc(
        user_info_manager_core_,
        logger(),
        config().GrpcConfig()->Endpoint().host().present() &&
          *(config().GrpcConfig()->Endpoint().host()) != "*" ?
          *config().GrpcConfig()->Endpoint().host() :
          "0.0.0.0",
        config().GrpcConfig()->Endpoint().port(),
        config().GrpcConfig()->threads());
      add_child_object(grpc_adapter_);
    }

    if(config().HttpConfig().present())
    {
      http_server_ = new AdServer::Commons::HttpServer::HttpServer(
        config().HttpConfig()->Endpoint().host().present() &&
          *(config().HttpConfig()->Endpoint().host()) != "*" ?
          *config().HttpConfig()->Endpoint().host() :
          "0.0.0.0",
        config().HttpConfig()->Endpoint().port(),
        4);
      AdServer::UserInfoSvcs::UserInfoManagerGrpc_var grpc_adapter =
        grpc_adapter_;
      http_server_->add_handler(
        "/stats",
        [grpc_adapter](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          const auto stats = grpc_adapter ?
            grpc_adapter->stats() :
            AdServer::UserInfoSvcs::UserInfoManagerGrpc::Stats{};
          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::string("{\"call_in_progress\":") +
              std::to_string(stats.call_in_progress) +
              ",\"match_in_progress\":" +
              std::to_string(stats.match_in_progress) +
              ",\"update_user_freq_caps_in_progress\":" +
              std::to_string(stats.update_user_freq_caps_in_progress) +
              ",\"confirm_user_freq_caps_in_progress\":" +
              std::to_string(stats.confirm_user_freq_caps_in_progress) +
              ",\"fraud_user_in_progress\":" +
              std::to_string(stats.fraud_user_in_progress) +
              ",\"remove_user_profile_in_progress\":" +
              std::to_string(stats.remove_user_profile_in_progress) +
              ",\"merge_in_progress\":" +
              std::to_string(stats.merge_in_progress) +
              ",\"consider_publishers_optin_in_progress\":" +
              std::to_string(stats.consider_publishers_optin_in_progress) +
              "}\n"
          };
        });
      add_child_object(http_server_);
    }

    const std::string pid_file = std::string(config().pid_file());
    lifecycle_debug_("before pid file create", pid_file);
    pid_file_guard =
      std::make_unique<AdServer::Commons::PidFileGuard>(pid_file);
    lifecycle_debug_("after pid file create", pid_file);

    AdServer::Commons::SignalActiveObject signal_active_object;

    lifecycle_debug_("before activate_object", pid_file);
    activate_object();
    lifecycle_debug_("after activate_object", pid_file);

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    lifecycle_debug_("before wait shutdown signal", pid_file);
    signal_active_object.wait_object();
    lifecycle_debug_("after wait shutdown signal", pid_file);

    lifecycle_debug_("before normal deactivate_object", pid_file);
    deactivate_object();
    lifecycle_debug_("after normal deactivate_object", pid_file);
    lifecycle_debug_("before normal wait_object", pid_file);
    wait_object();
    lifecycle_debug_("after normal wait_object", pid_file);

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

  lifecycle_debug_("before final deactivate_object");
  deactivate_object();
  lifecycle_debug_("after final deactivate_object");
  lifecycle_debug_("before final wait_object");
  wait_object();
  lifecycle_debug_("after final wait_object");
  lifecycle_debug_("before clear_children");
  clear_children();
  lifecycle_debug_("after clear_children");

  if (pid_file_guard)
  {
    lifecycle_debug_("before pid file remove");
    pid_file_guard.reset();
    lifecycle_debug_("after pid file remove");
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
