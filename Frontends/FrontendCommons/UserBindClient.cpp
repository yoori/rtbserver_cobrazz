#include <Commons/CorbaConfig.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

#include "UserBindClient.hpp"

namespace FrontendCommons
{
  UserBindClient::UserBindClient(
    const UserBindControllerGroupSeq& user_bind_controller_group,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Logging::Logger* logger)
    noexcept
  {
    AdServer::UserInfoSvcs::UserBindOperationDistributor::
      ControllerRefList controller_groups;

    for(UserBindControllerGroupSeq::const_iterator cg_it =
        user_bind_controller_group.begin();
        cg_it != user_bind_controller_group.end();
        ++cg_it)
    {
      AdServer::UserInfoSvcs::UserBindOperationDistributor::
        ControllerRef controller_ref_group;

      Config::CorbaConfigReader::read_multi_corba_ref(
        *cg_it,
        controller_ref_group);

      controller_groups.push_back(controller_ref_group);
    }

    AdServer::UserInfoSvcs::UserBindOperationDistributor_var distributor =
      new AdServer::UserInfoSvcs::UserBindOperationDistributor(
        logger,
        controller_groups,
        corba_client_adapter);
    user_bind_mapper_ = ReferenceCounting::add_ref(distributor);
    add_child_object(distributor);
  }

  AdServer::UserInfoSvcs::UserBindMapper*
  UserBindClient::user_bind_mapper() noexcept
  {
    return AdServer::UserInfoSvcs::
      UserBindMapper::_duplicate(user_bind_mapper_);
  }
}
