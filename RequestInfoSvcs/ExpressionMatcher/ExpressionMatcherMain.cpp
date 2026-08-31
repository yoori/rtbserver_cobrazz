#include "ExpressionMatcherMain.hpp"

#include <locale.h>
#include <string>

#include <eh/Exception.hpp>
#include <XMLUtility/Utility.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include "ExpressionMatcherHttp.hpp"
#include "ExpressionMatcherStats.hpp"

namespace
{
  const char ASPECT[] = "ExpressionMatcher";
}

ExpressionMatcherApp_::ExpressionMatcherApp_()
  /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(Logging::OStream::Config(std::cerr))),
      "ExpressionMatcherApp_", ASPECT, 0)
{
}

/* main start point */
void
ExpressionMatcherApp_::main(int& argc, char** argv) noexcept
{
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    const char* usage = "usage: ExpressionMatcher <config_file>";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
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

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ =
        ConfigPtr(new ExpressionMatcherConfigType(ad_configuration->ExpressionMatcherConfig()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '" << argv[1] << "'." << ": ";

      if (error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr, "ADS-IMPL-4000");
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "'." << ": " << e.what();
      throw Exception(ostr, "ADS-IMPL-4000");
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << "Unknown exception at parsing of config.";
      throw Exception(ostr, "ADS-IMPL-4000");
    }

    /* Initializing logger */
    try
    {
      logger(Config::LoggerConfigReader::create(config().Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << "got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr, "ADS-IMPL-4001");
    }

    logger()->log(
      String::SubString("ExpressionMatcherApp_::main(): start"),
      Logging::Logger::TRACE,
      ASPECT);

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(configuration_->pid_file()));

    AdServer::RequestInfoSvcs::ProcStatImpl_var proc_stat_impl;
    if (configuration_->SNMPConfig().present())
    {
      try
      {
        proc_stat_impl = new AdServer::RequestInfoSvcs::ProcStatImpl;

        unsigned snmp_index =
          configuration_->SNMPConfig().get().index().present() ?
          configuration_->SNMPConfig().get().index().get() :
          getpid();

        proc_stat_impl->set("index", static_cast<unsigned long>(snmp_index));

        snmp_stat_provider_ = new SNMPAgentX::SNMPStatsImpl(
          proc_stat_impl, snmp_index,
          Logging::Logger_var(new Logging::LoggerDefaultHolder(logger(), 0, "ADS-IMPL-4025")),
          "",
          "ExpressionMatcher-MIB:expressionMatcher",
          configuration_->SNMPConfig().get().mib_dirs().c_str());
      }
      catch (const eh::Exception& ex)
      {
        logger()->sstream(
          Logging::Logger::ERROR,
          ASPECT) << ": Can't init SNMP stats provider: " << ex.what();
      }
    }

    XMLUtility::initialize();

    /* Creating expression matcher active object */
    expression_matcher_impl_ =
      new AdServer::RequestInfoSvcs::ExpressionMatcherImpl(logger(), config(), proc_stat_impl);

    grpc_adapter_ = new AdServer::RequestInfoSvcs::ExpressionMatcherGrpc(
      expression_matcher_impl_,
      logger(),
      config().GrpcConfig().Endpoint().host().present() &&
        *config().GrpcConfig().Endpoint().host() != "*" ?
        *config().GrpcConfig().Endpoint().host() :
        "0.0.0.0",
      config().GrpcConfig().Endpoint().port(),
      config().GrpcConfig().cq_threads());

    active_objects_ = std::make_shared<Generics::CompositeActiveObject>(false, false);
    active_objects_->add_child_object(expression_matcher_impl_.in());
    active_objects_->add_child_object(grpc_adapter_.in());

    if (config().HttpConfig().present())
    {
      AdServer::Commons::HttpServer::HttpServer_var http_server =
        new AdServer::Commons::HttpServer::HttpServer(
          config().HttpConfig()->Endpoint().host().present() &&
            *config().HttpConfig()->Endpoint().host() != "*" ?
            *config().HttpConfig()->Endpoint().host() :
            "0.0.0.0",
          config().HttpConfig()->Endpoint().port(),
          config().HttpConfig()->process_threads());
      http_server->add_handler(
        "/get_user_navigation_profile",
        AdServer::RequestInfoSvcs::make_user_navigation_profile_http_handler(
          expression_matcher_impl_.in()));
      active_objects_->add_child_object(http_server.in());
    }

    AdServer::Commons::SignalActiveObject signal_active_object;
    active_objects_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    signal_active_object.wait_object();
    active_objects_->deactivate_object();
    active_objects_->wait_object();

    grpc_adapter_.reset();
    expression_matcher_impl_.reset();

    configuration_.reset();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL, ASPECT, e.code())
      << "ExpressionMatcherApp_::main(): "
        "Got UserInfoManagerApp_::Exception. : \n" << e.what();
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-4005")
      << "ExpressionMatcherApp_::main(): "
        "Got eh::Exception. : \n" << e.what();
  }
  catch (...)
  {
    logger()->log(String::SubString("ExpressionMatcherApp_::main(): " "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-4006");
  }
}

int
main(int argc, char** argv)
{
  ExpressionMatcherApp_* app = 0;

  try
  {
    app = &ExpressionMatcherApp::instance();
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
