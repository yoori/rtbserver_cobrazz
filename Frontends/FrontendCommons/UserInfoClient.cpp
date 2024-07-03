#include <Commons/CorbaConfig.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>
#include "UserInfoClient.hpp"

namespace FrontendCommons
{
  UserInfoClient::UserInfoClient(
    const UserInfoManagerControllerGroupSeq& user_info_manager_controller_group,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Logging::Logger* logger,
    GrpcUserInfoOperationDistributor* grpc_user_info_distributor) noexcept
    : grpc_user_info_distributor_(ReferenceCounting::add_ref(grpc_user_info_distributor))
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
  }

  AdServer::UserInfoSvcs::UserInfoMatcher*
  UserInfoClient::user_info_session() noexcept
  {
    return AdServer::UserInfoSvcs::
      UserInfoManagerSession::_duplicate(user_info_matcher_);
  }

  UserInfoClient::GrpcUserInfoOperationDistributor_var
  UserInfoClient::grpc_distributor() noexcept
  {
    return grpc_user_info_distributor_;
  }
}
