#ifndef AD_SERVER_CHANNEL_PROXY_IMPL_HPP_
#define AD_SERVER_CHANNEL_PROXY_IMPL_HPP_

#include <set>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelUpdateStatLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionImpl.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelServerSessionFactory.hpp>
#include <ChannelSvcs/ChannelProxy/ChannelProxy_s.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/ChannelSvcs/ChannelProxyConfig.hpp>

namespace AdServer
{
  namespace ChannelSvcs
  {
    /**
     * Implementation of common part ChannelProxy
     */
    class ChannelProxyImpl:
      public virtual Generics::ActiveObject,
      public virtual CORBACommons::ReferenceCounting::ServantImpl
       <POA_AdServer::ChannelSvcs::ChannelProxy_v33>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef xsd::AdServer::Configuration::ChannelProxyConfigType
        ChannelProxyConfig;

      //typedef
      //  CORBACommons::ObjectPoolRefConfiguration LoadSessionPoolConfig;
      struct LoadSessionPoolConfig:
        public CORBACommons::ObjectPoolRefConfiguration
      {
        LoadSessionPoolConfig(
          const CORBACommons::CorbaClientAdapter* corba_client_adapter,
          ChannelLoadSessionFactoryImpl* factory,
          bool proxy) noexcept :
          ObjectPoolRefConfiguration(corba_client_adapter),
          resolver(corba_client_adapter, factory, proxy)
        {};

        struct Resolver
        {
          Resolver(
            const CORBACommons::CorbaClientAdapter* corba_client_adapter,
            ChannelLoadSessionFactoryImpl* factory,
            bool proxy)
            noexcept;

          ~Resolver() noexcept;

          template <typename PoolType>
          PoolType*
          resolve(const ObjectRef& ref)
            /*throw(eh::Exception, CORBACommons::CorbaClientAdapter::Exception)*/;

        private:
          CORBACommons::CorbaClientAdapter_var c_adapter_;
          ChannelLoadSessionFactoryImpl_var load_session_factory_;
          bool proxy_mode_;
        };

        Resolver resolver;
      };


      typedef
        CORBACommons::ObjectPool<
          AdServer::ChannelSvcs::ChannelUpdateBase_v33,
          LoadSessionPoolConfig>
        LoadSessionPool;

      typedef std::unique_ptr<LoadSessionPool> LoadSessionPoolPtr;

      ChannelProxyImpl(
        Logging::Logger* logger,
        const ChannelProxyConfig* config)
        /*throw(Exception, eh::Exception)*/;

      virtual ~ChannelProxyImpl() noexcept;

    public:
      //
      // IDL:AdServer/ChannelSvcs/ChannelUpdate_v33/check:1.0
      //
      virtual void check(
        const ::AdServer::ChannelSvcs::ChannelUpdate_v33::CheckQuery& query,
        ::AdServer::ChannelSvcs::ChannelUpdate_v33::CheckData_out data) 
        /*throw(AdServer::ChannelSvcs::ImplementationException,
          AdServer::ChannelSvcs::NotConfigured)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServerUpdate/update_triggers:1.0
      //
      virtual void update_triggers(
        const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
        ::AdServer::ChannelSvcs::ChannelUpdate_v33::UpdateData_out result)
        /*throw(
          AdServer::ChannelSvcs::ImplementationException,
          AdServer::ChannelSvcs::NotConfigured)*/;

      virtual void update_all_ccg(
        const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& in,
        AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
        /*throw(AdServer::ChannelSvcs::ImplementationException,
          AdServer::ChannelSvcs::NotConfigured)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
      //
      virtual ::CORBA::ULong get_count_chunks()
        /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

    public:
      /** Generics::ActiveObject interface */
      virtual void activate_object()
        /*throw(ActiveObject::AlreadyActive,
          ActiveObject::Exception, eh::Exception)*/;

      virtual void deactivate_object()
        /*throw(ActiveObject::Exception, eh::Exception)*/;

      virtual void wait_object() /*throw(ActiveObject::Exception, eh::Exception)*/;

      virtual bool active() /*throw(eh::Exception)*/;

      Logging::Logger* logger() const noexcept;

    protected:
      typedef Generics::TaskGoal TaskBase;
      typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

      class InitTask: public TaskBase
      {
      public:

        InitTask(ChannelProxyImpl* impl,
          Generics::TaskRunner* task_runner) noexcept;
        virtual ~InitTask() noexcept;
        virtual void execute() noexcept;

      private:
        ChannelProxyImpl* proxy_impl_;
      };

      class LogTask: public TaskBase
      {
        public:
          LogTask(ChannelProxyImpl* proxy,
            Generics::TaskRunner* task_runner) noexcept;
          virtual ~LogTask() noexcept;
          virtual void execute() noexcept;

        private:
          ChannelProxyImpl* proxy_impl_;
      };

      void init_() /*throw(Exception)*/;

      void dump_logs_() noexcept;

      void init_corba_refs_(const ChannelProxyConfig* config)
        /*throw(eh::Exception, Exception)*/;

      void init_logger_(const ChannelProxyConfig* config) noexcept;

      void log_update_(unsigned long colo, const char* version) noexcept;

    protected:
      typedef Sync::PosixRWLock Mutex_;
      typedef Sync::PosixRGuard ReadGuard_;
      typedef Sync::PosixWGuard WriteGuard_;

    private:
      mutable Mutex_ lock_;
      AdServer::ChannelSvcs::ChannelLoadSession_var load_session_;
      LoadSessionPoolPtr load_pool_;
      CORBACommons::CorbaObjectRefList corba_object_refs_;

      bool active_;
      bool proxy_;

      Logging::ActiveObjectCallbackImpl_var callback_;
      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var task_runner_;

      /* common params for ChannelProxy's */
      unsigned long update_period_;
      unsigned long count_chunks_;

      CORBACommons::CorbaClientAdapter_var c_adapter_;
      ChannelUpdateStatLogger_var proxy_logger_;
      ChannelLoadSessionFactoryImpl_var load_session_factory_;
    };

