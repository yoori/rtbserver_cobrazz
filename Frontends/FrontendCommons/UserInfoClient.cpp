#include <Commons/CorbaConfig.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>
#include "UserInfoClient.hpp"

namespace FrontendCommons
{
  UserInfoClient::UserInfoClient(
    const UserInfoManagerControllerGroupSeq& user_info_manager_controller_group,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Logging::Logger* logger,
    ManagerCoro* manager_coro,
    const ConfigGrpcClient& config_grpc_client,
    const std::size_t timeout_grpc_client,
    const bool grpc_enable) noexcept
  {
    AdServer::UserInfoSvcs::UserInfoOperationDistributor::
      ControllerRefList controller_groups;

    for(UserInfoManagerControllerGroupSeq::const_iterator cg_it =
        user_info_manager_controller_group.begin();
        cg_it != user_info_manager_controller_group.end();
        ++cg_it)
    {
      AdServer::UserInfoSvcs::UserInfoOperationDistributor::
        ControllerRef controller_ref_group;

      Config::CorbaConfigReader::read_multi_corba_ref(
        *cg_it,
        controller_ref_group);

      controller_groups.push_back(controller_ref_group);
    }

    AdServer::UserInfoSvcs::UserInfoOperationDistributor_var distributor =
      new AdServer::UserInfoSvcs::UserInfoOperationDistributor(
        logger,
        controller_groups,
        corba_client_adapter);
    user_info_matcher_ = ReferenceCounting::add_ref(distributor);
    add_child_object(distributor);

    if (grpc_enable)
    {
      grpc_distributor_ = GrpcDistributor_var(
        new GrpcDistributor(
          logger,
          manager_coro,
          controller_groups,
          corba_client_adapter,
          config_grpc_client,
          timeout_grpc_client,
          Generics::Time::ONE_SECOND));
      add_child_object(grpc_distributor_);
    }
  }

  AdServer::UserInfoSvcs::UserInfoMatcher*
  UserInfoClient::user_info_session() noexcept
  {
    return AdServer::UserInfoSvcs::
      UserInfoManagerSession::_duplicate(user_info_matcher_);
  }

  UserInfoClient::GrpcDistributor_var
  UserInfoClient::grpc_distributor() noexcept
  {
    return grpc_distributor_;
  }
}
