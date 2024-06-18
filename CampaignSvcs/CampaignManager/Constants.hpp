#ifndef CAMPAIGNSVCS_CAMPAIGNMANAGER_CONSTANTS_HPP_
#define CAMPAIGNSVCS_CAMPAIGNMANAGER_CONSTANTS_HPP_

// STD
#include <cstdint>

namespace AdServer::CampaignSvcs
{

inline constexpr std::uint32_t CREATIVE_EXPANDING_LEFT = 1;
inline constexpr std::uint32_t CREATIVE_EXPANDING_RIGHT = CREATIVE_EXPANDING_LEFT << 1;
inline constexpr std::uint32_t CREATIVE_EXPANDING_UP = CREATIVE_EXPANDING_LEFT << 2;
inline constexpr std::uint32_t CREATIVE_EXPANDING_DOWN = CREATIVE_EXPANDING_LEFT << 3;

} // namespace AdServer::CampaignSvcs

#endif // CAMPAIGNSVCS_CAMPAIGNMANAGER_CONSTANTS_HPP_
