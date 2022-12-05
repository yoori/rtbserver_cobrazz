#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <xsd/UserInfoSvcs/UserInfoExchangerProxyConfig.hpp>

#include "UserInfoExchangerProxyMain.hpp"

namespace
{
  const char ASPECT[] = "UserInfoExchangerProxy";
  const char USER_INFO_EXCHANGER_OBJ_KEY[] = "UserInfoExchangerProxy";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

UserInfoExchangerProxyApp_::UserInfoExchangerProxyApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "UserInfoExchangerProxyApp_", ASPECT)
{
}

void
UserInfoExchangerProxyApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
UserInfoExchangerProxyApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
UserInfoExchangerProxyApp_::main(int& argc, char** argv)
  noexcept
{
  const char FUN[] = "UserInfoExchangerProxyApp_::main()";
  
  try
  {
    const char* usage = "usage: UserInfoExchangerProxy <config_file>";

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
        ConfigPtr(new UserInfoExchangerProxyConfigType(
          ad_configuration->UserInfoExchangerProxyConfig()));

    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '"
        << argv[1] << "'."
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
        << argv[1] << "'."
        << ": "
        << e.what();
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

    // Creating user info manager servant
    user_info_exchanger_proxy_impl_ = 
      new AdServer::UserInfoSvcs::UserInfoExchangerProxyImpl(
        callback(),
        logger(),
        config());
    register_vars_controller();

    corba_server_adapter_->add_binding(
      USER_INFO_EXCHANGER_OBJ_KEY,
      user_info_exchanger_proxy_impl_.in());

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
    try
    {
      logger()->sstream(Logging::Logger::CRITICAL,
                        ASPECT,
                        "ADS-IMPL-71")
       << "UserInfoExchangerProxyApp_::main(): "
       << "Got UserInfoExchangerProxyApp_::Exception. : \n"
       << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString(
                      "UserInfoExchangerProxyApp_::main(): "
                      "Got UserInfoExchangerProxyApp_::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-71");
    }
  }
  catch (const CORBA::SystemException& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-71")
       << "UserInfoExchangerProxyApp_::main(): "
       << "Got CORBA::SystemException. : \n"
       << e;
    }
    catch (...)
    {
      logger()->log(String::SubString(
                      "UserInfoExchangerProxyApp_::main(): "
                      "Got CORBA::SystemException. : \n"),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-71");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-71")
       << "UserInfoExchangerProxyApp_::main(): "
       << "Got eh::Exception. : \n"
       << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString(
                      "UserInfoExchangerProxyApp_::main(): "
                      "Got eh::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-71");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString(
                    "UserInfoExchangerProxyApp_::main(): "
                    "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-71");
  }
}

int
main(int argc, char** argv)
{
  UserInfoExchangerProxyApp_* app = 0;

  try
  {
    app = &UserInfoExchangerProxyApp::instance();
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

