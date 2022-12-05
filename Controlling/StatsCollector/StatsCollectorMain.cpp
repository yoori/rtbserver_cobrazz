#include<Generics/Singleton.hpp>
#include <Generics/ActiveObject.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/Logger.hpp>
#include"StatsCollectorMain.hpp"

namespace
{
  const char STAT_COLLECTOR_OBJ_KEY[] = "StatsCollector";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";

  const char ASPECT[] = "StatsCollectorApp";
}

namespace AdServer
{
  namespace Controlling
  {
    StatsCollectorApp::StatsCollectorApp() /*throw(eh::Exception)*/
      : AdServer::Commons::ProcessControlVarsLoggerImpl(
          "ChannelServerApp_", ASPECT)
    {
      //
    }

    void StatsCollectorApp::shutdown(CORBA::Boolean wait_for_completion)
      /*throw(CORBA::SystemException)*/
    {
      ShutdownGuard guard(shutdown_lock_);

      if(collector_impl_.in() != 0)
      {
        collector_impl_->deactivate_object();
        collector_impl_->wait_object();
      }

      CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
    }

    void StatsCollectorApp::load_config_(const char* name)
      /*throw(Exception)*/
    {
      const char FUN[] = "StatsCollectorApp::load_config_(): ";
      Config::ErrorHandler error_handler;
      try
      {
        std::string file_name(name);
        std::unique_ptr<xsd::AdServer::Configuration::AdConfigurationType>
          ad_configuration = xsd::AdServer::Configuration::AdConfiguration(
            file_name.c_str(), error_handler);
        if(error_handler.has_errors())
        {
          Stream::Error ostr;
          std::string error_string;
          ostr << FUN << error_handler.text(error_string);
          throw Exception(ostr);
        }
        configuration_ = ConfigPtr(
          new xsd::AdServer::Configuration::StatsCollectorConfigType(
            ad_configuration->StatsCollectorConfig()));
      }
      catch(const xml_schema::parsing& e)
      {
        Stream::Error ostr;
        ostr << FUN << "Can't parse config file '"
             << name << "'. : ";

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
        ostr << FUN << "Can't parse config file '"
             << name << "'."
             << ": "
             << e.what();

        throw Exception(ostr);
      }
    }

    void StatsCollectorApp::init_logger_(const char* name)
      /*throw(Exception)*/
    {
      const char FUN[] = "StatsCollectorApp::init_logger_(): ";
      try
      {
        logger(Config::LoggerConfigReader::create(
          configuration_->Logger(), name));
      }
      catch(const Config::LoggerConfigReader::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << "got LoggerConfigReader::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    void StatsCollectorApp::init_corba_()
      /*throw(Exception)*/
    {
      const char FUN[] = "StatsCollectorApp::init_corba_(): ";
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
        ostr << FUN << "Can't read Corba Config. : " << e.what();
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
        ostr << FUN << "Can't init CorbaServerAdapter. : "
             << e.what();
        throw Exception(ostr);
      }

      try
      {
        collector_impl_ = new StatsCollectorImpl(
          logger(), configuration_.get());
        register_vars_controller();

        corba_server_adapter_->add_binding(
          STAT_COLLECTOR_OBJ_KEY, collector_impl_.in());

        corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

        collector_impl_->activate_object();
      }
      catch(const StatsCollectorImpl::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          "Catch StatsCollectorImpl::Exception on creating "
          " StatsCollector servant. : " << e.what();
        throw Exception(ostr);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          "Catch eh::Exception on creating StatsCollector servant. "
          ": " << e.what();
        throw Exception(ostr);
      }
    }

    int StatsCollectorApp::run(int /*argc*/, char* argv[]) noexcept
    {
      const char FUN[] = "StatsCollectorApp::run(): ";
      try
      {
        load_config_(argv[1]);
        init_logger_(argv[0]);
        init_corba_();
        logger()->sstream(Logging::Logger::NOTICE, ASPECT)
          << "service started.";
        corba_server_adapter_->run();
        wait();
        logger()->sstream(Logging::Logger::NOTICE, ASPECT)
          << "service stopped.";
        return 0;
      }
      catch(const Exception& e)
      {
        logger()->sstream(
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-701")
          << FUN
          << "Got Exception. : \n"
          << e.what();
      }
      catch(const CORBA::SystemException& e)
      {
        logger()->sstream(
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-701")
          << FUN
          << "Got CORBA::SystemException. : \n"
          << e;
      }
      catch(const eh::Exception& e)
      {
        logger()->sstream(
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-701")
          << FUN
          << "Got eh::Exception. : \n"
          << e.what();
      }
      catch(...)
      {
        logger()->sstream(
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-701")
          << FUN
          << "Got  unknown exception. \n";
      }
      return 1;
    }
  }
}

int main(int argc, char* argv[])
{
  if(argc < 2)
  {
    const char* usage = "usage: StatsCollector <config_file>\n";
    std::cerr << "configuration file wasn't specified\n" << usage;
    return 1;
  }
  typedef Generics::Singleton<AdServer::Controlling::StatsCollectorApp, AdServer::Controlling::StatsCollectorApp_var>
    CollectorSingleton;
  AdServer::Controlling::StatsCollectorApp* app =
    &CollectorSingleton::instance();
  if(app == 0)
  {
    std::cerr << "can't instantiate application\n";
    return 1;
  }
  return app->run(argc, argv);
}

