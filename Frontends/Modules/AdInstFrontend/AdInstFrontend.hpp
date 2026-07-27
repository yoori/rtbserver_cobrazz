#pragma once

#include <functional>
#include <string>
#include <vector>
#include <list>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/Uuid.hpp>
#include <Sync/PosixLock.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/TaskScheduler.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Commons/Coro/Awaitable.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer::Instantiate
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::CoroFrontendInterface,
    public Generics::CompositeActiveObject,
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
      std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
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
    FrontendCommons::RequestTask
    co_handle_request(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

    FrontendCommons::RequestTask
    co_handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

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
    FrontendCommons::RequestTask
    co_process_request_(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept;

    FrontendCommons::RequestTask
    handle_request_noparams_(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept;

    void
    parse_configs_() /*throw(Exception)*/;

    struct MergeUsersResult
    {
      bool success = true;
      std::string error_message;
    };

    using MergeUsersTask = AdServer::Commons::Awaitable<MergeUsersResult>;
    using InstantiateTask = AdServer::Commons::Awaitable<int>;

    MergeUsersTask
    co_merge_users_(
      const RequestInfo& request_info)
      noexcept;

    InstantiateTask
    co_instantiate_click_(
      FCGI::HttpResponse_var response,
      const RequestInfo& request_info,
      const adserver::campaign_svcs::campaign_manager::InstantiateAdResult&
        inst_ad_result)
      noexcept;

    InstantiateTask
    co_instantiate_ad_(
      FCGI::HttpResponse_var response,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_creative_type)
      noexcept;

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
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
      user_info_client_coro_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
  };
}

// Inlines
namespace AdServer::Instantiate
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
