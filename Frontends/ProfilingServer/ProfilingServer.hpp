#ifndef FRONTENDS_PROFILINGSERVER_PROFILINGSERVER_HPP_
#define FRONTENDS_PROFILINGSERVER_PROFILINGSERVER_HPP_

#include <memory>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/zmq.hpp>

#include <Frontends/FrontendCommons/ChannelServerSessionPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/FrontendCommons/UserBindClient.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <xsd/Frontends/ProfilingServerConfig.hpp>
#include <Frontends/ProfilingServer/RequestInfoFiller.hpp>
#include <Frontends/ProfilingServer/HashFilter.hpp>
#include <Frontends/ProfilingServer/ProfilingServerStats.hpp>

#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>

namespace AdServer
{
namespace Profiling
{
  class ProfilingServer:
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

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    ProfilingServer() /*throw(eh::Exception)*/;

    void
    main(int& argc, char** argv) noexcept;

    //
    // IDL:CORBACommons/IProcessControl/shutdown:1.0
    //
    virtual void
    shutdown(CORBA::Boolean wait_for_completion)
      noexcept;

    //
    // IDL:CORBACommons/IProcessControl/is_alive:1.0
    //
    virtual CORBACommons::IProcessControl::ALIVE_STATUS
    is_alive() noexcept;

  private:
    virtual
    ~ProfilingServer() noexcept
    {}

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

    typedef xsd::AdServer::Configuration::ProfilingServerConfigType
      ProfilingServerConfig;

    typedef std::unique_ptr<ProfilingServerConfig> ProfilingServerConfigPtr;
    typedef std::function<void(zmq::message_t&)> Worker;

    class ZmqRouter : public Commons::DelegateActiveObject
    {
    public:
      ZmqRouter(
        Generics::ActiveObjectCallback* callback,
        zmq::context_t& zmq_context,
        const xsd::AdServer::Configuration::ZmqSocketType& socket_config,
        const char* inproc_address)
        /*throw(eh::Exception)*/;

    protected:
      virtual
      ~ZmqRouter() noexcept
      {}

      virtual void
      work_() noexcept;

    private:
      zmq::context_t& zmq_context_;
      zmq::socket_t bind_socket_;
      zmq::socket_t inproc_socket_;
      Generics::ActiveObjectCallback_var callback_;
    };

    class ZmqWorker : public Commons::DelegateActiveObject
    {
    public:
      ZmqWorker(
        Worker worker,
        Generics::ActiveObjectCallback* callback,
        zmq::context_t& zmq_context,
        const char* inproc_address,
        std::size_t work_threads)
        /*throw(eh::Exception)*/;

    protected:
      virtual
      ~ZmqWorker() noexcept
      {}

      void
      do_work_(const char* inproc_address) noexcept;

    private:
      zmq::context_t& zmq_context_;
      Generics::ActiveObjectCallback_var callback_;
      Worker worker_;
    };

  private:
    void
    read_config_(
      const char *filename,
      const char* argv0)
      /*throw(Exception, eh::Exception)*/;

    GrpcUserBindOperationDistributor_var
    create_grpc_user_bind_operation_distributor(
      const SchedulerPtr& scheduler,
      TaskProcessor& task_processor);

    void
    init_coroutine_();

    void
    init_corba_() /*throw(Exception)*/;

    void
    init_zeromq_() /*throw(Exception)*/;

    void
    process_dmp_profiling_info_(zmq::message_t& msg) noexcept;

    // user bind utils
    bool
    resolve_user_id_(
      AdServer::Commons::UserId& match_user_id,
      std::string& resolved_ext_user_id,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info)
      noexcept;

    void
    rebind_user_id_(
      const AdServer::Commons::UserId& user_id,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const RequestInfo& request_info,
      const String::SubString& resolved_ext_user_id,
      const Generics::Time& time)
      noexcept;

    // trigger match utils
    void
    trigger_match_(
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out trigger_matched_channels,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& user_id)
      noexcept;

    // user info utils
    static AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
    get_empty_history_matching_() /*throw(eh::Exception)*/;

    void
    prepare_ui_match_params_(
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
      const AdServer::ChannelSvcs::ChannelServerBase::MatchResult& match_result)
      /*throw(eh::Exception)*/;

    void
    history_match_(
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out history_match_result,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const RequestInfo& request_info,
      const AdServer::ChannelSvcs::ChannelServerBase::MatchResult* trigger_match_result)
      noexcept;

    void
    merge_users_(
      const AdServer::Commons::UserId& target_user_id,
      const AdServer::Commons::UserId& source_user_id,
      bool source_temporary,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params)
      noexcept;

    // request campaign manager
    void
    request_campaign_manager_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult*
        trigger_matched_channels,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
        history_match_result)
      /*throw(Exception)*/;

  private:
    ManagerCoro_var manager_coro_;
    CORBACommons::CorbaConfig corba_config_;
    ProfilingServerConfigPtr config_;
    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
    Logging::LoggerCallbackHolder logger_callback_holder_;
    std::unique_ptr<zmq::context_t> zmq_context_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    std::unique_ptr<FrontendCommons::ChannelServerSessionPool> channel_servers_;
    FrontendCommons::UserBindClient_var user_bind_client_;
    FrontendCommons::UserInfoClient_var user_info_client_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    DMPKafkaStreamer_var streamer_;
    HashFilter_var hash_filter_;
    ProfilingServerStats_var stats_;
    unsigned long hash_time_precision_;
  };

  typedef ReferenceCounting::QualPtr<ProfilingServer>
    ProfilingServer_var;

  typedef Generics::Singleton<ProfilingServer, ProfilingServer_var>
    ProfilingServerApp;
}
}

#endif /* FRONTENDS_PROFILINGSERVER_PROFILINGSERVER_HPP_ */
