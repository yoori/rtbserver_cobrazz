#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>
#include <UserBindServerGrpc.grpc-client.hpp>

namespace FrontendCommons
{
  class UserBindCorbaClient:
    public Generics::CompositeActiveObject,
    public AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient
  {
  public:
    typedef xsd::AdServer::Configuration::
      CommonFeConfigurationType::UserBindControllerGroup_sequence
      UserBindControllerGroupSeq;

  public:
    UserBindCorbaClient(
      const UserBindControllerGroupSeq& user_bind_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger)
      noexcept;

    ~UserBindCorbaClient() noexcept override = default;

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
    AdServer::UserInfoSvcs::UserBindOperationDistributor_var user_bind_mapper_;
  };

  using UserBindCorbaClientPtr = std::shared_ptr<UserBindCorbaClient>;
}
