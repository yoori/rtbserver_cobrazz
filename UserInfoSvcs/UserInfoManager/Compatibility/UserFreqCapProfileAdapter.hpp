#ifndef USERFREQCAPPROFILEADAPTER_HPP
#define USERFREQCAPPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    struct UserFreqCapProfileAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::ConstSmartMemBuf_var
      operator()(const Generics::ConstSmartMemBuf* mem_buf)
        /*throw(eh::Exception)*/;
    };

  }
}

#endif /* USERFREQCAPPROFILEADAPTER_HPP */
