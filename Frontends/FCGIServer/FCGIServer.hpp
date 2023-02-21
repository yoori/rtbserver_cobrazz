#pragma once

//#include <limits.h>

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <BiddingFrontend/BiddingFrontendStat.hpp>
#include <xsd/Frontends/FCGIServerConfig.hpp>

#include <FrontendCommons/FrontendInterface.hpp>
#include <UServerUtils/MetricsHTTPProvider.hpp>
#include <Generics/CompositeMetricsProvider.hpp>

namespace AdServer
{
namespace Frontends
{
  class FCGIServer:
    public AdServer::Commons::ProcessControlVarsLoggerImpl,
    private Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    FCGIServer() /*throw(eh::Exception)*/;

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
    typedef xsd::AdServer::Configuration::FCGIServerConfigType
      FCGIServerConfig;

    typedef std::unique_ptr<FCGIServerConfig> FCGIServerConfigPtr;

  private:
    virtual
    ~FCGIServer() noexcept
    {}

    void
    read_config_(
      const char *filename,
      const char* argv0)
      /*throw(Exception, eh::Exception)*/;

    void
    init_corba_() /*throw(Exception)*/;

    void
    init_fcgi_() /*throw(Exception)*/;

  private:
    CORBACommons::CorbaConfig corba_config_;
    FCGIServerConfigPtr config_;
    CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
    StatHolder_var stats_;
    // add CompositeMetricsProvider here

    FrontendCommons::Frontend_var frontend_pool_;
  };

  typedef ReferenceCounting::QualPtr<FCGIServer> FCGIServer_var;
  typedef Generics::Singleton<FCGIServer, FCGIServer_var> FCGIServerApp;  
}
}
