
#include "ActionRequest.hpp"

namespace AutoTest
{
  const char* ActionRequest::BASE_URL  = "/services/ActionServer/";
  const char* ActionRequest::COUNTRY_CODE = "gn";

  ActionRequest::ActionRequest(bool set_defs) :
    BaseRequest(BASE_URL, BaseRequest::RT_NOT_ENCODED),
    cid(this, "cid", 0, false),
    actionid(this, "actionid", 0, false),
    country(this, "country", COUNTRY_CODE, set_defs),
    testrequest(this, "testrequest", 0, set_defs),
    debug_time(this, "debug-time", 0, false),
    referer(this, "Referer", 0, false)
  { }

  ActionRequest::ActionRequest(const ActionRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_NOT_ENCODED),
    cid(this, other.cid),
    actionid(this, other.actionid),
    country(this, other.country),
    testrequest(this, other.testrequest),
    debug_time(this, other.debug_time),
    referer(this, other.referer)
  { }
}



