#pragma once

#include <memory>
#include <string>

#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/GrpcServer.hpp>

namespace AdServer::ChannelSvcs
{
  class ChannelServerCore;
  using ChannelServerCorePtr = std::shared_ptr<ChannelServerCore>;
}

namespace AdServer::ChannelSvcs
{
  class ChannelServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ChannelServerGrpc(
      ChannelServerCorePtr core,
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
