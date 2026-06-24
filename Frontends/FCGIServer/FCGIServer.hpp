#pragma once

//#include <limits.h>

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <BiddingFrontend/BiddingFrontendStat.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <xsd/Frontends/FCGIServerConfig.hpp>

#include <FrontendCommons/FrontendInterface.hpp>
#include <Generics/CompositeMetricsProvider.hpp>

#include "FCGIAcceptorStats.hpp"

namespace AdServer::Frontends
{
  class FCGIServer:
    private Logging::LoggerCallbackHolder,
    public virtual ReferenceCounting::AtomicImpl,
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

  private:
    using Logging::LoggerCallbackHolder::callback;
    using Logging::LoggerCallbackHolder::logger;

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
    init_fcgi_() /*throw(Exception)*/;

  private:
    FCGIServerConfigPtr config_;
    FCGIAcceptorStatHolder_var fcgi_stats_;
    StatHolder_var stats_;
    Generics::CompositeMetricsProvider_var composite_metrics_provider_;
    FrontendCommons::Frontend_var frontend_pool_;
    AdServer::Commons::HttpServer::HttpServer_var http_server_;
  };

  typedef ReferenceCounting::QualPtr<FCGIServer> FCGIServer_var;
  typedef Generics::Singleton<FCGIServer, FCGIServer_var> FCGIServerApp;
}
