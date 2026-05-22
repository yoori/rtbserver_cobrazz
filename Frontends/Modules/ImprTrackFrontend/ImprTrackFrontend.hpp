
#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <set>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Generics/FileCache.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/TextTemplateCache.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/ValueTask.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer::ImprTrack
{
  class ImprTrackRequestState;
  class ImprTrackMatchRequestState;

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
    friend class ImprTrackRequestState;
    friend class ImprTrackMatchRequestState;

  public:
    Frontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    FrontendCommons::RequestTask
    co_handle_request(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  protected:
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

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::ImprTrackFeConfiguration_type
      ImprTrackFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<ImprTrackFeConfiguration> ConfigPtr;

    struct BindURLRule: public ReferenceCounting::AtomicImpl
    {
      bool use_keywords;
      Generics::GnuHashSet<Generics::StringHashAdapter> keywords;
      Commons::TextTemplate_var url_template;

    protected:
      virtual ~BindURLRule() noexcept
      {};
    };

    typedef ReferenceCounting::SmartPtr<BindURLRule> BindURLRule_var;
    typedef std::vector<BindURLRule_var> BindURLRuleArray;

    typedef std::vector<Commons::TextTemplate_var>
      TextTemplateArray;

    struct ResolveUserBindResult
    {
      AdServer::Commons::UserId user_id;
      bool invalid_bind_operation = false;
    };

    using ResolveUserBindTask =
      FrontendCommons::ValueTask<ResolveUserBindResult>;

  private:
    struct MatchScheduleState;

    virtual ~Frontend() noexcept;

    void
    parse_config_() /*throw(Exception)*/;

    int
    finish_request_(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& result_user_id,
      bool invalid_bind_operation) noexcept;

    ResolveUserBindTask
    co_resolve_user_bind_(
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& input_user_id)
      noexcept;

    void
    try_schedule_match_channels_(
      const std::shared_ptr<MatchScheduleState>& state)
      noexcept;

    FrontendCommons::RequestTask
    co_match_request_(
      std::shared_ptr<ImprTrackMatchRequestState> state)
      noexcept;

    FrontendCommons::RequestTask
    co_verify_impression_(
      adserver::campaign_svcs::campaign_manager::VerifyImpressionRequest
        request,
      std::shared_ptr<MatchScheduleState> match_schedule_state)
      noexcept;

    FrontendCommons::RequestTask
    co_confirm_user_freq_caps_(
      adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest
        request)
      noexcept;

    FrontendCommons::RequestTask
    co_consider_web_operation_(
      adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
        request)
      noexcept;

    void
    finish_match_channels_request_()
      noexcept;

    void
    fill_match_request_info_(
      adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
      const ImprTrackMatchRequestState& state)
      const noexcept;

    RequestInfoFiller::EncryptionKeys_var
    read_keys_(
      const xsd::AdServer::Configuration::EncryptionKeysType& src)
      /*throw(eh::Exception)*/;

    void
    report_bad_user_(const RequestInfo& request_info)
      noexcept;

  private:
    // configuration
    std::string config_file_;

    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    std::unique_ptr<
      FrontendCommons::CookieManager<FCGI::HttpRequest, FCGI::HttpResponse> > cookie_manager_;
    FileCachePtr track_pixel_;
    std::string track_pixel_content_type_;
    BindURLRuleArray bind_url_rules_;

    typedef std::unique_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;
    IPMapPtr ip_map_;

    // external services
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>
      channel_client_coro_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
      user_bind_client_coro_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
      user_info_client_coro_;

    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
    FrontendCommons::FrontendWorkers_var match_workers_;
    std::atomic<unsigned long> match_tasks_count_{0};

    Generics::StringHashAdapter track_template_file_;
    Commons::TextTemplateCache_var template_files_;
  };
}

// Inlines
namespace AdServer::ImprTrack
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
