
#pragma once

#include <coroutine>
#include <exception>
#include <memory>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Uuid.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <HTTP/Http.hpp>

#include <Commons/AtomicInt.hpp>
#include <Commons/ExecutorPool.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
  class ClickRequestState;

  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class ClickFrontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::CoroFrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    typedef FrontendCommons::HTTPExceptions::Exception Exception;
    friend class ClickRequestState;

  public:
    ClickFrontend(
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

    /** Executed if will_handle returns true; processes an HTTP request. Provides
     *  browsing context to a Channel Manager, returns list of channels in the HTTP
     *  response.
     *
     * @param uri The URI for assessing.
     * @param params HTTP request parameters.
     * @param headers HTTP request headers. This is an in/out argument; handle_request may also
     * return headers.
     * @param cookies HTTP cookies. This is an in/out argument; handle_request may also
     * set cookies.
     * @param istr Input stream to read the HTTP request body from.
     * @param ostr Output stream to write the HTTP response body to.
     *
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

    bool
    log(const String::SubString& text,
      unsigned long severity = Logging::Logger::INFO,
      const char* aspect = 0,
      const char* error_code = 0) const
      noexcept;

    unsigned long
    log_level() noexcept;

  private:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::ClickFeConfiguration_type
      ClickFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<ClickFeConfiguration> ConfigPtr;

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

  private:
    virtual
    ~ClickFrontend() noexcept;

    FrontendCommons::RequestTask
    process_request_(
      FCGI::HttpRequestHolder_var request_holder);

    void
    parse_config_() /*throw(Exception)*/;

    void
    check_constraints_(
      const FrontendCommons::ParsedParamsMap& params,
      const FCGI::HttpRequest& request) const
      /*throw(ForbiddenException, InvalidParamException)*/;

    void
    resolve_channel_session_() /*throw(Exception)*/;

    void
    match_click_channels_(
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::UserId& cookie_user_id,
      const Generics::Time& now,
      unsigned long campaign_id,
      unsigned long advertiser_id,
      const String::SubString& peer_ip,
      const std::list<std::string>& markers)
      noexcept;

    void
    fill_match_request_info_(
      adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& now,
      const adserver::channel_svcs::channel_server::MatchResponse* trigger_match_result,
      const adserver::user_info_svcs::user_info_manager::MatchResponse*
        history_match_result,
      const String::SubString& peer_ip_val) const
      noexcept;

  private:
    std::string config_file_;

    /* configuration */
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<ClickFE::RequestInfoFiller> request_info_filler_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
      channel_client_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
      user_info_client_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;

    Generics::StringHashAdapter click_template_file_;
    Commons::TextTemplateCache_var template_files_;

    typedef std::shared_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;
    IPMapPtr ip_map_;

    std::shared_ptr<AdServer::Commons::ExecutorPool> match_workers_;
    Algs::AtomicInt match_task_count_;

    std::unique_ptr<FrontendCommons::CookieManager<
      FCGI::HttpRequest, FCGI::HttpResponse> > cookie_manager_;
    SetUidController_var set_uid_controller_;
  };

  typedef ReferenceCounting::QualPtr<ClickFrontend> ClickFrontend_var;
}

//
// Inlines
//
namespace AdServer
{
  inline
  ClickFrontend::~ClickFrontend() noexcept
  {}

  inline
  unsigned long
  ClickFrontend::log_level() noexcept
  {
    if (!logger())
    {
      return 0;
    }

    return config_->Logger().log_level();
  }
} // namespace AdServer
