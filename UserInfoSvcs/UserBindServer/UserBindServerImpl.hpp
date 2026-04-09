#pragma once

#include <list>
#include <vector>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/ServantImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/AccessActiveObject.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserBindServerConfig.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServer_s.hpp>

#include "UserBindContainer.hpp"
#include "BindRequestContainer.hpp"
#include "UserBindServerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  /**
   * Implementation of UserBindServer.
   */
  class UserBindServerImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl<
      POA_AdServer::UserInfoSvcs::UserBindServer>
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using UserBindServerConfig =
      xsd::AdServer::Configuration::UserBindServerConfigType;

  public:
    UserBindServerImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      UserBindServerCore* core)
      /*throw(Exception)*/;

    AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
    get_bind_request(
      const char* id,
      const CORBACommons::TimestampInfo& timestamp) override
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    void
    add_bind_request(
      const char* id,
      const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request,
      const CORBACommons::TimestampInfo& timestamp) override
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
    get_user_id(
      const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
        request_info) override
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
    add_user_id(
      const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
        request_info) override
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    AdServer::UserInfoSvcs::UserBindServer::Source*
    get_source() override
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady)*/;

    Logging::Logger*
    logger() noexcept;

    UserBindServerCore*
    core() noexcept;

  protected:
    virtual
    ~UserBindServerImpl() noexcept;

  private:
    const Generics::ActiveObjectCallback_var callback_;
    const Logging::Logger_var logger_;
    const UserBindServerCore_var core_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindServerImpl>
    UserBindServerImpl_var;
}

namespace AdServer::UserInfoSvcs
{
  inline
  Logging::Logger*
  UserBindServerImpl::logger() noexcept
  {
    return logger_;
  }

  inline
  UserBindServerCore*
  UserBindServerImpl::core() noexcept
  {
    return core_;
  }
}
