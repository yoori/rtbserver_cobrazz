#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

namespace AdServer::ChannelSvcs
{
  class ChannelServerCore;
  using ChannelServerCorePtr = std::shared_ptr<ChannelServerCore>;
}

namespace AdServer::ChannelSvcs
{
  class ChannelServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    struct Stats
    {
      std::uint64_t call_in_progress = 0;
      std::uint64_t match_in_progress = 0;
      std::uint64_t get_ccg_traits_in_progress = 0;
      std::uint64_t check_configuration_in_progress = 0;
      std::uint64_t set_sources_in_progress = 0;
      std::uint64_t set_proxy_sources_in_progress = 0;
      std::uint64_t batch_total = 0;
      std::uint64_t batch_total_time = 0;
      std::uint64_t batch_in_progress = 0;
      AdServer::Grpc::GrpcServiceBase::LifecycleStatsSnapshot
        grpc_lifecycle_stats;
    };

    ChannelServerGrpc(
      ChannelServerCorePtr core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t process_threads,
      std::size_t cq_threads,
      std::size_t max_split);

    Stats stats() const noexcept;

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~ChannelServerGrpc() noexcept;

  private:
    struct AtomicStats;

    const std::string bind_address_;
    const std::shared_ptr<AtomicStats> stats_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::shared_ptr<Impl> impl_;
  };

  using ChannelServerGrpc_var = ReferenceCounting::SmartPtr<ChannelServerGrpc>;
}
