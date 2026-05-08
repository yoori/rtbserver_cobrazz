#pragma once

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
    public virtual Generics::RefCountableActiveObject,
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
