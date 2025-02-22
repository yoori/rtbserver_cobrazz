#include <eh/Exception.hpp>

#include <Generics/ActiveObject.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <CORBACommons/Stats.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/GrpcService.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/UserverConfigUtils.hpp>

#include <UServerUtils/ComponentsBuilder.hpp>

#include "ChannelServer_service.cobrazz.pb.hpp"

#include "ChannelServerMain.hpp"
#include "ChannelServerImpl.hpp"
#include "ChannelServerControlImpl.hpp"
#include "ChannelUpdateImpl.hpp"
#include "ProcessStatsControl.hpp"

namespace
{
  const char ASPECT[] = "ChannelServer";
  const char CHANNEL_SERVER_OBJ_KEY[] = "ChannelServer";
  const char CHANNEL_SERVER_CONTROL_OBJ_KEY[] = "ChannelServerControl";
  const char CHANNEL_UPDATE_OBJ_KEY[] = "ChannelUpdate";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char PROCESS_STAT_CONTROL_OBJ_KEY[] = "ProcessStatsControl";
}

ChannelServerApp_::ChannelServerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ChannelServerApp_", ASPECT)
{
}

void ChannelServerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  deactivate_object();
  wait_object();

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
ChannelServerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void ChannelServerApp_::load_config_(const char* name) /*throw(Exception)*/
{
  Config::ErrorHandler error_handler;
  try
  {
    /* using xsd namespace */
    using namespace xsd::AdServer::Configuration;

    std::string file_name(name);

    std::unique_ptr<AdConfigurationType>
      ad_configuration = AdConfiguration(file_name.c_str(), error_handler);

    if(error_handler.has_errors())
    {
      std::string error_string;
      throw Exception(error_handler.text(error_string));
    }

    configuration_ =
      ConfigPtr(new ChannelServerConfigType(
        ad_configuration->ChannelServerConfig()));

  }
  catch(const xml_schema::parsing& e)
  {
    Stream::Error ostr;

    ostr << "Can't parse config file '"
         << name << "'."
         << ": ";

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
    ostr << "Can't parse config file '"
         << name << "'."
         << ": "
         << e.what();
    throw Exception(ostr);
  }
  catch(...)
  {
    Stream::Error ostr;
    ostr << "Unknown Exception at parsing of config " << name;
    throw Exception(ostr);
  }
}

void ChannelServerApp_::init_corba_() /*throw(Exception, CORBA::SystemException)*/
{
  //Fill corba_config
  try
  {
    corba_config_.custom_reactor = false;
    Config::CorbaConfigReader::read_config(
      configuration_->CorbaConfig(),
      corba_config_);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't read Corba Config. : "
         << e.what();
    throw Exception(ostr);
  }

  try
  {
    // init CORBA Server
    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

    shutdowner_ = corba_server_adapter_->shutdowner();
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't init CorbaServerAdapter. : "
         << e.what();
    throw Exception(ostr);
  }

  try
  {
    server_impl_ = new AdServer::ChannelSvcs::ChannelServerCustomImpl(
      logger(), configuration_.get());
    add_child_object(server_impl_.in());

    {
      using namespace AdServer::Commons;
      register_vars_controller();
      add_var_processor(DbStateProcessor::VAR_NAME,
        new DbStateProcessor(
          new_simple_db_state_changer(server_impl_)));
    }

    AdServer::ChannelSvcs::ChannelServerControlImpl_var server_control_impl(
      new AdServer::ChannelSvcs::ChannelServerControlImpl(server_impl_.in()));

    AdServer::ChannelSvcs::ChannelUpdateImpl_var update_impl(
      new AdServer::ChannelSvcs::ChannelUpdateImpl(server_impl_.in()));

    AdServer::ChannelSvcs::ChannelServerStatsImpl_var stat_impl(
      new AdServer::ChannelSvcs::ChannelServerStatsImpl(server_impl_.in()));

    corba_server_adapter_->add_binding(
      CHANNEL_SERVER_OBJ_KEY, server_impl_.in());

    corba_server_adapter_->add_binding(
      CHANNEL_UPDATE_OBJ_KEY, update_impl.in());

    corba_server_adapter_->add_binding(
      CHANNEL_SERVER_CONTROL_OBJ_KEY, server_control_impl.in());

    corba_server_adapter_->add_binding(
      PROCESS_STAT_CONTROL_OBJ_KEY, stat_impl.in());

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);
  }
  catch(const AdServer::ChannelSvcs::ChannelServerCustomImpl::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelServerApp::init_corba_: "
      "Catch ChannelServerCustomImpl::Exception on creating "
      " ChannelServer servants. : " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelServerApp::init_corba_: "
      "Catch eh::Exception on creating ChannelServer servants. "
      ": " << e.what();
    throw Exception(ostr);
  }
}

