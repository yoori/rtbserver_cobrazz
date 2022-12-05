#ifndef USERSITEREACHPROFILEADAPTER_HPP
#define USERSITEREACHPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_USER_SITE_REACH_PROFILE_VERSION = 25;

  struct UserSiteReachProfileAdapter
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*USERSITEREACHPROFILEADAPTER_HPP*/
