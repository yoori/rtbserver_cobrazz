#pragma once

#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>

namespace AdServer::UserInfoSvcs
{
  class UserBindDistributedGrpcClient:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public UserBindServerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using UserBindControllerRefs = std::vector<std::string>;

    UserBindDistributedGrpcClient(
      const UserBindControllerRefs& user_bind_controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      Logging::Logger* logger);

    ~UserBindDistributedGrpcClient() noexcept override = default;

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

    ReferenceCounting::SmartPtr<Distributor> user_bind_mapper_;
  };

  using UserBindDistributedGrpcClient_var = ReferenceCounting::SmartPtr<UserBindDistributedGrpcClient>;
}
