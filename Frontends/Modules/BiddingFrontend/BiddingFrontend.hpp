#pragma once

#include <functional>
#include <memory>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/AtomicInt.hpp>
#include "Generics/CompositeMetricsProvider.hpp"

//#include <UServerUtils/MetricsHTTPProvider.hpp>
//#include <UServerUtils/metrics_raii.hpp>

#include <Sync/PosixLock.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <Commons/AtomicInt.hpp>
#include <Commons/Interval.hpp>

#include <CampaignSvcs/CampaignManagerClient/CampaignManagerDistributedGrpcClient.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/ChannelClientConfig.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

#include "GroupLogger.hpp"
#include "CampaignManagerTypes.hpp"
#include "DebugSink.hpp"
#include "RequestInfoFiller.hpp"
#include "BiddingFrontendStat.hpp"
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>
#include "JsonFormatter.hpp"
//#include "UServerUtils/MetricsHTTPProvider.hpp"
#include "RequestMetricsProvider.hpp"
#include "Stage.hpp"

namespace AdServer::Bidding
{
  class BidRequestState;
  typedef ReferenceCounting::SmartPtr<BidRequestState>
    BidRequestState_var;
  class OpenRtbBidRequestState;
  class GoogleBidRequestState;

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private GroupLogger,
    public virtual FrontendCommons::FrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
    using GroupLogger::logger;

    friend class BidRequestState;
    friend class OpenRtbBidRequestState;
    friend class GoogleBidRequestState;

  public:
    typedef FrontendCommons::HTTPExceptions::Exception Exception;

    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::BidFeConfiguration_type
      BiddingFeConfiguration;

