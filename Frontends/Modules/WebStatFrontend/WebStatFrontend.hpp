#ifndef ADSERVER_WEBSTATFRONTEND_HPP
#define ADSERVER_WEBSTATFRONTEND_HPP

#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/FileCache.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <String/StringManip.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <CORBA/CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaObject.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace WebStat
{
  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public ReferenceCounting::AtomicImpl
  {
  public:
    using TaskProcessor = userver::engine::TaskProcessor;
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    Frontend(
      TaskProcessor& helper_task_processor,
      const GrpcContainerPtr& grpc_container,
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module,
      FrontendCommons::HttpResponseFactory* response_factory)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  private:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::WebStatFeConfiguration_type
      WebStatFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<WebStatFeConfiguration> ConfigPtr;

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

    typedef std::unique_ptr<
      Commons::CorbaObject<AdServer::CampaignSvcs::CampaignManager> >
      CampaignManagerRef;

  private:
    virtual ~Frontend() noexcept;

    void parse_config_() /*throw(Exception)*/;

    int
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      noexcept;

  private:
    TaskProcessor& helper_task_processor_;
    const GrpcContainerPtr grpc_container_;

    // configuration
    Configuration_var frontend_config_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;

    FileCachePtr pixel_;
  };
} /*WebStat*/
} /*AdServer*/

//
// Inlines
//
namespace AdServer
{
namespace WebStat
{
  inline
  Frontend::~Frontend() noexcept
  {
  }
}
} /* AdServer */

#endif // ADSERVER_WEBSTATFRONTEND_HPP
