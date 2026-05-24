/// @file AdFrontend/OptOutFrontendStat.cpp
#include "OptoutFrontendStat.hpp"

namespace
{
  const char OPT_OPT_IN_COUNT[] = "ooRequest-InOp-Count";
  const char OPT_OPT_OUT_COUNT[] = "ooRequest-OutOp-Count";
  const char OPT_OPT_STATUS_COUNT[] = "ooRequest-StatusOp-Count";
}

namespace Aspect
{
  extern const char OPTOUT_FRONTEND[];
}

namespace AdServer
{

  OptOutFrontendStat::StatData&
  OptOutFrontendStat::StatData::operator +=(const OptOutFrontendStat::StatData& rhs) noexcept
  {
    opt_in_user += rhs.opt_in_user;
    non_opt_on_user += rhs.non_opt_on_user;
    opt_status += rhs.opt_status;

    return *this;
  }

  OptOutFrontendStat::OptOutFrontendStat()
    /*throw(Exception)*/
    : stat_data_()
  {
  }

  void
  OptOutFrontendStat::consider_request(OptOperation op) noexcept
  {
    StatData d{ op == OO_IN ? 1UL : 0UL,
                op == OO_OUT ? 1UL : 0UL,
                op == OO_STATUS ? 1UL : 0UL};

    if (!d.opt_in_user && !d.non_opt_on_user && !d.opt_status)
    {
      return;
    }

    Sync::PosixGuard lock(mutex_);

    stat_data_ += d;
  }

  Generics::Values_var
  OptOutFrontendStat::extract_stats_values()
  {
    StatData d = {0, 0, 0}; //to prevent warning

    {
      Sync::PosixGuard lock(mutex_);
      std::swap(d, stat_data_);
    }

    Generics::Values_var v(new Generics::Values);

    v->set(OPT_OPT_IN_COUNT, d.opt_in_user);
    v->set(OPT_OPT_OUT_COUNT, d.non_opt_on_user);
    v->set(OPT_OPT_STATUS_COUNT, d.opt_status);

    return v;
  }

}
