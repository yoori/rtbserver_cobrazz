#ifndef USERINFOSVCS_USERBINDCONTROLLERIMPL_HPP_
#define USERINFOSVCS_USERBINDCONTROLLERIMPL_HPP_

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Sync/SyncPolicy.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserBindControllerConfig.hpp>

#include <Commons/CorbaObject.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServer.hpp>

#include <UserInfoSvcs/UserBindController/UserBindController_s.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  // UserBindControllerImpl
  class UserBindControllerImpl:
    public CORBACommons::ReferenceCounting::CorbaRefCountImpl<
      POA_AdServer::UserInfoSvcs::UserBindController>,
    public Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    typedef xsd::AdServer::Configuration::UserBindControllerConfigType
      UserBindControllerConfig;

    UserBindControllerImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const UserBindControllerConfig& user_bind_controller_config)
      /*throw(Exception)*/;

    virtual AdServer::UserInfoSvcs::UserBindDescriptionSeq*
    get_session_description()
      /*throw(AdServer::UserInfoSvcs::UserBindController::ImplementationException,
        AdServer::UserInfoSvcs::UserBindController::NotReady)*/;

    virtual CORBACommons::IProcessControl::ALIVE_STATUS
    get_status() noexcept;

    virtual char*
    get_comment() /*throw(CORBACommons::OutOfMemory)*/;

  protected:
    typedef Generics::TaskGoal TaskBase;
    typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

    class InitUserBindSourceTask: public TaskBase
    {
    public:
      InitUserBindSourceTask(
        UserBindControllerImpl* user_bind_controller_impl,
        Generics::TaskRunner* task_runner)
        noexcept;

      virtual void
      execute() noexcept;

    protected:
      virtual
      ~InitUserBindSourceTask() noexcept
      {}

    protected:
      UserBindControllerImpl* user_bind_controller_impl_;
    };

    class CheckUserBindServerStateTask: public TaskBase
    {
    public:
      CheckUserBindServerStateTask(
        UserBindControllerImpl* user_info_manager_controller_impl,
        Generics::TaskRunner* task_runner)
        noexcept;

      virtual void
      execute() noexcept;

    protected:
      virtual
      ~CheckUserBindServerStateTask() noexcept
      {}

    protected:
      UserBindControllerImpl* user_bind_controller_impl_;
    };

  protected:
    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef std::set<unsigned long> ChunkIdSet;

    struct UserBindServerRef
    {
      AdServer::Commons::CorbaObject<
        AdServer::UserInfoSvcs::UserBindServer> user_bind_server;
      AdServer::Commons::CorbaObject<
        CORBACommons::IProcessControl> process_control;
      std::string host_name;
      ChunkIdSet chunks;
      bool ready;
      std::size_t grpc_port = 0;
    };

    typedef std::vector<UserBindServerRef> UserBindServerRefArray;

    class UserBindConfig: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      UserBindConfig() noexcept
        : all_ready(false),
          first_all_ready(false),
          common_chunks_number(0)
      {}

      bool all_ready;
      bool first_all_ready;
      unsigned long common_chunks_number;
      UserBindServerRefArray user_bind_servers;

    private:
      virtual
      ~UserBindConfig() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<UserBindConfig>
      UserBindConfig_var;

    typedef Sync::Policy::PosixThreadRW SyncPolicy;

  protected:
    virtual
    ~UserBindControllerImpl() noexcept;

    bool
    get_user_bind_server_sources_(
      UserBindConfig* user_bind_config)
      /*throw(Exception)*/;

    void
    admit_user_info_managers_() /*throw(Exception)*/;

    // task functions implementation
    void
    init_user_bind_state_() noexcept;

    void
    check_user_bind_state_() noexcept;

    void
    fill_refs_() /*throw(Exception)*/;

    void
    check_source_consistency_(
      UserBindConfig* user_bind_config)
      /*throw(Exception)*/;

    /*
    UserInfoClusterControlImpl_var
    get_user_info_cluster_control_() noexcept;
    */

    void
    fill_user_bind_server_descr_seq_(
      AdServer::UserInfoSvcs::UserBindDescriptionSeq& user_bind_server_descr_seq)
      /*throw(AdServer::UserInfoSvcs::UserBindController::ImplementationException,
        AdServer::UserInfoSvcs::UserBindController::NotReady)*/;

  protected:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;

    mutable SyncPolicy::Mutex lock_;

    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;

    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    UserBindControllerConfig user_bind_controller_config_;

    UserBindConfig_var user_bind_config_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindControllerImpl>
    UserBindControllerImpl_var;

  // UserBindClusterControlImpl
  class UserBindClusterControlImpl:
    public CORBACommons::ProcessControlDefault<
      POA_AdServer::UserInfoSvcs::UserBindClusterControl>
  {
  public:
    UserBindClusterControlImpl(
      UserBindControllerImpl* controller)
      noexcept;

    virtual CORBACommons::IProcessControl::ALIVE_STATUS
    is_alive() noexcept;

    virtual char*
    comment() /*throw(CORBACommons::OutOfMemory)*/;

  protected:
    virtual
    ~UserBindClusterControlImpl() noexcept;

    UserBindControllerImpl_var user_bind_controller_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindClusterControlImpl>
    UserBindClusterControlImpl_var;
}
}

namespace AdServer
{
  namespace UserInfoSvcs
  {
    // UserBindControllerImpl::CheckUserBindServerStateTask
    inline
    UserBindControllerImpl::
    CheckUserBindServerStateTask::CheckUserBindServerStateTask(
      UserBindControllerImpl* user_info_manager_controller_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_bind_controller_impl_(
          user_info_manager_controller_impl)
    {}

    inline
    void
    UserBindControllerImpl::
    CheckUserBindServerStateTask::execute()
      noexcept
    {
      user_bind_controller_impl_->check_user_bind_state_();
    }

    // UserBindControllerImpl::InitUserBindSourceTask
    inline
    UserBindControllerImpl::
    InitUserBindSourceTask::InitUserBindSourceTask(
      UserBindControllerImpl* user_info_manager_controller_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_bind_controller_impl_(
          user_info_manager_controller_impl)
    {}

    inline
    void
    UserBindControllerImpl::
    InitUserBindSourceTask::execute()
      noexcept
    {
      user_bind_controller_impl_->init_user_bind_state_();
    }
  }
}

#endif /*USERINFOSVCS_USERBINDCONTROLLERIMPL_HPP_*/
