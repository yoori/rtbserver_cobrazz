// UNIX_COMMONS
#include <XMLUtility/StringManip.hpp>
#include <XMLUtility/Utility.hpp>

// THIS
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Frontends/HttpServer/Application.hpp>
#include <Frontends/FrontendCommons/FrontendsPool.hpp>
#include <Frontends/HttpServer/Handler.hpp>
#include <Frontends/HttpServer/HttpResponse.hpp>
#include <xsd/AdServerCommons/AdServerCommons.hpp>

namespace
{

const char ASPECT[] = "HttpApplication";
const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
const char SERVER_STATS_OBJ_KEY[] = "HttpServerStats";

}

namespace Configuration = xsd::AdServer::Configuration;

namespace AdServer::Frontends::Http
{

class XMLUtilityScope final : Generics::Uncopyable
{
public:
  XMLUtilityScope()
  {
    XMLUtility::initialize();
  }

  ~XMLUtilityScope()
  {
    XMLUtility::terminate();
  }
};


Application::Application()
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "HttpServer",
      ASPECT),
    stats_(new AdServer::StatHolder),
    composite_metrics_provider_(new Generics::CompositeMetricsProvider)
{
}

void Application::shutdown(Boolean wait_for_completion)
{
  deactivate_object();
  wait_object();

  if (frontend_)
  {
    frontend_->shutdown();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

Application::ALIVE_STATUS Application::is_alive()
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void Application::read_config(
  const char* filename,
  const char* argv0)
{
  try
  {
    Config::ErrorHandler error_handler;

    try
    {
      std::unique_ptr<Configuration::AdConfigurationType> ad_configuration =
        Configuration::AdConfiguration(filename, error_handler);

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      server_config_.reset(new ServerConfig(
        ad_configuration->ServerConfig()));

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }
    }
    catch (const xml_schema::parsing& /*exc*/)
    {
      Stream::Error stream;
      stream << "Can't parse config file '"
             << filename
             << "'. : ";
      if (error_handler.has_errors())
      {
        std::string error_string;
        stream << error_handler.text(error_string);
      }
      throw Exception(stream);
    }

    try
    {
      Config::CorbaConfigReader::read_config(
        server_config_->CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": Can't read Corba Config: "
             << exc.what();
      throw Exception(stream);
    }

    try
    {
      logger(Config::LoggerConfigReader::create(
        server_config_->Logger(),
        argv0));
    }
    catch (const Config::LoggerConfigReader::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": got LoggerConfigReader::Exception: "
             << exc.what();
      throw Exception(stream);
    }

    /*const auto& monitoring = server_config_->Monitoring();
    if (monitoring.present())
    {
      UServerUtils::MetricsHTTPProvider_var metrics_http_provider(
        new UServerUtils::MetricsHTTPProvider(
          composite_metrics_provider_,
          monitoring->port(),
          "/metrics"));

      add_child_object(metrics_http_provider);
    }*/
  }
  catch (const Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": got Exception. Invalid configuration: "
           << exc.what();
    throw Exception(stream);
  }
}

void Application::init_corba()
{
  try
  {
    corba_server_adapter_ = new CORBACommons::CorbaServerAdapter(
      corba_config_);
    shutdowner_ = corba_server_adapter_->shutdowner();
    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

    AdServer::BiddingFrontendStatsImpl_var bidding_frontend_stats =
      new AdServer::BiddingFrontendStatsImpl(stats_);

    corba_server_adapter_->add_binding(
      SERVER_STATS_OBJ_KEY,
      bidding_frontend_stats.in());
  }
  catch(const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    throw Exception(stream);
  }
}

void Application::init_http()
{
  using ModuleIdArray = AdServer::Frontends::FrontendsPool::ModuleIdArray;
  using ServerConfig = UServerUtils::Http::Server::ServerConfig;
  using HttpServerBuilder = UServerUtils::Http::Server::HttpServerBuilder;
  using HttpHandlerConfig = UServerUtils::Http::Server::HandlerConfig;

  try
  {
    auto task_processor_container_builder =
      Config::create_task_processor_container_builder(
        logger(),
        server_config_->Coroutine());

    ModuleIdArray modules;
    const auto& module = server_config_->Module();
    for (auto it = module.begin(); it != module.end(); ++it)
    {
      if (it->name() == "bidding")
      {
        modules.push_back(FrontendsPool::M_BIDDING);
      }
      else if (it->name() == "pubpixel")
      {
        modules.push_back(FrontendsPool::M_PUBPIXEL);
      }
      else if (it->name() == "content")
      {
        modules.push_back(FrontendsPool::M_CONTENT);
      }
      else if (it->name() == "directory")
      {
        modules.push_back(FrontendsPool::M_DIRECTORY);
      }
      else if (it->name() == "webstat")
      {
        modules.push_back(FrontendsPool::M_WEBSTAT);
      }
      else if (it->name() == "action")
      {
        modules.push_back(FrontendsPool::M_ACTION);
      }
      else if (it->name() == "userbind")
      {
        modules.push_back(FrontendsPool::M_USERBIND);
      }
      else if (it->name() == "passback")
      {
        modules.push_back(FrontendsPool::M_PASSBACK);
      }
      else if (it->name() == "passbackpixel")
      {
        modules.push_back(FrontendsPool::M_PASSBACKPIXEL);
      }
      else if (it->name() == "optout")
      {
        modules.push_back(FrontendsPool::M_OPTOUT);
      }
      else if (it->name() == "nullad")
      {
        modules.push_back(FrontendsPool::M_NULLAD);
      }
      else if (it->name() == "adinst")
      {
        modules.push_back(FrontendsPool::M_ADINST);
      }
      else if (it->name() == "click")
      {
        modules.push_back(FrontendsPool::M_CLICK);
      }
      else if (it->name() == "imprtrack")
      {
        modules.push_back(FrontendsPool::M_IMPRTRACK);
      }
      else if (it->name() == "ad")
      {
        modules.push_back(FrontendsPool::M_AD);
      }
      else
      {
        Stream::Error stream;
        stream << FNS
               << "unknown module name '"
               << it->name()
               << "'";
        throw Exception(stream);
      }
    }

    FrontendCommons::HttpResponseFactory_var response_factory(
      new HttpResponseFactory);

    FrontendCommons::Frontend_var frontend(
      new FrontendsPool(
        server_config_->fe_config().data(),
        modules,
        logger(),
        stats_,
        response_factory.in(),
        composite_metrics_provider_));

    auto init_func = [this, frontend = frontend] (TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor = task_processor_container.get_main_task_processor();
      auto components_builder = std::make_unique<ComponentsBuilder>();
      auto& statistic_storage = components_builder->get_statistics_storage();

      const auto& http_servers_config = server_config_->HttpServer();
      std::size_t number = 1;
      for (const auto& http_server_config : http_servers_config)
      {
        ServerConfig server_config;
        server_config.server_name = "HttpServer_" + std::to_string(number);

        auto& listener_config = server_config.listener_config;
        listener_config.max_connections = http_server_config.max_connections();
        listener_config.unix_socket_path = http_server_config.unix_socket_path();

        auto& connection_config = listener_config.connection_config;
        connection_config.keepalive_timeout = std::chrono::seconds{http_server_config.keepalive_timeout_seconds()};
        connection_config.in_buffer_size = http_server_config.in_buffer_size();

        listener_config.handler_defaults = {};

        auto http_server_builder = std::make_unique<HttpServerBuilder>(
          logger(),
          server_config,
          main_task_processor,
          statistic_storage);

        HttpHandlerConfig handler_config;
        handler_config.method = "GET";
        handler_config.path = "/";
        handler_config.response_body_stream = true;

        const std::string handler_get_name = "HttpHandlerGet_" + std::to_string(number);
        HttpHandler_var handler_get(
          new HttpHandler(
            handler_get_name,
            handler_config,
            {},
            logger(),
            frontend.in()));
        http_server_builder->add_handler(
          handler_get.in(),
          main_task_processor);

        handler_config.method = "POST";
        const std::string handler_post_name = "HttpHandlerPost_" + std::to_string(number);
        HttpHandler_var handler_post(
          new HttpHandler(
            handler_post_name,
            handler_config,
            {},
            logger(),
            frontend.in()));
        http_server_builder->add_handler(
          handler_post.in(),
          main_task_processor);

        /*handler_config.method = "HEAD";
        const std::string handler_head_name = "HttpHandlerHead_" + std::to_string(number);
        HttpHandler_var handler_head(
          new HttpHandler(
            handler_head_name,
            handler_config,
            {},
            logger(),
            frontend.in()));
        http_server_builder->add_handler(
          handler_head.in(),
          main_task_processor);*/

        components_builder->add_http_server(std::move(http_server_builder));
        number += 1;
      }

      return components_builder;
    };

    ManagerCoro_var manager(
      new ManagerCoro(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger()));

    add_child_object(manager.in());

    frontend_ = frontend;
    frontend_->init();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    throw Exception(stream);
  }
}

int Application::run(int argc, char** argv)
{
  try
  {
    XMLUtilityScope xml_utility_scope;

    if (argc < 2)
    {
      Stream::Error stream;
      stream << "config file or colocation config file is not specified\n"
                "usage: FCGIServer <config_file>";
      throw Exception(stream);
    }

    try
    {
      read_config(argv[1], argv[0]);
    }
    catch(const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << "Can't parse config file '"
             << argv[1]
             << "': "
             << exc.what();
      throw Exception(stream);
    }
    catch(...)
    {
      Stream::Error stream;
      stream << "Unknown Exception at parsing of config.";
      throw Exception(stream);
    }

    register_vars_controller();
    init_corba();
    init_http();

    activate_object();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    corba_server_adapter_->run();

    wait();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    return EXIT_SUCCESS;
  }
  catch (const Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger()->critical(
      stream.str(),
      ASPECT);
  }
  catch (const CORBA::SystemException& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": Got CORBA::SystemException: "
           << exc;
    logger()->emergency(
      stream.str(),
      ASPECT);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger()->log(stream.str(),
                  Logging::Logger::EMERGENCY,
                  ASPECT);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << ": Unknown exception";
    logger()->log(stream.str(),
                  Logging::Logger::EMERGENCY,
                  ASPECT);
  }

  return EXIT_FAILURE;
}

} // namespace AdServer::Frontends::Http