#include <eh/Exception.hpp>

#include <cstdint>
#include <string>
#include <utility>

#include <Logger/StreamLogger.hpp>

#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/SignalActiveObject.hpp>

#include "BillingServerMain.hpp"

namespace
{
  const char ASPECT[] = "BillingServer";

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
  append_billing_server_stats(
    std::string& body,
    bool& first,
    const AdServer::CampaignSvcs::BillingServerGrpc* grpc_adapter)
  {
    if(grpc_adapter == 0)
    {
      return;
    }

    const auto stats = grpc_adapter->stats();

    append_json_stat(body, first, "call_total", stats.call_total);
    append_json_stat(body, first, "call_total_time", stats.call_total_time);
    append_json_stat(body, first, "call_in_progress", stats.call_in_progress);
    append_json_stat(
      body,
      first,
      "check_available_bid_total",
      stats.check_available_bid_total);
    append_json_stat(
      body,
      first,
      "check_available_bid_total_time",
      stats.check_available_bid_total_time);
    append_json_stat(
      body,
      first,
      "check_available_bid_in_progress",
      stats.check_available_bid_in_progress);
    append_json_stat(body, first, "reserve_bid_total", stats.reserve_bid_total);
    append_json_stat(
      body,
      first,
      "reserve_bid_total_time",
      stats.reserve_bid_total_time);
    append_json_stat(
      body,
      first,
      "reserve_bid_in_progress",
      stats.reserve_bid_in_progress);
    append_json_stat(body, first, "confirm_bid_total", stats.confirm_bid_total);
    append_json_stat(
      body,
      first,
      "confirm_bid_total_time",
      stats.confirm_bid_total_time);
    append_json_stat(
      body,
      first,
      "confirm_bid_in_progress",
      stats.confirm_bid_in_progress);
    append_json_stat(body, first, "add_amount_total", stats.add_amount_total);
    append_json_stat(
      body,
      first,
      "add_amount_total_time",
      stats.add_amount_total_time);
    append_json_stat(
      body,
      first,
      "add_amount_in_progress",
      stats.add_amount_in_progress);
    append_json_stat(body, first, "batch_total", stats.batch_total);
    append_json_stat(
      body,
      first,
      "batch_total_time",
      stats.batch_total_time);
    append_json_stat(body, first, "batch_in_progress", stats.batch_in_progress);
  }
}

BillingServerApp_::BillingServerApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr))),
      "BillingServerApp_", ASPECT, 0)
{}

void
BillingServerApp_::main(int argc, char** argv)
  noexcept
{
  static const char* FUN = "BillingServerApp_::main()";

  try
  {
    static const char* USAGE = "usage: BillingServer <config_file>";

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

      configuration_ = ConfigPtr(new BillingServerConfigType(
        ad_configuration->BillingServer()));
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
      logger(Config::LoggerConfigReader::create(
        config().Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr);
    }

    billing_server_core_ = new AdServer::CampaignSvcs::BillingServerCore(
      callback(),
      logger(),
      config());

    add_child_object(billing_server_core_.in());

    std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;
    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(config().pid_file()));

    AdServer::CampaignSvcs::BillingServerGrpc_var grpc_adapter;
    if(config().GrpcConfig().present())
    {
      grpc_adapter = new AdServer::CampaignSvcs::BillingServerGrpc(
        billing_server_core_,
        logger(),
        config().GrpcConfig()->Endpoint().host().present() &&
          *(config().GrpcConfig()->Endpoint().host()) != "*" ?
          config().GrpcConfig()->Endpoint().host()->c_str() :
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
        [grpc_adapter](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          std::string body = "{";
          bool first = true;
          append_billing_server_stats(body, first, grpc_adapter.in());

          body += "}\n";
          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::move(body)
          };
        });
      add_child_object(http_server);
    }

    AdServer::Commons::SignalActiveObject signal_active_object;
    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    signal_active_object.wait_object();

    deactivate_object();
    wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch(const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
      ASPECT,
      "ADS-IMPL-58") << FUN <<
      ": Got BillingServerApp_::Exception: " << e.what();
  }
  catch(const CORBA::SystemException& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-59") << FUN <<
      ": Got CORBA::SystemException: " << e;
  }
  catch(const eh::Exception& e)
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
  BillingServerApp_* app = 0;

  try
  {
    app = &BillingServerApp::instance();
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
