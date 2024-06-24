#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/Config.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>

#include "UserInfoManager_service.cobrazz.pb.hpp"

#include <Commons/GrpcService.hpp>
#include "UserInfoManagerMain.hpp"
#include "UserInfoManagerStat.hpp"


namespace
{
  const char ASPECT[] = "UserInfoManager";
  const char USER_INFO_MANAGER_OBJ_KEY[] = "UserInfoManager";
  const char USER_INFO_MANAGER_CONTROL_OBJ_KEY[] = "UserInfoManagerControl";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char USER_INFO_MANAGER_STATS_OBJ_KEY[] = "UserInfoManagerStats";
}

UserInfoManagerApp_::UserInfoManagerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "UserInfoManagerApp_", ASPECT)
{}

void
UserInfoManagerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  deactivate_object();
  wait_object();

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
UserInfoManagerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

bool
UserInfoManagerApp_::is_ready_() noexcept
{
  return user_info_manager_impl_.in() != 0 &&
    user_info_manager_impl_->uim_ready();
}

char*
UserInfoManagerApp_::comment() /*throw(CORBACommons::OutOfMemory)*/
{
  try
  {
    if (user_info_manager_impl_.in() != 0)
    {
      return user_info_manager_impl_->get_progress();
    }
    CORBA::String_var r;
    r << std::string("0.0%");
    return r._retn();
  }
  catch(const CORBA::Exception&)
  {
    std::cerr << "ex" << std::endl;
    throw CORBACommons::OutOfMemory();
  }
  catch(const eh::Exception& e)
  {
    std::cerr << "ex" << std::endl;
    throw CORBACommons::OutOfMemory();
  }
}

void
UserInfoManagerApp_::main(int& argc, char** argv)
  noexcept
{
  using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Grpc::Manager;
  using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
  using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
  using UserInfoManagerImpl = AdServer::UserInfoSvcs::UserInfoManagerImpl;

  static const char* FUN = "UserInfoManagerApp_::main()";
  
  try
  {
    static const char* USAGE = "usage: UserInfoManager <config_file>";

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
        
      std::unique_ptr<AdConfigurationType>
        ad_configuration = AdConfiguration(file_name.c_str(), error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }
      
      configuration_ =
        ConfigPtr(new UserInfoManagerConfigType(
          ad_configuration->UserInfoManagerConfig()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '" << argv[1] << "'."
        ": ";
      
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

    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

    // Creating user info manager servant
    user_info_manager_impl_ = 
      new AdServer::UserInfoSvcs::UserInfoManagerImpl(
        callback(),
        logger(),
        config());

    add_child_object(user_info_manager_impl_);

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

      auto get_master_stamp_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_get_master_stamp_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::get_master_stamp>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        get_master_stamp_service.in(),
        main_task_processor);

      auto get_user_profile_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_get_user_profile_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::get_user_profile>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        get_user_profile_service.in(),
        main_task_processor);

      auto match_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_match_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::match>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        match_service.in(),
        main_task_processor);

      auto update_user_freq_caps_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_update_user_freq_caps_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::update_user_freq_caps>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        update_user_freq_caps_service.in(),
        main_task_processor);

      auto confirm_user_freq_caps_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_confirm_user_freq_caps_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::confirm_user_freq_caps>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        confirm_user_freq_caps_service.in(),
        main_task_processor);

      auto fraud_user_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_fraud_user_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::fraud_user>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        fraud_user_service.in(),
        main_task_processor);

      auto remove_user_profile_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_remove_user_profile_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::remove_user_profile>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        remove_user_profile_service.in(),
        main_task_processor);

      auto merge_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_merge_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::merge>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        merge_service.in(),
        main_task_processor);

      auto consider_publishers_optin_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_consider_publishers_optin_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::consider_publishers_optin>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        consider_publishers_optin_service.in(),
        main_task_processor);

      auto uim_ready_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_uim_ready_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::uim_ready>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        uim_ready_service.in(),
        main_task_processor);

      auto get_progress_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_get_progress_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::get_progress>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        get_progress_service.in(),
        main_task_processor);

      auto clear_expired_service = AdServer::Commons::create_grpc_service<
        AdServer::UserInfoSvcs::Proto::UserInfoManagerService_clear_expired_Service,
        UserInfoManagerImpl,
        &UserInfoManagerImpl::clear_expired>(
          logger(),
          user_info_manager_impl_.in());
      grpc_server_builder->add_service(
        clear_expired_service.in(),
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

    Generics::Time stat_dumper_period;
    CORBACommons::CorbaObjectRef dumper_ref;
    if(config().StatsDumper().present())
    {
      stat_dumper_period =
        Generics::Time(config().StatsDumper().get().period());
      try
      {
        Config::CorbaConfigReader::read_corba_ref(
          config().StatsDumper().get().StatsDumperRef(),
          dumper_ref);
      }
      catch(const eh::Exception& e)
      {
        logger()->sstream(Logging::Logger::EMERGENCY, ASPECT) <<
          "UserInfoManagerApp_::main(): failed to init StatsDumper: " <<
          e.what();
      }
    }

    AdServer::UserInfoSvcs::UserInfoManagerStatsImpl_var
      user_info_manager_stats_impl =
        new AdServer::UserInfoSvcs::UserInfoManagerStatsImpl(
          callback(),
          logger(),
          user_info_manager_impl_,
          dumper_ref,
          stat_dumper_period);

    add_child_object(user_info_manager_stats_impl);

    user_info_manager_control_impl_ =
      new AdServer::UserInfoSvcs::UserInfoManagerControlImpl(
        user_info_manager_impl_);

    register_vars_controller();

    corba_server_adapter_->add_binding(
      USER_INFO_MANAGER_OBJ_KEY, user_info_manager_impl_.in());

    corba_server_adapter_->add_binding(
      USER_INFO_MANAGER_CONTROL_OBJ_KEY, user_info_manager_control_impl_.in());

    corba_server_adapter_->add_binding(
      USER_INFO_MANAGER_STATS_OBJ_KEY, user_info_manager_stats_impl.in());

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
      ": Got UserInfoManagerApp_::Exception: " << e.what();
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
  UserInfoManagerApp_* app = 0;

  try
  {
    app = &UserInfoManagerApp::instance();
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

