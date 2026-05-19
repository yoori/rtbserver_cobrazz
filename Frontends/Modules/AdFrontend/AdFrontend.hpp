
#pragma once

#include <string>
#include <vector>
#include <list>
#include <memory>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Uuid.hpp>
#include <Sync/PosixLock.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <Commons/ExecutorPool.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>
#include <Frontends/FrontendCommons/ValueTask.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <CampaignSvcs/CampaignManagerClient/CampaignManagerDistributedGrpcClient.hpp>
#include <Frontends/FrontendCommons/TaskScheduler.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "AdFrontendStat.hpp"
#include "DebugSink.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer::Configuration
{
  using namespace xsd::AdServer::Configuration;
}

namespace AdServer
{
  class AdFrontend:
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

    typedef Configuration::FeConfig::AdFeConfiguration_type
      AdFeConfiguration;

    typedef Configuration::FeConfig::PassFeConfiguration_type
      PassFeConfiguration;

    typedef FCGI::HttpResponse HttpResponse;

    AdFrontend(
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

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void shutdown() noexcept;

    /**
     * Update ColoFlagsMap from CampaignManager
     */
    void
    update_colocation_flags() noexcept;

    static adserver::channel_svcs::channel_server::MatchResponse
    get_empty_trigger_matching()
      /*throw(eh::Exception)*/;

    static adserver::channel_svcs::channel_server::MatchRequest
    get_empty_matching_request();

    static std::shared_ptr<
      adserver::user_info_svcs::user_info_manager::MatchResponse>
    get_empty_history_matching()
      /*throw(eh::Exception)*/;

  protected:
    virtual ~AdFrontend() noexcept {}

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
    typedef std::unique_ptr<AdFeConfiguration> ConfigPtr;
    typedef std::unique_ptr<PassFeConfiguration> PassConfigPtr;
    typedef std::unique_ptr<
      FrontendCommons::CookieManager<FCGI::HttpRequest, HttpResponse> >
      CookieManagerPtr;

    typedef Sync::Policy::PosixThreadRW SyncPolicy;

  private:
    struct RequestContext;

    void parse_configs_() /*throw(Exception)*/;

    void sign_client_id(
      const std::string& uid,
      std::string& signed_uid)
      noexcept;

    void
    start_update_loop_() /*throw(Exception)*/;

    bool
    resolve_cookie_user_id_(
      AdServer::Commons::UserId& resolved_user_id,
      const RequestInfo& request_info)
      noexcept;

    using BoolTask = FrontendCommons::ValueTask<bool>;

    struct MergeUsersResult
    {
      bool success = true;
      Generics::Time merged_last_request = Generics::Time::ZERO;
      std::string error_message;
    };

    using MergeUsersTask = FrontendCommons::ValueTask<MergeUsersResult>;

    struct UserInfoMatcherResult
    {
      std::shared_ptr<
        adserver::user_info_svcs::user_info_manager::MatchResponse>
          history_match_result;
      bool profiling_available = false;
    };

    using UserInfoMatcherTask =
      FrontendCommons::ValueTask<UserInfoMatcherResult>;

    BoolTask
    co_acquire_ad_(
      const std::shared_ptr<RequestContext>& context)
      noexcept;

    FrontendCommons::RequestResult
    finish_request_(
      const std::shared_ptr<RequestContext>& context)
      noexcept;

    void
    request_campaign_manager_(
      PassbackInfo& passback_info,
      bool& log_as_test,
      adserver::campaign_svcs::campaign_manager::RequestCreativeResult&
        campaign_matching_result,
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_type,
      const adserver::channel_svcs::channel_server::MatchResponse*
        trigger_matched_channels,
      const adserver::user_info_svcs::user_info_manager::MatchResponse*
        history_match_result,
      const Generics::Time& merged_last_request,
      bool profiling_available,
      const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
        ccg_keywords,
      DebugSink* debug_sink,
      adserver::campaign_svcs::campaign_manager::RequestParams*
        request_params_out)
      /*throw(Exception)*/;

    BoolTask
    co_request_campaign_manager_(
      PassbackInfo& passback_info,
      bool& log_as_test,
      adserver::campaign_svcs::campaign_manager::RequestCreativeResult&
        campaign_matching_result,
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_type,
      const adserver::channel_svcs::channel_server::MatchResponse*
        trigger_matched_channels,
      const adserver::user_info_svcs::user_info_manager::MatchResponse*
        history_match_result,
      const Generics::Time& merged_last_request,
      bool profiling_available,
      const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
        ccg_keywords,
      DebugSink* debug_sink)
      noexcept;

    void
    convert_ccg_keywords_(
      google::protobuf::RepeatedPtrField<
        adserver::campaign_svcs::campaign_manager::CcgKeywordInfo>&
          ccg_keywords,
      const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
        src_ccg_keywords)
      noexcept;

    MergeUsersTask
    co_merge_users_(
      const std::shared_ptr<RequestContext>& context)
      noexcept;

    BoolTask
    co_match_triggers_(
      const std::shared_ptr<RequestContext>& context,
      adserver::channel_svcs::channel_server::MatchRequest request,
      std::shared_ptr<
        adserver::channel_svcs::channel_server::MatchResponse> response)
      noexcept;

    UserInfoMatcherTask
    co_acquire_user_info_matcher_(
      const std::shared_ptr<RequestContext>& context,
      std::shared_ptr<
        adserver::channel_svcs::channel_server::MatchResponse> trigger_matching_result,
      bool trigger_matching_result_present)
      noexcept;

    void
    user_info_post_match_(
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const adserver::campaign_svcs::campaign_manager::RequestCreativeResult&
        campaign_select_result)
      noexcept;

    FrontendCommons::RequestTask
    co_user_info_post_match_(
      adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest
        request)
      noexcept;

    FrontendCommons::RequestTask
    co_verify_opt_operation_(
      adserver::campaign_svcs::campaign_manager::VerifyOptOperationRequest
        request)
      noexcept;

    FrontendCommons::RequestTask
    co_update_colocation_flags_()
      noexcept;

    void
    opt_out_client_(
      const HTTP::CookieList& cookies,
      HttpResponse& response,
      const FCGI::HttpRequest& request,
      const RequestInfo& request_info)
      noexcept;

    static void
    fill_debug_channels_(
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>& in,
      char type,
      DebugStream& out)
      /*throw(eh::Exception)*/;

  private:
    void
    log_request(
      const char* function_name,
      const FCGI::HttpRequest& request,
      unsigned int log_level)
      /*throw(eh::Exception)*/;

    static void prepare_ui_match_params_(
      adserver::user_info_svcs::user_info_manager::MatchParams& match_params,
      const adserver::channel_svcs::channel_server::MatchResponse* match_result,
      const RequestInfo& request_info)
      /*throw(eh::Exception)*/;

    static void
    add_hit_channels_(
      google::protobuf::RepeatedPtrField<
        adserver::user_info_svcs::user_info_manager::ChannelTriggerMatch>&
          result_channel_ids,
      const AdServer::CampaignSvcs::ChannelIdArray& hit_channels);

  private:
    /* configuration */
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    PassConfigPtr pass_config_;
    std::string fe_config_path_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;

    CookieManagerPtr cookie_manager_;
    std::list<std::string> remove_cookies_holder_;
    FrontendCommons::CookieNameSet remove_cookies_;

    /* external services */
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>
      channel_client_coro_;

    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
    Generics::TaskRunner_var task_runner_;
    FrontendCommons::TaskScheduler_var task_scheduler_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
      user_bind_client_coro_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
      user_info_client_coro_;

    AdFrontendStat_var stats_;
  };
}
