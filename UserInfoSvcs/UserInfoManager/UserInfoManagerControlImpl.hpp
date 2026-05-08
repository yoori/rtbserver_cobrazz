#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROL_IMPL_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROL_IMPL_HPP_

#include <memory>

#include <eh/Exception.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerControl_s.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerCore.hpp>

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
        UserInfoManagerCorePtr user_info_manager_impl)
        /*throw(Exception)*/;

      virtual ~UserInfoManagerControlImpl() noexcept;

      virtual void
      get_source_info(
        AdServer::UserInfoSvcs::ChunksConfig_out user_info_source)
        /*throw(AdServer::UserInfoSvcs::UserInfoManagerControl::ImplementationException)*/;

      virtual void admit() noexcept;

    protected:
      Logging::Logger_var logger_;
      UserInfoManagerCorePtr user_info_manager_impl_;
    };

    typedef
      ReferenceCounting::SmartPtr<UserInfoManagerControlImpl>
      UserInfoManagerControlImpl_var;

  } /* UserInfoSvcs */
} /* AdServer */

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_CONTROL_IMPL_HPP_*/
