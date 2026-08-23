#include "CampaignBillingAccount.hpp"

namespace AdServer::CampaignSvcs
{
  AdvBillingCalculationFlags
  make_adv_billing_calculation_flags(
    const AdInstances::AccountDef& account,
    CCGRateType ccg_rate_type) noexcept
  {
    return {
      account.agency_profit_by_pub_amount() || ccg_rate_type == CR_MAXBID};
  }

  AdvBillingAmounts
  calculate_adv_billing_amounts(
    const AdInstances::AccountDef& account,
    CCGRateType ccg_rate_type,
    const RevenueDecimal& original_adv_amount,
    const RevenueDecimal& publisher_amount,
    const RevenueDecimal& adv_commission)
  {
    return calculate_adv_billing_amounts(
      make_adv_billing_calculation_flags(account, ccg_rate_type),
      original_adv_amount,
      publisher_amount,
      adv_commission);
  }
}
