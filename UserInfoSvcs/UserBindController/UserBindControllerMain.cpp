#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "UserBindControllerMain.hpp"

namespace
{
  const char ASPECT[] = "UserBindController";
  const char USER_BIND_CONTROLLER_OBJ_KEY[] = "UserBindController";
  const char USER_BIND_CLUSTER_OBJ_KEY[] = "UserBindClusterControl";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

UserBindControllerApp_::UserBindControllerApp_()
  /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "UserBindControllerApp_", ASPECT)
{}

void
UserBindControllerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  if(user_bind_controller_impl_.in() != 0)
  {
    user_bind_controller_impl_->deactivate_object();
    user_bind_controller_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
UserBindControllerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
UserBindControllerApp_::main(int& argc, char** argv)
  noexcept
{
  static const char* FUN = "UserBindControllerApp_::main()";

  try
  {
    const char* usage = "usage: UserBindController <config_file>";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
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
        ConfigPtr(new UserBindControllerConfigType(
          ad_configuration->UserBindControllerConfig()));
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
    catch(...)
    {
      Stream::Error ostr;
      ostr << "Unknown Exception at parsing of config.";
      throw Exception(ostr);
    }

    // Initializing logger
    try
    {
      logger_ = Config::LoggerConfigReader::create(
        config().Logger(), argv[0]);
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
      ostr << "Can't init CorbaServerAdapter: " << e.what();
      throw Exception(ostr);
    }

    // Creating user info manager servant
    user_bind_controller_impl_ =
      new AdServer::UserInfoSvcs::UserBindControllerImpl(
        callback(),
        logger(),
        config());

    register_vars_controller();

    AdServer::UserInfoSvcs::UserBindClusterControlImpl_var cluster_control =
      new AdServer::UserInfoSvcs::UserBindClusterControlImpl(
        user_bind_controller_impl_.in());

    corba_server_adapter_->add_binding(
      USER_BIND_CONTROLLER_OBJ_KEY,
      user_bind_controller_impl_.in());

    corba_server_adapter_->add_binding(
      USER_BIND_CLUSTER_OBJ_KEY,
      cluster_control.in());

    user_bind_controller_impl_->activate_object();

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

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
      "ADS-IMPL-73") <<
      FUN << ": caught UserBindControllerApp_::Exception: " << e.what();
  }
  catch (const CORBA::SystemException& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-73") <<
      FUN << ": caught CORBA::SystemException: " << e;
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-73") <<
      FUN << ": caught eh::Exception: " << e.what();
  }
}

int
main(int argc, char** argv)
{
  UserBindControllerApp_* app = 0;

  try
  {
    app = &UserBindControllerApp::instance();
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

