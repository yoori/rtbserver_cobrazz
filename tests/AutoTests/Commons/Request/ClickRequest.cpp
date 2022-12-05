
#include "ClickRequest.hpp"

namespace AutoTest
{
  const char* ClickRequest::BASE_URL = "/services/AdClickServer/";

  ClickRequest::ClickRequest() :
    BaseRequest(BASE_URL, BaseRequest::RT_NOT_ENCODED),
    ccid(this, "ccid"),
    uid(this, "uid"),
    requestid(this, "requestid"),
    ccgkeyword(this, "ccgkeyword"),
    debug_time(this, "debug-time")
  { }

  ClickRequest::ClickRequest(const ClickRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_NOT_ENCODED),
    ccid(this, other.ccid),
    uid(this, other.uid),
    requestid(this, other.requestid),
    ccgkeyword(this, other.ccgkeyword),
    debug_time(this, other.debug_time)
  { }

  ClickRequest::ClickRequest(const char* url)
    /*throw(HTTP::URLAddress::InvalidURL, eh::Exception)*/ :
    BaseRequest(BASE_URL, BaseRequest::RT_NOT_ENCODED),
    ccid(this, "ccid"),
    uid(this, "uid"),
    requestid(this, "requestid"),
    ccgkeyword(this, "ccgkeyword"),
    debug_time(this, "debug-time")
  {
    BaseRequest::decode_(url);
  }
} // namespace AutoTest
