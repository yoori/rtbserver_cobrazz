#include "ExpressionMatcherMain.hpp"

#include <locale.h>
#include <string>

#include <eh/Exception.hpp>
#include <XMLUtility/Utility.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/Config.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Statistics/CompositeStatisticsProvider.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/UserverConfigUtils.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/Statistics.hpp>
#include "ExpressionMatcherStats.hpp"

namespace
{
  const char ASPECT[] = "ExpressionMatcher";
  const char EXPRESSION_MATCHER_OBJ_KEY[] = "ExpressionMatcher";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

ExpressionMatcherApp_::ExpressionMatcherApp_()
  /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ExpressionMatcherApp_", ASPECT)
{
}

/** ProcessControl interface implementation */
void
ExpressionMatcherApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  if(expression_matcher_impl_.in() != 0)
  {
    expression_matcher_impl_->deactivate_object();
    expression_matcher_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
ExpressionMatcherApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

/* main start point */
void
ExpressionMatcherApp_::main(int& argc, char** argv) noexcept
{
  using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Grpc::Manager;
  using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
  using HttpServerConfig = UServerUtils::Http::Server::ServerConfig;
  using HttpListenerConfig = UServerUtils::Http::Server::ListenerConfig;
  using HttpServerBuilder = UServerUtils::Http::Server::HttpServerBuilder;

  try
  {
    const char* usage = "usage: ExpressionMatcher <config_file>";

    if(argc < 2)
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

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ =
        ConfigPtr(new ExpressionMatcherConfigType(
          ad_configuration->ExpressionMatcherConfig()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '"
           << argv[1] << "'."
           << ": ";

      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr, "ADS-IMPL-4000");
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '"
           << argv[1] << "'."
           << ": "
           << e.what();
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
      logger(Config::LoggerConfigReader::create(
        config().Logger(), argv[0]));
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

    try
    {
      auto task_processor_container_builder =
        Config::create_task_processor_container_builder(
          logger(),
          config().Coroutine());
      auto statistics_provider = std::make_shared<
        UServerUtils::Statistics::CompositeStatisticsProviderImpl<
          std::shared_mutex>>(logger());
      auto counter_provider = AdServer::RequestInfoSvcs::get_counter_statistics_provider();
      statistics_provider->add(counter_provider);

      auto init_func = [this, statistics_provider] (TaskProcessorContainer& task_processor_container) {
        auto& main_task_processor = task_processor_container.get_main_task_processor();

        ComponentsBuilder::StatisticsProviderInfo statistics_provider_info;
        statistics_provider_info.statistics_provider = statistics_provider;
        statistics_provider_info.statistics_prefix = "cobrazz";
        auto components_builder = std::make_unique<ComponentsBuilder>(statistics_provider_info);
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
        server_config.server_name = "HttpExpressionMatcher";
        server_config.monitor_listener_config = monitor_listener_config;

        auto& listener_config = server_config.listener_config;
        listener_config.unix_socket_path = "/tmp/http_expression_matcher" + std::to_string(getpid()) + ".sock";
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

      manager_ = new ManagerCoro(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger());
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't initialize coroutine system: "
             << exc.what();
      throw Exception(stream);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      throw Exception(stream);
    }

    /* fill corba_config */
    try
    {
      Config::CorbaConfigReader::read_config(
        config().CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't read Corba Config. : "
        << e.what();
      throw Exception(ostr, "ADS-IMPL-4002");
    }

    try
    {
      /* init CORBA Server */
      corba_server_adapter_ =
        new CORBACommons::CorbaServerAdapter(corba_config_);

      shutdowner_ = corba_server_adapter_->shutdowner();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't init CorbaServerAdapter. : "
        << e.what();
      throw Exception(ostr, "ADS-IMPL-4003");
    }

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
          Logging::Logger_var(new Logging::LoggerDefaultHolder(
            logger(), 0, "ADS-IMPL-4025")),
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
      new AdServer::RequestInfoSvcs::ExpressionMatcherImpl(
        logger(),
        config(),
        proc_stat_impl);
    register_vars_controller();

    corba_server_adapter_->add_binding(
      EXPRESSION_MATCHER_OBJ_KEY, expression_matcher_impl_.in());

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    manager_->activate_object();
    expression_matcher_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    /* Running orb loop */
    corba_server_adapter_->run();

    wait();

    expression_matcher_impl_.reset();
    configuration_.reset();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    manager_->deactivate_object();
    manager_->wait_object();
  }
  catch (const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
                      ASPECT, e.code())
      << "ExpressionMatcherApp_::main(): "
        "Got UserInfoManagerApp_::Exception. : \n"
      << e.what();
  }
  catch (const CORBA::SystemException& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-4004")
      << "ExpressionMatcherApp_::main(): "
        "Got CORBA::SystemException. : \n"
      << e;
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-4005")
      << "ExpressionMatcherApp_::main(): "
        "Got eh::Exception. : \n"
      << e.what();
  }
  catch (...)
  {
    logger()->log(String::SubString("ExpressionMatcherApp_::main(): "
                  "Got Unknown exception."),
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