  public:
    Frontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module,
      StatHolder* stats,
      Generics::CompositeMetricsProvider* composite_metrics_provider) /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    virtual void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

    RequestInfoFiller*
    request_info_filler() noexcept
    {
      return request_info_filler_.get();
    }

    Logging::Logger*
    logger() noexcept
    {
      return GroupLogger::logger();
    }

  protected:
    virtual ~Frontend() noexcept;

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
    typedef std::unique_ptr<BiddingFeConfiguration> ConfigPtr;

    class UpdateConfigTask;
    class FlushStateTask;

    struct ExtConfig: public ReferenceCounting::AtomicImpl
    {
      struct Colocation
      {
        unsigned long flags;
      };

      typedef std::map<unsigned long, Colocation>
        ColocationMap;

      ColocationMap colocations;

    protected:
      virtual ~ExtConfig() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<ExtConfig>
      ExtConfig_var;

    typedef Sync::Policy::PosixThreadRW
      ExtConfigSyncPolicy;
    typedef Sync::Policy::PosixThread
      MaxPendingSyncPolicy;

    typedef Commons::IntervalSet<Generics::Time>
      BlacklistedTimeIntervals;

  public:
    /*
    bool
    process_openrtb_request_(
      bool& bad_request,
      OpenRtbBidRequestState* request_task,
      RequestInfo& request_info,
      const char* bid_request)
      noexcept;

    bool
    process_google_request_(
      GoogleBidRequestState* request_task,
      RequestInfo& request_info,
      const Google::BidRequest& bid_request)
      noexcept;
    */

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    FrontendCommons::RequestTask
    co_process_bid_request_(
      BidRequestState_var request_task)
      noexcept;

    void
    interrupted_select_campaign_(
      BidRequestState* request_task) noexcept;

    FrontendCommons::RequestTask
    co_interrupted_select_campaign_(
      AdServer::Bidding::CampaignManager::RequestParams request_params)
      noexcept;

    void
    select_campaign_(
      AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result,
      const adserver::user_info_svcs::user_info_manager::MatchResponse*
        history_match_result,
      const adserver::channel_svcs::channel_server::MatchResponse* trigger_match_result,
      const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
        ccg_keywords,
      const RequestInfo& request_info,
      AdServer::Bidding::CampaignManager::RequestParams& request_params,
      const AdServer::Commons::UserId& user_id,
      bool passback,
      std::string& hostname,
      bool interrupted)
      noexcept;

    bool
    consider_campaign_selection_(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      const AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result,
      std::string& hostname)
      noexcept;

    FrontendCommons::RequestTask
    co_consider_campaign_selection_(
      AdServer::Commons::UserId user_id,
      Generics::Time time,
      std::shared_ptr<
        const AdServer::Bidding::CampaignManager::RequestCreativeResult>
          campaign_match_result,
      std::string hostname)
      noexcept;

    /*
    void
    fill_openrtb_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const AdServer::Bidding::CampaignManager::
        RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::Bidding::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

    void
    fill_yandex_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const AdServer::Bidding::CampaignManager::
        RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::Bidding::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;
    */

    void
    fill_account_traits_() noexcept;

  public:
    void
    limit_max_cpm_(
      AdServer::CampaignSvcs::RevenueDecimal& val,
      const AdServer::Bidding::CampaignManager::IdSeq& account_ids)
      const noexcept;

  private:
    /*
    static void
    protobuf_log_handler_(
      google::protobuf::LogLevel level,
      const char* filename,
      int line,
      const std::string& message);
    */

    void
    update_config_() noexcept;

    FrontendCommons::RequestTask
    co_update_config_() noexcept;

    void
    flush_state_() noexcept;

    void
    set_ext_config_(ExtConfig* config) noexcept;

    ExtConfig_var
    get_ext_config_() noexcept;

    bool
    check_interrupt_(
      const char* fun,
      const Stage stage,
      BidRequestState* task)
      noexcept;

    void
    interrupt_(
      const char* fun,
      const Stage stage,
      const BidRequestState* task)
      noexcept;

    Generics::Time
    get_request_timeout_(const FCGI::HttpRequest& request) noexcept;

    static
    AdServer::CampaignSvcs::AdInstantiateType
    adapt_instantiate_type_(const std::string& inst_type_str)
      /*throw(Exception)*/;

    static
    SourceTraits::NativeAdsInstantiateType
    adapt_native_ads_instantiate_type_(
      const std::string& inst_type_str)
      /*throw(Exception)*/;

    static
    AdServer::CampaignSvcs::NativeAdsImpressionTrackerType
    adapt_native_ads_impression_tracker_type_(
      const std::string& imp_type_str)
      /*throw(Exception)*/;

    static
    SourceTraits::ERIDReturnType
    adapt_erid_return_type_(
      const std::string& inst_type_str);

    /*
    void
    fill_native_response_(
      AdServer::Commons::JsonObject* json,
      const JsonAdSlotProcessingContext::Native& native_context,
      const AdServer::Bidding::CampaignManager::
        AdSlotResult& ad_slot_result,
      bool need_escape,
      bool add_root_native);
    */
  protected:
    // ADSC-10554
    // Interrupted requests queue
    std::shared_ptr<FrontendCommons::FrontendWorkers> passback_workers_;

    // configuration
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;
    CommonModule_var common_module_;
    unsigned long colo_id_;
    SourceMap sources_;
    Generics::Time request_timeout_;
    std::string server_id_;
    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    BlacklistedTimeIntervals blacklisted_time_intervals_;
    RequestInfoFiller::AccountTraitsById account_traits_;

    // external services
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
      user_bind_client_coro_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
      user_info_client_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
      user_info_client_coro_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
      channel_client_;
    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>
      channel_client_coro_;

    Generics::Planner_var planner_;
    std::shared_ptr<FrontendCommons::FrontendWorkers> bid_workers_;
    Generics::TaskRunner_var control_task_runner_;
    StatHolder_var stats_;

    mutable ExtConfigSyncPolicy::Mutex ext_config_lock_;
    ExtConfig_var ext_config_;

    Generics::AtomicInt bid_task_count_;
    Generics::AtomicInt passback_task_count_;

    mutable MaxPendingSyncPolicy::Mutex reached_max_pending_tasks_lock_;
    unsigned long reached_max_pending_tasks_;

  private:
    const Generics::CompositeMetricsProvider_var composite_metrics_provider_;
//    const RequestMetricsProvider_var request_metrics_provider_;
  };
}

// Inlines
namespace AdServer::Bidding
{
  inline
  Frontend::~Frontend() noexcept
  {}

  inline
  void
  Frontend::set_ext_config_(ExtConfig* config)
    noexcept
  {
    ExtConfig_var new_config = ReferenceCounting::add_ref(config);

    ExtConfigSyncPolicy::WriteGuard lock(ext_config_lock_);
    ext_config_.swap(new_config);
  }

  inline
  Frontend::ExtConfig_var
  Frontend::get_ext_config_() noexcept
  {
    ExtConfigSyncPolicy::ReadGuard lock(ext_config_lock_);
    return ext_config_;
  }
}
