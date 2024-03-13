#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>

#include "UserInfoManagerMain.hpp"
#include "UserInfoManagerStat.hpp"
//#include "UServerUtils/MetricsHTTPProvider.hpp"

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
    //, composite_metrics_provider_(new Generics::CompositeMetricsProvider)
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
        config()
        //, composite_metrics_provider_
        );

    add_child_object(user_info_manager_impl_);

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

    /*
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

