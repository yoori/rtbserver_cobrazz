#ifndef REQUESTINFOMANAGER_COMPATIBILITY_UTILS_HPP
#define REQUESTINFOMANAGER_COMPATIBILITY_UTILS_HPP

#include <Generics/Time.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  Generics::Time
  date_month_trunc(const Generics::Time& time);
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  Generics::Time
  date_month_trunc(const Generics::Time& time)
  {
    Generics::ExtendedTime date_day_ex(time.get_gm_time());
    
    return Generics::ExtendedTime(
      date_day_ex.tm_year + 1900,
      date_day_ex.tm_mon + 1,
      1,
      0,
      0,
      0,
      0);
  }
}
}

#endif /*REQUESTINFOMANAGER_COMPATIBILITY_UTILS_HPP*/
