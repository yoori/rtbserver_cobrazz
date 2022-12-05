#ifndef SCHEDULER_POOL_HPP_
#define SCHEDULER_POOL_HPP_

#include <vector>

#include <Generics/Time.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>

namespace AdServer
{
namespace Bidding
{
  class PlannerPool:
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    PlannerPool(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads,
      size_t stack_size = 0);

    void
    schedule(Generics::Goal* goal, const Generics::Time& time);

  protected:
    typedef std::vector<Generics::Planner_var> PlannerArray;

  protected:
    virtual
    ~PlannerPool() noexcept = default;

  protected:
    Generics::AtomicUInt planner_pos_;
    PlannerArray planners_;
  };

  typedef ReferenceCounting::SmartPtr<PlannerPool> PlannerPool_var;
}
}

#endif
