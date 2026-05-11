#pragma once

#include <memory>
#include <string>

#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/GrpcServer.hpp>

#include "CampaignManagerCore.hpp"

namespace AdServer::CampaignSvcs
{
  class CampaignManagerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    CampaignManagerGrpc(
      CampaignManagerCore* core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~CampaignManagerGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using CampaignManagerGrpc_var =
    ReferenceCounting::SmartPtr<CampaignManagerGrpc>;
}
