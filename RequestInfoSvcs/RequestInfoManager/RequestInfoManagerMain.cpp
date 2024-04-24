// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <CORBACommons/StatsImpl.hpp>
#include <SNMPAgent/SNMPAgentX.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/Config.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Statistics/CompositeStatisticsProvider.hpp>

// THIS
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Statistics.hpp>
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
      "RequestInfoManagerApp_", ASPECT)
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
  using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
  using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
  using HttpServerConfig = UServerUtils::Http::Server::ServerConfig;
  using HttpListenerConfig = UServerUtils::Http::Server::ListenerConfig;
  using HttpServerBuilder = UServerUtils::Http::Server::HttpServerBuilder;

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

    try
    {
      auto task_processor_container_builder =
        Config::create_task_processor_container_builder(
          logger(),
          config().Coroutine());
      auto statistics_provider = std::make_shared<
        UServerUtils::Statistics::CompositeStatisticsProviderImpl<
          std::shared_mutex>>(logger());
      auto common_counter_provider =
        AdServer::RequestInfoSvcs::get_common_counter_statistics_provider();
      statistics_provider->add(common_counter_provider);

      auto init_func = [this, statistics_provider] (TaskProcessorContainer& task_processor_container) {
        auto& main_task_processor = task_processor_container.get_main_task_processor();

        ComponentsBuilder::StatisticsProviderInfo statistics_provider_info;
        statistics_provider_info.statistics_provider = statistics_provider;
        statistics_provider_info.statistics_prefix = "cobrazz";
        auto components_builder = std::make_unique<ComponentsBuilder>(
          statistics_provider_info);
        auto& statistic_storage = components_builder->get_statistics_storage();

        const auto& monitoring_config = config().Monitoring();
        std::optional<HttpListenerConfig> monitor_listener_config;
        if (monitoring_config.present())
        {
          HttpListenerConfig config;
          config.port = monitoring_config->port();
          monitor_listener_config = config;
        }

        HttpServerConfig server_config;
        server_config.server_name = "HttpRequestInfoManager";
        server_config.monitor_listener_config = monitor_listener_config;

        auto& listener_config = server_config.listener_config;
        listener_config.unix_socket_path = "/tmp/http_request_info_manager.sock";
        listener_config.handler_defaults = {};

        auto& connection_config = listener_config.connection_config;
        connection_config.keepalive_timeout = std::chrono::seconds{10};

        auto http_server_builder = std::make_unique<HttpServerBuilder>(
          logger(),
          server_config,
          main_task_processor,
          statistic_storage);
        components_builder->add_http_server(std::move(http_server_builder));

        return components_builder;
      };

      manager_ = ManagerCoro_var(
        new ManagerCoro(
          std::move(task_processor_container_builder),
          std::move(init_func),
          logger()));
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FUN
           << "Can't initialize coroutine system: "
           << exc.what();
      throw Exception(ostr);
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << FUN
           << "Unknown error";
      throw Exception(ostr);
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

    // Creating user info manager servant
    request_info_manager_impl_ =
      new AdServer::RequestInfoSvcs::RequestInfoManagerImpl(
        callback(),
        logger(),
        config(),
        rim_stats_impl);

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
    manager_->activate_object();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    // Running orb loop
    corba_server_adapter_->run();
    wait();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    manager_->deactivate_object();
    manager_->wait_object();
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

