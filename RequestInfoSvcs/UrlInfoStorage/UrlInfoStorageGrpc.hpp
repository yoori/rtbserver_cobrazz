#pragma once

#include <Commons/Grpc/GrpcServer.hpp>
#include "UrlInfoStorage.hpp"

namespace AdServer::RequestInfoSvcs
{
  class UrlInfoStorageGrpc final: public Generics::RefCountableCompositeActiveObject
  {
  public:
    UrlInfoStorageGrpc(
      UrlInfoStorage* storage, Logging::Logger* logger, const std::string& address,
      std::size_t cq_threads, std::size_t threads_per_cq);

  protected:
    ~UrlInfoStorageGrpc() noexcept override;

  private:
    class ServiceImpl;
    std::shared_ptr<Grpc::GrpcServer<ServiceImpl>> impl_;
  };

  using UrlInfoStorageGrpc_var = ReferenceCounting::SmartPtr<UrlInfoStorageGrpc>;
}
