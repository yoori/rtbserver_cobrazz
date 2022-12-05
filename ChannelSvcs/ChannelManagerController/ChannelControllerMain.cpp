
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include <xsd/ChannelSvcs/ChannelManagerControllerConfig.hpp>

#include "ChannelControllerMain.hpp"

namespace
{
  const char ASPECT[] = "ChannelManagerController";
  const char CHANNEL_CONTROLLER_OBJ_KEY[] = "ChannelManagerController";
  const char CHANNEL_CLUSTER_OBJ_KEY[] = "ChannelClusterControl";
  const char PROCESS_STAT_OBJ_KEY[] = "ProcessStatsControl";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

ChannelControllerApp_::ChannelControllerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ChannelControllerApp_", ASPECT)
{
}

void ChannelControllerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);
  if(controller_impl_.in() != 0)
  {
    controller_impl_->deactivate_object();
    controller_impl_->wait_object();
  }
  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
ChannelControllerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void ChannelControllerApp_::load_config_(const char* name) /*throw(Exception)*/
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
      ConfigPtr(new ChannelControllerConfigType(
        ad_configuration->ChannelControllerConfig()));

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

void ChannelControllerApp_::init_corba_() /*throw(Exception)*/
{
  //Fill corba_config
  try
  {
    Config::CorbaConfigReader::read_config(
      configuration_->CorbaConfig(),
      corba_config_);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't read Corba Config. : " << e.what();
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
    ostr << "ChannelControllerApp_::init_corba_: Caught eh::Exception. "
      " Can't init CorbaServerAdapter. : " << e.what();
    throw Exception(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerApp_::init_corba_: "
      "Caught CORBA::SystemException. "
      "Can't init CorbaServerAdapter. : " << e;
    throw Exception(ostr);
  }

  try
  {
    controller_impl_ = new AdServer::ChannelSvcs::ChannelControllerImpl(
      logger(), configuration_.get());

    AdServer::ChannelSvcs::ChannelClusterControlImpl_var control_impl =
      new AdServer::ChannelSvcs::ChannelClusterControlImpl(
        controller_impl_.in());

    AdServer::ChannelSvcs::ChannelStatImpl_var stat_impl =
      new AdServer::ChannelSvcs::ChannelStatImpl(controller_impl_.in());

    corba_server_adapter_->add_binding(
      CHANNEL_CONTROLLER_OBJ_KEY, controller_impl_.in());

    corba_server_adapter_->add_binding(
      CHANNEL_CLUSTER_OBJ_KEY, control_impl.in());

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

    corba_server_adapter_->add_binding(PROCESS_STAT_OBJ_KEY, stat_impl.in());

    controller_impl_->activate_object();
  }
  catch(
      const AdServer::ChannelSvcs::ChannelControllerImpl::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerApp::init_corba_ "
      "Catch ChannelControllerImpl::Exception "
      "on creating ChannelController servants. "
      ": " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerApp::init_corba_ "
      "Catch eh::Exception on creating ChannelController servants. "
      ": " << e.what();
    throw Exception(ostr);
  }
  catch(CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerApp::init_corba_ "
      "Catch CORBA::SystemException on creating "
      "ChannelController servants. : " << e;
    throw Exception(ostr);
  }
}

void ChannelControllerApp_::main(int& argc, char** argv) noexcept
{
  const char FUN[] = "ChannelControllerApp_::main()";

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
      ostr << FUN << ": got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr);
    }

    logger()->log(String::SubString("ChannelControllerApp_::main(): start"),
                  Logging::Logger::TRACE,
                  ASPECT);

    //Initialization CORBA
    init_corba_();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    // Running orb loop
    corba_server_adapter_->run();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    wait();

    logger()->log(String::SubString("ChannelControllerApp_::main(): exit"),
                  Logging::Logger::TRACE,
                  ASPECT);
  }
  catch (const Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                       ASPECT,
                       "ADS-IMPL-23")
        << "ChannelControllerApp_::main(): "
        << "Got ChannelControllerApp_::Exception. : \n"
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelControllerApp_::main(): "
                    "Got ChannelControllerApp_::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-23");
    }
  }
  catch (const CORBA::SystemException& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                       ASPECT,
                       "ADS-IMPL-23")
        << "ChannelControllerApp_::main(): "
        << "Got CORBA::SystemException. : \n"
        << e;
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelControllerApp_::main(): "
                    "Got CORBA::SystemException. : \n"),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-23");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                       ASPECT,
                       "ADS-IMPL-23")
        << "ChannelControllerApp_::main(): "
        << "Got eh::Exception. : \n"
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelControllerApp_::main(): "
                    "Got eh::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-23");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString("ChannelControllerApp_::main(): "
                  "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-23");
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
    logger()->sstream(
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-23")
      << "ChannelControllerApp_::main(): "
      << "Got CORBA::Exception in destroy ORB. : \n"
      << ex;
  }
  catch(...)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                     ASPECT,
                     "ADS-IMPL-23")
      << "ChannelControllerApp_::main(): "
      << "Got unknown exception in destroy ORB \n";
  }
}

int main(int argc, char** argv)
{
  try
  {
    ChannelControllerApp_& app = ChannelControllerApp::instance();
    app.main(argc, argv);
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }
  return 0;
}

