#ifndef USERINFOSVCS_USERBINDSERVERMAIN_HPP
#define USERINFOSVCS_USERBINDSERVERMAIN_HPP

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/UserInfoSvcs/UserBindServerConfig.hpp>

#include "UserBindServerImpl.hpp"
//#include <UServerUtils/MetricsHTTPProvider.hpp>
//#include <Generics/CompositeMetricsProvider.hpp>


class UserBindServerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl,
    public virtual Generics::CompositeActiveObject
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  UserBindServerApp_() /*throw(eh::Exception)*/;

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
  virtual ~UserBindServerApp_() noexcept{};

  const AdServer::UserInfoSvcs::UserBindServerImpl::UserBindServerConfig&
    config() const noexcept;
  
  typedef std::unique_ptr<
    AdServer::UserInfoSvcs::UserBindServerImpl::UserBindServerConfig>
    ConfigPtr;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::UserInfoSvcs::UserBindServerImpl_var
    user_bind_server_impl_;
  
  ConfigPtr configuration_;
  
  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
  //Generics::CompositeMetricsProvider_var composite_metrics_provider_;
};

typedef ReferenceCounting::SmartPtr<UserBindServerApp_>
  UserBindServerApp_var;

typedef Generics::Singleton<UserBindServerApp_, UserBindServerApp_var>
  UserBindServerApp;

// Inlines
inline
const AdServer::UserInfoSvcs::UserBindServerImpl::UserBindServerConfig&
UserBindServerApp_::config() const noexcept
{
  return *configuration_.get();
}

#endif /*USERINFOSVCS_USERBINDSERVERMAIN_HPP*/
