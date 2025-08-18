#ifndef CAMPAIGNMANAGER_AVAILABLEANDMINCTRSETTER_HPP
#define CAMPAIGNMANAGER_AVAILABLEANDMINCTRSETTER_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer::CampaignSvcs
{
  struct AvailableAndMinCTRSetter: public virtual ReferenceCounting::Interface
  {
    friend class BillingStateContainer;
    friend class GrpcBillingStateContainer;

  protected:
    virtual void set_available(
      const bool available_val,
      const RevenueDecimal& goal_ctr) const noexcept = 0;
  };

  using CAvailableAndMinCTRSetter_var =
    ReferenceCounting::ConstPtr<AvailableAndMinCTRSetter>;
}

#endif /*CAMPAIGNMANAGER_AVAILABLEANDMINCTRSETTER_HPP*/
