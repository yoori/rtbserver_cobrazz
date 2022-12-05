package AdServer::LogProcessing::LogProxyConfigurator;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/LogProxy/UserProperties" .
    " && mkdir -p \${log_root}/LogProxy/UserProperties_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelPerformance" .
    " && mkdir -p \${log_root}/LogProxy/ChannelPerformance_" .
    " && mkdir -p \${log_root}/LogProxy/ExpressionPerformance" .
    " && mkdir -p \${log_root}/LogProxy/ExpressionPerformance_" .
    " && mkdir -p \${log_root}/LogProxy/ColoUsers" .
    " && mkdir -p \${log_root}/LogProxy/ColoUsers_" .
    " && mkdir -p \${log_root}/LogProxy/CCGStat" .
    " && mkdir -p \${log_root}/LogProxy/CCGStat_" .
    " && mkdir -p \${log_root}/LogProxy/CCGUserStat" .
    " && mkdir -p \${log_root}/LogProxy/CCGUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/CCStat" .
    " && mkdir -p \${log_root}/LogProxy/CCStat_" .
    " && mkdir -p \${log_root}/LogProxy/CCUserStat" .
    " && mkdir -p \${log_root}/LogProxy/CCUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/CampaignStat" .
    " && mkdir -p \${log_root}/LogProxy/CampaignStat_" .
    " && mkdir -p \${log_root}/LogProxy/CampaignUserStat" .
    " && mkdir -p \${log_root}/LogProxy/CampaignUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/AdvertiserUserStat" .
    " && mkdir -p \${log_root}/LogProxy/AdvertiserUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/CCGKeywordStat" .
    " && mkdir -p \${log_root}/LogProxy/CCGKeywordStat_" .
    " && mkdir -p \${log_root}/LogProxy/CCGSelectionFailureStat" .
    " && mkdir -p \${log_root}/LogProxy/CCGSelectionFailureStat_" .
    " && mkdir -p \${log_root}/LogProxy/CreativeStat" .
    " && mkdir -p \${log_root}/LogProxy/CreativeStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelPriceRange" .
    " && mkdir -p \${log_root}/LogProxy/ChannelPriceRange_" .
    " && mkdir -p \${log_root}/LogProxy/OptOutStat" .
    " && mkdir -p \${log_root}/LogProxy/OptOutStat_" .
    " && mkdir -p \${log_root}/LogProxy/WebStat" .
    " && mkdir -p \${log_root}/LogProxy/WebStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelTriggerStat" .
    " && mkdir -p \${log_root}/LogProxy/ChannelTriggerStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelTriggerImpStat" .
    " && mkdir -p \${log_root}/LogProxy/ChannelTriggerImpStat_" .
    " && mkdir -p \${log_root}/LogProxy/SiteChannelStat" .
    " && mkdir -p \${log_root}/LogProxy/SiteChannelStat_" .
    " && mkdir -p \${log_root}/LogProxy/SiteReferrerStat" .
    " && mkdir -p \${log_root}/LogProxy/SiteReferrerStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelHitStat" .
    " && mkdir -p \${log_root}/LogProxy/ChannelHitStat_" .
    " && mkdir -p \${log_root}/LogProxy/ActionStat" .
    " && mkdir -p \${log_root}/LogProxy/ActionStat_" .
    " && mkdir -p \${log_root}/LogProxy/ActionRequest" .
    " && mkdir -p \${log_root}/LogProxy/ActionRequest_" .
    " && mkdir -p \${log_root}/LogProxy/KeywordStat" .
    " && mkdir -p \${log_root}/LogProxy/KeywordStat_" .
    " && mkdir -p \${log_root}/LogProxy/PassbackStat" .
    " && mkdir -p \${log_root}/LogProxy/PassbackStat_" .
    " && mkdir -p \${log_root}/LogProxy/CMPStat" .
    " && mkdir -p \${log_root}/LogProxy/CMPStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelInventory" .
    " && mkdir -p \${log_root}/LogProxy/ChannelInventory_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelImpInventory" .
    " && mkdir -p \${log_root}/LogProxy/ChannelImpInventory_" .
    " && mkdir -p \${log_root}/LogProxy/PublisherInventory" .
    " && mkdir -p \${log_root}/LogProxy/PublisherInventory_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelInventoryEstimationStat" .
    " && mkdir -p \${log_root}/LogProxy/ChannelInventoryEstimationStat_" .
    " && mkdir -p \${log_root}/LogProxy/SearchEngineStat" .
    " && mkdir -p \${log_root}/LogProxy/SearchEngineStat_" .
    " && mkdir -p \${log_root}/LogProxy/SearchTermStat" .
    " && mkdir -p \${log_root}/LogProxy/SearchTermStat_" .
    " && mkdir -p \${log_root}/LogProxy/SiteStat" .
    " && mkdir -p \${log_root}/LogProxy/SiteStat_" .
    " && mkdir -p \${log_root}/LogProxy/SiteUserStat" .
    " && mkdir -p \${log_root}/LogProxy/SiteUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/UserAgentStat" .
    " && mkdir -p \${log_root}/LogProxy/UserAgentStat_" .
    " && mkdir -p \${log_root}/LogProxy/PageLoadsDailyStat" .
    " && mkdir -p \${log_root}/LogProxy/PageLoadsDailyStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelCountStat" .
    " && mkdir -p \${log_root}/LogProxy/ChannelCountStat_" .
    " && mkdir -p \${log_root}/LogProxy/TagAuctionStat" .
    " && mkdir -p \${log_root}/LogProxy/TagAuctionStat_" .
    " && mkdir -p \${log_root}/LogProxy/TagPositionStat" .
    " && mkdir -p \${log_root}/LogProxy/TagPositionStat_" .
    " && mkdir -p \${log_root}/LogProxy/CampaignReferrerStat" .
    " && mkdir -p \${log_root}/LogProxy/CampaignReferrerStat_" .
    " && mkdir -p \${log_root}/LogProxy/ColoUserStat" .
    " && mkdir -p \${log_root}/LogProxy/ColoUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/GlobalColoUserStat" .
    " && mkdir -p \${log_root}/LogProxy/GlobalColoUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/ChannelOverlapUserStat" .
    " && mkdir -p \${log_root}/LogProxy/ChannelOverlapUserStat_" .
    " && mkdir -p \${log_root}/LogProxy/ExtStat" .
    " && mkdir -p \${log_root}/LogProxy/ExtStat_" .
    " && mkdir -p \${log_root}/LogProxy/UserProperties/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelPerformance/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ExpressionPerformance/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ColoUsers/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CCGStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CCGUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CCStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CCUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CampaignStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CampaignUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/AdvertiserUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CCGKeywordStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CCGSelectionFailureStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CreativeStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelPriceRange/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/OptOutStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/WebStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelTriggerStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelTriggerImpStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/SiteChannelStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/SiteReferrerStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelHitStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ActionStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ActionRequest/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/KeywordStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/PassbackStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CMPStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelInventory/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelImpInventory/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/PublisherInventory/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelInventoryEstimationStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/SearchEngineStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/SearchTermStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/SiteStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/SiteUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/UserAgentStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/PageLoadsDailyStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelCountStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/TagAuctionStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/TagPositionStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/CampaignReferrerStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ColoUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/GlobalColoUserStat/Intermediate" .
    " && mkdir -p \${log_root}/LogProxy/ChannelOverlapUserStat/Intermediate" .
    " || exit -1 ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  return 1;
}

sub status
{
  return 1;
}

sub none
{
  return 1;
}

1;
