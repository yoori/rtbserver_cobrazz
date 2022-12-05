
#include "SecondStartWaiter.hpp"

// SecondStartWaiter class

SecondStartWaiter::SecondStartWaiter()  :
  second_start_time_(Generics::Time::get_time_of_day())
{ }

SecondStartWaiter::~SecondStartWaiter()
{
  Guard_ guard(condition_);
  Generics::Time wake_up_time(second_start_time_ + WAIT_DURATION);
  guard.timed_wait(&wake_up_time);
}
