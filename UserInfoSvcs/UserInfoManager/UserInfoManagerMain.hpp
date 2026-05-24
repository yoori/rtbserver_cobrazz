#pragma once

#include <eh/Exception.hpp>

#include <memory>

#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/HttpServer/HttpServer.hpp>

#include <xsd/UserInfoSvcs/UserInfoManagerConfig.hpp>

#include "UserInfoManagerCore.hpp"
#include "UserInfoManagerGrpc.hpp"

class UserInfoManagerApp_
  : private Logging::LoggerCallbackHolder,
    public virtual Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  UserInfoManagerApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

private:
  virtual ~UserInfoManagerApp_() noexcept{};

  const AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfoManagerConfig&
    config() const noexcept;

  typedef std::unique_ptr<
    AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfoManagerConfig>
    ConfigPtr;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  AdServer::UserInfoSvcs::UserInfoManagerCorePtr user_info_manager_core_;
  AdServer::UserInfoSvcs::UserInfoManagerGrpc_var grpc_adapter_;
  AdServer::Commons::HttpServer::HttpServer_var http_server_;

  ConfigPtr configuration_;
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
