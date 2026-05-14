#pragma once

#include <functional>
#include <memory>

#include <eh/Exception.hpp>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/FileCache.hpp>
#include <Generics/Uuid.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <String/TextTemplate.hpp>

#include <Commons/AtomicInt.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>

#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class UserBindFrontend;
  class UserBindMatchRequestState;

  typedef ReferenceCounting::SmartPtr<UserBindFrontend> UserBindFrontend_var;

  class UserBindFrontend:
    private FrontendCommons::HTTPExceptions,
    public Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::FrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
    typedef FrontendCommons::HTTPExceptions::Exception Exception;
    DECLARE_EXCEPTION(InvalidSource, eh::DescriptiveException);
    friend class UserBindMatchRequestState;

  public:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::UserBindFeConfiguration_type
      UserBindFeConfiguration;

  public:
    UserBindFrontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  protected:
    class BindRequestState;

    struct BindResult
    {
      BindResult() = default;

      explicit BindResult(const AdServer::Commons::UserId& result_user_id_val)
        : result_user_id(result_user_id_val)
      {}

      // Used for init redirect template.
      AdServer::Commons::UserId ssp_user_id;
      AdServer::Commons::UserId result_user_id;
    };

  protected:
    virtual ~UserBindFrontend() noexcept;

    void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
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

    using ProcessRequestCallback = std::function<void(int, BindResult)>;
    using GetUserIdCallback = std::function<void(
      const grpc::Status&,
      const adserver::user_info_svcs::user_bind::GetUserIdResponse&)>;
    using AddUserIdCallback = std::function<void(
      const grpc::Status&,
      const adserver::user_info_svcs::user_bind::AddUserIdResponse&)>;
    using AddBindRequestCallback = std::function<void(
      const grpc::Status&,
      const adserver::user_info_svcs::user_bind::AddBindRequestResponse&)>;

    void
    process_request_async_(
      UserBind::RequestInfo_var request_info,
      std::string dns_bind_request_id,
      ProcessRequestCallback callback)
      noexcept;

    bool
    has_user_bind_client_() const noexcept;

    void
    get_user_id_(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      GetUserIdCallback callback)
      noexcept;

    void
    add_user_id_(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      AddUserIdCallback callback)
      noexcept;

    void
    add_bind_request_(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      AddBindRequestCallback callback)
      noexcept;

    void
    post_request_stage_(std::function<void()> callback) noexcept;

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
    schedule_user_match_(
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
      adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& now,
      const adserver::channel_svcs::channel_server::MatchResponse*
        trigger_match_result,
      const adserver::user_info_svcs::user_info_manager::MatchResponse*
        history_match_result,
      const FrontendCommons::Location* location,
      const String::SubString& referer,
      const String::SubString& source)
      const noexcept;

    void handle_user_channels_request_async_(
      UserBind::RequestInfo_var request_info,
      FCGI::HttpResponse_var response,
      std::function<void(int)> callback)
      noexcept;

  private:
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
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
      user_info_client_;
    FrontendCommons::FrontendWorkers_var workers_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
      channel_client_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    std::unique_ptr<FrontendCommons::CookieManager<
      FCGI::HttpRequest, FCGI::HttpResponse> > cookie_manager_;

    FrontendCommons::FrontendWorkers_var match_workers_;

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
