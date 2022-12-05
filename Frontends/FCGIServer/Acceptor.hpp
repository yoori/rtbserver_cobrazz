#pragma once

#include <list>
#include <pthread.h>
#include <Sync/PosixLock.hpp>
#include <Sync/Condition.hpp>
#include <Generics/Time.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

#include "WorkerStatsObject.hpp"

namespace AdServer
{
namespace Frontends
{
  class Acceptor:
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Acceptor(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      Generics::ActiveObjectCallback* callback,
      const char* bind_address,
      unsigned long backlog,
      unsigned long accept_threads)
      /*throw(eh::Exception)*/;

    FrontendCommons::FrontendInterface*
    handler() noexcept;

    Logging::Logger*
    logger() noexcept;

    virtual void
    activate_object()
      /*throw(Exception, eh::Exception)*/;

    virtual void
    deactivate_object()
      /*throw(Exception, eh::Exception)*/;

    virtual void
    wait_object()
      /*throw(Exception, eh::Exception)*/;

  protected:
    struct State;
    typedef ReferenceCounting::SmartPtr<State> State_var;

    class Worker;
    typedef ReferenceCounting::SmartPtr<Worker> Worker_var;

    class AcceptActiveObject;

    class HttpResponseWriterImpl;

    typedef Sync::Policy::PosixThread WorkersSyncPolicy;
    typedef std::deque<Worker_var> WorkerArray;

  protected:
    virtual
    ~Acceptor() noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    FrontendCommons::Frontend_var frontend_;

    WorkerStatsObject_var worker_stats_object_;
    State_var state_;

    Generics::AtomicInt shutdown_uniq_;
  };
}
}
