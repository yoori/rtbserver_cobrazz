#include <eh/Exception.hpp>

#include <SNMPAgent/SNMPAgentX.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ScopeGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include "RequestInfoManagerMain.hpp"
#include "RequestInfoManagerStats.hpp"
#include "RequestInfoManagerStatsAgent.hpp"

namespace
{
  const char ASPECT[] = "RequestInfoManager";
}

RequestInfoManagerApp_::RequestInfoManagerApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr))),
      "RequestInfoManagerApp_", ASPECT, 0)
{
}

void
RequestInfoManagerApp_::main(int& argc, char** argv)
  noexcept
{
  const char FUN[] = "RequestInfoManagerApp_::main()";
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;
  AdServer::RequestInfoSvcs::SNMPStatsImpl_var snmp_stat_provider;
  try
  {
    const char* usage = "usage: UserInfoManager <config_file>";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << FUN << "config file is not specified\n" << usage;
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
        ConfigPtr(new RequestInfoManagerConfigType(
          ad_configuration->RequestInfoManagerConfig()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << FUN << "Can't parse config file '"
        << argv[1] << "'."
        << ": ";

      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr, "ADS-IMPL-3000");
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "Can't parse config file '"
        << argv[1] << "'."
        << ": "
        << e.what();
      throw Exception(ostr, "ADS-IMPL-3000");
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << FUN << "Unknown Exception at parsing of config.";
      throw Exception(ostr, "ADS-IMPL-3000");
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
      throw Exception(ostr, "ADS-IMPL-3001");
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(configuration_->pid_file()));

    AdServer::RequestInfoSvcs::RequestInfoManagerStatsImpl_var rim_stats_impl;
    if (configuration_->SNMPConfig().present())
    {
      try
      {
        rim_stats_impl =
          new AdServer::RequestInfoSvcs::RequestInfoManagerStatsImpl;

        unsigned snmp_index =
          configuration_->SNMPConfig().get().index().present() ?
          configuration_->SNMPConfig().get().index().get() :
          getpid();

        snmp_stat_provider = new AdServer::RequestInfoSvcs::SNMPStatsImpl(
          rim_stats_impl, snmp_index,
          Logging::Logger_var(new Logging::LoggerDefaultHolder(
            logger(), 0, "ADS-IMPL-?")),
          "",
          "RequestInfoManager-MIB:requestInfoManager",
          configuration_->SNMPConfig().get().mib_dirs().c_str());
      }
      catch (const eh::Exception& ex)
      {
        logger()->sstream(
          Logging::Logger::ERROR,
          ASPECT) << ": Can't init SNMP stats provider: " << ex.what();
      }
    }

    // Creating user info manager servant
    AdServer::RequestInfoSvcs::RequestInfoManagerImpl_var request_info_manager_impl =
      new AdServer::RequestInfoSvcs::RequestInfoManagerImpl(
        callback(),
        logger(),
        config(),
        rim_stats_impl);

    AdServer::RequestInfoSvcs::RequestInfoManagerGrpc_var grpc_adapter =
      new AdServer::RequestInfoSvcs::RequestInfoManagerGrpc(
        request_info_manager_impl,
        logger(),
        config().GrpcConfig().Endpoint().host().present() &&
          *config().GrpcConfig().Endpoint().host() != "*" ?
          *config().GrpcConfig().Endpoint().host() :
          "0.0.0.0",
        config().GrpcConfig().Endpoint().port(),
        config().GrpcConfig().cq_threads());

    auto active_objects =
      std::make_shared<Generics::CompositeActiveObject>(false, false);
    auto active_objects_shutdown_guard = AdServer::Commons::make_scope_guard(
      [&]() noexcept
      {
        if(active_objects->active())
        {
          active_objects->deactivate_object();
          active_objects->wait_object();
        }
      });

    active_objects->add_child_object(request_info_manager_impl.in());
    active_objects->add_child_object(grpc_adapter.in());

    AdServer::Commons::SignalActiveObject signal_active_object;
    active_objects->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    signal_active_object.wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::CRITICAL,
                        ASPECT, e.code())
        << "RequestInfoManagerApp_::main(): "
          "Got RequestInfoManagerApp_::Exception. : \n"
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("RequestInfoManagerApp_::main(): "
                    "Got RequestInfoManagerApp_::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-3003");
    }
  }
  catch (const CORBA::SystemException& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-3004")
        << "RequestInfoManagerApp_::main(): "
          "Got CORBA::SystemException. : \n"
        << e;
    }
    catch (...)
    {
      logger()->log(String::SubString("RequestInfoManagerApp_::main(): "
                    "Got CORBA::SystemException. : \n"),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-3003");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-3005")
        << "RequestInfoManagerApp_::main(): "
          "Got eh::Exception. : \n"
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("RequestInfoManagerApp_::main(): "
                    "Got eh::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-3003");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString("RequestInfoServerApp_::main(): "
                  "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-3006");
  }
}

int
main(int argc, char** argv)
{
  const char FUN[] = "::main()";
  RequestInfoManagerApp_* app = 0;

  try
  {
    app = &RequestInfoManagerApp::instance();
  }
  catch (...)
  {
    std::cerr << FUN << ": Critical: Got exception while "
      "creating application object." << std::endl;
    return -1;
  }

  if (app == 0)
  {
    std::cerr << FUN << ": Critical: got NULL application object."
              << std::endl;
    return -1;
  }

  app->main(argc, argv);
}
