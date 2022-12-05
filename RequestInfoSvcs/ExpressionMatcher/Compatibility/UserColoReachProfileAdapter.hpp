#ifndef USERCOLOREACHPROFILEADAPTER_HPP
#define USERCOLOREACHPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_USER_COLO_REACH_PROFILE_VERSION = 34;

  class UserColoReachProfileAdapter
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    UserColoReachProfileAdapter()
      noexcept;

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*USERCOLOREACHPROFILEADAPTER_HPP*/
