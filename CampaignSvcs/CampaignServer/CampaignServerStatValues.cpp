// @file CampaignServerStatValues.cpp
#include "CampaignServerStatValues.hpp"
#include "CampaignConfig.hpp"
#include "CampaignServerImpl.hpp"

namespace
{
  const AdServer::CampaignSvcs::ProcStatImpl::Key CAMPAIGN_COUNT(
    "campaignCount");
  const AdServer::CampaignSvcs::ProcStatImpl::Key TAG_COUNT(
    "tagCount");
  const AdServer::CampaignSvcs::ProcStatImpl::Key BEHAV_CHANNEL_COUNT(
    "behavChannelCount");
  const AdServer::CampaignSvcs::ProcStatImpl::Key EXPRESSION_CHANNEL_COUNT(
    "expressionChannelCount");
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    const AdServer::CampaignSvcs::ProcStatImpl::Key LAST_DB_UPDATE_TIME(
      "lastDBUpdateTime");
    const AdServer::CampaignSvcs::ProcStatImpl::Key LAST_DB_UPDATE_DELAY(
      "lastDBUpdateDelay");

    void
    CampaignServerStatValues::fill_values(CampaignConfig* new_config)
      /*throw(eh::Exception)*/
    {
      set(CAMPAIGN_COUNT,
        static_cast<unsigned long>(new_config->campaigns.active().size()));
      set(TAG_COUNT,
        static_cast<unsigned long>(new_config->tags.active().size()));
      set(BEHAV_CHANNEL_COUNT,
        static_cast<unsigned long>(
          new_config->simple_channels.active().size()));
      set(LAST_DB_UPDATE_TIME, new_config->master_stamp.as_double());
      set(EXPRESSION_CHANNEL_COUNT,
        static_cast<unsigned long>(
          new_config->expression_channels.active().size()));
    }

  }
}
