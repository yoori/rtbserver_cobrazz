#pragma once

#include <atomic>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>

namespace AdServer::ChannelSvcs
{
  class ChannelDistributedGrpcClient:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject,
    public ChannelServerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using ChannelServerRefs = std::vector<std::string>;

    ChannelDistributedGrpcClient(
      const ChannelServerRefs& channel_server_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      AdServer::Grpc::GrpcExecutor* grpc_executor);

    ~ChannelDistributedGrpcClient() noexcept override = default;

    AdServer::Grpc::Stats stats() const noexcept override;

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      MatchCallback callback) override;

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      GetCcgTraitsCallback callback) override;

  private:
    using Client = ChannelServerGrpcAsyncBatchingClient;
    using Client_var = ReferenceCounting::SmartPtr<Client>;
    using ClientArray = std::vector<Client_var>;

    Client* next_client_() noexcept;

    static void merge_stats_(
      AdServer::Grpc::Stats& result,
      const AdServer::Grpc::Stats& source) noexcept;

  private:
    ClientArray clients_;
    std::atomic<std::size_t> next_client_index_{0};
  };

  using ChannelDistributedGrpcClient_var =
    ReferenceCounting::SmartPtr<ChannelDistributedGrpcClient>;
}
