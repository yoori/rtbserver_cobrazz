package AdServer::CampaignSvcs::CampaignManager;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/CampaignManager.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/run && ".
    "mkdir -p \${log_root}/CampaignManager/Out/RequestBasicChannels && ".
    "mkdir -p \${log_root}/CampaignManager/Out/RequestBasicChannels_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/OptOutStat && ".
    "mkdir -p \${log_root}/CampaignManager/Out/OptOutStat_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/WebStat && ".
    "mkdir -p \${log_root}/CampaignManager/Out/WebStat_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ResearchWebStat && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ResearchWebStat_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ResearchProf && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ResearchProf_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ChannelTriggerStat && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ChannelTriggerStat_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ChannelHitStat && ".
    "mkdir -p \${log_root}/CampaignManager/Out/ChannelHitStat_ && ".
    "mkdir -p \${log_root}/CampaignManager/Out/CreativeStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/CreativeStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/ColoUsers && " .
    "mkdir -p \${log_root}/CampaignManager/Out/ColoUsers_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/KeywordStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/KeywordStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/ActionRequest && " .
    "mkdir -p \${log_root}/CampaignManager/Out/ActionRequest_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PassbackStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PassbackStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/Request && " .
    "mkdir -p \${log_root}/CampaignManager/Out/Request_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/Impression && " .
    "mkdir -p \${log_root}/CampaignManager/Out/Impression_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/Click && " .
    "mkdir -p \${log_root}/CampaignManager/Out/Click_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/AdvertiserAction && " .
    "mkdir -p \${log_root}/CampaignManager/Out/AdvertiserAction_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PassbackOpportunity && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PassbackOpportunity_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PassbackImpression && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PassbackImpression_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PublisherInventory && " .
    "mkdir -p \${log_root}/CampaignManager/Out/PublisherInventory_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/UserProperties && " .
    "mkdir -p \${log_root}/CampaignManager/Out/UserProperties_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/UserAgentStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/UserAgentStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/TagRequest && " .
    "mkdir -p \${log_root}/CampaignManager/Out/TagRequest_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/CCGStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/CCGStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/CCStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/CCStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/TagAuctionStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/TagAuctionStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/UserAgentStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/UserAgentStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/SearchTermStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/SearchTermStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/SearchEngineStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/SearchEngineStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/Out/TagPositionStat && " .
    "mkdir -p \${log_root}/CampaignManager/Out/TagPositionStat_ && " .
    "mkdir -p \${log_root}/CampaignManager/In/CTRConfig && " .
    "mkdir -p \${log_root}/CampaignManager/In/CapturedCTRConfig && " .
    "mkdir -p \${log_root}/CampaignManager/In/ConvRateConfig && " .
    "mkdir -p \${log_root}/CampaignManager/In/CapturedConvRateConfig && " .
    "mkdir -p \${log_root}/CampaignManager/In/BidCostConfig && " .
    "mkdir -p \${log_root}/CampaignManager/In/CapturedBidCostConfig && " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi && " .
    "ulimit -n 4096 && " .
    "export MALLOC_CONF=narenas:64,background_thread:true,dirty_decay_ms:5000,muzzy_decay_ms:5000 && " .
    "setsid -f \${VALGRIND_PREFIX} CampaignManager " .
    #"{ scl enable devtoolset-8 -- valgrind --tool=callgrind CampaignManager " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/CampaignManagerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}CampaignManager.out 2>&1 < /dev/null";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::stop_by_pidfile($host, $verbose, $pid_file);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && " .
    "kill -0 \$pid 2>/dev/null || exit 1; " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $verbose, $command);
}

1;
