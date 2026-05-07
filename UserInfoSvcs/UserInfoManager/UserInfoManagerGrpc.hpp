#pragma once

#include <memory>
#include <string>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "UserInfoManagerImpl.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoManagerGrpc:
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    UserInfoManagerGrpc(
      UserInfoManagerImpl* user_info_manager,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~UserInfoManagerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using UserInfoManagerGrpc_var =
    ReferenceCounting::SmartPtr<UserInfoManagerGrpc>;
}
