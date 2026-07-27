#pragma once

#include <atomic>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "BillingServerCore.hpp"

namespace AdServer::CampaignSvcs
{
  class BillingServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    struct Stats
    {
      std::uint64_t call_total = 0;
      std::uint64_t call_total_time = 0;
      std::uint64_t call_in_progress = 0;
      std::uint64_t check_available_bid_total = 0;
      std::uint64_t check_available_bid_total_time = 0;
      std::uint64_t check_available_bid_in_progress = 0;
      std::uint64_t reserve_bid_total = 0;
      std::uint64_t reserve_bid_total_time = 0;
      std::uint64_t reserve_bid_in_progress = 0;
      std::uint64_t confirm_bid_total = 0;
      std::uint64_t confirm_bid_total_time = 0;
      std::uint64_t confirm_bid_in_progress = 0;
      std::uint64_t add_amount_total = 0;
      std::uint64_t add_amount_total_time = 0;
      std::uint64_t add_amount_in_progress = 0;
      std::uint64_t batch_total = 0;
      std::uint64_t batch_total_time = 0;
      std::uint64_t batch_in_progress = 0;
    };

    BillingServerGrpc(
      BillingServerCore* core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t process_threads = 128,
      std::size_t cq_threads = 16,
      std::size_t max_sequential_ops = 0);

    Stats stats() const noexcept;

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~BillingServerGrpc() noexcept override;

  private:
    struct AtomicStats;

    const std::string bind_address_;
    const std::size_t max_sequential_ops_;
    const std::shared_ptr<AtomicStats> stats_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::shared_ptr<Impl> impl_;
  };

  using BillingServerGrpc_var = ReferenceCounting::SmartPtr<BillingServerGrpc>;
}
