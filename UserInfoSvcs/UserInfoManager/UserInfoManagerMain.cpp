#include <eh/Exception.hpp>

#include <memory>

#include <Commons/PidFileGuard.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/SignalActiveObject.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>

#include "UserInfoManagerMain.hpp"

namespace
{
  const char ASPECT[] = "UserInfoManager";
  const char USER_INFO_MANAGER_OBJ_KEY[] = "UserInfoManager";
  const char USER_INFO_MANAGER_CONTROL_OBJ_KEY[] = "UserInfoManagerControl";
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
  return user_info_manager_core_ &&
    user_info_manager_core_->uim_ready();
}

char*
UserInfoManagerApp_::comment() /*throw(CORBACommons::OutOfMemory)*/
{
  try
  {
    if (user_info_manager_core_)
    {
      CORBA::String_var result;
      result << user_info_manager_core_->get_progress();
      return result._retn();
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
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

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
    user_info_manager_core_ =
      std::make_shared<AdServer::UserInfoSvcs::UserInfoManagerCore>(
        callback(),
        logger(),
        config());
    user_info_manager_impl_ =
      new AdServer::UserInfoSvcs::UserInfoManagerImpl(
        user_info_manager_core_);

    add_child_object(user_info_manager_core_);

    user_info_manager_control_impl_ =
      new AdServer::UserInfoSvcs::UserInfoManagerControlImpl(
        user_info_manager_core_);

    register_vars_controller();

    corba_server_adapter_->add_binding(
      USER_INFO_MANAGER_OBJ_KEY, user_info_manager_impl_.in());

    corba_server_adapter_->add_binding(
      USER_INFO_MANAGER_CONTROL_OBJ_KEY, user_info_manager_control_impl_.in());

    if(config().GrpcConfig().present())
    {
      grpc_adapter_ = new AdServer::UserInfoSvcs::UserInfoManagerGrpc(
        user_info_manager_core_,
        logger(),
        config().GrpcConfig()->Endpoint().host().present() &&
          *(config().GrpcConfig()->Endpoint().host()) != "*" ?
          *config().GrpcConfig()->Endpoint().host() :
          "0.0.0.0",
        config().GrpcConfig()->Endpoint().port());
      add_child_object(grpc_adapter_);
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(config().pid_file()));

    add_child_object(corba_server_adapter_.in());
    AdServer::Commons::SignalActiveObject signal_active_object;

    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    signal_active_object.wait_object();

    deactivate_object();
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

  deactivate_object();
  wait_object();
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
