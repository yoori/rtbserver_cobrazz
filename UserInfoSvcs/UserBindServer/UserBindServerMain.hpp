#pragma once

#include <mutex>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/UserInfoSvcs/UserBindServerConfig.hpp>

#include "UserBindServerImpl.hpp"

#include "UserBindServerGrpc.hpp"

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
  virtual ~UserBindServerApp_() noexcept;

  const AdServer::UserInfoSvcs::UserBindServerImpl::UserBindServerConfig&
  config() const noexcept;

  typedef std::unique_ptr<
    AdServer::UserInfoSvcs::UserBindServerImpl::UserBindServerConfig>
    ConfigPtr;

private:
  ConfigPtr configuration_;

  AdServer::UserInfoSvcs::UserBindServerImpl_var user_bind_server_impl_; // CORBA impl
  AdServer::UserInfoSvcs::UserBindServerGrpc_var grpc_adapter_; // GRPC impl
  AdServer::Commons::HttpServer::HttpServer_var http_server_;

  std::mutex shutdown_lock_;
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
