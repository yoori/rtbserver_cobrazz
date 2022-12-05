#ifndef USER_INFO_CLUSTER_CONTROLLER_HPP
#define USER_INFO_CLUSTER_CONTROLLER_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoClusterControl.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerControl.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    typedef std::vector<CORBACommons::CorbaObjectRef> UIMRefVector;
    typedef std::vector<AdServer::UserInfoSvcs::UserInfoManager_var> UIMVector;

    class UserInfoClusterControlImpl:
      public CORBACommons::ProcessControlDefault<
        POA_CORBACommons::IProcessControl>
    {
    public:
      UserInfoClusterControlImpl()
        noexcept;

      UserInfoClusterControlImpl(
        const CORBACommons::CorbaClientAdapter* corba_client_adapter,
        const UIMRefVector& uims,
        const std::vector<std::string>& hosts)
        noexcept;

      virtual CORBACommons::IProcessControl::ALIVE_STATUS is_alive() noexcept;

      virtual char* comment() /*throw(CORBACommons::OutOfMemory)*/;

    private:
      virtual
      ~UserInfoClusterControlImpl() noexcept;

      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      UIMVector uims_;
      std::vector<std::string> hosts_;
    };
    
    typedef ReferenceCounting::SmartPtr<UserInfoClusterControlImpl>
      UserInfoClusterControlImpl_var;

  }
}

#endif //USER_INFO_CLUSTER_CONTROLLER_HPP
