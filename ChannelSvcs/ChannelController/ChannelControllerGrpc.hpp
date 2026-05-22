#pragma once

#include <memory>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "ChannelControllerImpl.hpp"

namespace AdServer::ChannelSvcs
{
  class ChannelControllerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ChannelControllerGrpc(
      ChannelControllerImpl* controller,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

    ~ChannelControllerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using ChannelControllerGrpc_var =
    ReferenceCounting::SmartPtr<ChannelControllerGrpc>;
}
