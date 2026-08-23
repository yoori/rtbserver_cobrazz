#pragma once

#include "CampaignBilling.hpp"
#include "CampaignConfig.hpp"

namespace AdServer::CampaignSvcs
{
  AdvBillingCalculationFlags
  make_adv_billing_calculation_flags(
    const AdInstances::AccountDef& account,
    CCGRateType ccg_rate_type) noexcept;

  AdvBillingAmounts
  calculate_adv_billing_amounts(
    const AdInstances::AccountDef& account,
    CCGRateType ccg_rate_type,
    const RevenueDecimal& original_adv_amount,
    const RevenueDecimal& publisher_amount,
    const RevenueDecimal& adv_commission);
}
