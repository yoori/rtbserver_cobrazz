package AdServer::Predictor::BidCostPredictModelGenerator;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/bidcost_predictor_merger.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor/ResearchLogs/BidCostStat && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/BidCostStatAgg && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/BidCostModel && " .
    "mkdir -p \${log_root}/Predictor/BidCostPredictorMerger && " .
    "mkdir -p \${log_root}/Predictor/BidCostConfig && " .
    "mkdir -p \${log_root}/Predictor/BidCostConfigTmp && " .
    "mkdir -p \${log_root}/Predictor/CTRTrivialConfig && " .
    "mkdir -p \${log_root}/Predictor/CTRTrivialConfigTmp && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ " .
      "rm -f \${workspace_root}/run/bidcost_predictor_merger.pid && " .
      "BidCostPredictorMerger.pl " .
        "--config-file-path=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BidCostPredictorMergerConfig.xml " .
        "--pid-file-path=\${workspace_root}/run/bidcost_predictor_merger.pid start " .
        ">\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}BidCostPredictorMerger.out 2>&1 < /dev/null & " .
    "}";

  $command = AdServer::Functions::pidfile_start_guard($pid_file, "BidCostPredictorMerger.pl") .
    " && " . $command;

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile(
    $host, $descr, $pid_file, "BidCostPredictorMerger.pl");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "BidCostPredictorMerger.pl"));
}

1;
