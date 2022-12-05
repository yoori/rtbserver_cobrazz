#ifndef COMMONS_USER_INFO_MANIP_HPP_
#define COMMONS_USER_INFO_MANIP_HPP_

#include <Generics/Uuid.hpp>
#include <Generics/CRC.hpp>
#include <Generics/Hash.hpp>

namespace AdServer
{
  namespace Commons
  {
    typedef Generics::Uuid UserId;
    typedef Generics::Uuid RequestId;

    extern const UserId PROBE_USER_ID;

    inline
    unsigned long
    uuid_distribution_hash(const Generics::Uuid& uuid) noexcept
    {
      return Generics::CRC::quick(0, &*uuid.begin(), uuid.size());
    }

    inline
    unsigned long
    external_id_distribution_hash(const String::SubString& external_id)
      noexcept
    {
      unsigned long ext_hash;

      {
        Generics::Murmur64Hash hash(ext_hash);
        hash.add(external_id.data(), external_id.size());
      }

      return ext_hash;
    }

    struct UserIdDistributionHashAdapter: public UserId
    {
      UserIdDistributionHashAdapter(const UserId& user_id)
        : UserId(user_id)
      {
        hash_ = uuid_distribution_hash(user_id);
      }

      unsigned long hash() const
      {
        return hash_;
      }

    private:
      unsigned long hash_;
    };
  }
}

#endif /*COMMONS_USER_INFO_MANIP_HPP*/
