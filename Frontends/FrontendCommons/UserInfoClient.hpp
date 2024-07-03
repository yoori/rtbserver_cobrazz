#ifndef _USER_INFO_CLIENT_
#define _USER_INFO_CLIENT_

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <UserInfoSvcs/UserInfoManagerController/GrpcUserInfoOperationDistributor.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>

namespace FrontendCommons
{
  class UserInfoClient final:
      public virtual ReferenceCounting::AtomicImpl,
      public Generics::CompositeActiveObject
  {
  public:
    using UserInfoManagerControllerGroupSeq =
      xsd::AdServer::Configuration::CommonFeConfigurationType::UserInfoManagerControllerGroup_sequence;
    using GrpcUserInfoOperationDistributor =
      AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor;
    using GrpcUserInfoOperationDistributor_var =
      AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var;

  public:
    explicit UserInfoClient(
      const UserInfoManagerControllerGroupSeq& user_info_manager_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger,
      GrpcUserInfoOperationDistributor* grpc_user_info_distributor) noexcept;

    AdServer::UserInfoSvcs::UserInfoMatcher*
    user_info_session() noexcept;

    GrpcUserInfoOperationDistributor_var
    grpc_distributor() noexcept;

  protected:
    ~UserInfoClient() override = default;

  private:
    AdServer::UserInfoSvcs::UserInfoMatcher_var user_info_matcher_;

    GrpcUserInfoOperationDistributor_var grpc_user_info_distributor_;
  };

  using UserInfoClient_var = ReferenceCounting::SmartPtr<UserInfoClient>;
}

#endif /* _USER_INFO_CLIENT_ */