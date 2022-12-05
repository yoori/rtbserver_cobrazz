

#include <locale.h>

#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "ChannelSearchServiceMain.hpp"

namespace
{
  const char ASPECT[] = "ChannelSearchService";
  const char CHANNEL_SEARCH_OBJ_KEY[] = "ChannelSearch";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

ChannelSearchServiceApp_::ChannelSearchServiceApp_()
  /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ChannelSearchApp_", ASPECT)
{}

void
ChannelSearchServiceApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  if(service_impl_.in())
  {
    service_impl_->deactivate_object();
    service_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
ChannelSearchServiceApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
ChannelSearchServiceApp_::main(int argc, char** argv)
  noexcept
{
  try
  {
    const char* usage = "usage: ChannelSearchService <config_file>";

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

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ =
        ConfigPtr(
          new ChannelSearchServiceConfigType(
            ad_configuration->ChannelSearchServiceConfig()));
    }
    catch (const xml_schema::parsing& ex)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '" << argv[1] << "'. : ";

      if (error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "'. "
        ": " << ex.what();
      throw Exception(ostr);
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << "Unknown Exception at parsing of config.";
      throw Exception(ostr);
    }

    // Initializing logger
    try
    {
      if (!config().Logger().filename().empty())
      {
        logger(Config::LoggerConfigReader::create(
          config().Logger(), argv[0]));
      }
    }
    catch (const Logging::LoggerException& ex)
    {
      Stream::Error ostr;
      ostr << "got Logging::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // fill corba_config
    try
    {
      Config::CorbaConfigReader::read_config(
        configuration_->CorbaConfig(),
        corba_config_);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't read Corba Config. : " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      // init CORBA Server
      corba_server_adapter_ =
        new CORBACommons::CorbaServerAdapter(corba_config_);

      shutdowner_ = corba_server_adapter_->shutdowner();
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't init CorbaServerAdapter. : " << ex.what();
      throw Exception(ostr);
    }

    service_impl_ =
      new AdServer::ChannelSearchSvcs::ChannelSearchServiceImpl(
        callback(),
        logger(),
        config());
    register_vars_controller();

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);
    corba_server_adapter_->add_binding(
      CHANNEL_SEARCH_OBJ_KEY, service_impl_.in());

    service_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    // Running orb loop
    corba_server_adapter_->run();

    wait();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    logger()->log(String::SubString("ChannelSearchServiceApp_::main(): exit"),
      Logging::Logger::TRACE,
      ASPECT);
  }
  catch (const Exception& ex)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
      ASPECT) <<
      "ChannelSearchServiceApp_::main(): "
      "Got ChannelSearchServiceApp_::Exception: " <<
      ex.what();
  }
  catch (const CORBA::SystemException& ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT) << "ChannelSearchServiceApp_::main(): "
      "Got CORBA::SystemException: " << ex;
  }
  catch (const eh::Exception& ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT) << "ChannelSearchServiceApp_::main(): "
      "Got eh::Exception: " << ex.what();
  }
  catch (...)
  {
    logger()->log(String::SubString("ChannelSearchServiceApp_::main(): "
      "Got Unknown exception."),
      Logging::Logger::EMERGENCY,
      ASPECT);
  }

  /* references to servants in ORB must be destroyed before
     destroying var pointer to its */
  corba_server_adapter_.reset();
  shutdowner_.reset();
}

int
main(int argc, char** argv)
{
  ChannelSearchServiceApp_* app = 0;

  try
  {
    app = &ChannelSearchServiceApp::instance();
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


