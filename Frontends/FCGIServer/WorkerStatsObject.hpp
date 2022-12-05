#ifndef WORKERSTATSOBJECT_HPP_
#define WORKERSTATSOBJECT_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Time.hpp>
#include <Sync/PosixLock.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Logger/Logger.hpp>

#include <Commons/DelegateActiveObject.hpp>

namespace AdServer
{
namespace Frontends
{
  class WorkerStatsObject: public Commons::DelegateActiveObject
  { 
    static const Generics::Time STATS_TIMEOUT;

  public:
    WorkerStatsObject(
      Logging::Logger* logger,
      Generics::ActiveObjectCallback* callback)
      /*throw(eh::Exception)*/;

    void
    work_() noexcept;

    virtual void
    terminate_() noexcept;

    void
    incr_use_workers();

    void
    dec_use_workers();

    void
    incr_free_workers();

    void
    dec_free_workers();

  protected:
    virtual
    ~WorkerStatsObject() noexcept;

    void
    eval_max_workers_i_();

  private:
    typedef Sync::Policy::PosixThread WorkerStatsSyncPolicy;

    WorkerStatsSyncPolicy::Mutex worker_stats_lock_;
    WorkerStatsSyncPolicy::Mutex worker_cond_lock_;
    Sync::Conditional workers_condition_;

    Logging::Logger_var logger_;

    unsigned long free_workers_count_;
    unsigned long use_workers_count_;

    unsigned long workers_interval_max_;
    unsigned long workers_max_;
  };

  typedef ReferenceCounting::SmartPtr<WorkerStatsObject>
    WorkerStatsObject_var;
}
}

#endif /*WORKERSTATSOBJECT_HPP_*/
