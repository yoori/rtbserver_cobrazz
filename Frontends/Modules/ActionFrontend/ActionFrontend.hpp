
#ifndef _AD_FRONTENDS_ACTION_FRONTEND_ACTION_FRONTEND_HPP_
#define _AD_FRONTENDS_ACTION_FRONTEND_ACTION_FRONTEND_HPP_

#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Generics/FileCache.hpp>
#include <Generics/Uuid.hpp>
#include <GeoIP/IPMap.hpp>
#include <String/TextTemplate.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/TextTemplateCache.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/UserBindClient.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/ChannelServerSessionPool.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "ActionFrontendStat.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Action
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
  private:
    using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
    using ManagerCoro = UServerUtils::Grpc::Manager;
    using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
    using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    typedef ReferenceCounting::SmartPtr<Frontend> Frontend_var;
    static Frontend_var instance;

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
    virtual void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponseWriter_var response_writer)
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
      Commons::TextTemplate_var url_template;

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

    int
    process_advertiser_request(
      FCGI::HttpResponse& response,
      const FCGI::HttpRequest& request,
      const RequestInfo& request_info,
      bool return_html)
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
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var& trigger_match_result,
      unsigned long conv_id,
      const Generics::Time& now,
      const AdServer::Commons::UserId& user_id,
      const String::SubString& referer)
      noexcept;

    bool
    resolve_user_id_(
      Commons::UserId& result_user_id,
      const String::SubString& external_user_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& time)
      noexcept;

    void
    fill_match_request_info_(
      AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo& mri,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& now,
      const AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var& trigger_match_result) const
      noexcept;

    class MatchActionChannelsTask;

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
    ChannelServerSessionFactoryImpl_var server_session_factory_;
    std::unique_ptr<FrontendCommons::ChannelServerSessionPool>
      channel_servers_;
    FrontendCommons::UserInfoClient_var user_info_client_;
    CookieManagerPtr cookie_manager_;
    ManagerCoro_var manager_coro_;

    IPMapPtr ip_map_;
    FileCachePtr track_pixel_;
    FileCachePtr track_html_;
    AcFrontendStat_var stats_;
    RedirectRuleArray redirect_rules_;

    Generics::TaskRunner_var task_runner_;

    /* external services */
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    FrontendCommons::UserBindClient_var user_bind_client_;
  };
}
}

namespace AdServer
{
namespace Action
{
  /* Frontend class */
  inline
  Frontend::~Frontend() noexcept
  {}
}
} // namespace AdServer

#endif // _AD_FRONTENDS_ACTION_FRONTEND_ACTION_FRONTEND_HPP_
