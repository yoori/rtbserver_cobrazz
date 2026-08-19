#pragma once

#include <cstdint>

namespace AdServer::CampaignSvcs::CTR
{
  inline uint32_t
  feature_hash_index(uint32_t hash, uint64_t features_size) noexcept
  {
    return hash % features_size;
  }
}
