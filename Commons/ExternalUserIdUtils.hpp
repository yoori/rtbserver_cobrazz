#ifndef COMMONS_EXTERNALUSERIDUTILS_HPP_
#define COMMONS_EXTERNALUSERIDUTILS_HPP_

#include <string>
#include <vector>
#include <String/SubString.hpp>

namespace AdServer
{
namespace Commons
{
  /*
  struct ExternalUserId
  {
    ExternalUserId()
    {}

    ExternalUserId(
      const String::SubString& source_id_val,
      const String::SubString& id_val)
      : source_id(source_id_val.str()),
        id(id_val.str())
    {}

    std::string source_id;
    std::string id;
  };
  */

  typedef std::vector<std::string> ExternalUserIdArray;

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
