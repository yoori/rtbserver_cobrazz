#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>

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
{}

UserBindServerApp_::~UserBindServerApp_() noexcept
{}

void
UserBindServerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  std::unique_lock<std::mutex> guard(shutdown_lock_);

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
UserBindServerApp_::main(int& argc, char** argv) noexcept
{
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
      logger(Config::LoggerConfigReader::create(config().Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr);
    }

    // Fill corba_config
    CORBACommons::CorbaConfig corba_config;

    try
    {
      Config::CorbaConfigReader::read_config(
        config().CorbaConfig(),
        corba_config);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't read Corba Config: " << e.what();
      throw Exception(ostr);
    }

    AdServer::UserInfoSvcs::UserBindServerCore_var user_bind_server_core =
      new AdServer::UserInfoSvcs::UserBindServerCore(
        config(),
        logger());
    add_child_object(user_bind_server_core);

    // Creating user info manager servant
    user_bind_server_impl_ = new AdServer::UserInfoSvcs::UserBindServerImpl(
      callback(),
      logger(),
      user_bind_server_core);

    CORBACommons::CorbaServerAdapter_var corba_server_adapter =
      new CORBACommons::CorbaServerAdapter(corba_config);

    corba_server_adapter->add_binding(
      USER_BIND_SERVER_OBJ_KEY, user_bind_server_impl_.in());

    corba_server_adapter->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    if(config().GrpcConfig().present())
    {
      grpc_adapter_ = new AdServer::UserInfoSvcs::UserBindServerGrpc(
        user_bind_server_core,
        logger(),
        config().GrpcConfig()->Endpoint().host().present() &&
          *(config().GrpcConfig()->Endpoint().host()) != "*" ?
          *config().GrpcConfig()->Endpoint().host() :
          "0.0.0.0",
        config().GrpcConfig()->Endpoint().port());
      add_child_object(grpc_adapter_);
    }

    shutdowner_ = corba_server_adapter->shutdowner();

    activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    // Running orb loop
    corba_server_adapter->run();

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
