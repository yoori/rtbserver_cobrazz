#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_IMPL_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_IMPL_HPP_

#include <list>
#include <map>
#include <set>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/CRC.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserInfoManagerControllerConfig.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/StatsImpl.hpp>

#include <Commons/CorbaObject.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController_s.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoClusterControl_s.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerControl.hpp>

#include "UserInfoClusterControlImpl.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /**
     * Implementation of UserInfoManagerController.
     */
    class UserInfoManagerControllerImpl :
      public CORBACommons::ReferenceCounting::CorbaRefCountImpl<
        POA_AdServer::UserInfoSvcs::UserInfoManagerController>,
      public Generics::CompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(NotReady, Exception);

      typedef xsd::AdServer::Configuration::UserInfoManagerControllerConfigType
        UserInfoManagerControllerConfig;

      UserInfoManagerControllerImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        const UserInfoManagerControllerConfig&
          user_info_manager_controller_config)
        /*throw(Exception)*/;

      /* UserInfoManagerController interface */
      virtual
      AdServer::UserInfoSvcs::UserInfoManagerSession* get_session()
        /*throw(AdServer::UserInfoSvcs::UserInfoManagerController::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManagerController::NotReady)*/;

      virtual
      void get_session_description(
        AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq_out session_description)
        /*throw(AdServer::UserInfoSvcs::UserInfoManagerController::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManagerController::NotReady)*/;
    public:
      struct TraceLevel
      {
        enum
        {
          LOW = Logging::Logger::TRACE,
          MIDDLE,
          HIGH
        };
      };

      virtual
      CORBACommons::IProcessControl::ALIVE_STATUS
      get_status() noexcept;

      virtual
      CORBACommons::StatsValueSeq*
      get_stats() noexcept;

      virtual
      char* get_comment() /*throw(CORBACommons::OutOfMemory)*/;

    protected:
      typedef Generics::TaskGoal TaskBase;
      typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

      class InitUserInfoManagerSourcesTask : public TaskBase
      {
      public:
        InitUserInfoManagerSourcesTask(
          UserInfoManagerControllerImpl* user_info_manager_controller_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~InitUserInfoManagerSourcesTask() noexcept
        {
        }

        UserInfoManagerControllerImpl* user_info_manager_controller_impl_;
      };

      class CheckUserInfoManagerStatesTask : public TaskBase
      {
      public:
        CheckUserInfoManagerStatesTask(
          UserInfoManagerControllerImpl* user_info_manager_controller_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        virtual
        ~CheckUserInfoManagerStatesTask() noexcept
        {
        }

        UserInfoManagerControllerImpl* user_info_manager_controller_impl_;
      };

    protected:
      typedef std::set<unsigned long> ChunkIdSet;

      struct UserInfoManagerRef
      {
        typedef AdServer::Commons::CorbaObject<
          AdServer::UserInfoSvcs::UserInfoManager> UIMCORBARef;
        typedef AdServer::Commons::CorbaObject<
          AdServer::UserInfoSvcs::UserInfoManagerControl> UIMControlCORBARef;
        typedef AdServer::Commons::CorbaObject<
          CORBACommons::ProcessStatsControl> ProcessControlCORBARef;

        UIMCORBARef user_info_manager;
        UIMControlCORBARef user_info_manager_control;
        std::string user_info_manager_host_name;
        ProcessControlCORBARef uim_stats;
        ChunkIdSet chunks;
        unsigned long common_chunks_number;
        bool ready;
      };

      typedef std::vector<UserInfoManagerRef> UserInfoManagerRefVector;

      class UserInfoManagersConfig: public ReferenceCounting::AtomicCopyImpl
      {
      public:
        UserInfoManagersConfig() noexcept
          : all_ready(false),
            first_all_ready(false)
        {
        }

        bool all_ready;
        bool first_all_ready;
        UserInfoManagerRefVector user_info_managers;
      private:
        virtual
        ~UserInfoManagersConfig() noexcept
        {}
      };

      typedef
        ReferenceCounting::SmartPtr<UserInfoManagersConfig>
        UserInfoManagersConfig_var;

    protected:
      virtual
      ~UserInfoManagerControllerImpl() noexcept;

      void fill_user_info_manager_refs_() /*throw(Exception)*/;

      bool get_user_info_manager_sources_(
        UserInfoManagersConfig* user_info_managers_config)
        /*throw(Exception)*/;

      void admit_user_info_managers_() /*throw(Exception)*/;

      /* task functions implementation */
      void init_user_info_manager_sources_() noexcept;

      void check_user_info_manager_states_() noexcept;

      void check_source_consistency_(
        UserInfoManagersConfig* user_info_managers_config)
        /*throw(Exception)*/;

      UserInfoClusterControlImpl_var
      get_user_info_cluster_control_() noexcept;

      void fill_user_info_manager_descr_seq_(
        AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq& user_info_manager_descr_seq)
        /*throw(AdServer::UserInfoSvcs::UserInfoManagerController::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManagerController::NotReady)*/;

    protected:
      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;

      typedef Sync::PosixRWLock Mutex_;
      typedef Sync::PosixRGuard ReadGuard_;
      typedef Sync::PosixWGuard WriteGuard_;

      mutable Mutex_ lock_;

      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var task_runner_;

      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;

      UserInfoManagersConfig_var user_info_managers_config_;

      UserInfoManagerControllerConfig
        user_info_manager_controller_config_;

      bool inited_;
    };

    typedef
      ReferenceCounting::SmartPtr<UserInfoManagerControllerImpl>
    UserInfoManagerControllerImpl_var;

    /* UserInfoClusterImpl */
    class UserInfoClusterImpl :
      public CORBACommons::ProcessControlDefault<
        POA_AdServer::UserInfoSvcs::UserInfoClusterControl>
    {
    public:
      UserInfoClusterImpl(
        UserInfoManagerControllerImpl* controller)
        noexcept;


      virtual CORBACommons::IProcessControl::ALIVE_STATUS is_alive() noexcept;

      virtual char* comment() /*throw(CORBACommons::OutOfMemory)*/;

    protected:
      virtual
      ~UserInfoClusterImpl() noexcept;

      UserInfoManagerControllerImpl_var uimc_;
    };

  typedef ReferenceCounting::SmartPtr<UserInfoClusterImpl>
      UserInfoClusterImpl_var;

    /* UserInfoClusterStatsImpl */
    class UserInfoClusterStatsImpl:
      public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::UserInfoSvcs::UserInfoClusterStats>
    {
    public:
      UserInfoClusterStatsImpl(
        UserInfoManagerControllerImpl* controller)
        noexcept;

      virtual CORBACommons::StatsValueSeq* get_stats() noexcept;

    protected:
      virtual
      ~UserInfoClusterStatsImpl() noexcept;

      UserInfoManagerControllerImpl_var uimc_;
    };

  typedef ReferenceCounting::SmartPtr<UserInfoClusterStatsImpl>
      UserInfoClusterStatsImpl_var;

  } /* UserInfoSvcs */
} /* AdServer */

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /* UserInfoManagerControllerImpl inlines */

    inline
    UserInfoManagerControllerImpl::
    CheckUserInfoManagerStatesTask::CheckUserInfoManagerStatesTask(
      UserInfoManagerControllerImpl* user_info_manager_controller_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_controller_impl_(
          user_info_manager_controller_impl)
    {
    }

    inline
    void
    UserInfoManagerControllerImpl::
    CheckUserInfoManagerStatesTask::execute()
      noexcept
    {
      user_info_manager_controller_impl_->check_user_info_manager_states_();
    }

    inline
    UserInfoManagerControllerImpl::
    InitUserInfoManagerSourcesTask::InitUserInfoManagerSourcesTask(
      UserInfoManagerControllerImpl* user_info_manager_controller_impl,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        user_info_manager_controller_impl_(
          user_info_manager_controller_impl)
    {
    }

    inline
    void
    UserInfoManagerControllerImpl::
    InitUserInfoManagerSourcesTask::execute()
      noexcept
    {
      user_info_manager_controller_impl_->init_user_info_manager_sources_();
    }

  }
}

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_IMPL_HPP_*/
