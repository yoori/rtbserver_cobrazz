#ifndef ADSERVER_USERBINDFRONTEND_HPP
#define ADSERVER_USERBINDFRONTEND_HPP

#include <eh/Exception.hpp>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/FileCache.hpp>
#include <Generics/Uuid.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <String/TextTemplate.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Commons/AtomicInt.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/FrontendCommons/UserBindClient.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/ChannelServerSessionPool.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  struct ChannelMatch
  {
    ChannelMatch(
      unsigned long channel_id_val,
      unsigned long channel_trigger_id_val)
      : channel_id(channel_id_val),
        channel_trigger_id(channel_trigger_id_val)
    {}

    bool operator<(const ChannelMatch& right) const
    {
      return
        (channel_id < right.channel_id ||
          (channel_id == right.channel_id &&
            channel_trigger_id < right.channel_trigger_id));
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  class UserBindFrontend;

  typedef ReferenceCounting::SmartPtr<UserBindFrontend> UserBindFrontend_var;

  class UserBindFrontend:
    private FrontendCommons::HTTPExceptions,
    public Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
  private:
    using Exception = FrontendCommons::HTTPExceptions::Exception;

    DECLARE_EXCEPTION(InvalidSource, eh::DescriptiveException);

  public:
    using TaskProcessor = userver::engine::TaskProcessor;
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using CommonFeConfiguration = Configuration::FeConfig::CommonFeConfiguration_type;
    using UserBindFeConfiguration = Configuration::FeConfig::UserBindFeConfiguration_type;

  public:
    UserBindFrontend(
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

  protected:
    class RequestTask;
    typedef ReferenceCounting::SmartPtr<RequestTask>
      RequestTask_var;

    class UserMatchTask;

    struct BindResult;
    struct BindResultHolder;

  protected:
    virtual ~UserBindFrontend() noexcept;

    int
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      noexcept;

    int
    handle_delete_request_(
      const UserBind::RequestInfo& request_info)
      noexcept;

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
    typedef std::unique_ptr<UserBindFeConfiguration> ConfigPtr;

    typedef CORBACommons::ObjectPoolRefConfiguration
      UserBindServerPoolConfig;
    typedef CORBACommons::ObjectPool<
      AdServer::UserInfoSvcs::UserBindServer, UserBindServerPoolConfig>
      UserBindServerPool;
    typedef std::unique_ptr<UserBindServerPool> UserBindServerPoolPtr;
    typedef UserBindServerPool::ObjectHandlerType UserBindServerHandler;

    struct RedirectRule final: public ReferenceCounting::AtomicImpl
    {
      using AllowedParams = std::unordered_map<std::string, std::string>;

      bool use_keywords;
      bool passback;
      unsigned long weight;
      bool redirect_empty_uid;
      AllowedParams allowed_params;

      Generics::GnuHashSet<Generics::StringHashAdapter> keywords;
      String::TextTemplate::IStream redirect;
      std::unique_ptr<String::TextTemplate::IStream> location;

      bool init_bind_request;

    protected:
      ~RedirectRule() override = default;
    };

    typedef ReferenceCounting::SmartPtr<RedirectRule> RedirectRule_var;

    class SourceEntity final: public ReferenceCounting::AtomicImpl
    {
    public:
      std::list<RedirectRule_var> rules;

    protected:
      ~SourceEntity() override = default;
    };

    typedef ReferenceCounting::SmartPtr<SourceEntity> SourceEntity_var;
    typedef std::map<std::string, SourceEntity_var> SourceMap;

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    int
    process_request_(
      BindResultHolder& result_holder,
      const UserBind::RequestInfo& request_info,
      const String::SubString& dns_bind_request_id)
      noexcept;

    uint32_t
    calc_yandex_sign_(
      const UserBind::RequestInfo& request_info,
      const std::string& user_data,
      const std::string* location,
      const std::string& secure_key)
      noexcept;

    void
    report_bad_user_(
      const UserBind::RequestInfo& request_info)
      noexcept;

    void
    log_cookie_mapping_(const UserBind::RequestInfo& request_info) noexcept;

    static RedirectRule_var
    init_redirect_rule_(
      const String::SubString& redirect,
      const String::SubString* keywords,
      const bool passback,
      const unsigned long weight,
      const String::SubString& location,
      const bool redirect_empty_uid,
      RedirectRule::AllowedParams&& allowed_params)
      /*throw(UserBindFrontend::InvalidSource)*/;

    static void
    init_redirect_template_(
      String::TextTemplate::IStream& templ,
      const String::SubString& text)
      /*throw(UserBindFrontend::InvalidSource)*/;

    void
    user_match_(
      const Commons::UserId& result_user_id,
      const Commons::UserId& merge_user_id,
      bool create_user_profile,
      const String::SubString& keywords,
      const String::SubString& cohort,
      const String::SubString& referer,
      unsigned long colo_id,
      const FrontendCommons::Location* location,
      const String::SubString& source)
      noexcept;

    void
    fill_match_request_info_(
      AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo& mri,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& now,
      const std::vector<ChannelMatch>& trigger_match_page_channels,
      const std::vector<std::uint32_t>& history_match_result_channel_ids,
      const FrontendCommons::Location* location,
      const String::SubString& referer,
      const String::SubString& source)
      const noexcept;

  private:
    TaskProcessor& helper_task_processor_;
    const GrpcContainerPtr grpc_container_;

    // configuration
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    FileCachePtr pixel_;
    std::string fe_config_path_;
    std::string pixel_content_type_;
    SourceMap sources_;
    std::unique_ptr<GeoIPMapping::IPMapCity2> ip_map_;
    std::unique_ptr<UserBind::RequestInfoFiller> request_info_filler_;

    // external services
    //std::unique_ptr<Logging::LoggerCallbackHolder> callback_holder_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::UserBindClient_var user_bind_client_;
    FrontendCommons::UserInfoClient_var user_info_client_;
    ChannelServerSessionFactoryImpl_var server_session_factory_;
    std::unique_ptr<FrontendCommons::ChannelServerSessionPool>
      channel_servers_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    std::unique_ptr<FrontendCommons::CookieManager<
      FrontendCommons::HttpRequest, FrontendCommons::HttpResponse> > cookie_manager_;

    Generics::TaskExecutor_var bind_task_runner_;
    Generics::TaskExecutor_var match_task_runner_;

    Algs::AtomicInt bind_task_count_;
    Algs::AtomicInt match_task_count_;
  };
}

// Inlines
namespace AdServer
{
  inline
  UserBindFrontend::~UserBindFrontend() noexcept
  {}
}

#endif // ADSERVER_USERBINDFRONTEND_HPP
