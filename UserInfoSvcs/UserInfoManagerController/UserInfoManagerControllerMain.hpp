#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_MAIN_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_MAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/UserInfoSvcs/UserInfoManagerControllerConfig.hpp>

#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

#include "UserInfoManagerControllerImpl.hpp"

class UserInfoManagerControllerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  UserInfoManagerControllerApp_() /*throw(eh::Exception)*/;

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

private:
  virtual ~UserInfoManagerControllerApp_() noexcept{};

  virtual Logging::Logger*
  logger() const noexcept;

  typedef
    AdServer::UserInfoSvcs::
    UserInfoManagerControllerImpl::UserInfoManagerControllerConfig
    ConfigType;

  typedef std::unique_ptr<ConfigType> ConfigPtr;

protected:
  const ConfigType& config() const noexcept;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::UserInfoSvcs::UserInfoManagerControllerImpl_var
    user_info_manager_controller_impl_;

  ConfigPtr configuration_;
  Logging::Logger_var logger_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<UserInfoManagerControllerApp_>
  UserInfoManagerControllerApp_var;

typedef Generics::Singleton<
  UserInfoManagerControllerApp_, UserInfoManagerControllerApp_var>
  UserInfoManagerControllerApp;


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
Logging::Logger*
UserInfoManagerControllerApp_::logger() const noexcept
{
  return logger_;
}

inline
const UserInfoManagerControllerApp_::ConfigType&
UserInfoManagerControllerApp_::config() const noexcept
{
  return *configuration_.get();
}

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_MAIN_HPP_*/
