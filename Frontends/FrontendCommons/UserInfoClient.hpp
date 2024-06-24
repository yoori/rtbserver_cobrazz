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
    using GrpcDistributor =
      AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor;
    using GrpcDistributor_var =
      AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var;
    using ConfigGrpcClient = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;
    using TaskProcessor = userver::engine::TaskProcessor;

  public:
    explicit UserInfoClient(
      const UserInfoManagerControllerGroupSeq& user_info_manager_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger,
      TaskProcessor& task_processor,
      const ConfigGrpcClient& config_grpc_client,
      const std::size_t timeout_grpc_client,
      const bool grpc_enable) noexcept;

    AdServer::UserInfoSvcs::UserInfoMatcher*
    user_info_session() noexcept;

    GrpcDistributor_var grpc_distributor() noexcept;

  protected:
    ~UserInfoClient() override = default;

  private:
    AdServer::UserInfoSvcs::UserInfoMatcher_var user_info_matcher_;

    GrpcDistributor_var grpc_distributor_;
  };

  using UserInfoClient_var = ReferenceCounting::SmartPtr<UserInfoClient>;
}

#endif /* _USER_INFO_CLIENT_ */
