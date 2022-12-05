#include <iostream>

#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <XMLUtility/Utility.hpp>

#include "UserOperationGeneratorMain.hpp"

namespace
{
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

namespace AdServer
{
  namespace UserInfoSvcs
  {
    namespace Aspect
    {
      const char USER_OPERATION_GENERATOR[] = "UserOperationGenerator";
    }

    UserOperationGeneratorApp::UserOperationGeneratorApp()
      /*throw(eh::Exception)*/
    {}

    void
    UserOperationGeneratorApp::main(
      int argc,
      char** argv)
      noexcept
    {
      static const char* FUN = "UserOperationGeneratorApp::main";

      try
      {
        if (argc < 2)
        {
          const char *usage = "usage: UserOperationGeneratorApp <config_file>";

          Stream::Error ostr;
          ostr << "config file is not specified\n"
              << usage;
          throw Exception(ostr);
        }

        XMLUtility::initialize();
        Config::ErrorHandler error_handler;

        try
        {
          using namespace xsd::AdServer::Configuration;

          std::unique_ptr<AdConfigurationType>
            ad_configuration = AdConfiguration(argv[1], error_handler);

          if (error_handler.has_errors())
          {
            std::string error_string;
            throw Exception(error_handler.text(error_string));
          }

          config_ = std::make_shared<UserOperationGeneratorImpl::ConfigType>(
            ad_configuration->UserOperationGeneratorConfig());

          if (error_handler.has_errors())
          {
            std::string error_string;
            throw Exception(error_handler.text(error_string));
          }
        }
        catch (const xml_schema::parsing &ex)
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

        try
        {
          Config::CorbaConfigReader::read_config(
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
          logger(Config::LoggerConfigReader::create(
            config_->Logger(), argv[0]));
        }
        catch (const Config::LoggerConfigReader::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
          throw Exception(ostr);
        }

        register_vars_controller();

        user_operation_generator_impl_ = new UserOperationGeneratorImpl(
          config_, callback(), logger());

        corba_server_adapter_ =
          new CORBACommons::CorbaServerAdapter(corba_config_);
        shutdowner_ = corba_server_adapter_->shutdowner();
        corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

        user_operation_generator_impl_->activate_object();
        logger()->sstream(Logging::Logger::NOTICE, Aspect::USER_OPERATION_GENERATOR) << "service started.";
        corba_server_adapter_->run();
        wait();

        logger()->sstream(Logging::Logger::NOTICE, Aspect::USER_OPERATION_GENERATOR) << "service stopped.";
      }
      catch (const Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_OPERATION_GENERATOR) <<
          FUN << " : Got Exception: " << ex.what();
      }
      catch (const eh::Exception &ex)
      {
        logger()->sstream(Logging::Logger::EMERGENCY, Aspect::USER_OPERATION_GENERATOR) <<
          FUN << " : Got eh::Exception: " << ex.what();
      }

      XMLUtility::terminate();
    }

    void
    UserOperationGeneratorApp::shutdown(CORBA::Boolean wait_for_completion)
      /*throw(CORBA::SystemException)*/
    {
      Sync::PosixGuard guard(shutdown_lock_);

      if (user_operation_generator_impl_.in())
      {
        user_operation_generator_impl_->deactivate_object();
        user_operation_generator_impl_->wait_object();
      }

      CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
    }
  }
}

int
main(int argc, char** argv)
{
  AdServer::UserInfoSvcs::UserOperationGeneratorApp* app = 0;

  try
  {
    app = &AdServer::UserInfoSvcs::UserOperationGeneratorSingleton::instance();
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
