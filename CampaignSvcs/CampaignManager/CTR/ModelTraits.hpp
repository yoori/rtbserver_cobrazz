#pragma once

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
