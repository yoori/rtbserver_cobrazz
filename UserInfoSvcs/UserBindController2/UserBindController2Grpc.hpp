#pragma once

#include <memory>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "UserBindController2Impl.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindController2Grpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    UserBindController2Grpc(
      UserBindController2Impl* controller,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

    ~UserBindController2Grpc() noexcept override;

  private:
    const std::string bind_address_;
    const ReferenceCounting::SmartPtr<Impl> impl_;
  };

  using UserBindController2Grpc_var =
    ReferenceCounting::SmartPtr<UserBindController2Grpc>;
}
