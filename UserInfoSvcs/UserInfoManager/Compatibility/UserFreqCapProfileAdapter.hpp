#pragma once

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer::UserInfoSvcs
{
  struct UserFreqCapProfileAdapter
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::SmartMemBuf_var
    operator()(Generics::SmartMemBuf* mem_buf)
      /*throw(eh::Exception)*/;
  };

}
