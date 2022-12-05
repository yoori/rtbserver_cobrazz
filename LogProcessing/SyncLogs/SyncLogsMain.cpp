
#include <eh/Errno.hpp>
#include <eh/Exception.hpp>
#include <String/StringManip.hpp>
#include <XMLUtility/Utility.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "SyncLogsMain.hpp"

namespace
{
  const char ASPECT[] = "SyncLogs";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

SyncLogsApp_::SyncLogsApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "SyncLogsApp_", ASPECT)
{
}

void
SyncLogsApp_::shutdown(CORBA::Boolean wait_for_completion)
  noexcept
{
  ShutdownGuard guard(shutdown_lock_);

  if (sync_logs_impl_.in() != 0)
  {
    sync_logs_impl_->deactivate_object();
    sync_logs_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
SyncLogsApp_::is_alive()
  noexcept
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
SyncLogsApp_::main(int &argc, char **argv) noexcept
{
  try
  {
    XMLUtility::initialize();
  }
  catch(const eh::Exception& ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT,
      "ADS-IMPL-205") <<
      "SyncLogsApp_::main(): Got eh::Exception: " << ex.what();

    return;
  }

  try
  {
    // Checking SyncLogs params
    if (argc < 2)
    {
      const char *usage = "usage: SyncLogs <config_file>";

      Stream::Error ostr;
      ostr << "config file or colocation config file is not specified\n"
          << usage;
      throw Exception(ostr);
    }

    XMLUtility::initialize();

    /* parse configuration */
    try
    {
      read_config(argv[1], argv[0]);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "'. "
          << ": " << ex.what();
      throw Exception(ostr);
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << "Unknown Exception at parsing of config.";
      throw Exception(ostr);
    }

    // Creating synclogs servant
    sync_logs_impl_ =
      new AdServer::LogProcessing::SyncLogsImpl(
        callback(),
        logger(),
        *configuration_);
    register_vars_controller();

    // Initialization of CORBA
    init_corba();

    sync_logs_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    // Running orb loop
    corba_server_adapter_->run();

    wait();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    XMLUtility::terminate();
  }
  catch (const Exception &ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-205") <<
      "SyncLogsApp_::main(): Got SyncLogsApp_::Exception: " << ex.what();
  }
  catch (const eh::Exception &ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT,
      "ADS-IMPL-205") <<
      "SyncLogsApp_::main(): Got eh::Exception: " << ex.what();
  }

  XMLUtility::terminate();
}

void
SyncLogsApp_::read_config(const char *filename, const char* argv0)
  /*throw(Exception, eh::Exception)*/
{
  const char FUN[] = "SyncLogsApp_::read_config()";

  try
  {
    using namespace xsd::AdServer::Configuration;
    typedef AdServer::LogProcessing::SyncLogsImpl::Configuration SyncLogsConfig;
    typedef AdServer::LogProcessing::SyncLogsImpl::ConfigurationPtr SyncLogsConfigPtr;

    Config::ErrorHandler error_handler;

    try
    {
      std::unique_ptr<AdConfigurationType>
        ad_configuration = AdConfiguration(filename, error_handler);

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_ = SyncLogsConfigPtr(
        new SyncLogsConfig(ad_configuration->SyncLogsConfig()));

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }
    }
    catch (const xml_schema::parsing &ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << filename << "'. : ";
      if (error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }
      throw Exception(ostr);
    }

    // Fill corba_config
    try
    {
      Config::CorbaConfigReader::read_config(
        configuration_->CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't read Corba Config. : " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      logger(Config::LoggerConfigReader::create(
        configuration_->Logger(), argv0));
    }
    catch (const Config::LoggerConfigReader::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
  catch (const Exception &ex)
  {
    Stream::Error ostr;
    ostr << FUN << ": got Exception. Invalid SyncLogs configuration: " <<
      ex.what();
    throw Exception(ostr);
  }
}

void SyncLogsApp_::init_corba() /*throw(Exception)*/
{
  try
  {
    // init CORBA Server
    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

    shutdowner_ = corba_server_adapter_->shutdowner();

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error ostr;
    ostr << "SyncLogsApp_::init_corba(): "
        << "Can't init CorbaServerAdapter. : " << ex.what();
    throw Exception(ostr);
  }
}

int
main(int argc, char** argv)
{
  SyncLogsApp_* app = 0;

  try
  {
    app = &SyncLogsApp::instance();
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

