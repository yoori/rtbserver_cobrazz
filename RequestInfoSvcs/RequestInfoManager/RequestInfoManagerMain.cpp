#include <eh/Exception.hpp>

#include <CORBACommons/StatsImpl.hpp>
#include <SNMPAgent/SNMPAgentX.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "RequestInfoManagerMain.hpp"
#include "RequestInfoManagerStats.hpp"
#include "RequestInfoManagerStatsAgent.hpp"

namespace
{
  const char ASPECT[] = "RequestInfoManager";
  const char REQUEST_INFO_MANAGER_OBJ_KEY[] = "RequestInfoManager";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char PROCESS_STATS_CONTROL_OBJ_KEY[] = "ProcessStatsControl";
}

RequestInfoManagerApp_::RequestInfoManagerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "RequestInfoManagerApp_", ASPECT),
      cmprim_(new CompositeMetricsProviderRIM()
{
}

void
RequestInfoManagerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  if(request_info_manager_impl_.in() != 0)
  {
    request_info_manager_impl_->deactivate_object();
    request_info_manager_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
RequestInfoManagerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
RequestInfoManagerApp_::main(int& argc, char** argv)
  noexcept
{
  const char FUN[] = "RequestInfoManagerApp_::main()";
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

    // fill corba_config
    try
    {
      Config::CorbaConfigReader::read_config(
        config().CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "Can't read Corba Config. : "
        << e.what();
      throw Exception(ostr, "ADS-IMPL-3002");
    }

    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

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

    if(config().Monitoring().present())
    {
        UServerUtils::MetricsHTTPProvider_var metrics_http_provider =
          new UServerUtils::MetricsHTTPProvider(
            cmprim_.operator->(),
            config().Monitoring()->port(),
            "/metrics");

        add_child_object(metrics_http_provider);
    }

    // Creating user info manager servant
    request_info_manager_impl_ =
      new AdServer::RequestInfoSvcs::RequestInfoManagerImpl(
        callback(),
        logger(),
        config(),
        rim_stats_impl,cmprim_);

    typedef CORBACommons::ProcessStatsGen<
      AdServer::RequestInfoSvcs::RequestInfoManagerStatsImpl>
        ProcessStatsImpl;

    CORBACommons::POA_ProcessStatsControl_var proc_stat_ctrl =
      new ProcessStatsImpl(rim_stats_impl);

    register_vars_controller();

    corba_server_adapter_->add_binding(
      REQUEST_INFO_MANAGER_OBJ_KEY, request_info_manager_impl_.in());

    corba_server_adapter_->add_binding(
      PROCESS_STATS_CONTROL_OBJ_KEY, proc_stat_ctrl.in());

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    shutdowner_ = corba_server_adapter_->shutdowner();

    request_info_manager_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    // Running orb loop
    corba_server_adapter_->run();

    wait();
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

