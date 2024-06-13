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
#include <XMLUtility/Utility.cpp>

// THIS
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Frontends/FrontendCommons/FrontendsPool.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/Statistics.hpp>
#include "Acceptor.hpp"
#include "AcceptorBoostAsio.hpp"
#include "FCGIServer.hpp"

namespace
{
  const char ASPECT[] = "FCGIServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char FCGI_SERVER_STATS_OBJ_KEY[] = "FCGIServerStats";
}

namespace AdServer
{
namespace Frontends
{
  FCGIServer::FCGIServer() /*throw(eh::Exception)*/
    : AdServer::Commons::ProcessControlVarsLoggerImpl(
        "FCGIServer", ASPECT),
      stats_(new StatHolder())
  {
  }

  void
  FCGIServer::shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/
  {
    deactivate_object();
    wait_object();

    if(frontend_pool_)
    {
      frontend_pool_->shutdown();
    }

    CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
  }

  CORBACommons::IProcessControl::ALIVE_STATUS
  FCGIServer::is_alive() /*throw(CORBA::SystemException)*/
  {
    return CORBACommons::ProcessControlImpl::is_alive();
  }

  void
  FCGIServer::read_config_(
    const char *filename,
    const char* argv0)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "FCGIServer::read_config()";

    try
    {
      Config::ErrorHandler error_handler;

      try
      {
        using namespace xsd::AdServer::Configuration;

        std::unique_ptr<AdConfigurationType>
          ad_configuration = AdConfiguration(filename, error_handler);

        if (error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }

        config_.reset(
          new FCGIServerConfig(ad_configuration->FCGIServerConfig()));

        if (error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }
      }
      catch (const xml_schema::parsing &ex)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << filename << "'. : ";
        if (error_handler.has_errors())
        {
          std::string error_string;
          ostr << error_handler.text(error_string);
        }
        throw Exception(ostr);
      }

      try
      {
        Config::CorbaConfigReader::read_config(
          config_->CorbaConfig(),
          corba_config_);
      }
      catch(const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't read Corba Config: " << ex.what();
        throw Exception(ostr);
      }

