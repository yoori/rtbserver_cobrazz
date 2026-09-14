#include <iostream>
#include <stdexcept>

#include <CampaignSvcs/CampaignManager/CampaignConfig.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs;
  using namespace AdServer::CampaignSvcs::AdInstances;

  void
  check(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }
}

int
main()
{
  try
  {
    Campaign_var campaign(new Campaign());
    campaign->ctr_modifiable = true;

    campaign->bid_strategy = BS_MIN_CTR_GOAL;
    campaign->base_min_ctr_goal = RevenueDecimal::ZERO;
    campaign->set_min_ctr_goal(RevenueDecimal::ZERO);
    check(!campaign->use_ctr_goal(), "zero CTR goal must be disabled");

    campaign->bid_strategy = BS_MAX_REACH;
    campaign->base_min_ctr_goal = RevenueDecimal("0.001");
    campaign->set_min_ctr_goal(campaign->base_min_ctr_goal);
    check(campaign->use_ctr_goal(), "positive CTR goal must be enabled");

    campaign->base_min_ctr_goal = RevenueDecimal::ZERO;
    campaign->set_min_ctr_goal(RevenueDecimal("0.002"));
    check(campaign->use_ctr_goal(), "positive dynamic CTR goal must be enabled");

    campaign->set_min_ctr_goal(RevenueDecimal::ZERO);
    check(!campaign->use_ctr_goal(), "cleared CTR goal must be disabled");

    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
