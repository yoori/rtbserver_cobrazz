#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include "UserBindServerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    struct Stats
    {
      std::uint64_t call_in_progress = 0;
      std::uint64_t get_bind_request_in_progress = 0;
      std::uint64_t add_bind_request_in_progress = 0;
      std::uint64_t get_user_id_in_progress = 0;
      std::uint64_t add_user_id_in_progress = 0;
    };

    UserBindServerGrpc(
      UserBindServerCore* core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t process_threads = 128,
      std::size_t max_split = 0,
      std::shared_ptr<std::atomic_uint> response_sleep_ms = nullptr);

    Stats stats() const noexcept;

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~UserBindServerGrpc() noexcept;

  private:
    struct AtomicStats;

    const std::string bind_address_;
    const std::size_t max_batch_split_;
    const std::shared_ptr<AtomicStats> stats_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::shared_ptr<Impl> impl_;
  };

  using UserBindServerGrpc_var = ReferenceCounting::SmartPtr<UserBindServerGrpc>;
}
