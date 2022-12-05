#ifndef USERINFOSVCS_USERBINDCONTROLLERMAIN_HPP_
#define USERINFOSVCS_USERBINDCONTROLLERMAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/UserInfoSvcs/UserBindControllerConfig.hpp>

#include <UserInfoSvcs/UserBindController/UserBindController.hpp>

#include "UserBindControllerImpl.hpp"

class UserBindControllerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  UserBindControllerApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

protected:
  typedef AdServer::UserInfoSvcs::
    UserBindControllerImpl::UserBindControllerConfig
    ConfigType;

  typedef std::unique_ptr<ConfigType> ConfigPtr;

  const ConfigType& config() const noexcept;

protected:
  virtual
  ~UserBindControllerApp_() noexcept{};

  // IProcessControl
  virtual void shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

  virtual Logging::Logger*
  logger() const noexcept;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::UserInfoSvcs::UserBindControllerImpl_var
    user_bind_controller_impl_;

  ConfigPtr configuration_;
  Logging::Logger_var logger_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<UserBindControllerApp_>
  UserBindControllerApp_var;

typedef Generics::Singleton<
  UserBindControllerApp_, UserBindControllerApp_var>
  UserBindControllerApp;

// Inlines
inline
Logging::Logger*
UserBindControllerApp_::logger() const noexcept
{
  return logger_;
}

inline
const UserBindControllerApp_::ConfigType&
UserBindControllerApp_::config() const noexcept
{
  return *configuration_.get();
}

#endif /*USERINFOSVCS_USERBINDCONTROLLERMAIN_HPP_*/
