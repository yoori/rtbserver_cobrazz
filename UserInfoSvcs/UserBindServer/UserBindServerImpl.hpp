#ifndef USERINFOSVCS_USERBINDSERVERIMPL_HPP
#define USERINFOSVCS_USERBINDSERVERIMPL_HPP

#include <list>
#include <vector>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/ServantImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/AccessActiveObject.hpp>
#include <Commons/UserIdBlackList.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserBindServerConfig.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServer_s.hpp>

#include "UserBindContainer.hpp"
#include "BindRequestContainer.hpp"
#include "UserBindServer_service.cobrazz.pb.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  /**
   * Implementation of UserBindServer.
   */
  class UserBindServerImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl<
      POA_AdServer::UserInfoSvcs::UserBindServer>,
    public virtual Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    using UserBindServerConfig = xsd::AdServer::Configuration::UserBindServerConfigType;

    using GetBindRequestPtr = std::unique_ptr<GetBindRequest>;
    using GetBindResponsePtr = std::unique_ptr<GetBindResponse>;
    using AddBindRequestPtr = std::unique_ptr<AddBindRequest>;
    using AddBindResponsePtr = std::unique_ptr<AddBindResponse>;
    using GetUserIdRequestPtr = std::unique_ptr<GetUserIdRequest>;
    using GetUserIdResponsePtr = std::unique_ptr<GetUserIdResponse>;
    using AddUserIdRequestPtr = std::unique_ptr<AddUserIdRequest>;
    using AddUserIdResponsePtr = std::unique_ptr<AddUserIdResponse>;
    using GetSourceRequestPtr = std::unique_ptr<GetSourceRequest>;
    using GetSourceResponsePtr = std::unique_ptr<GetSourceResponse>;

  public:
    UserBindServerImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const UserBindServerConfig& user_bind_server_config)
      /*throw(Exception)*/;

    virtual
    AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
    get_bind_request(
      const char* id,
      const CORBACommons::TimestampInfo& timestamp)
      /*throw(NotReady, ChunkNotFound)*/;

    GetBindResponsePtr
    get_bind_request(GetBindRequestPtr&& get_bind_request);

    virtual
    void
    add_bind_request(
      const char* id,
      const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request,
      const CORBACommons::TimestampInfo& timestamp)
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    AddBindResponsePtr
    add_bind_request(AddBindRequestPtr&& request);

    virtual
    AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
    get_user_id(
      const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
        request_info)
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    GetUserIdResponsePtr
    get_user_id(GetUserIdRequestPtr&& request);

    virtual
    AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
    add_user_id(
      const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
        request_info)
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
        AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/;

    AddUserIdResponsePtr
    add_user_id(AddUserIdRequestPtr&& request);

    virtual
    AdServer::UserInfoSvcs::UserBindServer::Source*
    get_source()
      /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady)*/;

    GetSourceResponsePtr
    get_source(GetSourceRequestPtr&& request);

    Logging::Logger*
    logger() noexcept;

    virtual void
    wait_object()
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

  protected:
    typedef Sync::Policy::PosixThreadRW SyncPolicy;

    typedef AdServer::Commons::AccessActiveObject<
      UserBindProcessor_var>
      UserBindProcessorHolder;

    typedef ReferenceCounting::SmartPtr<UserBindProcessorHolder>
      UserBindProcessorHolder_var;

    typedef AdServer::Commons::AccessActiveObject<
      BindRequestProcessor_var>
      BindRequestProcessorHolder;

    typedef ReferenceCounting::SmartPtr<BindRequestProcessorHolder>
      BindRequestProcessorHolder_var;

    class LoadUserBindTask: public Generics::TaskGoal
    {
    public:
      LoadUserBindTask(
        Generics::TaskRunner* task_runner,
        UserBindServerImpl* user_bind_server_impl)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserBindServerImpl* user_bind_server_impl_;
      bool reschedule_;
    };

    class LoadBindRequestTask: public Generics::TaskGoal
    {
    public:
      LoadBindRequestTask(
        Generics::TaskRunner* task_runner,
        UserBindServerImpl* user_bind_server_impl)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserBindServerImpl* user_bind_server_impl_;
      bool reschedule_;
    };

    class ClearUserBindExpiredTask: public Generics::TaskGoal
    {
    public:
      ClearUserBindExpiredTask(
        Generics::TaskRunner* task_runner,
        UserBindServerImpl* user_bind_server_impl,
        bool reschedule)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserBindServerImpl* user_bind_server_impl_;
      bool reschedule_;
    };

    class ClearBindRequestExpiredTask: public Generics::TaskGoal
    {
    public:
      ClearBindRequestExpiredTask(
        Generics::TaskRunner* task_runner,
        UserBindServerImpl* user_bind_server_impl,
        bool reschedule)
        noexcept;

      virtual void execute() noexcept;

    protected:
      UserBindServerImpl* user_bind_server_impl_;
      bool reschedule_;
    };

    class DumpUserBindTask: public Generics::TaskGoal
    {
    public:
      DumpUserBindTask(
        Generics::TaskRunner* task_runner,
        UserBindServerImpl* user_bind_server_impl)
        noexcept;

      virtual void
      execute() noexcept;

    protected:
      UserBindServerImpl* user_bind_server_impl_;
    };

  protected:
    virtual
    ~UserBindServerImpl() noexcept {};

    void
    load_user_bind_() noexcept;

    void
    load_bind_request_() noexcept;

    void
    clear_user_bind_expired_(bool reschedule) noexcept;

    void
    clear_bind_request_expired_(bool reschedule) noexcept;

    void
    dump_user_bind_() noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;

    const UserBindServerConfig user_bind_server_config_;
    UserBindContainer::ChunkPathMap chunks_;

    UserBindProcessorHolder_var user_bind_container_;
    BindRequestProcessorHolder_var bind_request_container_;

    Commons::UserIdBlackList user_id_black_list_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindServerImpl>
    UserBindServerImpl_var;

} /* UserInfoSvcs */
} /* AdServer */

namespace AdServer
{
namespace UserInfoSvcs
{
  inline
  Logging::Logger*
  UserBindServerImpl::logger() noexcept
  {
    return logger_;
  }
}
}

#endif /*USERINFOSVCS_USERBINDSERVERIMPL_HPP*/
