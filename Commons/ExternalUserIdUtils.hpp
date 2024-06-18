#ifndef COMMONS_EXTERNALUSERIDUTILS_HPP_
#define COMMONS_EXTERNALUSERIDUTILS_HPP_

#include <string>
#include <vector>
#include <String/SubString.hpp>

namespace AdServer
{
namespace Commons
{
  using ExternalUserIdArray = std::vector<std::string>;

  void
  dns_encode_external_user_ids(
    std::string& res,
    const ExternalUserIdArray& user_ids)
    noexcept;

  void
  dns_decode_external_user_ids(
    ExternalUserIdArray& user_ids,
    const String::SubString& hostname)
    noexcept;
}
}

#endif /*COMMONS_EXTERNALUSERIDUTILS_HPP_*/
