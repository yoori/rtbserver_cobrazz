#ifndef FRONTENDS_ADFRONTENDGRPC_ADFRONTEND_HPP
#define FRONTENDS_ADFRONTENDGRPC_ADFRONTEND_HPP

// STD
#include <list>
#include <string>
#include <vector>

// UNIXCOMMONS
#include <CORBACommons/CorbaAdapters.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Uuid.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/Http.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Logger/Logger.hpp>
#include <Sync/PosixLock.hpp>
#include <eh/Exception.hpp>

// THIS
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/TaskScheduler.hpp>
#include <Frontends/Modules/AdFrontend/AdFrontendStat.hpp>
#include <Frontends/Modules/AdFrontendGrpc/DebugSink.hpp>
#include <Frontends/Modules/AdFrontendGrpc/RequestInfoFiller.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer::Grpc
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
  public:
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using CommonFeConfiguration = Configuration::FeConfig::CommonFeConfiguration_type;
    using AdFeConfiguration = Configuration::FeConfig::AdFeConfiguration_type;
    using PassFeConfiguration = Configuration::FeConfig::PassFeConfiguration_type;
    using HttpResponse = FrontendCommons::HttpResponse;

  private:
    using Exception = FrontendCommons::HTTPExceptions::Exception;
    using CommonConfigPtr = std::unique_ptr<CommonFeConfiguration>;
    using ConfigPtr = std::unique_ptr<AdFeConfiguration>;
    using PassConfigPtr = std::unique_ptr<PassFeConfiguration>;
    using CookieManagerPtr = std::unique_ptr<
      FrontendCommons::CookieManager<FrontendCommons::HttpRequest, HttpResponse>>;
    using SyncPolicy = Sync::Policy::PosixThreadRW;

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

  public:
    AdFrontend(
      const GrpcContainerPtr& grpc_container,
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module,
      FrontendCommons::HttpResponseFactory* response_factory);

    /** Determines whether the module is able to process the URI.
     * @param uri URI provided by Apache.
     **/
    bool will_handle(const String::SubString& uri) noexcept override;

    /**
     * Executed if will_handle returns true; processes an HTTP request. Provides
     * browsing context to a Channel Manager, returns list of channels in the HTTP
     * response.
     * @param request HTTP request.
     * @param response The object to write the HTTP response body.
     * @return HTTP status code.
     **/
    void handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer) noexcept override;

    /** Performs initialization for the module child process. */
    void init()  override;

    /** Performs shutdown for the module child process. */
    void shutdown() noexcept override;

    /**
     * Update ColoFlagsMap from CampaignManager
     */
    void update_colocation_flags() noexcept;

    static std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>
    get_empty_trigger_matching();

    static std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
    get_empty_history_matching();

  protected:
    ~AdFrontend() override = default;

  private:
    void parse_configs_();

    void sign_client_id(
      const std::string& uid,
      std::string& signed_uid)
      noexcept;

    void start_update_loop_();

    bool resolve_cookie_user_id_(
      AdServer::Commons::UserId& resolved_user_id,
      const RequestInfo& request_info)
      noexcept;

    int acquire_ad(
      HttpResponse& response,
      const FrontendCommons::HttpRequest& request,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_creative_type,
      std::string& str_response,
      PassbackInfo& passback_info,
      bool& log_as_test,
      DebugSink* debug_sink,
      RequestTimeMetering& request_time_metering);

    void request_campaign_manager_(
      PassbackInfo& passback_info,
      bool& log_as_test,
      std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>&
        campaign_matching_result,
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const Generics::SubStringHashAdapter& instantiate_type,
      AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_matched_channels,
      AdServer::UserInfoSvcs::Proto::MatchResult* history_match_result,
      const Generics::Time&,
      bool profiling_available,
      const google::protobuf::RepeatedField<AdServer::ChannelSvcs::Proto::CCGKeyword>*
        ccg_keywords,
      const google::protobuf::RepeatedField<AdServer::ChannelSvcs::Proto::CCGKeyword>*
        hid_ccg_keywords,
      DebugSink* debug_sink);

    void convert_ccg_keywords_(
      std::vector<FrontendCommons::GrpcCampaignManagerPool::CCGKeyword>& ccg_keywords,
      const google::protobuf::RepeatedField<AdServer::ChannelSvcs::Proto::CCGKeyword>*
        src_ccg_keywords) noexcept;

    void merge_users(
      RequestTimeMetering& time_metering,
      bool& merge_success,
      Generics::Time& last_request,
      std::string& merge_error_message,
      const RequestInfo& request_info) noexcept;

    void match_triggers_(
      RequestTimeMetering& request_time_metering,
      std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>&
        trigger_matched_channels,
      const RequestInfo& request_info);

    void acquire_user_info_matcher(
      const RequestInfo& request_info,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_matching_result,
      std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>& match_result_out,
      bool& profiling_available,
      RequestTimeMetering& request_time_metering) noexcept;

    void user_info_post_match_(
      RequestTimeMetering& request_time_metering,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_select_result) noexcept;

    void opt_out_client_(
      const HTTP::CookieList& cookies,
      HttpResponse& response,
      const FrontendCommons::HttpRequest& request,
      const RequestInfo& request_info) noexcept;

    static void fill_debug_channels_(
      const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::ChannelAtom>& in,
      const char type,
      DebugStream& out);

  private:
    void log_request(
      const char* function_name,
      const FrontendCommons::HttpRequest& request,
      const unsigned int log_level);

    static void prepare_ui_match_params_(
      AdServer::UserInfoSvcs::Types::MatchParams& match_params,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo* match_result,
      const RequestInfo& request_info);

    static void add_hit_channels_(
      std::vector<AdServer::UserInfoSvcs::Types::ChannelTriggerMatch>& result_channel_ids,
      const AdServer::CampaignSvcs::ChannelIdArray& hit_channels);

  private:
    const GrpcContainerPtr grpc_container_;

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

    Generics::TaskRunner_var task_runner_;
    FrontendCommons::TaskScheduler_var task_scheduler_;

    AdFrontendStat_var stats_;
  };
} // namespace AdServer::Grpc

#endif // FRONTENDS_ADFRONTENDGRPC_ADFRONTEND_HPP
