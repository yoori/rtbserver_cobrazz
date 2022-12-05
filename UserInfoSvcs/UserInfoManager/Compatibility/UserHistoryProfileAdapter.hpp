#ifndef USERHISTORYPROFILEADAPTER_HPP
#define USERHISTORYPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    struct HistoryProfileAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::ConstSmartMemBuf_var
      operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
    };

  }
}

#endif /* USERHISTORYPROFILEADAPTER_HPP */
