#ifndef CAMPAIGNMANAGER_CTR_MODELTRAITS_HPP_
#define CAMPAIGNMANAGER_CTR_MODELTRAITS_HPP_

namespace AdServer::CampaignSvcs::CTR
{
  struct ModelTraits
  {
    bool push_hour = false;
    bool push_week_day = false;
    bool push_campaign_freq = false;
    bool push_campaign_freq_log = false;
    bool creative_dependent = true;
  };
}

#endif /*CAMPAIGNMANAGER_CTR_MODELTRAITS_HPP_*/
