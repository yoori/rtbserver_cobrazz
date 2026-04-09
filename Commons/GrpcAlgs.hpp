#ifndef _GRPCALGS_HPP_
#define _GRPCALGS_HPP_

#include <cassert>
#include <string>

#include <Generics/Time.hpp>

#include "UserInfoManip.hpp"

namespace GrpcAlgs
{
  inline
  std::string
  pack_time(const Generics::Time& ts)
  {
    std::string result;
    result.resize(Generics::Time::TIME_PACK_LEN);
    ts.pack(reinterpret_cast<unsigned char*>(&result[0]));
    return result;
  }

  inline
  Generics::Time
  unpack_time(const std::string& str)
  {
    Generics::Time result;
    assert(str.size() == Generics::Time::TIME_PACK_LEN);
    if(str.size() == Generics::Time::TIME_PACK_LEN)
    {
      result.unpack(reinterpret_cast<const unsigned char*>(str.data()));
    }
    return result;
  }

  inline
  std::string
  pack_user_id(const AdServer::Commons::UserId& user_id)
  {
    return std::string(user_id.begin(), user_id.end());
  }

  inline
  AdServer::Commons::UserId
  unpack_user_id(const std::string& user_id_str)
  {
    if(user_id_str.empty())
    {
      return AdServer::Commons::UserId();
    }

    return AdServer::Commons::UserId(
      reinterpret_cast<const unsigned char*>(user_id_str.data()),
      reinterpret_cast<const unsigned char*>(user_id_str.data()) + user_id_str.size());
  }
}

#endif
