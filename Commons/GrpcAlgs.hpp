#ifndef _GRPCALGS_HPP_
#define _GRPCALGS_HPP_

// STD
#include <cassert>

// THIS
#include <Commons/UserInfoManip.hpp>

// UNIX_COMMONS
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>

namespace GrpcAlgs
{

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

inline
Generics::Time
unpack_time(const std::string& data)
{
  assert(Generics::Time::TIME_PACK_LEN == data.length());

  Generics::Time time;
  time.unpack(data.data());

  return time;
}

inline
AdServer::Commons::UserId
unpack_user_id(const std::string& uid)
{
  if(!uid.empty())
  {
    return AdServer::Commons::UserId(
      uid.data(),
      uid.data() + uid.size());
  }
  else
  {
    return AdServer::Commons::UserId();
  }
}

} // namespace GrpcAlgs

#endif /*_GRPCALGS_HPP_*/