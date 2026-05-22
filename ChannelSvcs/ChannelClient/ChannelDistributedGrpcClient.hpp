#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/RefPool.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>

namespace AdServer::ChannelSvcs
{
  class ChannelDistributedGrpcClient:
    public Generics::CompositeActiveObject,
    public ChannelServerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using ChannelControllerRefs = std::vector<std::string>;

    ChannelDistributedGrpcClient(
      const ChannelControllerRefs& channel_controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner = {});

    ~ChannelDistributedGrpcClient() noexcept override = default;

    AdServer::Grpc::Stats stats() const noexcept override;

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      MatchCallback callback) override;

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      GetCcgTraitsCallback callback) override;

    void set_sources(
      const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
      SetSourcesCallback callback) override;

    void set_proxy_sources(
      const adserver::channel_svcs::channel_server::SetProxySourcesRequest& request,
      SetProxySourcesCallback callback) override;

  private:
    using Client = ChannelServerGrpcAsyncBatchingClient;
    using ClientPtr = std::shared_ptr<Client>;

    class ResolveRefsTask;

    struct ClientHolder;
    using ClientHolderPtr = std::shared_ptr<ClientHolder>;

    struct RefHolder;
    using Pool = AdServer::Grpc::RefPool<RefHolder>;
    using PoolPtr = std::shared_ptr<Pool>;
    using ControllerRefsState =
      std::vector<std::pair<std::string, std::vector<std::string>>>;

    void activate_object_() override;
    void deactivate_object_() override;

    std::optional<Pool::Ref> get_ref_() const noexcept;
    void resolve_refs_() noexcept;
    bool fill_refs_state_(ControllerRefsState& refs_state) noexcept;
    bool update_pool_if_changed_(ControllerRefsState refs_state) noexcept;
    ClientHolderPtr get_or_create_client_holder_(const std::string& endpoint);
    void wait_next_resolve_() noexcept;

    static void merge_stats_(
      AdServer::Grpc::Stats& result,
      const AdServer::Grpc::Stats& source) noexcept;

  private:
    const ChannelControllerRefs channel_controller_refs_;
    const AdServer::Grpc::BatchingOptions batching_options_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner_;
    const Generics::Time pool_timeout_;
    const Generics::Time resolve_period_;

    Generics::ActiveObjectCallback_var callback_;
    Generics::FixedTaskRunner_var task_runner_;

    mutable std::shared_mutex pool_lock_;
    PoolPtr pool_;
    ControllerRefsState refs_state_;
    std::vector<ClientHolderPtr> current_client_holders_;
    std::map<std::string, std::weak_ptr<ClientHolder>> client_holders_;

    std::mutex resolve_lock_;
    std::condition_variable resolve_cond_;
    std::atomic_bool deactivated_{false};
  };

}
