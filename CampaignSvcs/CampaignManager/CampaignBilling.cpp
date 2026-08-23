#include "CampaignBilling.hpp"

namespace AdServer::CampaignSvcs
{
  AdvBillingAmounts
  calculate_adv_billing_amounts(
    const AdvBillingCalculationFlags& flags,
    const RevenueDecimal& original_adv_amount,
    const RevenueDecimal& publisher_amount,
    const RevenueDecimal& adv_commission)
  {
    // Schema #1: the advertiser amount is fixed, and the difference from the
    // publisher charge is the agency profit.
    if (!flags.agency_profit_by_pub_amount)
    {
      return {
        original_adv_amount,
        original_adv_amount - publisher_amount};
    }

    // Schema #2: the advertiser amount follows the publisher charge, and the
    // agency profit is calculated as a share of that charge.
    const RevenueDecimal adv_comm_amount = RevenueDecimal::mul(
      publisher_amount,
      adv_commission,
      Generics::DMR_FLOOR);

    return {
      publisher_amount + adv_comm_amount,
      adv_comm_amount};
  }
}
