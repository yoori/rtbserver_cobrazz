#pragma once

#include <memory>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include "ChannelController2Impl.hpp"

namespace AdServer::ChannelSvcs
{
  class ChannelController2Grpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ChannelController2Grpc(
      ChannelController2Impl* controller,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

    ~ChannelController2Grpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using ChannelController2Grpc_var =
    ReferenceCounting::SmartPtr<ChannelController2Grpc>;
}
