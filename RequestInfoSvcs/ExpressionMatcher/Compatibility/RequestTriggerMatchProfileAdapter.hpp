#ifndef REQUESTTRIGGERMATCHPROFILEADAPTER_HPP
#define REQUESTTRIGGERMATCHPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_REQUEST_TRIGGER_MATCH_PROFILE_VERSION = 360;

  class RequestTriggerMatchProfileAdapter
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*REQUESTTRIGGERMATCHPROFILEADAPTER_HPP*/
