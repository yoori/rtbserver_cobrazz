// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <CORBACommons/StatsImpl.hpp>
#include <SNMPAgent/SNMPAgentX.hpp>
#include <UServerUtils/Grpc/Server/Config.hpp>
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>
#include <UServerUtils/Statistics/CompositeStatisticsProvider.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>
#include <XMLUtility/Utility.cpp>

// THIS
#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/FrontendsPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/FrontendCommons/Statistics.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>
#include <xsd/Frontends/FeConfig.hpp>
#include "AcceptorBoostAsio.hpp"
#include "FCGIServer.hpp"

namespace
{
  const char ASPECT[] = "FCGIServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char FCGI_SERVER_STATS_OBJ_KEY[] = "FCGIServerStats";
  const char HELPER_TASK_PROCESSOR_NAME[] = "HelperTaskProcessorName";
}

namespace AdServer::Frontends
{
  FCGIServer::FCGIServer() /*throw(eh::Exception)*/
    : AdServer::Commons::ProcessControlVarsLoggerImpl(
        "FCGIServer", ASPECT),
      stats_(new StatHolder())
  {
  }

  void FCGIServer::shutdown(
    CORBA::Boolean wait_for_completion)
  {
    deactivate_object();
    wait_object();

    CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
  }

  CORBACommons::IProcessControl::ALIVE_STATUS
  FCGIServer::is_alive()
  {
    return CORBACommons::ProcessControlImpl::is_alive();
  }

