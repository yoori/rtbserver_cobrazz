
#pragma once

#include <memory>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Generics/FileCache.hpp>
#include <Generics/Uuid.hpp>
#include <GeoIP/IPMap.hpp>
#include <String/TextTemplate.hpp>

#include <Commons/AtomicInt.hpp>
#include <Commons/ExecutorPool.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Commons/TextTemplateAsyncCache.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Commons/Coro/Awaitable.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "ActionFrontendStat.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer::Action::Configuration
{
  using namespace xsd::AdServer::Configuration;
}

namespace AdServer::Action
{
  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::CoroFrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
    typedef FrontendCommons::HTTPExceptions::Exception Exception;

  public:
    typedef ReferenceCounting::SmartPtr<Frontend> Frontend_var;
    static Frontend_var instance;

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
    co_handle_request(FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  private:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::ActionFeConfiguration_type
      ActionFeConfiguration;

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<ActionFeConfiguration> ConfigPtr;
    typedef std::unique_ptr<GeoIPMapping::IPMap> IPMapPtr;

    struct DerivedConfig
    {
      xsd::AdServer::Configuration::UriListType::Uri_sequence advertiser_service_uri;
      Commons::LogReferrer::Setting use_referrer;
    };

    struct RedirectRule: public ReferenceCounting::AtomicImpl
    {
    public:
      bool use_keywords;
      Generics::GnuHashSet<Generics::StringHashAdapter> keywords;
      Commons::TextTemplatePtr url_template;

    protected:
      virtual
      ~RedirectRule() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<RedirectRule> RedirectRule_var;

    typedef std::vector<RedirectRule_var> RedirectRuleArray;

  private:
    virtual
    ~Frontend() noexcept;

    void
    parse_config_() /*throw(Exception)*/;

    void
    write_pixel(FCGI::HttpResponse& response)
      /*throw(eh::Exception)*/;

    void
    write_html(FCGI::HttpResponse& response)
      /*throw(eh::Exception)*/;

    FrontendCommons::RequestTask
    process_advertiser_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponse_var response,
      RequestInfo request_info,
      bool return_html)
      noexcept;

    int
    fill_advertiser_response_(
      FCGI::HttpResponse& response,
      const FCGI::HttpRequest& request,
      const RequestInfo& request_info,
      bool return_html,
      const Commons::UserId& cookie_resolved_user_id,
      const Commons::UserId& utm_cookie_resolved_user_id)
      /*throw(Exception, eh::Exception)*/;

    void
    action_taken_all_(
      const RequestInfo& request_info,
      const Commons::UserId& utm_cookie_resolved_user_id)
      noexcept;

    void
    trigger_match_all_(
      const RequestInfo& request_info,
      const Commons::UserId& utm_cookie_resolved_user_id)
      noexcept;

    void
    relink_user_id_all_(
      const RequestInfo& request_info,
      const Commons::UserId& utm_cookie_resolved_user_id,
      const Commons::UserId& link_user_id)
      noexcept;

    void
    trigger_match_(
      unsigned long conv_id,
      const Generics::Time& now,
      const AdServer::Commons::UserId& user_id,
      const String::SubString& referer)
      noexcept;

    struct ResolveUserIdResult
    {
      bool success = false;
      Commons::UserId user_id;
    };

    using ResolveUserIdTask = AdServer::Commons::Awaitable<ResolveUserIdResult>;

    ResolveUserIdTask
    co_resolve_user_id_(
      std::string external_user_id,
      Commons::UserId current_user_id,
      Generics::Time time)
      noexcept;

    FrontendCommons::RequestTask
    co_trigger_match_(
      unsigned long conv_id,
      Generics::Time now,
      AdServer::Commons::UserId user_id,
      std::string referer)
      noexcept;

    FrontendCommons::RequestTask
    co_add_user_id_(std::string external_user_id, Commons::UserId user_id, Generics::Time time)
      noexcept;

    FrontendCommons::RequestTask
    co_action_taken_(adserver::campaign_svcs::campaign_manager::ActionTakenRequest request)
      noexcept;

    void
    fill_match_request_info_(
      adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& now,
      const adserver::channel_svcs::channel_server::MatchResponse& trigger_match_result) const
      noexcept;

    typedef std::unique_ptr<FrontendCommons::CookieManager<
      FCGI::HttpRequest, FCGI::HttpResponse> >
      CookieManagerPtr;

  private:
    // configuration
    //std::string config_file_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    DerivedConfig derived_config_;
    CommonModule_var common_module_;

    std::unique_ptr<AdServer::Action::RequestInfoFiller> request_info_filler_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>
      channel_client_coro_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
      user_info_client_coro_;
    CookieManagerPtr cookie_manager_;

    IPMapPtr ip_map_;
    FileCachePtr track_pixel_;
    FileCachePtr track_html_;
    AcFrontendStat_var stats_;
    RedirectRuleArray redirect_rules_;

    std::shared_ptr<AdServer::Commons::ExecutorPool> match_workers_;
    Algs::AtomicInt match_task_count_;

    /* external services */
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
      user_bind_client_coro_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
  };
}

namespace AdServer::Action
{
  /* Frontend class */
  inline
  Frontend::~Frontend() noexcept
  {}
}
