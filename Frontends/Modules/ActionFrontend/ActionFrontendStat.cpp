/// @file AdFrontend/AcFrontendStat.cpp
#include "ActionFrontendStat.hpp"

namespace
{
  const char AC_OPT_IN_COUNT[] = "actionRequest-OptInUser-Count";
  const char AC_OPT_OUT_COUNT[] = "actionRequest-nonOptInUser-Count";
}

namespace Aspect
{
  extern const char ACTION_FRONTEND[];
}

namespace AdServer
{

  AcFrontendStat::StatData&
  AcFrontendStat::StatData::operator +=(const AcFrontendStat::StatData& rhs) noexcept
  {
    opt_in_user += rhs.opt_in_user;
    non_opt_on_user += rhs.non_opt_on_user;

    return *this;
  }

  AcFrontendStat::AcFrontendStat()
    /*throw(Exception)*/
    : stat_data_()
  {
  }

  void
  AcFrontendStat::consider_request(
    bool test_request,
    unsigned long user_status)
    noexcept
  {
    if (test_request)
    {
      return;
    }

    StatData d{
      user_status == AdServer::CampaignSvcs::US_OPTIN ? 1UL : 0UL,
      user_status == AdServer::CampaignSvcs::US_OPTOUT ? 1UL : 0UL};

    if (!d.opt_in_user && !d.non_opt_on_user)
    {
      return;
    }

    Sync::PosixGuard lock(mutex_);

    stat_data_ += d;
  }

  Generics::Values_var
  AcFrontendStat::extract_stats_values()
  {
    StatData d = {0, 0}; //to prevent warning

    {
      Sync::PosixGuard lock(mutex_);
      std::swap(d, stat_data_);
    }

    Generics::Values_var v(new Generics::Values);

    v->set(AC_OPT_IN_COUNT, d.opt_in_user);
    v->set(AC_OPT_OUT_COUNT, d.non_opt_on_user);

    return v;
  }
}
