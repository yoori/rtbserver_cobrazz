#include <eh/Exception.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcService.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <UserInfoSvcs/UserInfoCommons/Statistics.hpp>

#include "UserBindServerMain.hpp"

// PROTOBUF
#include "UserBindServer_service.cobrazz.pb.hpp"

// UNIXCOMMONS
#include <UServerUtils/Grpc/Server/Config.hpp>
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>
#include <UServerUtils/Statistics/CompositeStatisticsProvider.hpp>
#include <UServerUtils/Config.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>

namespace
{
  const char ASPECT[] = "UserBindServer";
  const char USER_BIND_SERVER_OBJ_KEY[] = "UserBindServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

UserBindServerApp_::UserBindServerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "UserBindServerApp_", ASPECT)
{}

void
UserBindServerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  deactivate_object();
  wait_object();

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
UserBindServerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
UserBindServerApp_::main(int& argc, char** argv)
  noexcept
{
  using ComponentsBuilder = UServerUtils::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Manager;
  using ManagerCoro_var = UServerUtils::Manager_var;
  using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
  using HttpServerConfig = UServerUtils::Http::Server::ServerConfig;
  using HttpListenerConfig = UServerUtils::Http::Server::ListenerConfig;
  using HttpServerBuilder = UServerUtils::Http::Server::HttpServerBuilder;
  using ServiceMode = UServerUtils::Grpc::Server::ServiceMode;
  using SecdistConfig = userver::storages::secdist::SecdistConfig;

  static const char* FUN = "UserBindServerApp_::main()";
  
  try
  {
    static const char* USAGE = "usage: UserBindServer <config_file>";

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

      configuration_ = ConfigPtr(new UserBindServerConfigType(
        ad_configuration->UserBindServerConfig()));
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
      ostr << "Can't read Corba Config: " << e.what();
      throw Exception(ostr);
    }

    // Creating user info manager servant
    user_bind_server_impl_ = 
      new AdServer::UserInfoSvcs::UserBindServerImpl(
        callback(),
        logger(),
        config());

    add_child_object(user_bind_server_impl_);

    Generics::TaskPool_var task_pool = new Generics::TaskPool(
      callback(),
      configuration_->number_grpc_helper_threads(),
      1024 * 1024 // stack_size
    );
    add_child_object(task_pool);

    // Creating coroutine manager
    auto task_processor_container_builder =
      Config::create_task_processor_container_builder(
        logger(),
        config().Coroutine());

    auto time_statistics_provider = AdServer::UserInfoSvcs::get_time_statistics_provider();
    auto statistics_provider = std::make_shared<
      UServerUtils::Statistics::CompositeStatisticsProviderImpl<
        std::shared_mutex>>(logger());
    statistics_provider->add(time_statistics_provider);

    auto init_func = [this, statistics_provider, task_pool] (
      TaskProcessorContainer& task_processor_container) mutable {
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
          UServerUtils::Http::Server::PortConfig port_config;
          port_config.port = monitoring_config->port();

          HttpListenerConfig config;
          config.ports.emplace_back(std::move(port_config));
          monitor_listener_config = config;
        }

        HttpServerConfig server_config;
        server_config.server_name = "HttpUserBindServer";
        server_config.monitor_listener_config = monitor_listener_config;

        UServerUtils::Http::Server::PortConfig port_config;
        port_config.unix_socket_path =
          "/tmp/http_user_bind_server" + std::to_string(getpid()) + ".sock";

        auto& listener_config = server_config.listener_config;
        listener_config.ports.emplace_back(std::move(port_config));
        listener_config.handler_defaults = {};

        auto& connection_config = listener_config.connection_config;
        connection_config.keepalive_timeout = std::chrono::seconds{10};

        SecdistConfig sec_dist_config;
        auto http_server_builder = std::make_unique<HttpServerBuilder>(
          logger(),
          server_config,
          sec_dist_config,
          main_task_processor,
          statistic_storage);
        components_builder->add_http_server(std::move(http_server_builder));

        auto grpc_server_builder = Config::create_grpc_server_builder(
          logger(),
          config().GrpcServer());

        // GrpcService only for ServiceMode::EventToCoroutine(optimisation reason)
        ServiceMode service_mode = ServiceMode::EventToCoroutine;

        auto get_bind_request_service = AdServer::Commons::create_grpc_service<
          AdServer::UserInfoSvcs::UserBindService_get_bind_request_Service,
          AdServer::UserInfoSvcs::UserBindServerImpl,
          &AdServer::UserInfoSvcs::UserBindServerImpl::get_bind_request>(
            logger(),
            user_bind_server_impl_.in(),
            task_pool.in());

        grpc_server_builder->add_service(
          get_bind_request_service.in(),
          main_task_processor,
          service_mode);

        auto add_bind_request_service = AdServer::Commons::create_grpc_service<
          AdServer::UserInfoSvcs::UserBindService_add_bind_request_Service,
          AdServer::UserInfoSvcs::UserBindServerImpl,
          &AdServer::UserInfoSvcs::UserBindServerImpl::add_bind_request>(
          logger(),
          user_bind_server_impl_.in(),
          task_pool.in());

        grpc_server_builder->add_service(
          add_bind_request_service.in(),
          main_task_processor,
          service_mode);

        auto get_user_id_service = AdServer::Commons::create_grpc_service<
          AdServer::UserInfoSvcs::UserBindService_get_user_id_Service,
          AdServer::UserInfoSvcs::UserBindServerImpl,
          &AdServer::UserInfoSvcs::UserBindServerImpl::get_user_id>(
          logger(),
          user_bind_server_impl_.in(),
          task_pool.in());

        grpc_server_builder->add_service(
          get_user_id_service.in(),
          main_task_processor,
          service_mode);

        auto add_user_id_service = AdServer::Commons::create_grpc_service<
          AdServer::UserInfoSvcs::UserBindService_add_user_id_Service,
          AdServer::UserInfoSvcs::UserBindServerImpl,
          &AdServer::UserInfoSvcs::UserBindServerImpl::add_user_id>(
            logger(),
            user_bind_server_impl_.in(),
            task_pool.in());

        grpc_server_builder->add_service(
          add_user_id_service.in(),
          main_task_processor,
          service_mode);

        auto get_source_service = AdServer::Commons::create_grpc_service<
          AdServer::UserInfoSvcs::UserBindService_get_source_Service,
          AdServer::UserInfoSvcs::UserBindServerImpl,
          &AdServer::UserInfoSvcs::UserBindServerImpl::get_source>(
            logger(),
            user_bind_server_impl_.in(),
            task_pool.in());

        grpc_server_builder->add_service(
          get_source_service.in(),
          main_task_processor,
          service_mode);

        components_builder->add_grpc_cobrazz_server(
          std::move(grpc_server_builder));

        return components_builder;
      };

    ManagerCoro_var manager_coro = new ManagerCoro(
      std::move(task_processor_container_builder),
      std::move(init_func),
      logger());

    add_child_object(manager_coro);

    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

    corba_server_adapter_->add_binding(
      USER_BIND_SERVER_OBJ_KEY, user_bind_server_impl_.in());

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    shutdowner_ = corba_server_adapter_->shutdowner();

    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    // Running orb loop
    corba_server_adapter_->run();
    wait();
  }
  catch (const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
      ASPECT,
      "ADS-IMPL-58") << FUN <<
      ": Got UserBindServerApp_::Exception: " << e.what();
  }
  catch (const CORBA::SystemException& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-59") << FUN <<
      ": Got CORBA::SystemException: " << e;
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-59") << FUN <<
      ": Got eh::Exception: " << e.what();
  }

  logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
}

int
main(int argc, char** argv)
{
  UserBindServerApp_* app = 0;

  try
  {
    app = &UserBindServerApp::instance();
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

