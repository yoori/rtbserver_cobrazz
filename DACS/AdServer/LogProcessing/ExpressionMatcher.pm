package AdServer::LogProcessing::ExpressionMatcher;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $expression_matcher_port = "expression_matcher_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/ExpressionMatcher/In/RequestBasicChannels && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/RequestBasicChannels/Error && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/RequestBasicChannels/Intermediate && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderAction && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderAction/Error && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderAction/Intermediate && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderClick && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderClick/Error && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderClick/Intermediate && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderImpression && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderImpression/Error && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderImpression/Intermediate && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderRequest && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderRequest/Error && " .
   "mkdir -p \${log_root}/ExpressionMatcher/In/ConsiderRequest/Intermediate && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelImpInventory && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelImpInventory_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelInventory && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelInventory_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelPriceRange && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelPriceRange_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelInventoryEstimationStat && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelInventoryEstimationStat_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelPerformance && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelPerformance_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelTriggerImpStat && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelTriggerImpStat_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/GlobalColoUserStat && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/GlobalColoUserStat_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ColoUserStat && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ColoUserStat_ && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelOverlapUserStat && " .
   "mkdir -p \${log_root}/ExpressionMatcher/Out/ChannelOverlapUserStat_ && " .
   "mkdir -p \${cache_root}/ExpressionMatcher && " .
   "mkdir -p \${cache_root}/Estimation && " .
   "mkdir -p \${cache_root}/RequestTriggerMatch && " .
   "ulimit -n 4096 && " .
   "export MALLOC_ARENA_MAX=4 && " .
   "{ " .
     "\${VALGRIND_PREFIX} ExpressionMatcher " .
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ExpressionMatcherConfig.xml " .
     " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ExpressionMatcher.out 2>&1 & " .
   "} ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $expression_matcher_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $expression_matcher_port,
    $descr);
}

1;
