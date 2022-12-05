#ifndef _USER_INFO_CLIENT_
#define _USER_INFO_CLIENT_

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoOperationDistributor.hpp>

namespace FrontendCommons
{
  class UserInfoClient:
      public virtual ReferenceCounting::AtomicImpl,
      public Generics::CompositeActiveObject
  {
  public:
    typedef xsd::AdServer::Configuration::
      CommonFeConfigurationType::UserInfoManagerControllerGroup_sequence
      UserInfoManagerControllerGroupSeq;

  public:
    UserInfoClient(
      const UserInfoManagerControllerGroupSeq& user_info_manager_controller_group,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Logging::Logger* logger)
      noexcept;

    virtual ~UserInfoClient() noexcept {};

    AdServer::UserInfoSvcs::UserInfoMatcher*
    user_info_session() noexcept;

  private:
    AdServer::UserInfoSvcs::UserInfoMatcher_var user_info_matcher_;
  };

  typedef ReferenceCounting::SmartPtr<UserInfoClient> UserInfoClient_var;
}

#endif /* _USER_INFO_CLIENT_ */
