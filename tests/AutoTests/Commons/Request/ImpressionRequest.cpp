
#include "ImpressionRequest.hpp"

namespace AutoTest
{
  const char* ImpressionRequest::BASE_URL = "/track.gif";

  ImpressionRequest::ImpressionRequest() :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    uid(this, "uid"),
    requestid(this, "requestid"),
    ccid(this, "ccid"),
    debug_time(this, "debug-time")
  { }

  ImpressionRequest::ImpressionRequest(const ImpressionRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    uid(this, other.uid),
    requestid(this, other.requestid),
    ccid(this, other.ccid),
    debug_time(this, other.debug_time)
  { }

  ImpressionRequest::ImpressionRequest(const char* url)
    /*throw(HTTP::URLAddress::InvalidURL, eh::Exception)*/ :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    uid(this, "uid"),
    requestid(this, "requestid"),
    ccid(this, "ccid"),
    debug_time(this, "debug-time")
  {
    BaseRequest::decode_(url);
  }
}
