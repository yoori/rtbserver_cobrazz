#pragma once

#include <memory>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/GrpcServer.hpp>

#include "ChannelServerImpl.hpp"

namespace AdServer::ChannelSvcs
{
  class ChannelServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ChannelServerGrpc(
      ChannelServerCustomImpl* server_impl,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~ChannelServerGrpc() noexcept;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using ChannelServerGrpc_var = ReferenceCounting::SmartPtr<ChannelServerGrpc>;
}
