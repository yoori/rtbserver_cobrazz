
#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>

#include "DictionaryProviderMain.hpp"
#include "DictionaryProviderImpl.hpp"

namespace
{
  char ASPECT[] = "DictionaryProvider";
  char DICTIONARY_PROVIDER[] = "DictionaryProvider";
  char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

DictionaryProviderApp_::DictionaryProviderApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "DictionaryProviderApp_", ASPECT),
    server_impl_()
{
}

void DictionaryProviderApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);
  if(server_impl_)
  {
    server_impl_->deactivate_object();
    server_impl_->wait_object();
    server_impl_.reset();
  }
  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
DictionaryProviderApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void DictionaryProviderApp_::load_config_(const char* name) /*throw(Exception)*/
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
      ConfigPtr(new DictionaryProviderConfigType(
        ad_configuration->DictionaryProviderConfig()));

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

void DictionaryProviderApp_::init_corba_() /*throw(Exception, CORBA::SystemException)*/
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
    ostr << "Can't read Corba Config. : "
         << e.what();
    throw Exception(ostr);
  }

  try
  {
    corba_server_adapter_ = new CORBACommons::CorbaServerAdapter(
      corba_config_);

    shutdowner_ = corba_server_adapter_->shutdowner();

    server_impl_ = 
      new AdServer::ChannelSvcs::DictionaryProviderImpl(
        logger(), configuration_.get());

    corba_server_adapter_->add_binding(
        DICTIONARY_PROVIDER,
        server_impl_.in());

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

    server_impl_->activate_object();
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "DictionaryProviderApp::init_corba_"
      << "Catch exception on creating ChannelServer servants. "
      << ":" << e.what();
    throw Exception(ostr);
  }

}

void DictionaryProviderApp_::main(int& argc, char** argv) noexcept
{
  const char FUN[] = "DictionaryProviderApp_::main()";
  try
  {

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n"
        <<  "usage:" << ASPECT <<" <config_file>";
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
      std::ostringstream ostr;
      ostr << FUN << ": got LoggerConfigReader::Exception: "
           << e.what();
      throw Exception(ostr.str());
    }

    //Initialization CORBA
    init_corba_();

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
      logger()->sstream(Logging::Logger::EMERGENCY,
                       ASPECT,
                       "ADS-IMPL-33")
        << "DictionaryProviderApp_::main(): "
        << "Got DictionaryProviderApp_::Exception. : "
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("DictionaryProviderApp_::main(): "
                    "Got DictionaryProviderApp_::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-33");
    }
  }
  catch (const CORBA::SystemException& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                       ASPECT,
                       "ADS-IMPL-33")
        << "DictionaryProviderApp_::main(): "
        << "Got CORBA::SystemException. : \n"
        << e;
    }
    catch (...)
    {
      logger()->log(String::SubString("DictionaryProviderApp_::main(): "
                    "Got CORBA::SystemException. : \n"),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-33");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                       ASPECT,
                       "ADS-IMPL-33")
        << "DictionaryProviderApp_::main(): "
        << "Got eh::Exception. : \n"
        << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("DictionaryProviderApp_::main(): "
                    "Got eh::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-33");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString("DictionaryProviderApp_::main(): "
                  "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-33");
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
    logger()->sstream(Logging::Logger::EMERGENCY,
                     ASPECT)
      << "DictionaryProviderApp_::main(): "
      << "Got CORBA::Exception in destroy ORB. : \n"
      << ex;
  }
  catch(...)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                     ASPECT)
      << "DictionaryProviderApp_::main(): "
      << "Got unknown exception in destroy ORB \n";
  }
}

int main(int argc, char** argv)
{
  DictionaryProviderApp_* app = 0;
  try
  {
    app = &DictionaryProviderApp::instance();
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

