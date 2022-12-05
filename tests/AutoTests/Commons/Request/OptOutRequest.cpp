
#include "OptOutRequest.hpp"

namespace AutoTest
{
  const char* OptOutRequest::BASE_URL = "/services/OO";

  OptOutRequest::OptOutRequest(bool set_defs) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    op(this, "op"),
    debug_time(this, "debug-time"),
    opted_in_url(this, "opted_in_url"),
    opted_out_url(this, "opted_out_url"),
    opt_undef_url(this, "opt_undef_url"),
    already_url(this, "already_url"),
    success_url(this, "success_url"),
    testrequest(this, "testrequest"),
    colo(
      this, "colo",
      AutoTest::GlobalSettings::instance().optout_colo(),
      set_defs),
    ct(this, "ct"),
    ce(this, "ce"),
    fail_url(this, "fail_url"),
    user_agent(this, "User-Agent", DEFAULT_USER_AGENT, set_defs)
  { }

  OptOutRequest::OptOutRequest(const OptOutRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),      
    op(this, other.op),
    debug_time(this, other.debug_time),
    opted_in_url(this, other.opted_in_url),
    opted_out_url(this, other.opted_out_url),
    opt_undef_url(this, other.opt_undef_url),
    already_url(this, other.already_url),
    success_url(this, other.success_url),
    testrequest(this, other.testrequest),
    colo(this, other.colo),
    ct(this, other.ct),
    ce(this, other.ce),
    fail_url(this, other.fail_url),
    user_agent(this, other.user_agent)
  { }
}
