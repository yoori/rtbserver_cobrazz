
#include <eh/Exception.hpp>

#include <iostream>
#include <memory>

#include <Logger/StreamLogger.hpp>
#include <XMLUtility/Utility.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include "SyncLogsMain.hpp"

namespace
{
  const char ASPECT[] = "SyncLogs";
}

SyncLogsApp_::SyncLogsApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(Logging::OStream::Config(std::cerr))),
      "SyncLogsApp_",
      ASPECT,
      nullptr)
{
}

void
SyncLogsApp_::main(int& argc, char** argv) noexcept
{
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    XMLUtility::initialize();
  }
  catch(const eh::Exception& ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-205") <<
      "SyncLogsApp_::main(): Got eh::Exception: " << ex.what();

    return;
  }

  try
  {
    // Checking SyncLogs params
    if (argc < 2)
    {
      const char* usage = "usage: SyncLogs <config_file>";

      Stream::Error ostr;
      ostr << "config file or colocation config file is not specified\n" << usage;
      throw Exception(ostr);
    }

    /* parse configuration */
    try
    {
      read_config(argv[1], argv[0]);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "'. " << ": " << ex.what();
      throw Exception(ostr);
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << "Unknown Exception at parsing of config.";
      throw Exception(ostr);
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(configuration_->pid_file()));

    sync_logs_impl_ =
      new AdServer::LogProcessing::SyncLogsImpl(callback(), logger(), *configuration_);

    AdServer::Commons::SignalActiveObject signal_active_object;
    sync_logs_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    signal_active_object.wait_object();

    sync_logs_impl_->deactivate_object();
    sync_logs_impl_->wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception &ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-205") <<
      "SyncLogsApp_::main(): Got SyncLogsApp_::Exception: " << ex.what();
  }
  catch (const eh::Exception &ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-205") <<
      "SyncLogsApp_::main(): Got eh::Exception: " << ex.what();
  }

  XMLUtility::terminate();
}

void
SyncLogsApp_::read_config(const char* filename, const char* argv0)
  /*throw(Exception, eh::Exception)*/
{
  const char FUN[] = "SyncLogsApp_::read_config()";

  try
  {
    using namespace xsd::AdServer::Configuration;
    using SyncLogsConfig = AdServer::LogProcessing::SyncLogsImpl::Configuration;
    using SyncLogsConfigPtr = AdServer::LogProcessing::SyncLogsImpl::ConfigurationPtr;

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

      configuration_ = SyncLogsConfigPtr(new SyncLogsConfig(ad_configuration->SyncLogsConfig()));

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

    try
    {
      logger(Config::LoggerConfigReader::create(configuration_->Logger(), argv0));
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
    ostr << FUN << ": got Exception. Invalid SyncLogs configuration: " << ex.what();
    throw Exception(ostr);
  }
}

int
main(int argc, char** argv)
{
  AdServer::Commons::PidFileGuard::enable_process_exit_cleanup();

  SyncLogsApp_* app = nullptr;

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

  if (app == nullptr)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  app->main(argc, argv);
}
