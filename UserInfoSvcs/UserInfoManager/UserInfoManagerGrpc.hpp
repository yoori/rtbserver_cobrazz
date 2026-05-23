#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

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
      std::uint64_t call_in_progress = 0;
      std::uint64_t match_in_progress = 0;
      std::uint64_t update_user_freq_caps_in_progress = 0;
      std::uint64_t confirm_user_freq_caps_in_progress = 0;
      std::uint64_t fraud_user_in_progress = 0;
      std::uint64_t remove_user_profile_in_progress = 0;
      std::uint64_t merge_in_progress = 0;
      std::uint64_t consider_publishers_optin_in_progress = 0;
    };

    UserInfoManagerGrpc(
      UserInfoManagerCorePtr user_info_manager,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

    Stats stats() const noexcept;

  protected:
    struct StatsCounters;
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~UserInfoManagerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<StatsCounters> stats_counters_;
    const std::shared_ptr<Impl> impl_;
  };

  using UserInfoManagerGrpc_var =
    ReferenceCounting::SmartPtr<UserInfoManagerGrpc>;
}
