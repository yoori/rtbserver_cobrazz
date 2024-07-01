
#ifndef _AD_SERVER_OPTOUT_FRONTEND_HPP_
#define _AD_SERVER_OPTOUT_FRONTEND_HPP_

#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <String/StringManip.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>
#include <userver/engine/task/task_processor.hpp>

#include <CORBA/CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaConfig.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>

#include "OptoutFrontendStat.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class OptoutFrontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
    typedef FrontendCommons::HTTPExceptions::Exception Exception;
    DECLARE_EXCEPTION(RequestFailure, Exception);

  public:
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using TaskProcessor = userver::engine::TaskProcessor;
    using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
    using HttpResponse = FrontendCommons::HttpResponse;

  public:
    OptoutFrontend(
      const GrpcContainerPtr& grpc_container,
      TaskProcessor& task_processor,
      const SchedulerPtr& scheduler,
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

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::OptOutFeConfiguration_type
      OptOutFeConfiguration;

    typedef std::list<std::string> StringList;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<OptOutFeConfiguration> ConfigPtr;
    typedef std::unique_ptr<FrontendCommons::CookieManager<
      FrontendCommons::HttpRequest, FrontendCommons::HttpResponse> >
      CookieManagerPtr;

  private:
    virtual
    ~OptoutFrontend() noexcept;

    void
    parse_config_() /*throw(Exception)*/;

    int
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      HttpResponse& response)
      noexcept;

    void
    set_OO_cookie(
      const String::SubString& oo_value,
      HttpResponse& response,
      const FrontendCommons::HttpRequest& request,
      const Generics::Time& cookie_expire_time)
      /*throw(eh::Exception)*/;

    unsigned long calculate_status_(
      OptOperation operation,
      OptOperation old_operation) noexcept;

    int handle_status_operation_(
      const OptOperation old_oo_type,
      const std::string& oo_status_in_redirect_url,
      const std::string& oo_status_out_redirect_url,
      const std::string& oo_status_undef_redirect_url,
      HttpResponse& response) noexcept;

  private:
    const GrpcContainerPtr grpc_container_;
    TaskProcessor& task_processor_;
    const SchedulerPtr scheduler_;

    /* configuration */
    std::string config_file_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    CookieManagerPtr cookie_manager_;
    OptOutFrontendStat_var stats_;

    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;

    std::unique_ptr<OptOut::RequestInfoFiller> request_info_filler_;
  };
}

//
// Inlines
//
namespace AdServer
{
  /**
   * AdFrontend class
   */
  inline
  OptoutFrontend::~OptoutFrontend() noexcept
  {
  }
} /* AdServer */

#endif // _AD_SERVER_OPTOUT_FRONTEND_HPP_
