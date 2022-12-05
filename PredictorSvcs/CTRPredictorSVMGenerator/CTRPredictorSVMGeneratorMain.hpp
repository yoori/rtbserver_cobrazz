#pragma once

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <xsd/Predictor/CTRPredictorSVMGeneratorConfig.hpp>
#include <Utils/CTRGenerator/CTRGenerator.hpp>
#include "FeatureContainer.hpp"

namespace AdServer
{
  namespace Predictor
  {
    class CTRPredictorSVMGenerator:
      public AdServer::Commons::ProcessControlVarsLoggerImpl,
      private Generics::CompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
     
    public:
      CTRPredictorSVMGenerator() /*throw(eh::Exception)*/;

      /**
       * Parses command line, opens config file,
       * creates corba objects, initialize.
       */
      void
      main(int& argc, char** argv) noexcept;

      //
      // IDL:CORBACommons/IProcessControl/shutdown:1.0
      //
      virtual void
      shutdown(CORBA::Boolean wait_for_completion)
        /*throw(CORBA::SystemException)*/;
      
      //
      // IDL:CORBACommons/IProcessControl/is_alive:1.0
      //
      virtual CORBACommons::IProcessControl::ALIVE_STATUS
      is_alive() /*throw(CORBA::SystemException)*/;
      
    private:
      typedef xsd::AdServer::Configuration::SVMGeneratorConfigurationType SVMGeneratorConfig;
      typedef std::unique_ptr<SVMGeneratorConfig> SVMGeneratorConfigPtr;
      
    private:
      virtual
      ~CTRPredictorSVMGenerator() noexcept
      {}
      
      void
      read_config_(
        const char *filename,
        const char* argv0)
        /*throw(Exception, eh::Exception)*/;
      
      void
      init_corba_() /*throw(Exception)*/;
      
    private:
      CORBACommons::CorbaConfig corba_config_;
      SVMGeneratorConfigPtr config_;
      CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
    };

    typedef ReferenceCounting::QualPtr<CTRPredictorSVMGenerator>
      CTRPredictorSVMGenerator_var;
    typedef Generics::Singleton<CTRPredictorSVMGenerator, CTRPredictorSVMGenerator_var>
      CTRPredictorSVMGeneratorApp;
  }
}
