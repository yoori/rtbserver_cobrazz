#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/GrpcServer.hpp>

#include "ExpressionMatcherImpl.hpp"

namespace AdServer::RequestInfoSvcs
{
  class ExpressionMatcherGrpc:
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    ExpressionMatcherGrpc(
      ExpressionMatcherImpl* expression_matcher,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t grpc_threads);

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

    ~ExpressionMatcherGrpc() noexcept override;

  private:
    const std::string bind_address_;
    const std::shared_ptr<Impl> impl_;
  };

  using ExpressionMatcherGrpc_var =
    ReferenceCounting::SmartPtr<ExpressionMatcherGrpc>;
}
