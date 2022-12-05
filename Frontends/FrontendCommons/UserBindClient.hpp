#ifndef USERBINDCLIENT_HPP_
#define USERBINDCLIENT_HPP_

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

namespace FrontendCommons
{
  class UserBindClient:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject
  {
  public:
    typedef xsd::AdServer::Configuration::
      CommonFeConfigurationType::UserBindControllerGroup_sequence
      UserBindControllerGroupSeq;

  public:
    UserBindClient(
      const UserBindControllerGroupSeq& user_bind_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger)
      noexcept;

    virtual ~UserBindClient() noexcept {};

    AdServer::UserInfoSvcs::UserBindMapper*
    user_bind_mapper() noexcept;

  private:
    AdServer::UserInfoSvcs::UserBindOperationDistributor_var user_bind_mapper_;
    //AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindClient>
    UserBindClient_var;
}

#endif /*USERBINDCLIENT_HPP_*/
