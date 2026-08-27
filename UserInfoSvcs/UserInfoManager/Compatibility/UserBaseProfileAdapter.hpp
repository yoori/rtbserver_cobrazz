#pragma once

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer::UserInfoSvcs
{
  struct BaseProfileAdapter
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::SmartMemBuf_var
    operator()(Generics::SmartMemBuf* mem_buf,
      bool ignore_future_versions = false) /*throw(Exception)*/;
  };

}
