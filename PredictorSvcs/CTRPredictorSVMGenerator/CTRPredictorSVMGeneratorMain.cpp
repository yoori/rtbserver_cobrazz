#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <XMLUtility/Utility.cpp>
#include "CTRPredictorSVMGeneratorMain.hpp"
#include "CTRPredictorSVMGeneratorImpl.hpp"

namespace
{
  const char ASPECT[] = "CTRPredictorSVMGenerator";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
 }

namespace AdServer
{
  namespace Predictor
  {
    CTRPredictorSVMGenerator::CTRPredictorSVMGenerator() /*throw(eh::Exception)*/
      : AdServer::Commons::ProcessControlVarsLoggerImpl(
        "CTRPredictorSVMGenerator", ASPECT)
    {}

    void
    CTRPredictorSVMGenerator::shutdown(CORBA::Boolean wait_for_completion)
      /*throw(CORBA::SystemException)*/
    {
      deactivate_object();
      wait_object();
      
      CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
    }

    CORBACommons::IProcessControl::ALIVE_STATUS
    CTRPredictorSVMGenerator::is_alive() /*throw(CORBA::SystemException)*/
    {
      return CORBACommons::ProcessControlImpl::is_alive();
    }

    void
    CTRPredictorSVMGenerator::read_config_(
      const char *filename,
      const char* argv0)
      /*throw(Exception, eh::Exception)*/
    {
      static const char* FUN = "CTRPredictorSVMGenerator::read_config()";
      
      try
      {
        Config::ErrorHandler error_handler;
        
        try
        {
          using namespace xsd::AdServer::Configuration;
          
          std::unique_ptr<SVMGeneratorConfigurationType>
            configuration = SVMGeneratorConfiguration(filename, error_handler);
          
          if (error_handler.has_errors())
          {
            std::string error_string;
            throw Exception(error_handler.text(error_string));
          }
          
          config_.reset(
            new SVMGeneratorConfig(*configuration));
          
          if (error_handler.has_errors())
          {
            std::string error_string;
            throw Exception(error_handler.text(error_string));
          }

          // Parse model config

          FeatureContainer::instance().init(config_->Model().Feature());
          
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
          Config::CorbaConfigReader::read_config(
            config_->CorbaConfig(),
            corba_config_);
        }
        catch(const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't read Corba Config: " << ex.what();
          throw Exception(ostr);
        }
        
        try
        {
          logger(Config::LoggerConfigReader::create(
                   config_->Logger(), argv0));
          
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
        ostr << FUN << ": got Exception. Invalid configuration: " <<
          ex.what();
        throw Exception(ostr);
      }
    }
    
    void
    CTRPredictorSVMGenerator::init_corba_() /*throw(Exception)*/
    {
      try
      {
        corba_server_adapter_ =
          new CORBACommons::CorbaServerAdapter(corba_config_);
        shutdowner_ = corba_server_adapter_->shutdowner();
        corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "CTRPredictorSVMGenerator::init_corba(): "
          "Can't init CorbaServerAdapter: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CTRPredictorSVMGenerator::main(int& argc, char** argv) noexcept
    {
      static const char* FUN = "CTRPredictorSVMGenerator::main()";
      
      try
      {
        XMLUtility::initialize();
      }
      catch(const eh::Exception& ex)
      {
        logger()->sstream(
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-205") << FUN << ": Got eh::Exception: " << ex.what();
        return;
      }
      
      try
      {
        if (argc < 2)
        {
          Stream::Error ostr;
          ostr << "config file or colocation config file is not specified\n"
            "usage: CTRPredictorSVMGenerator <config_file>";
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
        catch(...)
        {
          Stream::Error ostr;
          ostr << "Unknown Exception at parsing of config.";
          throw Exception(ostr);
        }
        
        register_vars_controller();
        init_corba_();
        activate_object();

        add_child_object(
          Generics::ActiveObject_var(
            new CTRPredictorSVMGeneratorImpl(
              logger(),
              callback(),
              config_->input_path().c_str(),
              config_->output_path().c_str(),
              config_->log_days_to_keep(),
              config_->Model().features_dimension())));
        
        logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
        corba_server_adapter_->run();
        wait();
        logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
        XMLUtility::terminate();
      }
      catch (const Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Got CTRPredictorSVMGeneratorApp_::Exception: " <<
          e.what();
        logger()->log(
          ostr.str(),
          Logging::Logger::CRITICAL,
          ASPECT,
          "ADS-IMPL-150");
      }
      catch (const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Got CORBA::SystemException: " << e;
        logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-150");
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Got eh::Exception: " << e.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-150");
      }
      catch (...)
      {
        Stream::Error ostr;
        ostr << FUN << ": Got unknown exception";
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-150");
      }
    }
  }
}

int
main(int argc, char** argv)
{
  AdServer::Predictor::CTRPredictorSVMGenerator* app = 0;

  try
  {
    app = &AdServer::Predictor::CTRPredictorSVMGeneratorApp::instance();
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
