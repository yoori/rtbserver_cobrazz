#ifndef FRONTENDS_PROFILINGSERVER_PROFILINGSERVER_HPP
#define FRONTENDS_PROFILINGSERVER_PROFILINGSERVER_HPP

// STD
#include <memory>

// UNIXCOMMONS
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>
#include <eh/Exception.hpp>

// THIS
#include <Commons/DelegateActiveObject.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/zmq.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/ProfilingServer/HashFilter.hpp>
#include <Frontends/ProfilingServer/ProfilingServerStats.hpp>
#include <Frontends/ProfilingServer/RequestInfoFiller.hpp>
#include <xsd/Frontends/ProfilingServerConfig.hpp>

namespace AdServer::Profiling
{
  class ProfilingServer final :
    public AdServer::Commons::ProcessControlVarsLoggerImpl,
    private Generics::CompositeActiveObject
  {
  private:
    using ComponentsBuilder = UServerUtils::ComponentsBuilder;
    using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
    using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
    using ManagerCoro = UServerUtils::Manager;
    using ManagerCoro_var = UServerUtils::Manager_var;
    using TaskProcessor = userver::engine::TaskProcessor;
    using GrpcUserBindOperationDistributor = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor;
    using GrpcUserBindOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var;
    using GrpcUserInfoOperationDistributor = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor;
    using GrpcUserInfoOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var;
    using GrpcChannelOperationPool = AdServer::ChannelSvcs::GrpcChannelOperationPool;
    using GrpcChannelOperationPoolPtr = std::shared_ptr<GrpcChannelOperationPool>;
    using GrpcCampaignManagerPool = FrontendCommons::GrpcCampaignManagerPool;
    using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;
    using GrpcContainerPtr = std::shared_ptr<FrontendCommons::GrpcContainer>;
    using ProfilingServerConfig = xsd::AdServer::Configuration::ProfilingServerConfigType;
    using ProfilingServerConfigPtr = std::unique_ptr<ProfilingServerConfig>;
    using Worker = std::function<void(zmq::message_t&)>;

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    class ZmqRouter final : public Commons::DelegateActiveObject
    {
    public:
      ZmqRouter(
        Generics::ActiveObjectCallback* callback,
        zmq::context_t& zmq_context,
        const xsd::AdServer::Configuration::ZmqSocketType& socket_config,
        const char* inproc_address);

    protected:
      ~ZmqRouter() override = default;

      void work_() noexcept override;

    private:
      zmq::context_t& zmq_context_;
      zmq::socket_t bind_socket_;
      zmq::socket_t inproc_socket_;
      Generics::ActiveObjectCallback_var callback_;
    };

    class ZmqWorker final : public Commons::DelegateActiveObject
    {
    public:
      ZmqWorker(
        Worker worker,
        Generics::ActiveObjectCallback* callback,
        zmq::context_t& zmq_context,
        const char* inproc_address,
        std::size_t work_threads);

    protected:
      ~ZmqWorker() override = default;

      void do_work_(const char* inproc_address) noexcept;

    private:
      zmq::context_t& zmq_context_;
      Generics::ActiveObjectCallback_var callback_;
      Worker worker_;
    };

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    ProfilingServer();

    void main(int& argc, char** argv) noexcept;

    void shutdown(CORBA::Boolean wait_for_completion) noexcept override;

    CORBACommons::IProcessControl::ALIVE_STATUS
    is_alive() noexcept override;

  private:
    ~ProfilingServer() override = default;

  private:
    void read_config_(
      const char *filename,
      const char* argv0)
      /*throw(Exception, eh::Exception)*/;

    GrpcUserBindOperationDistributor_var
    create_grpc_user_bind_operation_distributor(
      const SchedulerPtr& scheduler,
      TaskProcessor& task_processor);

    GrpcUserInfoOperationDistributor_var
    create_grpc_user_info_operation_distributor(
      const SchedulerPtr& scheduler,
      TaskProcessor& task_processor);

    GrpcChannelOperationPoolPtr create_grpc_channel_operation_pool(
      const SchedulerPtr& scheduler,
      TaskProcessor& task_processor);

    GrpcCampaignManagerPoolPtr create_grpc_campaign_manager_pool(
      const SchedulerPtr& scheduler,
      TaskProcessor& task_processor);

    void init_coroutine_();

    void init_grpc_();

    void init_corba_();

    void init_zeromq_();

    void process_dmp_profiling_info_(
      zmq::message_t& msg) noexcept;

    // user bind utils
    bool resolve_user_id_(
      AdServer::Commons::UserId& match_user_id,
      std::string& resolved_ext_user_id,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info) noexcept;

    void rebind_user_id_(
      const AdServer::Commons::UserId& user_id,
      const RequestInfo& request_info,
      const String::SubString& resolved_ext_user_id,
      const Generics::Time& time)
      noexcept;

    // trigger match utils
    void trigger_match_(
      std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>& trigger_matched_channels,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& user_id) noexcept;

    // user info utils
    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
    get_empty_history_matching_();

    void prepare_ui_match_params_(
      AdServer::UserInfoSvcs::Types::MatchParams& match_params,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo& match_result);

    void history_match_(
      std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>& match_result_out,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::ChannelSvcs::Proto::MatchResponseInfo* const trigger_matching_result) noexcept;

    void merge_users_(
      const AdServer::Commons::UserId& target_user_id,
      const AdServer::Commons::UserId& source_user_id,
      const bool source_temporary,
      const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params) noexcept;

    // request campaign manager
    void request_campaign_manager_(
      const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params);

  private:
    ManagerCoro_var manager_coro_;
    GrpcContainerPtr grpc_container_;
    CORBACommons::CorbaConfig corba_config_;
    ProfilingServerConfigPtr config_;
    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
    Logging::LoggerCallbackHolder logger_callback_holder_;
    std::unique_ptr<zmq::context_t> zmq_context_;
    DMPKafkaStreamer_var streamer_;
    HashFilter_var hash_filter_;
    ProfilingServerStats_var stats_;
    unsigned long hash_time_precision_;
  };

  using ProfilingServer_var = ReferenceCounting::QualPtr<ProfilingServer>;
  using ProfilingServerApp = Generics::Singleton<ProfilingServer, ProfilingServer_var>;
} // namespace AdServer::Profiling

#endif /* FRONTENDS_PROFILINGSERVER_PROFILINGSERVER_HPP */