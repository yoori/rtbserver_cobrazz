#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_MAIN_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_MAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/RequestInfoSvcs/RequestInfoManagerConfig.hpp>
#include <UServerUtils/Grpc/Manager.hpp>

#include "RequestInfoManager.hpp"
#include "RequestInfoManagerImpl.hpp"


class RequestInfoManagerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl,
    public Generics::CompositeActiveObject
{
public:
  using ManagerCoro = UServerUtils::Grpc::Manager;
  using ManagerCoro_var = UServerUtils::Grpc::Manager_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  RequestInfoManagerApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

protected:
  /** ProcessControl interface */
  virtual void shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

private:
  virtual ~RequestInfoManagerApp_() noexcept{};

  const AdServer::RequestInfoSvcs::RequestInfoManagerImpl::RequestInfoManagerConfig&
    config() const noexcept;

  typedef std::unique_ptr<
    AdServer::RequestInfoSvcs::RequestInfoManagerImpl::RequestInfoManagerConfig>
    ConfigPtr;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::RequestInfoSvcs::RequestInfoManagerImpl_var
    request_info_manager_impl_;

  ConfigPtr configuration_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;

  ManagerCoro_var manager_;
};

typedef ReferenceCounting::SmartPtr<RequestInfoManagerApp_>
  RequestInfoManagerApp_var;

typedef Generics::Singleton<RequestInfoManagerApp_, RequestInfoManagerApp_var>
  RequestInfoManagerApp;


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
const AdServer::RequestInfoSvcs::RequestInfoManagerImpl::RequestInfoManagerConfig&
RequestInfoManagerApp_::config() const noexcept
{
  return *configuration_.get();
}

#endif /*_REQUEST_INFO_SVCS_REQUEST_INFO_MANAGER_MAIN_HPP_*/
