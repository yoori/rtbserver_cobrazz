#ifndef _USER_INFO_SVCS_USER_INFO_EXCHANGER_IMPL_HPP_
#define _USER_INFO_SVCS_USER_INFO_EXCHANGER_IMPL_HPP_

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/UserInfoSvcs/UserInfoExchangerConfig.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <UserInfoSvcs/UserInfoExchanger/UserInfoExchanger_s.hpp>

#include "UserInfoExchangePool.hpp"

namespace
{
  const Generics::Time DEFAULT_EXPIRE_TIME(4*60*60); // 4 hours
}

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /**
     * Implementation of UserInfoExchanger.
     */
    class UserInfoExchangerImpl :
      public virtual POA_AdServer::UserInfoSvcs::UserInfoExchanger,
      public virtual Generics::CompositeActiveObject,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(NotReady, Exception);
      
      typedef xsd::AdServer::Configuration::UserInfoExchangerConfigType
        UserInfoExchangerConfig;

      UserInfoExchangerImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        const UserInfoExchangerConfig& user_info_exchanger_config)
        /*throw(Exception)*/;

      virtual ~UserInfoExchangerImpl() noexcept;

      /* UserInfoExchanger interface */
      virtual void 
      register_users_request(
        const char* customer_id,
        const AdServer::UserInfoSvcs::ColoUsersRequestSeq& users)
        /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/;

      virtual void
      receive_users(
        const char* customer_id, 
        AdServer::UserInfoSvcs::UserProfileSeq_out user_profiles,
        const AdServer::UserInfoSvcs::ReceiveCriteria& receive_criteria)
        /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/;

      virtual void 
      get_users_requests(
        const char* customer_id,
        AdServer::UserInfoSvcs::UserIdSeq_out users)
        /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/;

      virtual void
      send_users(
        const char* customer_id,
        const AdServer::UserInfoSvcs::UserProfileSeq& user_profiles)
        /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/;

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

    protected:
      typedef Generics::TaskGoal TaskBase;
      typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;

      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      UserInfoExchangerConfig user_info_exchanger_config_;
      UserInfoExchangePool_var user_info_exchange_pool_;

      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var task_runner_;

      bool inited_;

      Generics::Time expire_time_;

    protected:
      class EraseOldProfilesTask : public TaskBase
      {
      public:
        EraseOldProfilesTask(
          UserInfoExchangerImpl* user_info_exchanger_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        UserInfoExchangerImpl* user_info_exchanger_impl_;
      };
    protected:
      void erase_old_profiles_() noexcept;
    };

    typedef
      ReferenceCounting::SmartPtr<UserInfoExchangerImpl>
      UserInfoExchangerImpl_var;

  } /* UserInfoSvcs */
} /* AdServer */

namespace AdServer
{
  namespace UserInfoSvcs
  {
    inline
    UserInfoExchangerImpl::EraseOldProfilesTask::EraseOldProfilesTask(
      UserInfoExchangerImpl* user_info_exchanger_impl,
      Generics::TaskRunner* task_runner)
      noexcept
        : TaskBase(task_runner),
          user_info_exchanger_impl_(user_info_exchanger_impl)
    {
    }

    inline
    void
    UserInfoExchangerImpl::EraseOldProfilesTask::execute()
      noexcept
    {
      user_info_exchanger_impl_->erase_old_profiles_();
    }
  }
}

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_CONTROLLER_IMPL_HPP_*/