void ChannelServerApp_::init_coro_()
{
  using ComponentsBuilder = UServerUtils::ComponentsBuilder;
  using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
  using ServiceMode = UServerUtils::Grpc::Server::ServiceMode;

  Generics::TaskPool_var task_pool = new Generics::TaskPool(
    callback(),
    configuration_->number_grpc_helper_threads(),
    1024 * 1024 // stack_size
  );
  add_child_object(task_pool.in());

  // Creating coroutine manager
  auto task_processor_container_builder =
    Config::create_task_processor_container_builder(
      logger(),
      configuration_->Coroutine());

  auto init_func = [this, task_pool] (TaskProcessorContainer& task_processor_container) mutable {
    auto& main_task_processor = task_processor_container.get_main_task_processor();
    auto components_builder = std::make_unique<ComponentsBuilder>();

    auto grpc_server_builder = Config::create_grpc_server_builder(
      logger(),
      configuration_->GrpcServer());

    // GrpcService only for ServiceMode::EventToCoroutine(optimisation reason)
    ServiceMode service_mode = ServiceMode::EventToCoroutine;

    auto match_service = AdServer::Commons::create_grpc_service<
      AdServer::ChannelSvcs::Proto::ChannelServer_match_Service,
      AdServer::ChannelSvcs::ChannelServerCustomImpl,
      &AdServer::ChannelSvcs::ChannelServerCustomImpl::match>(
        logger(),
        server_impl_.in(),
        task_pool.in());
    grpc_server_builder->add_service(
      match_service.in(),
      main_task_processor,
      service_mode);

    auto get_ccg_traits_service = AdServer::Commons::create_grpc_service<
      AdServer::ChannelSvcs::Proto::ChannelServer_get_ccg_traits_Service,
      AdServer::ChannelSvcs::ChannelServerCustomImpl,
      &AdServer::ChannelSvcs::ChannelServerCustomImpl::get_ccg_traits>(
        logger(),
        server_impl_.in(),
        task_pool.in());
    grpc_server_builder->add_service(
      get_ccg_traits_service.in(),
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
  add_child_object(manager_coro.in());
}

void ChannelServerApp_::main(int& argc, char** argv) noexcept
{
  const char FUN[] = "ChannelServerApp_::main(): ";

  try
  {
    const char* usage = "usage: ChannelServer <config_file>";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
      throw InvalidArgument(ostr);
    }

    //Load configuration
    load_config_(argv[1]);

    //Initializing logger
    try
    {
      logger(Config::LoggerConfigReader::create(
        configuration_->Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got LoggerConfigReader::Exception: "
        << e.what();
      throw Exception(ostr);
    }

    //Initialization CORBA
    init_corba_();

    //Initialization Coroutine system
    init_coro_();

    activate_object();

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
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-11")
        << FUN
        << "Got ChannelServerApp_::Exception. : \n"
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): "
                    "Got ChannelServerApp_::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (const CORBA::SystemException& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-11")
        << FUN
        << "Got CORBA::SystemException. : \n"
        << e;
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): "
                    "Got unknown exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-11")
       << FUN
       << "Got eh::Exception. : \n"
       << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): "
                    "Got unknown exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString("ChannelServerApp_::main(): "
                  "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-11");
  }

  /* references to servants in ORB must be destroyed before
     destroying var pointer to its */
  try
  {
    corba_server_adapter_.reset();
    shutdowner_.reset();
  }
  catch(const CORBA::Exception& ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-11")
      << FUN
      << "Got CORBA::Exception in destroy ORB. : \n"
      << ex;
  }
  catch(...)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-11")
      << FUN
      << "Got unknown exception in destroy ORB \n";
  }
}

int main(int argc, char** argv)
{
  ChannelServerApp_* app = 0;
  try
  {
    app = &ChannelServerApp::instance();
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
  return 0;
}

