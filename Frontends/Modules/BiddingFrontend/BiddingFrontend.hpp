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
#include <ReferenceCounting/SmartPtr.hpp>

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
#include "BiddingFrontendCore.hpp"
#include "CampaignManagerTypes.hpp"
#include "DebugSink.hpp"
#include "RequestInfoFiller.hpp"
#include "BiddingFrontendStat.hpp"
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>
#include "JsonFormatter.hpp"
#include "Stage.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoDistributedGrpcClient;
}

namespace AdServer::Bidding
{
  class BidRequestState;
  class RequestParamsHolder;
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

    friend class BiddingFrontendCore;

  public:
    typedef FrontendCommons::HTTPExceptions::Exception Exception;
    using ExtConfig = BiddingFrontendCore::ExtConfig;
    using ExtConfig_var = BiddingFrontendCore::ExtConfig_var;

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
      Generics::CompositeMetricsProvider* composite_metrics_provider,
      std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
      std::shared_ptr<AdServer::Commons::ExecutorPool> timeout_workers,
      unsigned long service_index) /*throw(eh::Exception)*/;

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

    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
    user_bind_client_coro() const noexcept
    {
      return user_bind_client_coro_;
    }

    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient>
    user_info_distributed_client() const noexcept
    {
      return user_info_distributed_client_;
    }

    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
    user_info_client() const noexcept
    {
      return user_info_client_;
    }

    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
    user_info_client_coro() const noexcept
    {
      return user_info_client_coro_;
    }

    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient>
    channel_client() const noexcept
    {
      return channel_client_;
    }

    std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>
    channel_client_coro() const noexcept
    {
      return channel_client_coro_;
    }

    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
    campaign_manager() const noexcept
    {
      return campaign_manager_;
    }

    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
    campaign_manager_coro() const noexcept
    {
      return campaign_manager_coro_;
    }

    Generics::AtomicInt&
    bid_task_count() noexcept
    {
      return bid_task_count_;
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

    typedef Sync::Policy::PosixThread
      MaxPendingSyncPolicy;

  private:
    void
    parse_configs_() /*throw(Exception)*/;

    void
    fill_account_traits_() noexcept;

  private:
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

  protected:
    // configuration
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;
    CommonModule_var common_module_;
    unsigned long colo_id_;
    std::shared_ptr<SourceMap> sources_;
    std::string server_id_;
    std::shared_ptr<RequestInfoFiller> request_info_filler_;
    std::shared_ptr<RequestInfoFiller::AccountTraitsById> account_traits_;

    // external services
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      user_bind_client_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>
      user_bind_client_coro_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
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

    Generics::Planner_var planner_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> bid_workers_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> timeout_workers_;
    Generics::TaskRunner_var control_task_runner_;
    StatHolder_var stats_;

    std::unique_ptr<BiddingFrontendCore> core_;

    Generics::AtomicInt bid_task_count_;

    mutable MaxPendingSyncPolicy::Mutex reached_max_pending_tasks_lock_;
    unsigned long reached_max_pending_tasks_;

  private:
    const Generics::CompositeMetricsProvider_var composite_metrics_provider_;
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
    core_->set_config(config);
  }

  inline
  Frontend::ExtConfig_var
  Frontend::get_ext_config_() noexcept
  {
    return core_->get_config();
  }
}
