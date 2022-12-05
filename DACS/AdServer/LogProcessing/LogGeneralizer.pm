package AdServer::LogProcessing::LogGeneralizer;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $service_port = "log_generalizer_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/LogGeneralizer/In/UserProperties/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelPerformance/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ExpressionPerformance/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ColoUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/GlobalColoUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ColoUsers/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CreativeStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CreativeStat/Deferred/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/OptOutStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/SiteReferrerStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/SiteChannelStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CCGKeywordStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CCGSelectionFailureStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CCGStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CCStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CCGUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CCUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CampaignStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CampaignUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/AdvertiserUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelInventory/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelImpInventory/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelPriceRange/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelTriggerStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelTriggerImpStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelHitStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelCountStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ActionStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ActionRequest/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/KeywordStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CMPStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CMPStat/Deferred/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ColoUpdateStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/PassbackStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/WebStat/Intermediate/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelInventoryEstimationStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/PublisherInventory/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/SiteStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/SiteUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/TagAuctionStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/PageLoadsDailyStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/ChannelOverlapUserStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/DeviceChannelCountStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/UserAgentStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/SearchTermStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/SearchEngineStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/TagPositionStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/In/CampaignReferrerStat/Intermediate && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ExtStat/ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ExtStat_/ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/UserProperties && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/UserProperties_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelPerformance && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelPerformance_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ExpressionPerformance && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ExpressionPerformance_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ColoUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ColoUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/GlobalColoUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/GlobalColoUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ColoUsers && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ColoUsers_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CreativeStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CreativeStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/OptOutStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/OptOutStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteReferrerStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteReferrerStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteChannelStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteChannelStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGKeywordStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGKeywordStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGSelectionFailureStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGSelectionFailureStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCGUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CCUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CampaignStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CampaignStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CampaignUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CampaignUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/AdvertiserUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/AdvertiserUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelInventory && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelInventory_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelImpInventory && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelImpInventory_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelPriceRange && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelPriceRange_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelTriggerStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelTriggerStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelTriggerImpStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelTriggerImpStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelHitStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelHitStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelCountStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelCountStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ActionStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ActionStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ActionRequest && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ActionRequest_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/KeywordStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/KeywordStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CMPStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CMPStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ColoUpdateStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ColoUpdateStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/PassbackStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/PassbackStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/WebStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/WebStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelInventoryEstimationStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelInventoryEstimationStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/PublisherInventory && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/PublisherInventory_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SiteUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/PageLoadsDailyStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/PageLoadsDailyStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelOverlapUserStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/ChannelOverlapUserStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/DeviceChannelCountStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/DeviceChannelCountStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/UserAgentStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/UserAgentStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SearchTermStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SearchTermStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SearchEngineStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/SearchEngineStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/TagPositionStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/TagPositionStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CampaignReferrerStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/CampaignReferrerStat_ && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/TagAuctionStat && " .
       "mkdir -p \${log_root}/LogGeneralizer/Out/TagAuctionStat_ && " .
       "mkdir -p \${cache_root}/SearchPhrases && " .
       "{ " .
         "\${VALGRIND_PREFIX} LogGeneralizer " .
         "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/LogGeneralizerConfig.xml " .
         " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}LogGeneralizer.out 2>&1 & " .
       "} ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host, $service_port, $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $service_port,
    $descr);
}

sub db_status
{
  my ($host, $descr) = @_;
  my $status = is_alive @_;
  if($status == 1)
  {
    Utils::Functions::process_control_any(
      $host,
      $service_port,
      $host . "/" . ${AdServer::Path::ADSERVER_CURRENTENV_DIR},
      "-control DB -1",
      $descr);
  }
  return $status;
}

1;
