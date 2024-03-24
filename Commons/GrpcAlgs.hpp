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
std::string
pack_time(const Generics::Time& time)
{
  std::string result(Generics::Time::TIME_PACK_LEN, '0');
  time.pack(result.data());
  return result;
}

inline
Generics::Time
unpack_time(const std::string& data)
{
  assert(Generics::Time::TIME_PACK_LEN == data.length());

  Generics::Time time;
  time.unpack(data.c_str());

  return time;
}

inline
AdServer::Commons::UserId
unpack_user_id(const std::string& user_id)
{
  if(user_id.empty())
  {
    return AdServer::Commons::UserId();
  }
  else
  {
    return AdServer::Commons::UserId(
      user_id.data(),
      user_id.data() + user_id.size());
  }
}

template<typename DecimalType>
std::string pack_decimal(const DecimalType& dec)
{
  std::string result;
  result.resize(DecimalType::PACK_SIZE);
  dec.pack(result.data());
  return result;
}

} // namespace GrpcAlgs

#endif /*_GRPCALGS_HPP_*/