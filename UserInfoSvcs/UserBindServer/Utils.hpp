#ifndef RTBSERVER_COBRAZZ_USERBINDSERVER_UTILS_HPP
#define RTBSERVER_COBRAZZ_USERBINDSERVER_UTILS_HPP

// UNIXCOMMONS
#include <Generics/Hash.hpp>
#include <String/SubString.hpp>

namespace AdServer::UserInfoSvcs::Utils
{

inline std::size_t hash(const String::SubString& str)
{
  std::size_t result;
  {
    Generics::Murmur64Hash murmur_hash(result);
    murmur_hash.add(str.data(), str.size());
  }
  return result;
}

inline std::size_t hash(const std::string& str)
{
  std::size_t result;
  {
    Generics::Murmur64Hash murmur_hash(result);
    murmur_hash.add(str.data(), str.size());
  }
  return result;
}

} // namespace AdServer::UserInfoSvcs::Utils

#endif //RTBSERVER_COBRAZZ_USERBINDSERVER_UTILS_HPP
