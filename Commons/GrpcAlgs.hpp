#pragma once

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
  std::string
  pack_request_id(const AdServer::Commons::RequestId& request_id)
  {
    return std::string(request_id.begin(), request_id.end());
  }

  inline
  AdServer::Commons::RequestId
  unpack_request_id(const std::string& request_id_str)
  {
    if(request_id_str.empty())
    {
      return AdServer::Commons::RequestId();
    }

    return AdServer::Commons::RequestId(
      reinterpret_cast<const unsigned char*>(request_id_str.data()),
      reinterpret_cast<const unsigned char*>(request_id_str.data()) +
        request_id_str.size());
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

  template<typename DecimalType>
  std::string
  pack_decimal(const DecimalType& dec)
  {
    std::string result;
    result.resize(DecimalType::PACK_SIZE);
    dec.pack(reinterpret_cast<unsigned char*>(&result[0]));
    return result;
  }

  template<typename DecimalType>
  DecimalType
  unpack_decimal(const std::string& str)
  {
    DecimalType result;
    assert(str.size() == DecimalType::PACK_SIZE);
    if(str.size() == DecimalType::PACK_SIZE)
    {
      result.unpack(reinterpret_cast<const unsigned char*>(str.data()));
    }
    return result;
  }
}
