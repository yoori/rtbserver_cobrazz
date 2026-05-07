#pragma once

#include <memory>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "UserInfoController2Impl.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoController2Grpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    UserInfoController2Grpc(
      UserInfoController2Impl* controller,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

    ~UserInfoController2Grpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using UserInfoController2Grpc_var =
    ReferenceCounting::SmartPtr<UserInfoController2Grpc>;
}
