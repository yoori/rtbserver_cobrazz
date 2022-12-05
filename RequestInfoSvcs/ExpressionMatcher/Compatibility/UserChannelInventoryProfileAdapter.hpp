#ifndef USERCHANNELINVENTORYPROFILEADAPTER_HPP
#define USERCHANNELINVENTORYPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_CHANNEL_INVENTORY_PROFILE_VERSION = 34;

  class UserChannelInventoryProfileAdapter
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*USERCHANNELINVENTORYPROFILEADAPTER_HPP*/
