#ifndef ADSERVER_BIDDINGFRONTEND_HPP
#define ADSERVER_BIDDINGFRONTEND_HPP

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/AtomicInt.hpp>
//#include "Generics/CompositeMetricsProvider.hpp"

//#include <UServerUtils/MetricsHTTPProvider.hpp>
//#include <UServerUtils/MetricsRAII.hpp>

#include <Sync/PosixLock.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/AtomicInt.hpp>
#include <Commons/Interval.hpp>

#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServer.hpp>

#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/ChannelServerSessionPool.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/FrontendCommons/UserBindClient.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>

#include "GroupLogger.hpp"
#include "RequestInfoFiller.hpp"
#include "BiddingFrontendStat.hpp"
#include "JsonFormatter.hpp"
#include "PlannerPool.hpp"
//#include "RequestMetricsProvider.hpp"

namespace AdServer
{
namespace Bidding
{
  class BidRequestTask;
  class OpenRtbBidRequestTask;
  class GoogleBidRequestTask;
  class AppNexusBidRequestTask;

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private GroupLogger,
    public virtual FrontendCommons::FrontendInterface,
    public Generics::CompositeActiveObject,
    //public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
    using GroupLogger::logger;

    friend class BidRequestTask;
    friend class OpenRtbBidRequestTask;
    friend class GoogleBidRequestTask;
    friend class AppNexusBidRequestTask;

    using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
    using ManagerCoro = UServerUtils::Grpc::Manager;
    using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
    using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;

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
      StatHolder* stats
      //, Generics::CompositeMetricsProvider* composite_metrics_provider
      ) /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    virtual void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponseWriter_var response_writer)
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
    class InterruptPassbackTask;

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
      OpenRtbBidRequestTask* request_task,
      RequestInfo& request_info,
      const char* bid_request)
      noexcept;

    void
    process_appnexus_request_(
      bool& bad_request,
      AppNexusBidRequestTask* request_task,
      RequestInfo& request_info,
      const char* bid_request)
      noexcept;

    bool
    process_google_request_(
      GoogleBidRequestTask* request_task,
      RequestInfo& request_info,
      const Google::BidRequest& bid_request)
      noexcept;
    */

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    void
    resolve_user_id_(
      AdServer::Commons::UserId& match_user_id,
      AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& common_info,
      RequestInfo& request_info)
      noexcept;

    void
    trigger_match_(
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out trigger_matched_channels,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& user_id,
      CORBA::String_var& hostname,
      const char* keywords = 0)
      noexcept;

    void
    history_match_(
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out history_match_result,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::ChannelSvcs::ChannelServerBase::MatchResult* trigger_match_result,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      CORBA::String_var& hostname)
      noexcept;

    void
    get_ccg_keywords_(
      AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var& ccg_keywords,
      const RequestInfo& request_info,
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult& history_match_result)
      noexcept;

    bool
    process_bid_request_(
      const char* fn,
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out
        campaign_match_result,
      AdServer::Commons::UserId& user_id,
      BidRequestTask* request_task,
      RequestInfo& request_info,
      const std::string& keywords)
      noexcept;

    void
    interrupted_select_campaign_(
      BidRequestTask* request_task) noexcept;

    void
    select_campaign_(
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out
        campaign_match_result,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult&
        history_match_result,
      const AdServer::ChannelSvcs::ChannelServerBase::MatchResult* trigger_match_result,
      const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* ccg_keywords,
      const RequestInfo& request_info,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::Commons::UserId& user_id,
      bool passback,
      CORBA::String_var& hostname,
      bool interrupted)
      noexcept;

    bool
    consider_campaign_selection_(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
        campaign_match_result,
      CORBA::String_var& hostname)
      noexcept;

    /*
    void
    fill_openrtb_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

    void
    fill_yandex_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;
    */

    void
    fill_account_traits_() noexcept;

  public:
    void
    limit_max_cpm_(
      AdServer::CampaignSvcs::RevenueDecimal& val,
      const AdServer::CampaignSvcs::ULongSeq& account_ids)
      const noexcept;

  private:
    static AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
    get_empty_history_matching_()
      /*throw(eh::Exception)*/;

    static void
    protobuf_log_handler_(
      google::protobuf::LogLevel level,
      const char* filename,
      int line,
      const std::string& message);

    void
    update_config_() noexcept;

    void
    flush_state_() noexcept;

    void
    set_ext_config_(ExtConfig* config) noexcept;

    ExtConfig_var
    get_ext_config_() noexcept;

    bool
    check_interrupt_(
      const char* fun,
      const char* stage,
      const BidRequestTask* task)
      noexcept;

    void
    interrupt_(
      const char* fun,
      const char* stage,
      const BidRequestTask* task)
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
      const AdServer::CampaignSvcs::CampaignManager::
        AdSlotResult& ad_slot_result,
      bool need_escape,
      bool add_root_native);
    */
  protected:
    // ADSC-10554
    // Interrupted requests queue
    Generics::TaskExecutor_var passback_task_runner_;
    
    // configuration
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;
    CommonModule_var common_module_;
    unsigned long colo_id_;
    SourceMap sources_;
    Generics::Time request_timeout_;
    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    BlacklistedTimeIntervals blacklisted_time_intervals_;
    RequestInfoFiller::AccountTraitsById account_traits_;

    // external services
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::UserBindClient_var user_bind_client_;
    FrontendCommons::UserInfoClient_var user_info_client_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    std::unique_ptr<FrontendCommons::ChannelServerSessionPool> channel_servers_;

    Generics::Planner_var planner_;
    Generics::TaskExecutor_var task_runner_;
    Generics::TaskRunner_var control_task_runner_;
    PlannerPool_var planner_pool_;
    StatHolder_var stats_;

    ManagerCoro_var manager_coro_;

    mutable ExtConfigSyncPolicy::Mutex ext_config_lock_;
    ExtConfig_var ext_config_;

    Generics::AtomicInt bid_task_count_;
    Generics::AtomicInt passback_task_count_;

    mutable MaxPendingSyncPolicy::Mutex reached_max_pending_tasks_lock_;
    unsigned long reached_max_pending_tasks_;

  private:
    //const Generics::CompositeMetricsProvider_var composite_metrics_provider_;
    //const RequestMetricsProvider_var request_metrics_provider_;
  };
}
}

// Inlines
namespace AdServer
{
namespace Bidding
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
}

#endif // ADSERVER_BIDDINGFRONTEND_HPP
