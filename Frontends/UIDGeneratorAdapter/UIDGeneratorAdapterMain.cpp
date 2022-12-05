#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "UIDGeneratorAdapterMain.hpp"

namespace
{
  const char* ASPECT = "UIDGeneratorAdapter";
  //const char* UID_GENERATOR_ADAPTER_OBJ_KEY = "UIDGeneratorAdapter";
  const char* PROCESS_CONTROL_OBJ_KEY = "ProcessControl";
}

UIDGeneratorAdapterApp_::~UIDGeneratorAdapterApp_()
  noexcept
{}

void
UIDGeneratorAdapterApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownSyncPolicy::WriteGuard guard(shutdown_lock_);

  if(uid_generator_adapter_impl_.in())
  {
    uid_generator_adapter_impl_->deactivate_object();
    uid_generator_adapter_impl_->wait_object();

    // stop listening
    uid_generator_adapter_impl_ = AdServer::Frontends::UIDGeneratorAdapterImpl_var();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
UIDGeneratorAdapterApp_::is_alive()
  /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
UIDGeneratorAdapterApp_::read_config_(
  const char *filename,
  const char* argv0)
  /*throw(Exception, eh::Exception)*/
{
  static const char* FUN = "UIDGeneratorAdapterApp_::read_config_()";

  try
  {
    ::Config::ErrorHandler error_handler;

    try
    {
      using namespace xsd::AdServer::Configuration;

      std::unique_ptr<AdConfigurationType> ad_configuration =
        AdConfiguration(filename, error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      config_.reset(new Config(
        ad_configuration->UIDGeneratorAdapterConfig()));

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }
    }
    catch(const xml_schema::parsing& ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << filename << "'. : ";
      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr);
    }

    try
    {
      ::Config::CorbaConfigReader::read_config(
        config_->CorbaConfig(),
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
      logger(::Config::LoggerConfigReader::create(
        config_->Logger(), argv0));
    }
    catch(const ::Config::LoggerConfigReader::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
  catch(const Exception &ex)
  {
    Stream::Error ostr;
    ostr << FUN << ": got Exception. Invalid configuration: " <<
      ex.what();
    throw Exception(ostr);
  }
}

void
UIDGeneratorAdapterApp_::init_corba_()
  /*throw(Exception)*/
{
  static const char* FUN = "UIDGeneratorAdapterApp_::init_corba_()";

  try
  {
    corba_server_adapter_ = new CORBACommons::CorbaServerAdapter(
      corba_config_);

    shutdowner_ = corba_server_adapter_->shutdowner();

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << FUN << ": Can't init CorbaServerAdapter: " << ex.what();
    throw Exception(ostr);
  }
}

void
UIDGeneratorAdapterApp_::main(int& argc, char** argv) noexcept
{
  static const char* FUN = "UIDGeneratorAdapterApp_::main()";

  try
  {
    if(argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file or colocation config file is not specified\n"
        "usage: UIDGeneratorAdapter <config_file>";
      throw Exception(ostr);
    }

    try
    {
      read_config_(argv[1], argv[0]);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "': " <<
        ex.what();
      throw Exception(ostr);
    }

    corba_server_adapter_ = new CORBACommons::CorbaServerAdapter(
      corba_config_);

    shutdowner_ = corba_server_adapter_->shutdowner();

    uid_generator_adapter_impl_ = new AdServer::Frontends::UIDGeneratorAdapterImpl(
      logger(),
      *config_);

    register_vars_controller();

    /*
    corba_server_adapter_->add_binding(
      UID_GENERATOR_ADAPTER_OBJ_KEY,
      uid_generator_adapter_impl_.in());
    */

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY,
      this);

    uid_generator_adapter_impl_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    corba_server_adapter_->run();

    wait();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

    uid_generator_adapter_impl_.reset();
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << FUN << ": Got CORBA::SystemException: " << e;
    logger()->log(ostr.str(),
      Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-150");
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << FUN << ": Got eh::Exception: " << e.what();
    logger()->log(ostr.str(),
      Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-150");
  }
}

int
main(int argc, char** argv)
{
  UIDGeneratorAdapterApp_* app = 0;

  try
  {
    app = &UIDGeneratorAdapterApp::instance();
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

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
