
#include <eh/Exception.hpp>

#include <memory>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>

#include "DictionaryProviderMain.hpp"
#include "DictionaryProviderImpl.hpp"

namespace
{
  char ASPECT[] = "DictionaryProvider";
  char DICTIONARY_PROVIDER[] = "DictionaryProvider";

  template<typename T>
  std::shared_ptr<T>
  to_shared(ReferenceCounting::SmartPtr<T> ptr)
  {
    T* raw_ptr = ptr.in();
    return std::shared_ptr<T>(
      raw_ptr,
      [ptr = std::move(ptr)](T*) mutable
      {
        ptr.reset();
      });
  }
}

DictionaryProviderApp_::DictionaryProviderApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr))),
      "DictionaryProviderApp_", ASPECT, 0),
    server_impl_()
{
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

    server_impl_ = to_shared<AdServer::ChannelSvcs::DictionaryProviderImpl>(
      new AdServer::ChannelSvcs::DictionaryProviderImpl(
        logger(), configuration_.get()));

    corba_server_adapter_->add_binding(
        DICTIONARY_PROVIDER,
        server_impl_.get());

    active_objects_ =
      std::make_shared<Generics::CompositeActiveObject>(false, false);
    active_objects_->add_child_object(
      std::static_pointer_cast<Generics::ActiveObject>(server_impl_));
    active_objects_->add_child_object(corba_server_adapter_.in());
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
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

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

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(configuration_->pid_file()));

    //Initialization CORBA
    init_corba_();

    AdServer::Commons::SignalActiveObject signal_active_object;
    active_objects_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    signal_active_object.wait_object();
    active_objects_->deactivate_object();
    active_objects_->wait_object();

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
