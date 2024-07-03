#ifndef USERBINDCLIENT_HPP_
#define USERBINDCLIENT_HPP_

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <UserInfoSvcs/UserBindController/GrpcUserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

namespace FrontendCommons
{
  class UserBindClient final:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject
  {
  public:
    using UserBindControllerGroupSeq = xsd::AdServer::Configuration::
      CommonFeConfigurationType::UserBindControllerGroup_sequence;
    using GrpcUserBindOperationDistributor = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor;
    using GrpcUserBindOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var;

  public:
    explicit UserBindClient(
      const UserBindControllerGroupSeq& user_bind_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger,
      GrpcUserBindOperationDistributor* grpc_distributor) noexcept;

    AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper() noexcept;

    GrpcUserBindOperationDistributor_var grpc_distributor() noexcept;

  protected:
    ~UserBindClient() override = default;

  private:
    AdServer::UserInfoSvcs::UserBindOperationDistributor_var user_bind_mapper_;

    GrpcUserBindOperationDistributor_var grpc_distributor_;
  };

  using UserBindClient_var = ReferenceCounting::SmartPtr<UserBindClient>;
}

#endif /*USERBINDCLIENT_HPP_*/
