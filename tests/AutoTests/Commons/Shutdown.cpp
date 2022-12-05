
#include "Shutdown.hpp"

namespace AutoTest
{

  Shutdown_::Shutdown_()
    : value_(false)
  { }

  void
  Shutdown_::set()
    /*throw(eh::Exception)*/
  {
    Sync::ConditionalGuard condition(cond_);
    value_ = true;
    cond_.broadcast();   
  }
  
  void
  Shutdown_::wait(
    const Time& timeout)
    /*throw(ShutdownException, eh::Exception)*/
  {
    Sync::ConditionalGuard condition(cond_);
    if ( value_ || condition.timed_wait(&timeout, true) )
    {
      throw ShutdownException("Shutting down");
    }
  }

  bool
  Shutdown_::get()
  {
    return value_;
  }

 
}


