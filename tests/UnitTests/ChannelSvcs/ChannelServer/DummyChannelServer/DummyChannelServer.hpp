#ifndef DUMMY_CHANNEL_SERVER_HPP_
#define DUMMY_CHANNEL_SERVER_HPP_

#include <string>
#include <set>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>

namespace AdServer
{
  namespace ChannelSvcs
  {
    typedef AdServer::ChannelSvcs::ChannelUpdate_v33 ChannelCurrent;
  }

  namespace UnitTests
  {
    class DummyChannelServer:
      public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::ChannelSvcs::ChannelServer>,
      protected virtual Generics::ActiveObjectCallback
    {
    private:
      virtual ~DummyChannelServer() noexcept {};

    public:
      DummyChannelServer(
        CORBACommons::OrbShutdowner* shutdowner,
        unsigned long time,
        unsigned long count_matching,
        bool verbose) noexcept;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServer/match:1.0
      //
      virtual void match(
        const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
        ::AdServer::ChannelSvcs::ChannelServer::MatchResult_out result)
        /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServer/match:1.0
      //
      virtual void
        get_ccg_traits(
          const ::AdServer::ChannelSvcs::ChannelIdSeq& query,
          ::AdServer::ChannelSvcs::ChannelServer::TraitsResult_out result)
        /*throw(AdServer::ChannelSvcs::ImplementationException,
              AdServer::ChannelSvcs::NotConfigured)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_sources:1.0
      //
      virtual void set_sources(
        const ::AdServer::ChannelSvcs::
          ChannelServerControl::DBSourceInfo& db_info,
        const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
        noexcept;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_proxy_sources:1.0
      //
      virtual void set_proxy_sources(
        const ::AdServer::ChannelSvcs::ChannelServerControl::ProxySourceInfo&
          poxy_info,
        const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
        noexcept;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServerControl/get_queries_counter:1.0
      //
      ::CORBA::ULong get_queries_counter() noexcept;

      //
      // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
      //
      ::CORBA::ULong get_count_chunks()
        /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_triggers:1.0
      //
      void update_triggers(
        const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
        ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
        /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

      virtual void activate_object()
        /*throw(Generics::ActiveObject::AlreadyActive,
              Generics::ActiveObject::Exception, eh::Exception)*/;

      virtual void deactivate_object()
        /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

      virtual void wait_object()
        /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

      virtual void report_error(
        Generics::ActiveObjectCallback::Severity severity,
        const String::SubString& description,
        const char* error_code = 0) noexcept;

      virtual bool active() /*throw(eh::Exception)*/;

      void finish() noexcept;

    private:
      class FinishTask: public Generics::TaskGoal
      {
      public:
        FinishTask(
          DummyChannelServer* server_impl,
          Generics::TaskRunner* task_runner) noexcept;

        virtual ~FinishTask() noexcept;
        virtual void execute() noexcept;

      private:
        DummyChannelServer* server_impl_;
      };
      typedef ReferenceCounting::SmartPtr<FinishTask> Task_var;

      typedef std::set<unsigned long> TriggerSet;

    private:
      bool active_;
      bool verbose_;
      unsigned long count_matching_;
      CORBACommons::OrbShutdowner_var shutdowner_;
      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var task_runner_;
      TriggerSet response_info_;
    };
    typedef ReferenceCounting::SmartPtr<DummyChannelServer>
      DummyChannelServer_var;
  }
}

namespace AdServer
{
  namespace UnitTests
  {
    inline
    bool DummyChannelServer::active() /*throw(eh::Exception)*/
    {
      return active_;
    }

    inline
    DummyChannelServer::FinishTask::FinishTask(
      DummyChannelServer* server_impl,
      Generics::TaskRunner* task_runner) 
      noexcept
      : Generics::TaskGoal(task_runner), server_impl_(server_impl)
    {
    }

    inline
    DummyChannelServer::FinishTask::~FinishTask() noexcept
    {
    }

    inline
    void DummyChannelServer::FinishTask::execute() noexcept
    {
      server_impl_->finish();
    }
    
  }
}
#endif //DUMMY_CHANNEL_SERVER_HPP_
