#include "PlannerPool.hpp"

namespace AdServer
{
namespace Bidding
{
  PlannerPool::PlannerPool(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads,
    size_t stack_size)
    : planner_pos_(0)
  {
    for(unsigned long i = 0; i < threads; ++i)
    {
      planners_.emplace_back(
        new Generics::Planner(callback, stack_size));

      add_child_object(planners_.back());
    }
  }

  void
  PlannerPool::schedule(Generics::Goal* goal, const Generics::Time& time)
  {
    unsigned long use_pos = planner_pos_++ % planners_.size();
    planners_[use_pos]->schedule(goal, time);
  }
}
}
