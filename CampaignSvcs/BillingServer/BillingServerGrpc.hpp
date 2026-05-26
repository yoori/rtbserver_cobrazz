#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <string_view>

#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/GrpcServer.hpp>

#include "BillingServerCore.hpp"

namespace AdServer::CampaignSvcs
{
  class BillingServerGrpc:
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    BillingServerGrpc(
      BillingServerCore* core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t grpc_threads);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~BillingServerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using BillingServerGrpc_var = ReferenceCounting::SmartPtr<BillingServerGrpc>;
}
