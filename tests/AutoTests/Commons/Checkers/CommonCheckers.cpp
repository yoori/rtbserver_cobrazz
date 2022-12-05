
#include "CommonCheckers.hpp"

namespace AutoTest
{

  void predicate_checker(bool predicate) /*throw(CheckFailed)*/
  {
    if(!predicate)
    {
      throw CheckFailed("Fail");
    }
  }
    
  // TimeLessChecker
  TimeLessChecker::TimeLessChecker(const Generics::Time& now_less_then)
    noexcept
    : now_less_then_(now_less_then)
  {}

  TimeLessChecker::~TimeLessChecker() noexcept
  {}

  bool
  TimeLessChecker::check(bool throw_error)
    /*throw(TimeLessCheckFailed)*/
  {
    const Generics::Time now = Generics::Time::get_time_of_day();
    if(now < now_less_then_)
    {
      return true;
    }

    if(throw_error)
    {
      Stream::Error err;
      err << "Deadline time has expired: " << now << " (now) > "
          << now_less_then_ << " (deadline)";
      throw TimeLessCheckFailed(err);
    }

    return false;
  }
}
