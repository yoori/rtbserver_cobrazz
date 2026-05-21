#pragma once

#include <eh/Exception.hpp>

#include <memory>

#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/UserInfoSvcs/UserInfoManagerConfig.hpp>

#include "UserInfoManager.hpp"
#include "UserInfoManagerCore.hpp"
#include "UserInfoManagerImpl.hpp"
#include "UserInfoManagerControlImpl.hpp"
#include "UserInfoManagerGrpc.hpp"

class UserInfoManagerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl,
    public virtual Generics::CompositeActiveObject
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  UserInfoManagerApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

protected:
  //
  // IDL:CORBACommons/IProcessControl/shutdown:1.0
  //
  virtual void shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  //
  // IDL:CORBACommons/IProcessControl/is_alive:1.0
  //
  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

  virtual bool is_ready_() noexcept;

  virtual char* comment()
    /*throw(CORBACommons::OutOfMemory)*/;
private:
  virtual ~UserInfoManagerApp_() noexcept{};

  const AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfoManagerConfig&
    config() const noexcept;

  typedef std::unique_ptr<
    AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfoManagerConfig>
    ConfigPtr;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::UserInfoSvcs::UserInfoManagerCorePtr user_info_manager_core_;
  AdServer::UserInfoSvcs::UserInfoManagerImpl_var
    user_info_manager_impl_;
  AdServer::UserInfoSvcs::UserInfoManagerControlImpl_var
    user_info_manager_control_impl_;
  AdServer::UserInfoSvcs::UserInfoManagerGrpc_var grpc_adapter_;

  ConfigPtr configuration_;
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<UserInfoManagerApp_>
  UserInfoManagerApp_var;

typedef Generics::Singleton<UserInfoManagerApp_, UserInfoManagerApp_var>
  UserInfoManagerApp;


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
const AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfoManagerConfig&
UserInfoManagerApp_::config() const noexcept
{
  return *configuration_.get();
}
