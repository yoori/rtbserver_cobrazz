
#pragma once

#include <string>
#include <vector>
#include <list>
#include <memory>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Uuid.hpp>
#include <Sync/PosixLock.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <ChannelServerGrpc.grpc.pb.h>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <UserInfoSvcs/UserBindClient/UserBindClientUtils.hpp>
#include <ChannelSvcs/ChannelClient/ChannelClientUtils.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/TaskScheduler.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "AdFrontendStat.hpp"
#include "DebugSink.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class AdFrontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
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
    virtual void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer) noexcept;

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

    static AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
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

    int
    acquire_ad(
      HttpResponse& response,
      const FCGI::HttpRequest& request,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_creative_type,
      std::string& str_response,
      PassbackInfo& passback_info,
      bool& log_as_test,
      DebugSink* debug_sink,
      RequestTimeMetering& request_time_metering)
      /*throw(Exception)*/;

    void
    request_campaign_manager_(
      PassbackInfo& passback_info,
      bool& log_as_test,
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_var&
        campaign_matching_result,
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_type,
      const adserver::channel_svcs::channel_server::MatchResponse*
        trigger_matched_channels,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* history_match_result,
      const Generics::Time& merged_last_request,
      bool profiling_available,
      const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq*
        ccg_keywords,
      DebugSink* debug_sink)
      /*throw(Exception)*/;

    void
    convert_ccg_keywords_(
      AdServer::CampaignSvcs::CampaignManager::CCGKeywordSeq& ccg_keywords,
      const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* src_ccg_keywords)
      noexcept;

    void
    merge_users(
      RequestTimeMetering& time_metering,
      bool& merge_success,
      Generics::Time& last_request,
      std::string& merge_error_message,
      const RequestInfo& request_info)
      noexcept;

    void
    match_triggers_(
      RequestTimeMetering& request_time_metering,
      adserver::channel_svcs::channel_server::MatchRequest& request,
      adserver::channel_svcs::channel_server::MatchResponse&
        trigger_matched_channels,
      bool& trigger_matched_channels_present,
      const RequestInfo& request_info)
      /*throw(Exception)*/;

    void
    acquire_user_info_matcher(
      const RequestInfo& request_info,
      const adserver::channel_svcs::channel_server::MatchResponse*
        trigger_matching_result,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result_out,
      bool& profiling_available,
      RequestTimeMetering& request_time_metering)
      noexcept;

    void
    user_info_post_match_(
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
        campaign_select_result)
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
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
      channel_client_;

    Generics::TaskRunner_var task_runner_;
    FrontendCommons::TaskScheduler_var task_scheduler_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
      user_info_client_;

    AdFrontendStat_var stats_;
  };
}
