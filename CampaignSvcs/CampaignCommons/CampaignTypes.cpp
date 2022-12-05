#include "CampaignTypes.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    const char*
    to_str(UserStatus status) noexcept
    {
      switch (status)
      {
      case US_UNDEFINED:
        return "UNDEFINED";

      case US_OPTIN:
        return "OPTIN";

      case US_OPTOUT:
        return "OPTOUT";

      case US_PROBE:
        return "PROBE";

      case US_TEMPORARY:
        return "TEMPORARY";

      case US_NONE:
        return "NONE";

      case US_NOEXTERNALID:
        return "NOEXTERNALID";

      case US_EXTERNALPROBE:
        return "EXTERNALPROBE";

      case US_FOREIGN:
        return "FOREIGN";

      case US_BLACKLISTED:
        return "BLACKLISTED";
      }

      return "";
    }

    const char*
    to_str(CampaignMode mode) noexcept
    {
      switch (mode)
      {
      case CM_RANDOM:
        return "RANDOM";

      case CM_NON_RANDOM:
        return "NON-RANDOM";
      }

      return "";
    }

    const char*
    to_str(AuctionType type) noexcept
    {
      switch (type)
      {
      case AT_RANDOM:
        return "RANDOM";

      case AT_MAX_ECPM:
        return "MAX ECPM";

      case AT_PROPORTIONAL_PROBABILITY:
        return "PROPORTIONAL PROBABILITY";
      }

      return "";
    }
  }
}
