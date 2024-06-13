#include <Commons/CorbaConfig.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

#include "UserBindClient.hpp"

namespace FrontendCommons
{
  UserBindClient::UserBindClient(
    const UserBindControllerGroupSeq& user_bind_controller_group,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Logging::Logger* logger,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const ConfigGrpcClient& config_grpc_client,
    const std::size_t timeout_grpc_client,
    const bool grpc_enable) noexcept
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

    if (grpc_enable)
    {
      grpc_distributor_ = GrpcDistributor_var(
        new GrpcDistributor(
          logger,
          task_processor,
          scheduler,
          controller_groups,
          corba_client_adapter,
          config_grpc_client,
          timeout_grpc_client,
          Generics::Time::ONE_SECOND));
      add_child_object(grpc_distributor_);
    }
  }

  AdServer::UserInfoSvcs::UserBindMapper*
  UserBindClient::user_bind_mapper() noexcept
  {
    return AdServer::UserInfoSvcs::
      UserBindMapper::_duplicate(user_bind_mapper_);
  }

  UserBindClient::GrpcDistributor_var
  UserBindClient::grpc_distributor() noexcept
  {
    return grpc_distributor_;
  }
}
