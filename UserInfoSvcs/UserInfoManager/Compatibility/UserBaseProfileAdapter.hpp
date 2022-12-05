#ifndef USERBASEPROFILEADAPTER_HPP
#define USERBASEPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    struct BaseProfileAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::ConstSmartMemBuf_var
      operator()(const Generics::ConstSmartMemBuf* mem_buf,
        bool ignore_future_versions = false) /*throw(Exception)*/;
    };

  }
}

#endif /* USERBASEPROFILEADAPTER_HPP */
