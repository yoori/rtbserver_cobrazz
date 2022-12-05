#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROL_IMPL_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROL_IMPL_HPP_

#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Sync/SyncPolicy.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerControl_s.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerImpl.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /**
     * Implementation of UserInfoManagerControl.
     */
    class UserInfoManagerControlImpl :
      public virtual CORBACommons::ReferenceCounting::ServantImpl<
        POA_AdServer::UserInfoSvcs::UserInfoManagerControl>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      
      UserInfoManagerControlImpl(
        UserInfoManagerImpl* user_info_manager_impl)
        /*throw(Exception)*/;

      virtual ~UserInfoManagerControlImpl() noexcept;

      /** UserInfoManagerControl interface */
      virtual AdServer::UserInfoSvcs::UserInfoManagerStatus status() noexcept;

      virtual void 
      get_source_info(
        AdServer::UserInfoSvcs::ChunksConfig_out user_info_source)
        /*throw(AdServer::UserInfoSvcs::UserInfoManagerControl::ImplementationException)*/;

      virtual void admit() noexcept;

    protected:
      typedef Sync::PosixRWLock Mutex_;
      typedef Sync::PosixRGuard ReadGuard_;
      typedef Sync::PosixWGuard WriteGuard_;
     
      mutable Mutex_ lock_;
      
      Logging::Logger_var logger_;
      UserInfoManagerImpl_var user_info_manager_impl_;
      bool admitted_;
    };

    typedef
      ReferenceCounting::SmartPtr<UserInfoManagerControlImpl>
      UserInfoManagerControlImpl_var;

  } /* UserInfoSvcs */
} /* AdServer */

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_CONTROL_IMPL_HPP_*/
