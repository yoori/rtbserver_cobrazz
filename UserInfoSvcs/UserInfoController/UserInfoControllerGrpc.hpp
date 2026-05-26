#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "UserInfoControllerImpl.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoControllerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    UserInfoControllerGrpc(
      UserInfoControllerImpl* controller,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t grpc_threads);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

    ~UserInfoControllerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using UserInfoControllerGrpc_var =
    ReferenceCounting::SmartPtr<UserInfoControllerGrpc>;
}
