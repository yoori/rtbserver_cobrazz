#pragma once

#include <memory>
#include <string>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include "UserBindServerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    UserBindServerGrpc(
      UserBindServerCore* core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class Impl;

  protected:
    ~UserBindServerGrpc() noexcept;

  private:
    const std::string bind_address_;
    const ReferenceCounting::SmartPtr<Impl> impl_;
  };

  using UserBindServerGrpc_var = ReferenceCounting::SmartPtr<UserBindServerGrpc>;
}
