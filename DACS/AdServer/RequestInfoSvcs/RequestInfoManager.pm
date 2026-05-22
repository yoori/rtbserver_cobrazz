package AdServer::RequestInfoSvcs::RequestInfoManager;

use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/RequestInfoManager.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/RequestInfoManager/In/Request && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/Request/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/Impression && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/Impression/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/Click && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/Click/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/PassbackImpression && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/PassbackImpression/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/AdvertiserAction && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/AdvertiserAction/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/TagRequest && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/TagRequest/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/RequestOperation && " .
    "mkdir -p \${log_root}/RequestInfoManager/In/RequestOperation/Intermediate && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CreativeStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CreativeStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteChannelStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteChannelStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCGKeywordStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCGKeywordStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/KeywordStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/KeywordStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CMPStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CMPStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/UserProperties && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/UserProperties_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ChannelPerformance && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ChannelPerformance_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ExpressionPerformance && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ExpressionPerformance_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ActionStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ActionStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCGUserStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCGUserStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCUserStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCUserStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CampaignUserStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CampaignUserStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/AdvertiserUserStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/AdvertiserUserStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/PassbackStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/PassbackStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ChannelImpInventory && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ChannelImpInventory_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteUserStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteUserStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteReferrerStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteReferrerStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/PageLoadsDailyStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/PageLoadsDailyStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/TagPositionStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/TagPositionStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CampaignReferrerStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CampaignReferrerStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchAction && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchAction_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchBid && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchBid_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchImpression && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchImpression_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchClick && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ResearchClick_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/RequestOperation && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/RequestOperation_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderAction && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderAction_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderClick && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderClick_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderImpression && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderImpression_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderRequest && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/ConsiderRequest_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/BidCostStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/BidCostStat_ && " .
    "mkdir -p \${cache_root}/UserActions && " .
    "mkdir -p \${cache_root}/UserCampaignReach && " .
    "mkdir -p \${cache_root}/Requests && " .
    "mkdir -p \${cache_root}/Bid && " .
    "mkdir -p \${cache_root}/Passback && " .
    "mkdir -p \${cache_root}/UserFraudProtection && " .
    "mkdir -p \${cache_root}/UserSiteReach && " .
    "mkdir -p \${cache_root}/UserTagRequestGroup && " .
    "mkdir -p \${cache_root}/RequestInfoManager && " .
    # compatibility 2.3 block (SyncLogs try to move files from these folders)
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/SiteStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCGStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCGStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CCStat_ && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CampaignStat && " .
    "mkdir -p \${log_root}/RequestInfoManager/Out/CampaignStat_ && " .

    "mkdir -p \${workspace_root}/run && " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi && " .
    "ulimit -n 16000 && " .
    "export MALLOC_ARENA_MAX=2 && " .
    "{ " .
      "setsid -f \${VALGRIND_PREFIX} RequestInfoManager " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/RequestInfoManagerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}RequestInfoManager.out 2>&1 < /dev/null ; " .
    "} ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && " .
    "kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 1; } && " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

1;
