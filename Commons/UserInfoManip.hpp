#pragma once

#include <cstdint>
#include <string_view>

#include <Generics/Uuid.hpp>
#include <Generics/CRC.hpp>
#include <Generics/Hash.hpp>

namespace AdServer::Commons
{
  using UserId = Generics::Uuid;
  using RequestId = Generics::Uuid;

  extern const UserId PROBE_USER_ID;

  std::uint32_t
  user_id_sampling_hash(std::string_view encoded_user_id) noexcept;

  std::uint32_t
  user_id_sampling_hash(const UserId& user_id) noexcept;

  inline constexpr unsigned long SAMPLING_RESOLUTION = 1000000;

  inline
  bool
  check_percentage_sampling(unsigned long hash, double percentage) noexcept
  {
    if (percentage >= 100)
    {
      return true;
    }

    if (percentage <= 0)
    {
      return false;
    }

    return hash % SAMPLING_RESOLUTION <
      static_cast<unsigned long>(percentage * (SAMPLING_RESOLUTION / 100.0));
  }

  inline
  unsigned long
  uuid_distribution_hash(const Generics::Uuid& uuid) noexcept
  {
    return ::Generics::CRC::quick(0, &*uuid.begin(), uuid.size());
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
