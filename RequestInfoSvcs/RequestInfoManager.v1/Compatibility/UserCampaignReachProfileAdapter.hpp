#ifndef USERCAMPAIGNREACHPROFILEADAPTER_HPP
#define USERCAMPAIGNREACHPROFILEADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  const unsigned long CURRENT_CAMPAIGN_REACH_PROFILE_VERSION = 24;

  struct UserCampaignReachProfileAdapter
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Generics::ConstSmartMemBuf_var
    operator()(const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/;
  };
}
}

#endif /*USERCAMPAIGNREACHPROFILEADAPTER_HPP*/
