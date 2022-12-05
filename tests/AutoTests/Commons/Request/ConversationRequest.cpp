
#include "ConversationRequest.hpp"

namespace AutoTest
{
  const char* ConversationRequest::BASE_URL  = "/conv";

  ConversationRequest::ConversationRequest(bool set_defs) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    convid(this, "convid", 0, false),
    value(this, "value", 0, false),
    orderid(this, "orderid", 0, false),
    test(this, "test", 0, set_defs),
    debug_time(this, "debug-time", 0, false),
    referer(this, "Referer", 0, false)
  { }

  ConversationRequest::ConversationRequest(
    const ConversationRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    convid(this, other.convid),
    value(this, other.value),
    orderid(this, other.orderid),
    test(this, other.test),
    debug_time(this, other.debug_time),
    referer(this, other.referer)
  { }
}
