#include <eh/Errno.hpp>
#include "TimeoutControl.hpp"

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  const Generics::Time TimeoutControl::MIN_SLEEP_TIME(0, 100); // 0.0001 sec
  const Generics::Time TimeoutControl::MAX_SLEEP_TIME(0, 500000); // 0.5 sec

  TimeoutControl::TimeoutControl(
    const Generics::Time* timeout) noexcept
    : timeout_(timeout ? *timeout : Generics::Time::ZERO)
  {}
      
  bool
  TimeoutControl::sleep_step() /*throw(Exception, TimedOut)*/
  {
    /* init start time at first step */
    Generics::Time now(Generics::Time::get_time_of_day());
    if(start_time_ == Generics::Time::ZERO)
    {
      start_time_ = now;
    }

    Generics::Time passed_time(now - start_time_);
    Generics::Time sleep_time = passed_time;
    if(timeout_ != Generics::Time::ZERO &&
       passed_time + sleep_time > timeout_)
    {
      if(passed_time < timeout_)
      {
        sleep_time = timeout_ - passed_time;
      }
      else
      {
        return false;
      }
    }
    if(sleep_time < MIN_SLEEP_TIME)
    {
      sleep_time = MIN_SLEEP_TIME;
    }
    if(sleep_time > MAX_SLEEP_TIME)
    {
      sleep_time = MAX_SLEEP_TIME;
    }
        
    timespec rem;
    rem.tv_sec = sleep_time.tv_sec;
    rem.tv_nsec = sleep_time.tv_usec * 1000;

    timespec req;
    int result;
    do
    {
      req = rem;
      result = ::nanosleep(&req, &rem);
    }
    while(result == -1 && errno == EINTR);
        
    if(result == -1)
    {
      eh::throw_errno_exception<Oracle::Exception>("can't make nanosleep");
    }

    return true;
  }

  Generics::Time
  TimeoutControl::passed_time() const noexcept
  {
    return Generics::Time::get_time_of_day() - start_time_;
  }

  Generics::Time
  TimeoutControl::timeout() const noexcept
  {
    return timeout_;
  }
}
}
}

