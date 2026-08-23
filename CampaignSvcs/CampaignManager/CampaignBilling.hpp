#pragma once

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer::CampaignSvcs
{
  struct AdvBillingCalculationFlags
  {
    bool agency_profit_by_pub_amount;
  };

  struct AdvBillingAmounts
  {
    RevenueDecimal adv_amount;
    RevenueDecimal adv_comm_amount;
  };

  // publisher_amount is already converted to the advertiser currency and
  // includes the self-service commission and the colocation revenue share.
  AdvBillingAmounts
  calculate_adv_billing_amounts(
    const AdvBillingCalculationFlags& flags,
    const RevenueDecimal& original_adv_amount,
    const RevenueDecimal& publisher_amount,
    const RevenueDecimal& adv_commission);
}
