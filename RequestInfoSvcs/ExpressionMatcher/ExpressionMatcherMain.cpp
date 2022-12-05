#include "ExpressionMatcherMain.hpp"

#include <locale.h>
#include <string>

#include <eh/Exception.hpp>
#include <XMLUtility/Utility.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "ExpressionMatcherStats.hpp"

namespace
{
  const char ASPECT[] = "ExpressionMatcher";
  const char EXPRESSION_MATCHER_OBJ_KEY[] = "ExpressionMatcher";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

ExpressionMatcherApp_::ExpressionMatcherApp_()
  /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ExpressionMatcherApp_", ASPECT)
{
}

/** ProcessControl interface implementation */
void
ExpressionMatcherApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  if(expression_matcher_impl_.in() != 0)
  {
    expression_matcher_impl_->deactivate_object();
    expression_matcher_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
ExpressionMatcherApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

/* main start point */
void
ExpressionMatcherApp_::main(int& argc, char** argv) noexcept
{
  try
  {
    const char* usage = "usage: ExpressionMatcher <config_file>";

    if(argc < 2)
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
        ConfigPtr(new ExpressionMatcherConfigType(
          ad_configuration->ExpressionMatcherConfig()));
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

      throw Exception(ostr, "ADS-IMPL-4000");
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '"
           << argv[1] << "'."
           << ": "
           << e.what();
      throw Exception(ostr, "ADS-IMPL-4000");
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << "Unknown exception at parsing of config.";
      throw Exception(ostr, "ADS-IMPL-4000");
    }

    /* Initializing logger */
    try
    {
      logger(Config::LoggerConfigReader::create(
        config().Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << "got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr, "ADS-IMPL-4001");
    }

    logger()->log(
      String::SubString("ExpressionMatcherApp_::main(): start"),
      Logging::Logger::TRACE,
      ASPECT);

    /* fill corba_config */
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
      throw Exception(ostr, "ADS-IMPL-4002");
    }

    try
    {
      /* init CORBA Server */
      corba_server_adapter_ =
        new CORBACommons::CorbaServerAdapter(corba_config_);

      shutdowner_ = corba_server_adapter_->shutdowner();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't init CorbaServerAdapter. : "
        << e.what();
      throw Exception(ostr, "ADS-IMPL-4003");
    }

    AdServer::RequestInfoSvcs::ProcStatImpl_var proc_stat_impl;
    if (configuration_->SNMPConfig().present())
    {
      try
      {
        proc_stat_impl = new AdServer::RequestInfoSvcs::ProcStatImpl;

        unsigned snmp_index =
          configuration_->SNMPConfig().get().index().present() ?
          configuration_->SNMPConfig().get().index().get() :
          getpid();

        proc_stat_impl->set("index", static_cast<unsigned long>(snmp_index));

        snmp_stat_provider_ = new SNMPAgentX::SNMPStatsImpl(
          proc_stat_impl, snmp_index,
          Logging::Logger_var(new Logging::LoggerDefaultHolder(
            logger(), 0, "ADS-IMPL-4025")),
          "",
          "ExpressionMatcher-MIB:expressionMatcher",
          configuration_->SNMPConfig().get().mib_dirs().c_str());
      }
      catch (const eh::Exception& ex)
      {
        logger()->sstream(
          Logging::Logger::ERROR,
          ASPECT) << ": Can't init SNMP stats provider: " << ex.what();
      }
    }

    XMLUtility::initialize();

    /* Creating expression matcher active object */
    expression_matcher_impl_ =
      new AdServer::RequestInfoSvcs::ExpressionMatcherImpl(
        logger(),
        config(),
        proc_stat_impl);
    register_vars_controller();

    corba_server_adapter_->add_binding(
      EXPRESSION_MATCHER_OBJ_KEY, expression_matcher_impl_.in());

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    expression_matcher_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    /* Running orb loop */
    corba_server_adapter_->run();

    wait();

    expression_matcher_impl_.reset();

    configuration_.reset();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
                      ASPECT, e.code())
      << "ExpressionMatcherApp_::main(): "
        "Got UserInfoManagerApp_::Exception. : \n"
      << e.what();
  }
  catch (const CORBA::SystemException& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-4004")
      << "ExpressionMatcherApp_::main(): "
        "Got CORBA::SystemException. : \n"
      << e;
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-4005")
      << "ExpressionMatcherApp_::main(): "
        "Got eh::Exception. : \n"
      << e.what();
  }
  catch (...)
  {
    logger()->log(String::SubString("ExpressionMatcherApp_::main(): "
                  "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-4006");
  }
}

int
main(int argc, char** argv)
{
  ExpressionMatcherApp_* app = 0;

  try
  {
    app = &ExpressionMatcherApp::instance();
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

