#ifndef BIDDINGFRONTENDGRPC_BIDDINGFRONTEND_HPP
#define BIDDINGFRONTENDGRPC_BIDDINGFRONTEND_HPP

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/AtomicInt.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/TaskRunner.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

// USERVER
#include <userver/concurrent/background_task_storage.hpp>

// THIS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/AtomicInt.hpp>
#include <Commons/Interval.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/Modules/BiddingFrontend/GroupLogger.hpp>
#include <Frontends/Modules/BiddingFrontend/BiddingFrontendStat.hpp>
#include <Frontends/Modules/BiddingFrontend/JsonFormatter.hpp>
#include <Frontends/Modules/BiddingFrontend/PlannerPool.hpp>
#include <Frontends/Modules/BiddingFrontend/Stage.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/RequestInfoFiller.hpp>

namespace AdServer::Bidding::Grpc
{
  class BidRequestTask;
  class OpenRtbBidRequestTask;
  class GoogleBidRequestTask;
  class AppNexusBidRequestTask;

  class Frontend final:
    private FrontendCommons::HTTPExceptions,
    private GroupLogger,
    public FrontendCommons::FrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  private:
    using BackgroundTaskStorage = userver::concurrent::BackgroundTaskStorage;
    using CommonFeConfiguration = Configuration::FeConfig::CommonFeConfiguration_type;
    using BiddingFeConfiguration = Configuration::FeConfig::BidFeConfiguration_type;
    using CommonConfigPtr = std::unique_ptr<CommonFeConfiguration>;
    using ConfigPtr = std::unique_ptr<BiddingFeConfiguration>;

    enum class TraceLevel
    {
      LOW = Logging::Logger::TRACE,
      MIDDLE,
      HIGH
    };

    struct ExtConfig final: public ReferenceCounting::AtomicImpl
    {
      struct Colocation
      {
        unsigned long flags;
      };

      using ColocationMap = std::map<unsigned long, Colocation>;

      ColocationMap colocations;

    protected:
      ~ExtConfig() override = default;
    };

    using ExtConfig_var = ReferenceCounting::SmartPtr<ExtConfig>;
    using BlacklistedTimeIntervals = Commons::IntervalSet<Generics::Time>;

    DECLARE_EXCEPTION(NotReadyException, eh::DescriptiveException);
    DECLARE_EXCEPTION(ChunkNotFoundException, eh::DescriptiveException);
    DECLARE_EXCEPTION(ImplementationException, eh::DescriptiveException);
    DECLARE_EXCEPTION(IncorrectArgumentException, eh::DescriptiveException);

    using GroupLogger::logger;

    friend class BidRequestTask;
    friend class OpenRtbBidRequestTask;
    friend class GoogleBidRequestTask;
    friend class AppNexusBidRequestTask;

  public:
    using TaskProcessor = userver::engine::TaskProcessor;
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    Frontend(
      TaskProcessor& helper_task_processor,
      const GrpcContainerPtr& grpc_container,
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module,
      StatHolder* stats,
      FrontendCommons::HttpResponseFactory* response_factory);

    bool will_handle(const String::SubString& uri) noexcept override;

    void handle_request(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer) noexcept override;

    void init() override;

    void shutdown() noexcept override;

    RequestInfoFiller* request_info_filler() noexcept;

    Logging::Logger* logger() noexcept;

  protected:
    ~Frontend() override = default;

  private:
    void parse_configs_();

    void resolve_user_id_(
      AdServer::Commons::UserId& match_user_id,
      FrontendCommons::GrpcCampaignManagerPool::CommonAdRequestInfo& common_info,
      RequestInfo& request_info)
      noexcept;

    void trigger_match_(
      std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>& trigger_matched_channels,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& user_id,
      const char* keywords = 0) noexcept;

    void history_match_(
      std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>& history_match_result,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo* const trigger_match_result,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time) noexcept;

    bool process_bid_request_(
      const char* fn,
      std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>& campaign_match_result,
      AdServer::Commons::UserId& user_id,
      BidRequestTask* request_task,
      RequestInfo& request_info,
      const std::string& keywords)
      noexcept;

    void interrupted_select_campaign_(
      BidRequestTask* request_task) noexcept;

    void select_campaign_(
      std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>& campaign_match_result,
      AdServer::UserInfoSvcs::Proto::MatchResult& history_match_result,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_match_result,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      std::string& hostname,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& user_id,
      const bool passback,
      const bool interrupted) noexcept;

    bool consider_campaign_selection_(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result) noexcept;

    void fill_account_traits_() noexcept;

  public:
    void limit_max_cpm_(
      AdServer::CampaignSvcs::RevenueDecimal& val,
      const std::vector<std::uint32_t>& account_ids) const noexcept;

  private:
    static std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
    get_empty_history_matching_();

    static void protobuf_log_handler_(
      google::protobuf::LogLevel level,
      const char* filename,
      int line,
      const std::string& message);

    void update_config_() noexcept;

    void flush_state_() noexcept;

    void set_ext_config_(ExtConfig* config) noexcept;

    ExtConfig_var get_ext_config_() noexcept;

    bool check_interrupt_(
      const char* fun,
      const Stage stage,
      BidRequestTask* task)
      noexcept;

    void interrupt_(
      const char* fun,
      const Stage stage,
      const BidRequestTask* task)
      noexcept;

    Generics::Time get_request_timeout_(
      const FrontendCommons::HttpRequest& request) noexcept;

    static
    AdServer::CampaignSvcs::AdInstantiateType
    adapt_instantiate_type_(const std::string& inst_type_str);

    static
    SourceTraits::NativeAdsInstantiateType
    adapt_native_ads_instantiate_type_(
      const std::string& inst_type_str);

    static
    AdServer::CampaignSvcs::NativeAdsImpressionTrackerType
    adapt_native_ads_impression_tracker_type_(
      const std::string& imp_type_str);

    static
    SourceTraits::ERIDReturnType
    adapt_erid_return_type_(
      const std::string& inst_type_str);

  protected:
    TaskProcessor& helper_task_processor_;

    const GrpcContainerPtr grpc_container_;

    BackgroundTaskStorage background_task_storage_;

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

    StatHolder_var stats_;

    mutable userver::engine::SharedMutex ext_config_lock_;

    ExtConfig_var ext_config_;
  };
} // namespace AdServer::Bidding::Grpc

#endif // BIDDINGFRONTENDGRPC_BIDDINGFRONTEND_HPP
