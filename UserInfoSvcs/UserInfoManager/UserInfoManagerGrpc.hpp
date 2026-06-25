#pragma once

#include <cstddef>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/ExecutorPool.hpp>

#include "UserInfoManagerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoManagerGrpc:
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
      std::uint64_t match_total = 0;
      std::uint64_t match_total_time = 0;
      std::uint64_t match_in_progress = 0;
      std::uint64_t update_user_freq_caps_total = 0;
      std::uint64_t update_user_freq_caps_total_time = 0;
      std::uint64_t update_user_freq_caps_in_progress = 0;
      std::uint64_t confirm_user_freq_caps_total = 0;
      std::uint64_t confirm_user_freq_caps_total_time = 0;
      std::uint64_t confirm_user_freq_caps_in_progress = 0;
      std::uint64_t fraud_user_total = 0;
      std::uint64_t fraud_user_total_time = 0;
      std::uint64_t fraud_user_in_progress = 0;
      std::uint64_t remove_user_profile_total = 0;
      std::uint64_t remove_user_profile_total_time = 0;
      std::uint64_t remove_user_profile_in_progress = 0;
      std::uint64_t merge_total = 0;
      std::uint64_t merge_total_time = 0;
      std::uint64_t merge_in_progress = 0;
      std::uint64_t consider_publishers_optin_total = 0;
      std::uint64_t consider_publishers_optin_total_time = 0;
      std::uint64_t consider_publishers_optin_in_progress = 0;
      std::uint64_t batch_total = 0;
      std::uint64_t batch_total_time = 0;
      std::uint64_t batch_in_progress = 0;
      std::uint64_t call_inflight = 0;
      std::optional<Generics::Time> min_time_of_request_in_progress;
      AdServer::Grpc::GrpcServiceBase::LifecycleStatsSnapshot
        grpc_lifecycle_stats;
    };

    UserInfoManagerGrpc(
      UserInfoManagerCorePtr user_info_manager,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t process_threads = 128,
      std::size_t cq_threads = 16,
      std::size_t max_split = 0);

    Stats stats() const noexcept;

  protected:
    struct StatsCounters;
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~UserInfoManagerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::size_t max_batch_split_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::shared_ptr<StatsCounters> stats_counters_;
    const std::shared_ptr<Impl> impl_;
  };

  using UserInfoManagerGrpc_var =
    ReferenceCounting::SmartPtr<UserInfoManagerGrpc>;
}