      try
      {
        logger(Config::LoggerConfigReader::create(
                 config_->Logger(), argv0));
      }
      catch (const Config::LoggerConfigReader::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
    catch (const Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception. Invalid configuration: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  FCGIServer::init_corba_() /*throw(Exception)*/
  {
    try
    {
      corba_server_adapter_ =
        new CORBACommons::CorbaServerAdapter(corba_config_);
      shutdowner_ = corba_server_adapter_->shutdowner();
      corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

      AdServer::BiddingFrontendStatsImpl_var
        bidding_frontend_stats_impl =
        new AdServer::BiddingFrontendStatsImpl(
          stats_);
    
      corba_server_adapter_->add_binding(
        FCGI_SERVER_STATS_OBJ_KEY, bidding_frontend_stats_impl.in());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "FCGIServer::init_corba(): "
        "Can't init CorbaServerAdapter: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  FCGIServer::init_fcgi_() /*throw(Exception)*/
  {
    using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
    using ManagerCoro = UServerUtils::Grpc::Manager;
    using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
    using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
    using HttpServerConfig = UServerUtils::Http::Server::ServerConfig;
    using HttpListenerConfig = UServerUtils::Http::Server::ListenerConfig;
    using HttpServerBuilder = UServerUtils::Http::Server::HttpServerBuilder;
    using SchedulerPtr = UServerUtils::Grpc::Core::Common::SchedulerPtr;

    static const char* FUN = "FCGIServer::init_fcgi_()";

    ManagerCoro_var manager;
    try
    {
      auto task_processor_container_builder =
        Config::create_task_processor_container_builder(
          logger(),
          config_->Coroutine());
      auto statistics_provider = std::make_shared<
        UServerUtils::Statistics::CompositeStatisticsProviderImpl<
          std::shared_mutex>>(logger());
      auto time_statistics_provider = FrontendCommons::get_time_statistics_provider();
      statistics_provider->add(time_statistics_provider);
      auto common_counter_statistics_provider = FrontendCommons::get_common_counter_statistics_provider();
      statistics_provider->add(common_counter_statistics_provider);

      auto init_func = [this, statistics_provider] (TaskProcessorContainer& task_processor_container) {
        auto& main_task_processor = task_processor_container.get_main_task_processor();

        ComponentsBuilder::StatisticsProviderInfo statistics_provider_info;
        statistics_provider_info.statistics_provider = statistics_provider;
        statistics_provider_info.statistics_prefix = "cobrazz";
        auto components_builder = std::make_unique<ComponentsBuilder>(
          statistics_provider_info);
        auto& statistic_storage = components_builder->get_statistics_storage();

        const auto& monitoring_config = config_->Monitoring();
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
        listener_config.unix_socket_path = "/tmp/http_fcgi.sock";
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

      manager = new ManagerCoro(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger());

      add_child_object(manager.in());
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

    try
    {
      FrontendsPool::ModuleIdArray modules;

      for(auto module_it = config_->Module().begin();
        module_it != config_->Module().end(); ++module_it)
      {
        if(module_it->name() == "bidding")
        {
          modules.push_back(FrontendsPool::M_BIDDING);
        }
        else if(module_it->name() == "pubpixel")
        {
          modules.push_back(FrontendsPool::M_PUBPIXEL);
        }
        else if(module_it->name() == "content")
        {
          modules.push_back(FrontendsPool::M_CONTENT);
        }
        else if(module_it->name() == "directory")
        {
          modules.push_back(FrontendsPool::M_DIRECTORY);
        }
        else if(module_it->name() == "webstat")
        {
          modules.push_back(FrontendsPool::M_WEBSTAT);
        }
        else if(module_it->name() == "action")
        {
          modules.push_back(FrontendsPool::M_ACTION);
        }
        else if(module_it->name() == "userbind")
        {
          modules.push_back(FrontendsPool::M_USERBIND);
        }
        else if(module_it->name() == "passback")
        {
          modules.push_back(FrontendsPool::M_PASSBACK);
        }
        else if(module_it->name() == "passbackpixel")
        {
          modules.push_back(FrontendsPool::M_PASSBACKPIXEL);
        }
        else if(module_it->name() == "optout")
        {
          modules.push_back(FrontendsPool::M_OPTOUT);
        }
        else if(module_it->name() == "nullad")
        {
          modules.push_back(FrontendsPool::M_NULLAD);
        }
        else if(module_it->name() == "adinst")
        {
          modules.push_back(FrontendsPool::M_ADINST);
        }
        else if(module_it->name() == "click")
        {
          modules.push_back(FrontendsPool::M_CLICK);
        }
        else if(module_it->name() == "imprtrack")
        {
          modules.push_back(FrontendsPool::M_IMPRTRACK);
        }
        else if(module_it->name() == "ad")
        {
          modules.push_back(FrontendsPool::M_AD);
        }
        else
        {
          Stream::Error ostr;
          ostr << "unknown module name '" << module_it->name() << "'";
          throw Exception(ostr);
        }
      }

      FrontendCommons::HttpResponseFactory_var response_factory(
        new FCGI::HttpResponseFactory);

      auto number_scheduler_threads = std::thread::hardware_concurrency();
      if (number_scheduler_threads == 0)
      {
        Stream::Error ostr;
        ostr << FNS
             << "hardware_concurrency is failed";
        throw Exception(ostr);
      }
      SchedulerPtr scheduler = UServerUtils::Grpc::Core::Common::Utils::create_scheduler(
        number_scheduler_threads,
        logger());

      FrontendCommons::Frontend_var frontend_pool = new FrontendsPool(
        manager->get_main_task_processor(),
        scheduler,
        config_->fe_config().data(),
        modules,
        logger(),
        stats_,
        response_factory.in());
      frontend_pool_ = frontend_pool;
      frontend_pool_->init();

      for(auto bind_it = config_->BindSocket().begin(); bind_it != config_->BindSocket().end();
        ++bind_it)
      {
        /*
        add_child_object(
          Generics::ActiveObject_var(
            new Acceptor(
              logger(),
              frontend_pool,
              callback(),
              bind_it->bind().data(),
              bind_it->backlog(),
              bind_it->accept_threads())));
        */
        add_child_object(
          Generics::ActiveObject_var(
            new AcceptorBoostAsio(
              logger(),
              frontend_pool,
              callback(),
              bind_it->bind(), // bind_it->bind().data(),
              bind_it->backlog(),
              bind_it->accept_threads())));
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  FCGIServer::main(int& argc, char** argv) noexcept
  {
    static const char* FUN = "FCGIServer::main()";

    try
    {
      XMLUtility::initialize();
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-205") << FUN << ": Got eh::Exception: " << ex.what();
      return;
    }

    try
    {
      if (argc < 2)
      {
        Stream::Error ostr;
        ostr << "config file or colocation config file is not specified\n"
          "usage: FCGIServer <config_file>";
        throw Exception(ostr);
      }

      try
      {
        read_config_(argv[1], argv[0]);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << argv[1] << "': " <<
          ex.what();
        throw Exception(ostr);
      }
      catch(...)
      {
        Stream::Error ostr;
        ostr << "Unknown Exception at parsing of config.";
        throw Exception(ostr);
      }

      register_vars_controller();
      init_corba_();
      init_fcgi_();
      activate_object();
      logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
      corba_server_adapter_->run();

      wait();
      logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
      XMLUtility::terminate();
    }
    catch (const Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got BiddingFCGIServerApp_::Exception: " <<
        e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got CORBA::SystemException: " << e;
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got eh::Exception: " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got unknown exception";
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
  }
} // Frontends
} // AdServer

int
main(int argc, char** argv)
{
  AdServer::Frontends::FCGIServer* app = 0;

  try
  {
    app = &AdServer::Frontends::FCGIServerApp::instance();
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

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
