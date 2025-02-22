// UNIX_COMMONS
#include <XMLUtility/Utility.hpp>
#include <UServerUtils/Statistics/CompositeStatisticsProvider.hpp>

// THIS
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Frontends/HttpServer/Application.hpp>
#include <Frontends/FrontendCommons/FrontendsPool.hpp>
#include <Frontends/FrontendCommons/Statistics.hpp>
#include <Frontends/HttpServer/Handler.hpp>
#include <Frontends/HttpServer/HttpResponse.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace
{

inline constexpr char ASPECT[] = "HttpApplication";
inline constexpr char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
inline constexpr char SERVER_STATS_OBJ_KEY[] = "HttpServerStats";
inline constexpr char HELPER_TASK_PROCESSOR_NAME[] = "HelperTaskProcessorName";
inline constexpr char FRONTENDS_POOL_COMPONENT_NAME[] = "FrontendsPoolComponent";

} // namespace

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
    try
    {
      XMLUtility::terminate();
    }
    catch (...)
    {
    }
  }
};

Application::Application()
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "HttpServer",
      ASPECT),
    stats_(new AdServer::StatHolder)
{
}

void Application::shutdown(Boolean wait_for_completion)
{
  deactivate_object();
  wait_object();

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
  using ListenerConfig = UServerUtils::Http::Server::ListenerConfig;
  using SecdistConfig = userver::storages::secdist::SecdistConfig;

  ManagerCoro_var manager_coro;
  try
  {
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
      else if (it->name() == "echo")
      {
        modules.push_back(FrontendsPool::M_ECHO);
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

    auto number_scheduler_threads = std::thread::hardware_concurrency();
    if (number_scheduler_threads == 0)
    {
      Stream::Error stream;
      stream << FNS
             << "hardware_concurrency is failed";
      throw Exception(stream);
    }

    SchedulerPtr scheduler = UServerUtils::Grpc::Common::Utils::create_scheduler(
      number_scheduler_threads,
      logger());

    FrontendCommons::HttpResponseFactory_var response_factory(
      new HttpResponseFactory);

    auto init_func = [
      this,
      modules = std::move(modules),
      scheduler = std::move(scheduler),
      response_factory = std::move(response_factory)] (
        TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor = task_processor_container.get_main_task_processor();
      auto& helper_task_processor = task_processor_container.get_task_processor(
        HELPER_TASK_PROCESSOR_NAME);

      auto grpc_channel_operation_pool = create_grpc_channel_operation_pool(
        scheduler,
        main_task_processor);
      auto grpc_campaign_manager_pool = create_grpc_campaign_manager_pool(
        scheduler,
        main_task_processor);
      auto grpc_user_bind_operation_distributor = create_grpc_user_bind_operation_distributor(
        scheduler,
        main_task_processor);
      auto grpc_user_info_operation_distributor = create_grpc_user_info_operation_distributor(
        scheduler,
        main_task_processor);

      auto grpc_container = std::make_shared<FrontendCommons::GrpcContainer>();
      grpc_container->grpc_channel_operation_pool = grpc_channel_operation_pool;
      grpc_container->grpc_campaign_manager_pool = grpc_campaign_manager_pool;
      grpc_container->grpc_user_bind_operation_distributor = grpc_user_bind_operation_distributor;
      grpc_container->grpc_user_info_operation_distributor = grpc_user_info_operation_distributor;

      auto statistics_provider = std::make_shared<
        UServerUtils::Statistics::CompositeStatisticsProviderImpl<
          userver::engine::SharedMutex>>(logger());
      auto time_statistics_provider = FrontendCommons::get_time_statistics_provider();
      statistics_provider->add(time_statistics_provider);
      auto common_counter_statistics_provider = FrontendCommons::get_common_counter_statistics_provider();
      statistics_provider->add(common_counter_statistics_provider);

      ComponentsBuilder::StatisticsProviderInfo statistics_provider_info;
      statistics_provider_info.statistics_provider = statistics_provider;
      statistics_provider_info.statistics_prefix = "cobrazz";
      auto components_builder = std::make_unique<ComponentsBuilder>(statistics_provider_info);

      FrontendsPool_var frontends_pool = new FrontendsPool(
        helper_task_processor,
        FrontendsPool::ServerType::HTTP,
        grpc_container,
        server_config_->fe_config().data(),
        modules,
        logger(),
        stats_,
        response_factory.in());
      components_builder->add_user_component(
        FRONTENDS_POOL_COMPONENT_NAME,
        frontends_pool.in());

      const auto monitoring_config = server_config_->Monitoring();
      std::optional<ListenerConfig> monitor_listener_config;
      if (monitoring_config.present())
      {
        UServerUtils::Http::Server::PortConfig port_config;
        port_config.port = monitoring_config->port();

        ListenerConfig config;
        config.ports.emplace_back(std::move(port_config));
        monitor_listener_config = config;
      }

      const auto& http_servers_config = server_config_->HttpServer();
      std::size_t number = 1;
      for (const auto& http_server_config : http_servers_config)
      {
        ServerConfig server_config;
        server_config.server_name = "HttpServer_" + std::to_string(number);
        if (number == 1)
        {
          server_config.monitor_listener_config = monitor_listener_config;
        }

        UServerUtils::Http::Server::PortConfig port_config;
        port_config.unix_socket_path = http_server_config.unix_socket_path();

        auto& listener_config = server_config.listener_config;
        listener_config.max_connections = http_server_config.max_connections();
        listener_config.ports.emplace_back(std::move(port_config));

        auto& connection_config = listener_config.connection_config;
        connection_config.keepalive_timeout = std::chrono::seconds{
          http_server_config.keepalive_timeout_seconds()};
        connection_config.in_buffer_size = http_server_config.in_buffer_size();

        listener_config.handler_defaults = {};

        auto& statistic_storage = components_builder->get_statistics_storage();
        SecdistConfig sec_dist_config;
        auto http_server_builder = std::make_unique<HttpServerBuilder>(
          logger(),
          server_config,
          sec_dist_config,
          main_task_processor,
          statistic_storage);

        HttpHandlerConfig handler_config;
        handler_config.method = "GET";
        handler_config.path = "/*";
        handler_config.response_body_stream = true;

        const std::string handler_get_name = "HttpHandlerGet_" + std::to_string(number);
        HttpHandler_var handler_get = new HttpHandler(
          handler_get_name,
          handler_config,
          {},
          logger(),
          frontends_pool.in());
        http_server_builder->add_handler(
          handler_get.in(),
          main_task_processor);

        handler_config.method = "POST";
        const std::string handler_post_name = "HttpHandlerPost_" + std::to_string(number);
        HttpHandler_var handler_post = new HttpHandler(
          handler_post_name,
          handler_config,
          {},
          logger(),
          frontends_pool.in());
        http_server_builder->add_handler(
          handler_post.in(),
          main_task_processor);

        components_builder->add_http_server(std::move(http_server_builder));
        number += 1;
      }

      return components_builder;
    };

    auto task_processor_container_builder =
      Config::create_task_processor_container_builder(
        logger(),
        server_config_->Coroutine());

    UServerUtils::TaskProcessorConfig helper_task_processor_config;
    helper_task_processor_config.name = HELPER_TASK_PROCESSOR_NAME;
    helper_task_processor_config.overload_action =
      UServerUtils::TaskProcessorConfig::OverloadAction::Ignore;
    helper_task_processor_config.worker_threads = 5;
    task_processor_container_builder->add_task_processor(
      helper_task_processor_config);

    manager_coro = new ManagerCoro(
      std::move(task_processor_container_builder),
      std::move(init_func),
      logger());

    add_child_object(manager_coro.in());
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    throw Exception(stream);
  }
}

Application::GrpcChannelOperationPoolPtr
Application::create_grpc_channel_operation_pool(
  const SchedulerPtr& scheduler,
  TaskProcessor& task_processor)
{
  using Host = std::string;
  using Port = std::size_t;
  using Endpoint = std::pair<Host, Port>;
  using Endpoints = std::vector<Endpoint>;

  std::shared_ptr<GrpcChannelOperationPool> grpc_channel_operation_pool;
  if (server_config_->ChannelGrpcClientPool().enable())
  {
    Endpoints endpoints;
    const auto& endpoints_config = server_config_->ChannelServerEndpointList().Endpoint();
    for (const auto& endpoint_config : endpoints_config)
    {
      endpoints.emplace_back(
        endpoint_config.host(),
        endpoint_config.port());
    }

    const auto config_grpc_data = Config::create_pool_client_config(
      server_config_->ChannelGrpcClientPool());
    grpc_channel_operation_pool = std::make_shared<GrpcChannelOperationPool>(
      logger(),
      task_processor,
      scheduler,
      endpoints,
      config_grpc_data.first,
      config_grpc_data.second,
      server_config_->time_duration_grpc_client_mark_bad());
  }

  return grpc_channel_operation_pool;
}

Application::GrpcCampaignManagerPoolPtr
Application::create_grpc_campaign_manager_pool(
  const SchedulerPtr& scheduler,
  TaskProcessor& task_processor)
{
  using Endpoints = FrontendCommons::GrpcCampaignManagerPool::Endpoints;

  std::shared_ptr<GrpcCampaignManagerPool> grpc_campaign_manager_pool;
  if (server_config_->CampaignGrpcClientPool().enable())
  {
    Endpoints endpoints;
    const auto& endpoints_config = server_config_->CampaignManagerEndpointList().Endpoint();
    for (const auto& endpoint_config : endpoints_config)
    {
      if (!endpoint_config.service_index().present())
      {
        Stream::Error stream;
        stream << FNS
               << "Service index not exist in CampaignManagerEndpointList";
        throw Exception(stream);
      }

      const std::string host = endpoint_config.host();
      const std::size_t port = endpoint_config.port();
      const std::string service_id = *endpoint_config.service_index();
      endpoints.emplace_back(
        host,
        port,
        service_id);
    }

    const auto config_grpc_data = Config::create_pool_client_config(
      server_config_->CampaignGrpcClientPool());
    grpc_campaign_manager_pool = std::make_shared<GrpcCampaignManagerPool>(
      logger(),
      task_processor,
      scheduler,
      endpoints,
      config_grpc_data.first,
      config_grpc_data.second,
      server_config_->time_duration_grpc_client_mark_bad());
  }

  return grpc_campaign_manager_pool;
}

Application::GrpcUserBindOperationDistributor_var
Application::create_grpc_user_bind_operation_distributor(
  const SchedulerPtr& scheduler,
  TaskProcessor& task_processor)
{
  const std::string fe_config_path = server_config_->fe_config();
  Config::ErrorHandler error_handler;
  const auto fe_config = xsd::AdServer::Configuration::FeConfiguration(
    fe_config_path.c_str(),
    error_handler);
  if (error_handler.has_errors())
  {
    std::string error_string;
    throw Exception(error_handler.text(error_string));
  }

  if (!fe_config->CommonFeConfiguration().present())
  {
    Stream::Error stream;
    stream << FNS
           << "CommonFeConfiguration not presented";
    throw Exception(stream);
  }
  const auto& common_fe_config = *fe_config->CommonFeConfiguration();

  GrpcUserBindOperationDistributor_var grpc_user_bind_operation_distributor;
  if (server_config_->UserBindGrpcClientPool().enable())
  {
    using ControllerRefList = AdServer::UserInfoSvcs::UserBindOperationDistributor::ControllerRefList;
    using ControllerRef = AdServer::UserInfoSvcs::UserBindOperationDistributor::ControllerRef;

    ControllerRefList controller_groups;
    const auto& user_bind_controller_group = common_fe_config.UserBindControllerGroup();
    for (auto cg_it = std::begin(user_bind_controller_group);
         cg_it != std::end(user_bind_controller_group);
         ++cg_it)
    {
      ControllerRef controller_ref_group;
      Config::CorbaConfigReader::read_multi_corba_ref(
        *cg_it,
        controller_ref_group);
      controller_groups.push_back(controller_ref_group);
    }

    CORBACommons::CorbaClientAdapter_var corba_client_adapter = new CORBACommons::CorbaClientAdapter();
    const auto config_grpc_data = Config::create_pool_client_config(
      server_config_->UserBindGrpcClientPool());

    grpc_user_bind_operation_distributor = new GrpcUserBindOperationDistributor(
      logger(),
      task_processor,
      scheduler,
      controller_groups,
      corba_client_adapter.in(),
      config_grpc_data.first,
      config_grpc_data.second,
      Generics::Time::ONE_SECOND);
  }

  return grpc_user_bind_operation_distributor;
}

Application::GrpcUserInfoOperationDistributor_var
Application::create_grpc_user_info_operation_distributor(
  const SchedulerPtr& scheduler,
  TaskProcessor& task_processor)
{
  const std::string fe_config_path = server_config_->fe_config();
  Config::ErrorHandler error_handler;
  const auto fe_config = xsd::AdServer::Configuration::FeConfiguration(
    fe_config_path.c_str(),
    error_handler);
  if (error_handler.has_errors())
  {
    std::string error_string;
    throw Exception(error_handler.text(error_string));
  }

  if (!fe_config->CommonFeConfiguration().present())
  {
    Stream::Error stream;
    stream << FNS
           << "CommonFeConfiguration not presented";
    throw Exception(stream);
  }
  const auto& common_fe_config = *fe_config->CommonFeConfiguration();

  GrpcUserInfoOperationDistributor_var grpc_user_info_operation_distributor;
  if (server_config_->UserInfoGrpcClientPool().enable())
  {
    using ControllerRefList = AdServer::UserInfoSvcs::UserInfoOperationDistributor::ControllerRefList;
    using ControllerRef = AdServer::UserInfoSvcs::UserInfoOperationDistributor::ControllerRef;

    ControllerRefList controller_groups;
    const auto& user_info_controller_group = common_fe_config.UserInfoManagerControllerGroup();
    for (auto cg_it = std::begin(user_info_controller_group);
         cg_it != std::end(user_info_controller_group);
         ++cg_it)
    {
      ControllerRef controller_ref_group;
      Config::CorbaConfigReader::read_multi_corba_ref(
        *cg_it,
        controller_ref_group);
      controller_groups.push_back(controller_ref_group);
    }

    CORBACommons::CorbaClientAdapter_var corba_client_adapter = new CORBACommons::CorbaClientAdapter();
    const auto config_grpc_data = Config::create_pool_client_config(
      server_config_->UserInfoGrpcClientPool());

    grpc_user_info_operation_distributor = new GrpcUserInfoOperationDistributor(
      logger(),
      task_processor,
      scheduler,
      controller_groups,
      corba_client_adapter.in(),
      config_grpc_data.first,
      config_grpc_data.second,
      Generics::Time::ONE_SECOND);
  }

  return grpc_user_info_operation_distributor;
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
                "usage: HttpAdServerConfig <config_file>";
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
    logger()->critical(stream.str(), ASPECT);
  }
  catch (const CORBA::SystemException& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": Got CORBA::SystemException: "
           << exc;
    logger()->emergency(stream.str(), ASPECT);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger()->log(stream.str(), Logging::Logger::EMERGENCY, ASPECT);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << ": Unknown exception";
    logger()->log(stream.str(), Logging::Logger::EMERGENCY, ASPECT);
  }

  return EXIT_FAILURE;
}

} // namespace AdServer::Frontends::Http