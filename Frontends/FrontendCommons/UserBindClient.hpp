#ifndef USERBINDCLIENT_HPP_
#define USERBINDCLIENT_HPP_

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <UserInfoSvcs/UserBindController/GrpcUserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

#include <UServerUtils/Grpc/Manager.hpp>

namespace FrontendCommons
{
  class UserBindClient final:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject
  {
  public:
    using UserBindControllerGroupSeq = xsd::AdServer::Configuration::
      CommonFeConfigurationType::UserBindControllerGroup_sequence;
    using GrpcDistributor =
      AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor;
    using GrpcDistributor_var =
      AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var;
    using ManagerCoro = UServerUtils::Grpc::Manager;
    using ConfigGrpcClient = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  public:
    explicit UserBindClient(
      const UserBindControllerGroupSeq& user_bind_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger,
      ManagerCoro* manager_coro,
      const ConfigGrpcClient& config_grpc_client,
      const std::size_t timeout_grpc_client,
      const bool grpc_enable) noexcept;

    AdServer::UserInfoSvcs::UserBindMapper*
    user_bind_mapper() noexcept;

    GrpcDistributor_var grpc_distributor() noexcept;

  protected:
    ~UserBindClient() override = default;

  private:
    AdServer::UserInfoSvcs::UserBindOperationDistributor_var user_bind_mapper_;

    GrpcDistributor_var grpc_distributor_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindClient>
    UserBindClient_var;
}

#endif /*USERBINDCLIENT_HPP_*/
