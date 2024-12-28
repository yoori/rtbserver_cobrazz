// UNIXCOMMONS
#include <eh/Exception.hpp>

// THIS
#include <CampaignSvcs/BillingServer/BillingServerImpl.hpp>
#include <CampaignSvcs/BillingServer/BillingServerMain.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcService.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

namespace
{
  const char ASPECT[] = "BillingServer";
  const char USER_BIND_SERVER_OBJ_KEY[] = "BillingServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

BillingServerApp_::BillingServerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "BillingServerApp_", ASPECT)
{}

void
BillingServerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownSyncPolicy::WriteGuard guard(shutdown_lock_);

  deactivate_object();
  wait_object();

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
BillingServerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
BillingServerApp_::init_coro()
{
  using ComponentsBuilder = UServerUtils::ComponentsBuilder;
  using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
  using ServiceMode = UServerUtils::Grpc::Server::ServiceMode;

  auto task_processor_container_builder =
    Config::create_task_processor_container_builder(
      logger(),
      configuration_->Coroutine());

  auto init_func = [this] (TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor = task_processor_container.get_main_task_processor();
    auto components_builder = std::make_unique<ComponentsBuilder>();

    auto grpc_server_builder = Config::create_grpc_server_builder(
      logger(),
      configuration_->GrpcServer());

    ServiceMode service_mode = ServiceMode::EventToCoroutine;

    auto check_available_bid = AdServer::Commons::create_grpc_service<
      AdServer::CampaignSvcs::Billing::Proto::BillingService_check_available_bid_Service,
      AdServer::CampaignSvcs::BillingServerImpl,
      &AdServer::CampaignSvcs::BillingServerImpl::check_available_bid>(
      logger(),
      billing_server_impl_.in(),
      service_mode != ServiceMode::EventToCoroutine);
    grpc_server_builder->add_service(
      check_available_bid.in(),
      main_task_processor,
      service_mode);

    auto reserve_bid = AdServer::Commons::create_grpc_service<
      AdServer::CampaignSvcs::Billing::Proto::BillingService_reserve_bid_Service,
      AdServer::CampaignSvcs::BillingServerImpl,
      &AdServer::CampaignSvcs::BillingServerImpl::reserve_bid>(
      logger(),
      billing_server_impl_.in(),
      service_mode != ServiceMode::EventToCoroutine);
    grpc_server_builder->add_service(
      reserve_bid.in(),
      main_task_processor,
      service_mode);

   auto confirm_bid = AdServer::Commons::create_grpc_service<
      AdServer::CampaignSvcs::Billing::Proto::BillingService_confirm_bid_Service,
      AdServer::CampaignSvcs::BillingServerImpl,
      &AdServer::CampaignSvcs::BillingServerImpl::confirm_bid>(
      logger(),
      billing_server_impl_.in(),
      service_mode != ServiceMode::EventToCoroutine);
    grpc_server_builder->add_service(
      confirm_bid.in(),
      main_task_processor,
      service_mode);

    auto add_amount = AdServer::Commons::create_grpc_service<
      AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_Service,
      AdServer::CampaignSvcs::BillingServerImpl,
      &AdServer::CampaignSvcs::BillingServerImpl::add_amount>(
      logger(),
      billing_server_impl_.in(),
      service_mode != ServiceMode::EventToCoroutine);
    grpc_server_builder->add_service(
      add_amount.in(),
      main_task_processor,
      service_mode);

    components_builder->add_grpc_cobrazz_server(
      std::move(grpc_server_builder));

    return components_builder;
  };

  manager_coro_ = new ManagerCoro(
    std::move(task_processor_container_builder),
    std::move(init_func),
    logger());

  add_child_object(manager_coro_);
}

void
BillingServerApp_::main(int argc, char** argv)
  noexcept
{
  static const char* FUN = "BillingServerApp_::main()";

  try
  {
    static const char* USAGE = "usage: BillingServer <config_file>";

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

      configuration_ = ConfigPtr(new BillingServerConfigType(
        ad_configuration->BillingServer()));
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

    billing_server_impl_ = new AdServer::CampaignSvcs::BillingServerImpl(
      callback(),
      logger(),
      config());

    add_child_object(billing_server_impl_);

    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

    corba_server_adapter_->add_binding(
      USER_BIND_SERVER_OBJ_KEY, billing_server_impl_.in());

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    shutdowner_ = corba_server_adapter_->shutdowner();

    init_coro();

    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    // Running orb loop
    corba_server_adapter_->run();

    wait();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch(const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
      ASPECT,
      "ADS-IMPL-58") << FUN <<
      ": Got BillingServerApp_::Exception: " << e.what();
  }
  catch(const CORBA::SystemException& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-59") << FUN <<
      ": Got CORBA::SystemException: " << e;
  }
  catch(const eh::Exception& e)
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
  BillingServerApp_* app = 0;

  try
  {
    app = &BillingServerApp::instance();
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

