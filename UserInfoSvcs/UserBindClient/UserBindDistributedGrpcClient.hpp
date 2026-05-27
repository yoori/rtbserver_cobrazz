#pragma once

#include <memory>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>

namespace AdServer::UserInfoSvcs
{
  class UserBindDistributedGrpcClient:
    public Generics::CompositeActiveObject,
    public UserBindServerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using UserBindControllerRefGroup = std::vector<std::string>;
    using UserBindControllerRefs = std::vector<UserBindControllerRefGroup>;

    UserBindDistributedGrpcClient(
      const UserBindControllerRefs& user_bind_controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner = {});

    ~UserBindDistributedGrpcClient() noexcept override;

    AdServer::Grpc::Stats stats() const noexcept override;

    void get_bind_request(
      const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
      GetBindRequestCallback callback) override;

    void add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      AddBindRequestCallback callback) override;

    void get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      GetUserIdCallback callback) override;

    void add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      AddUserIdCallback callback) override;

    void get_source(
      const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
      GetSourceCallback callback) override;

  private:
    class Distributor;

    std::shared_ptr<Distributor> user_bind_mapper_;
  };
}
