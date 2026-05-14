
#pragma once

#include <memory>
#include <functional>
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
#include <Commons/TextTemplateCache.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>
#include <ChannelServerGrpc.grpc.pb.h>
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
    public virtual FrontendCommons::FrontendInterface,
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
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
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

  private:
    struct MatchScheduleState;

    virtual ~Frontend() noexcept;

    void
    parse_config_() /*throw(Exception)*/;

    int
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      FCGI::HttpResponse_var response_ptr) noexcept;

    int
    finish_request_(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& result_user_id,
      bool invalid_bind_operation) noexcept;

    void
    resolve_user_bind_(
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& input_user_id,
      std::function<void(
        const AdServer::Commons::UserId& result_user_id,
        bool invalid_bind_operation)> finish)
      noexcept;

    void
    try_schedule_match_channels_(
      const std::shared_ptr<MatchScheduleState>& state)
      noexcept;

    void
    start_match_channels_(
      const std::shared_ptr<ImprTrackMatchRequestState>& state)
      noexcept;

    void
    start_history_match_(
      const std::shared_ptr<ImprTrackMatchRequestState>& state)
      noexcept;

    void
    process_match_request_(
      const std::shared_ptr<ImprTrackMatchRequestState>& state)
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
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
      channel_client_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
      user_info_client_;

    FrontendCommons::FrontendWorkers_var workers_;
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