    typedef ReferenceCounting::SmartPtr<ChannelProxyImpl>
        ChannelProxyImpl_var;

  } /* ChannelSvcs */
} /* AdServer */

namespace AdServer
{
  namespace ChannelSvcs
  {
    /* ChannelProxyImpl */
    inline
    bool
    ChannelProxyImpl::active() /*throw(eh::Exception)*/
    {
      ReadGuard_ guard(lock_);
      return active_;
    }

    inline
    Logging::Logger*
    ChannelProxyImpl::logger() const noexcept
    {
      return callback_->logger();
    }

    inline
    ChannelProxyImpl::InitTask::InitTask(
      ChannelProxyImpl* impl, Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner), proxy_impl_(impl)
    {
    }

    inline
    ChannelProxyImpl::InitTask::~InitTask() noexcept
    {
    }

    inline void
    ChannelProxyImpl::InitTask::execute() noexcept
    {
      proxy_impl_->init_();
    }

    inline
    ChannelProxyImpl::LogTask::LogTask(
      ChannelProxyImpl *proxy,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner), proxy_impl_(proxy)
    {
    }

    inline
    ChannelProxyImpl::LogTask::~LogTask() noexcept
    {
    }

    inline void
    ChannelProxyImpl::LogTask::execute() noexcept
    {
      proxy_impl_->dump_logs_();
    }

    inline
    ChannelProxyImpl::LoadSessionPoolConfig::Resolver::Resolver(
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      ChannelLoadSessionFactoryImpl* factory,
      bool proxy) noexcept:
      c_adapter_(ReferenceCounting::add_ref(corba_client_adapter)),
      load_session_factory_(factory? ReferenceCounting::add_ref(factory): 0),
      proxy_mode_(proxy)
    {
    }

    inline
    ChannelProxyImpl::LoadSessionPoolConfig::Resolver::~Resolver() noexcept
    {
    }

    template <typename PoolType>
    PoolType*
    ChannelProxyImpl::LoadSessionPoolConfig::Resolver::resolve(
      const ObjectRef& ref)
      /*throw(eh::Exception, CORBACommons::CorbaClientAdapter::Exception)*/
    {
      try
      {
        CORBA::Object_var obj = c_adapter_->resolve_object(ref);
        if(CORBA::is_nil(obj.in()))
        {
          return 0;
        }
        AdServer::ChannelSvcs::ChannelUpdateBase_v33_var update;
        if(proxy_mode_)
        {
          update = AdServer::ChannelSvcs::ChannelUpdate_v33::_narrow(
            obj.in());
          if(CORBA::is_nil(update.in()))
          {
            return 0;
          }
          return update._retn();
        }
        else
        {
          AdServer::ChannelSvcs::ChannelManagerController_var manager =
            AdServer::ChannelSvcs::ChannelManagerController::_narrow(obj.in());
          if(CORBA::is_nil(manager.in()))
          {
            return 0;
          }

          AdServer::ChannelSvcs::ChannelLoadSession_var load_session =
            manager->get_load_session();

          if(CORBA::is_nil(load_session.in()))
          {
            return 0;
          }

          update = AdServer::ChannelSvcs::ChannelLoadSession::_narrow(
            load_session.in());

          if(CORBA::is_nil(update.in()))
          {
            return 0;
          }
          return update._retn();
        }
      }
      catch(...)
      {
        return 0;
      }
    }

  } /* ChannelSvcs */
} /* AdServer */


#endif /*AD_SERVER_CHANNEL_PROXY_IMPL_HPP_*/