  void FCGIServer::read_config_(
    const char *filename,
    const char* argv0)
  {
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
        ostr << FNS
             << "Can't parse config file '"
             << filename << "'. : ";
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
      catch (const eh::Exception& exc)
      {
        Stream::Error ostr;
        ostr << FNS
             << "Can't read Corba Config: "
             << exc.what();
        throw Exception(ostr);
      }

      try
      {
        logger(Config::LoggerConfigReader::create(
                 config_->Logger(), argv0));
      }
      catch (const Config::LoggerConfigReader::Exception& exc)
      {
        Stream::Error ostr;
        ostr << FNS
             << "Got LoggerConfigReader::Exception: "
             << exc.what();
        throw Exception(ostr);
      }
    }
    catch (const Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Got Exception. Invalid configuration: "
           << exc.what();
      throw Exception(ostr);
    }
  }

  void FCGIServer::init_corba_()
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
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Can't init CorbaServerAdapter: "
           << exc.what();
      throw Exception(ostr);
    }
  }

  void FCGIServer::init_fcgi_()
  {
    using Host = std::string;
    using Port = std::size_t;
    using Endpoint = std::pair<Host, Port>;
    using Endpoints = std::vector<Endpoint>;
    using ComponentsBuilder = UServerUtils::ComponentsBuilder;
    using ManagerCoro = UServerUtils::Manager;
    using ManagerCoro_var = UServerUtils::Manager_var;
    using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
    using HttpServerConfig = UServerUtils::Http::Server::ServerConfig;
    using HttpListenerConfig = UServerUtils::Http::Server::ListenerConfig;
    using HttpServerBuilder = UServerUtils::Http::Server::HttpServerBuilder;
    using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
    using GrpcChannelOperationPool = AdServer::ChannelSvcs::GrpcChannelOperationPool;
    using GrpcCampaignManagerPool = FrontendCommons::GrpcCampaignManagerPool;
    using GrpcUserBindOperationDistributor = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor;
    using GrpcUserBindOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var;
    using GrpcUserInfoOperationDistributor = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor;
    using GrpcUserInfoOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var;

    ManagerCoro_var manager_coro;
    userver::engine::TaskProcessor* helper_task_processor;
    try
    {
      auto init_func = [this] (TaskProcessorContainer& task_processor_container) {
        auto& main_task_processor = task_processor_container.get_main_task_processor();

        auto statistics_provider = std::make_shared<
          UServerUtils::Statistics::CompositeStatisticsProviderImpl<
            std::shared_mutex>>(logger());
        auto time_statistics_provider = FrontendCommons::get_time_statistics_provider();
        statistics_provider->add(time_statistics_provider);
        auto common_counter_statistics_provider = FrontendCommons::get_common_counter_statistics_provider();
        statistics_provider->add(common_counter_statistics_provider);

        ComponentsBuilder::StatisticsProviderInfo statistics_provider_info;
        statistics_provider_info.statistics_provider = statistics_provider;
        statistics_provider_info.statistics_prefix = "cobrazz";
        auto components_builder = std::make_unique<ComponentsBuilder>(statistics_provider_info);
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
        listener_config.unix_socket_path = "/tmp/http_fcgi" + std::to_string(getpid()) + ".sock";
        listener_config.handler_defaults = {};

        auto http_server_builder = std::make_unique<HttpServerBuilder>(
          logger(),
          server_config,
          main_task_processor,
          statistic_storage);
        components_builder->add_http_server(std::move(http_server_builder));

        return components_builder;
      };

      auto task_processor_container_builder =
        Config::create_task_processor_container_builder(
          logger(),
          config_->Coroutine());

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

      helper_task_processor = &manager_coro->get_task_processor(
        HELPER_TASK_PROCESSOR_NAME);

      add_child_object(manager_coro.in());
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Can't initialize coroutine system: "
           << exc.what();
      throw Exception(ostr);
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Unknown error";
      throw Exception(ostr);
    }

    try
    {
      FrontendsPool::ModuleIdArray modules;

      for (auto module_it = config_->Module().begin();
        module_it != config_->Module().end(); ++module_it)
      {
        if (module_it->name() == "bidding")
        {
          modules.push_back(FrontendsPool::M_BIDDING);
        }
        else if (module_it->name() == "pubpixel")
        {
          modules.push_back(FrontendsPool::M_PUBPIXEL);
        }
        else if (module_it->name() == "content")
        {
          modules.push_back(FrontendsPool::M_CONTENT);
        }
        else if (module_it->name() == "directory")
        {
          modules.push_back(FrontendsPool::M_DIRECTORY);
        }
        else if (module_it->name() == "webstat")
        {
          modules.push_back(FrontendsPool::M_WEBSTAT);
        }
        else if (module_it->name() == "action")
        {
          modules.push_back(FrontendsPool::M_ACTION);
        }
        else if (module_it->name() == "userbind")
        {
          modules.push_back(FrontendsPool::M_USERBIND);
        }
        else if (module_it->name() == "passback")
        {
          modules.push_back(FrontendsPool::M_PASSBACK);
        }
        else if (module_it->name() == "passbackpixel")
        {
          modules.push_back(FrontendsPool::M_PASSBACKPIXEL);
        }
        else if (module_it->name() == "optout")
        {
          modules.push_back(FrontendsPool::M_OPTOUT);
        }
        else if (module_it->name() == "nullad")
        {
          modules.push_back(FrontendsPool::M_NULLAD);
        }
        else if (module_it->name() == "adinst")
        {
          modules.push_back(FrontendsPool::M_ADINST);
        }
        else if (module_it->name() == "click")
        {
          modules.push_back(FrontendsPool::M_CLICK);
        }
        else if (module_it->name() == "imprtrack")
        {
          modules.push_back(FrontendsPool::M_IMPRTRACK);
        }
        else if (module_it->name() == "ad")
        {
          modules.push_back(FrontendsPool::M_AD);
        }
        else
        {
          Stream::Error ostr;
          ostr << "unknown module name '"
               << module_it->name()
               << "'";
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
      SchedulerPtr scheduler =
        UServerUtils::Grpc::Common::Utils::create_scheduler(
          number_scheduler_threads,
          logger());

      std::shared_ptr<GrpcChannelOperationPool> grpc_channel_operation_pool;
      if (config_->ChannelGrpcClientPool().enable())
      {
        Endpoints endpoints;
        const auto& endpoints_config = config_->ChannelServerEndpointList().Endpoint();
        for (const auto& endpoint_config : endpoints_config)
        {
          endpoints.emplace_back(
            endpoint_config.host(),
            endpoint_config.port());
        }

        const auto config_grpc_data = Config::create_pool_client_config(
          config_->ChannelGrpcClientPool());
        grpc_channel_operation_pool = std::make_shared<GrpcChannelOperationPool>(
          logger(),
          manager_coro->get_main_task_processor(),
          scheduler,
          endpoints,
          config_grpc_data.first,
          config_grpc_data.second,
          config_->time_duration_grpc_client_mark_bad());
      }

      std::shared_ptr<GrpcCampaignManagerPool> grpc_campaign_manager_pool;
      if (config_->CampaignGrpcClientPool().enable())
      {
        FrontendCommons::GrpcCampaignManagerPool::Endpoints endpoints;
        const auto& endpoints_config = config_->CampaignManagerEndpointList().Endpoint();
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
          config_->CampaignGrpcClientPool());
        grpc_campaign_manager_pool = std::make_shared<GrpcCampaignManagerPool>(
          logger(),
          manager_coro->get_main_task_processor(),
          scheduler,
          endpoints,
          config_grpc_data.first,
          config_grpc_data.second,
          config_->time_duration_grpc_client_mark_bad());
      }

      const std::string fe_config_path = config_->fe_config();
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
      if (config_->UserBindGrpcClientPool().enable())
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
          config_->UserBindGrpcClientPool());

        grpc_user_bind_operation_distributor = new GrpcUserBindOperationDistributor(
          logger(),
          manager_coro->get_main_task_processor(),
          scheduler,
          controller_groups,
          corba_client_adapter,
          config_grpc_data.first,
          config_grpc_data.second,
          Generics::Time::ONE_SECOND);
      }

      GrpcUserInfoOperationDistributor_var grpc_user_info_operation_distributor;
      if (config_->UserInfoGrpcClientPool().enable())
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
          config_->UserInfoGrpcClientPool());

        grpc_user_info_operation_distributor = new GrpcUserInfoOperationDistributor(
          logger(),
          manager_coro->get_main_task_processor(),
          scheduler,
          controller_groups,
          corba_client_adapter,
          config_grpc_data.first,
          config_grpc_data.second,
          Generics::Time::ONE_SECOND);
      }

      auto grpc_container = std::make_shared<FrontendCommons::GrpcContainer>();
      grpc_container->grpc_channel_operation_pool = grpc_channel_operation_pool;
      grpc_container->grpc_campaign_manager_pool = grpc_campaign_manager_pool;
      grpc_container->grpc_user_bind_operation_distributor = grpc_user_bind_operation_distributor;
      grpc_container->grpc_user_info_operation_distributor = grpc_user_info_operation_distributor;

      FrontendsPool_var frontend_pool = new FrontendsPool(
        *helper_task_processor,
        FrontendsPool::ServerType::FCGI,
        grpc_container,
        fe_config_path.c_str(),
        modules,
        logger(),
        stats_,
        response_factory.in());
      add_child_object(frontend_pool);

      for(auto bind_it = config_->BindSocket().begin();
          bind_it != config_->BindSocket().end();
          ++bind_it)
      {
        add_child_object(
          Generics::ActiveObject_var(
            new AcceptorBoostAsio(
              logger(),
              frontend_pool,
              callback(),
              bind_it->bind(),
              bind_it->backlog(),
              bind_it->accept_threads())));
      }
    }
    catch(const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Got Exception: "
           << exc.what();
      throw Exception(ostr);
    }
  }

  void FCGIServer::main(int& argc, char** argv) noexcept
  {
    try
    {
      XMLUtility::initialize();
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-205")
          << FNS
          << "Got eh::Exception: "
          << ex.what();
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
      catch(const eh::Exception& exc)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '"
             << argv[1]
             << "': "
             << exc.what();
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
      logger()->sstream(Logging::Logger::NOTICE, ASPECT)
        << "service started.";
      corba_server_adapter_->run();

      wait();
      logger()->sstream(Logging::Logger::NOTICE, ASPECT)
        << "service stopped.";
      XMLUtility::terminate();
    }
    catch (const Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Got BiddingFCGIServerApp_::Exception: "
           << exc.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (const CORBA::SystemException& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Got CORBA::SystemException: "
           << exc;
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Got eh::Exception: "
           << exc.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Got unknown exception";
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
  }
} // namespace AdServer::Frontends

int main(int argc, char** argv)
{
  AdServer::Frontends::FCGIServer* app = nullptr;

  try
  {
    app = &AdServer::Frontends::FCGIServerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return EXIT_FAILURE;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return EXIT_FAILURE;
  }

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: "
              << ex.what()
              << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
