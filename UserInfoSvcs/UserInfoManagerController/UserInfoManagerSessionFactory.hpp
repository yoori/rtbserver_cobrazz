#pragma once

#include <eh/Exception.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /** UserInfoManagerSessionFactory */
    class UserInfoManagerSessionFactory
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      static void
      init(const CORBACommons::CorbaClientAdapter& corba_client_adapter)
        /*throw(eh::Exception)*/;
    };
  } /* UserInfoSvcs */
} /* AdServer */
