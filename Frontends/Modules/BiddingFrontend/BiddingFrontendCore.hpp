#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include <Logger/Logger.hpp>
#include <Generics/AtomicInt.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/SyncPolicy.hpp>

#include <HTTP/Http.hpp>

#include <CampaignSvcs/CampaignManagerClient/CampaignManagerDistributedGrpcClient.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoDistributedGrpcClient.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>

#include "CampaignManagerTypes.hpp"
#include "RequestInfoFiller.hpp"
#include "BiddingFrontendStat.hpp"
#include "BiddingFrontendLogger.hpp"
#include "Stage.hpp"

namespace AdServer::Bidding
{
  class BidRequestState;
  using BidRequestState_var = std::shared_ptr<BidRequestState>;

  class BiddingFrontendCore:
    public ReferenceCounting::AtomicImpl
  {
  public:
    struct ProcessCoefInterval
    {
      unsigned long from_minute = 0;
      unsigned long to_minute = 0;
      double coef = 1;
    };

    struct ProcessCoefSchedule
    {
      double coef = 1;
      std::vector<ProcessCoefInterval> intervals;
    };

    struct InitParams
    {
      using UriList = std::vector<std::string>;

      Logging::Logger_var logger;
      CommonModule_var common_module;
      UserIdControllerBase_var user_id_controller;
      std::shared_ptr<RequestInfoFiller> request_info_filler;
      std::shared_ptr<const RequestInfoFiller::SourceMap> sources;
      std::shared_ptr<const RequestInfoFiller::AccountTraitsById> account_traits;
      StatHolder_var stats;
      std::shared_ptr<AdServer::Commons::ExecutorPool> bid_workers;
      std::shared_ptr<AdServer::Commons::ExecutorPool> timeout_workers;
      BiddingFrontendLogger_var bidding_frontend_logger;
      std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
        user_bind_client;
      std::shared_ptr<AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient>
        user_info_distributed_client;
      std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
        user_info_client;
      std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
        campaign_manager;
      std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
        channel_client;
      UriList google_uris;
      UriList adxml_uris;
      UriList clickstar_uris;
      UriList dao_uris;
      unsigned long max_pending_tasks;
      ProcessCoefSchedule process_coef;
      unsigned long threads;
      Generics::Time request_timeout;
      std::string server_id;
      unsigned long colo_id;
      bool trace_mapping;
      bool enable_profile_referer;
    };

    struct ExtConfig: public ReferenceCounting::AtomicImpl
    {
      struct Colocation
      {
        unsigned long flags;
      };

      using ColocationMap = std::map<unsigned long, Colocation>;

      ColocationMap colocations;

    protected:
      virtual ~ExtConfig() noexcept {}
    };

    using ExtConfig_var = ReferenceCounting::SmartPtr<ExtConfig>;

    explicit
    BiddingFrontendCore(const InitParams& params);

    void
    set_config(ExtConfig* config) noexcept;

    ExtConfig_var
    get_config() noexcept;

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer) noexcept;

    FrontendCommons::RequestTask
    co_process_bid_request_(BidRequestState_var request_task) noexcept;

    FrontendCommons::RequestTask
    co_resolve_user_id_(BidRequestState_var request_task) noexcept;

    bool
    consider_campaign_selection_(
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& time,
      std::shared_ptr<
        const AdServer::Bidding::CampaignManager::RequestCreativeResult>
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

    FrontendCommons::RequestTask
    co_add_user_id_(
      std::string external_user_id,
      AdServer::Commons::UserId user_id,
      Generics::Time time)
      noexcept;

    void
    prepare_get_campaign_creative_request_(
      adserver::campaign_svcs::campaign_manager::GetCampaignCreativeRequest& request,
      const adserver::user_info_svcs::user_info_manager::MatchResponse*
        history_match_result,
      const adserver::channel_svcs::channel_server::MatchResponse*
        trigger_match_result,
      const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
        ccg_keywords,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& user_id,
      bool passback,
      bool profiling_available,
      bool interrupted)
    noexcept;

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

    void
    limit_max_cpm_(
      AdServer::CampaignSvcs::RevenueDecimal& val,
      const Generics::MonoVector<unsigned long>& account_ids)
      const noexcept;

    RequestInfoFiller*
    request_info_filler() noexcept
    {
      return request_info_filler_.get();
    }

    void
    process_geo(const RequestInfo& request_info) noexcept;

    Logging::Logger*
    logger() noexcept
    {
      return logger_;
    }

    CommonModule*
    common_module() noexcept
    {
      return common_module_.in();
    }

    UserIdControllerBase*
    user_id_controller() noexcept
    {
      return user_id_controller_.in();
    }

    const RequestInfoFiller::AccountTraitsById&
    account_traits() const noexcept
    {
      return *account_traits_;
    }

    StatHolder_var
    stats() noexcept
    {
      return stats_;
    }

    const std::string&
    server_id() const noexcept
    {
      return server_id_;
    }

    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
    user_bind_client() const noexcept
    {
      return user_bind_client_;
    }

    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient>
    user_info_distributed_client() const noexcept
    {
      return user_info_distributed_client_;
    }

    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
    channel_client() const noexcept
    {
      return channel_client_;
    }

    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
    campaign_manager() const noexcept
    {
      return campaign_manager_;
    }

    Generics::AtomicInt&
    bid_task_count() noexcept
    {
      return bid_task_count_;
    }

    unsigned long
    reset_reached_max_pending_tasks() noexcept;

    virtual
    ~BiddingFrontendCore() noexcept = default;

  private:
    using ExtConfigSyncPolicy = Sync::Policy::PosixThreadRW;
    using UriList = InitParams::UriList;

    enum class RequestType
    {
      Google,
      AdXml,
      ClickStar,
      Dao,
      OpenRtb
    };

    RequestType
    resolve_request_type_(const FCGI::HttpRequest& request) const noexcept;

    Generics::Time
    get_request_timeout_(const FCGI::HttpRequest& request) const noexcept;

    double
    resolve_process_coef_(const Generics::Time& time) const noexcept;

    mutable ExtConfigSyncPolicy::Mutex ext_config_lock_;
    ExtConfig_var ext_config_;

    Logging::Logger_var logger_;
    CommonModule_var common_module_;
    UserIdControllerBase_var user_id_controller_;
    std::shared_ptr<RequestInfoFiller> request_info_filler_;
    std::shared_ptr<const RequestInfoFiller::SourceMap> sources_;
    std::shared_ptr<const RequestInfoFiller::AccountTraitsById> account_traits_;
    StatHolder_var stats_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> bid_workers_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> timeout_workers_;
    BiddingFrontendLogger_var bidding_frontend_logger_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
      user_bind_client_coro_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient>
      user_info_distributed_client_;
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
    UriList google_uris_;
    UriList adxml_uris_;
    UriList clickstar_uris_;
    UriList dao_uris_;
    unsigned long max_pending_tasks_ = 0;
    ProcessCoefSchedule process_coef_;
    unsigned long threads_ = 0;
    Generics::Time request_timeout_;
    std::string server_id_;
    unsigned long colo_id_ = 0;
    bool trace_mapping_ = false;
    bool enable_profile_referer_ = false;
    Generics::AtomicInt bid_task_count_;
    std::atomic<unsigned long> reached_max_pending_tasks_{0};
  };
}
