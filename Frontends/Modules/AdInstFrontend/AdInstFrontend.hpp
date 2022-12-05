#ifndef ADSERVER_ADINSTFRONTEND_HPP
#define ADSERVER_ADINSTFRONTEND_HPP

#include <string>
#include <vector>
#include <list>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Uuid.hpp>
#include <Sync/PosixLock.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/TaskScheduler.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Instantiate
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
    typedef FrontendCommons::HTTPExceptions::Exception Exception;

  public:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::AdInstFeConfiguration_type
      AdInstFeConfiguration;

    typedef FCGI::HttpResponse HttpResponse;

    Frontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    /** Determines whether the module is able to process the URI.
     *
     * @param uri URI provided by Apache.
     */
    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    /**
     * Executed if will_handle returns true; processes an HTTP request. Provides
     * browsing context to a Channel Manager, returns list of channels in the HTTP
     * response.
     * @param request HTTP request.
     * @param response The object to write the HTTP response body.
     * @return HTTP status code.
     */
    virtual void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponseWriter_var response_writer)
      noexcept;

    virtual void
    handle_request_noparams_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponseWriter_var response_writer)
      noexcept;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  protected:
    virtual ~Frontend() noexcept;

  private:
    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<AdInstFeConfiguration> ConfigPtr;
    typedef std::unique_ptr<
      FrontendCommons::CookieManager<FCGI::HttpRequest, FCGI::HttpResponse> >
      CookieManagerPtr;

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    void
    merge_users_(
      bool& merge_success,
      std::string& merge_error_message,
      const RequestInfo& request_info)
      noexcept;

    int
    instantiate_click_(
      HttpResponse& response,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult*
        inst_ad_result)
      /*throw(Exception)*/;

    int
    instantiate_ad_(
      HttpResponse& response,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_creative_type)
      /*throw(Exception)*/;

    void
    log_request_(
      const char* function_name,
      const FCGI::HttpRequest& request,
      unsigned int log_level)
      /*throw(eh::Exception)*/;

  private:
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    //std::string fe_config_path_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    CookieManagerPtr cookie_manager_;

    /* external services */
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    FrontendCommons::UserInfoClient_var user_info_client_;
  };
}
}

// Inlines
namespace AdServer
{
namespace Instantiate
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
}

#endif /*ADSERVER_ADINSTFRONTEND_HPP*/
