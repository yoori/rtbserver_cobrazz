#ifndef USERTRIGGERMATCHPROFILEADAPTER_HPP
#define USERTRIGGERMATCHPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_USER_TRIGGER_MATCH_PROFILE_VERSION = 330;

  class UserTriggerMatchProfileAdapter
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*USERTRIGGERMATCHPROFILEADAPTER_HPP*/
