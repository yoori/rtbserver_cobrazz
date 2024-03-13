#include <eh/Exception.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/Config.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>

#include "GrpcService.hpp"
#include "UserBindServerMain.hpp"

namespace
{
  const char ASPECT[] = "UserBindServer";
  const char USER_BIND_SERVER_OBJ_KEY[] = "UserBindServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

UserBindServerApp_::UserBindServerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "UserBindServerApp_", ASPECT)
    //, composite_metrics_provider_(new Generics::CompositeMetricsProvider())
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
  using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Grpc::Manager;
  using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
  using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
  using GetBindRequestService = AdServer::UserInfoSvcs::GetBindRequestService;
  using GetBindRequestService_var = AdServer::UserInfoSvcs::GetBindRequestService_var;
  using AddBindRequestService = AdServer::UserInfoSvcs::AddBindRequestService;
  using AddBindRequestService_var = AdServer::UserInfoSvcs::AddBindRequestService_var;
  using GetUserIdService = AdServer::UserInfoSvcs::GetUserIdService;
  using GetUserIdService_var = AdServer::UserInfoSvcs::GetUserIdService_var;
  using AddUserIdService = AdServer::UserInfoSvcs::AddUserIdService;
  using AddUserIdService_var = AdServer::UserInfoSvcs::AddUserIdService_var;
  using GetSourceService = AdServer::UserInfoSvcs::GetSourceService;
  using GetSourceService_var = AdServer::UserInfoSvcs::GetSourceService_var;

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
        config()
        //, composite_metrics_provider_
        );

    add_child_object(user_bind_server_impl_);

    /*
    // init CompositeMetricsProvider here, pass to MetricsHTTPProvider and to modules
    // init metrics http provider
    if(config().Monitoring().present())
    {
      UServerUtils::MetricsHTTPProvider_var metrics_http_provider =
        new UServerUtils::MetricsHTTPProvider(
          composite_metrics_provider_,
          config().Monitoring()->port(),
          "/metrics");

      add_child_object(metrics_http_provider);
    }
    */

    // Creating coroutine manager
    auto task_processor_container_builder =
      Config::create_task_processor_container_builder(
        logger(),
        config().Coroutine());

    auto init_func = [this] (
      TaskProcessorContainer& task_processor_container) {
        auto& main_task_processor =
          task_processor_container.get_main_task_processor();
        auto components_builder =
          std::make_unique<ComponentsBuilder>();

        auto grpc_server_builder =
          Config::create_grpc_cobrazz_server_builder(
            logger(),
            config().GrpcServer());

        GetBindRequestService_var get_bind_request_service(
          new GetBindRequestService(
            logger(),
            user_bind_server_impl_.in()));
        grpc_server_builder->add_service(
          get_bind_request_service.in(),
          main_task_processor);

        AddBindRequestService_var add_bind_request_service(
          new AddBindRequestService(
            logger(),
            user_bind_server_impl_.in()));
        grpc_server_builder->add_service(
          add_bind_request_service.in(),
          main_task_processor);

        GetUserIdService_var get_user_id_service(
          new GetUserIdService(
            logger(),
            user_bind_server_impl_.in()));
        grpc_server_builder->add_service(
          get_user_id_service.in(),
          main_task_processor);

        AddUserIdService_var add_user_id_service(
          new AddUserIdService(
            logger(),
            user_bind_server_impl_.in()));
        grpc_server_builder->add_service(
          add_user_id_service.in(),
          main_task_processor);

        GetSourceService_var get_source_service(
          new GetSourceService(
            logger(),
            user_bind_server_impl_.in()));
        grpc_server_builder->add_service(
          get_source_service.in(),
          main_task_processor);

        components_builder->add_grpc_cobrazz_server(
          std::move(grpc_server_builder));

        return components_builder;
     };

     ManagerCoro_var manager_coro(
       new ManagerCoro(
         std::move(task_processor_container_builder),
         std::move(init_func),
         logger()));

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

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
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

