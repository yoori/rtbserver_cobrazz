#ifndef REQUESTOPERATIONIMPRESSIONPROFILEPROFILEADAPTER_HPP
#define REQUESTOPERATIONIMPRESSIONPROFILEPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_REQUESTOPERATIONIMPRESSION_PROFILE_VERSION = 331;

  struct RequestOperationImpressionProfileAdapter
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    void
    operator()(Generics::MemBuf& mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*REQUESTOPERATIONIMPRESSIONPROFILEPROFILEADAPTER_HPP*/
