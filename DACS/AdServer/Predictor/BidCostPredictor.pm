package AdServer::Predictor::BidCostPredictor;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor/ResearchLogs/BidCostStat && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/BidCostStatAgg && " .
    "mkdir -p \${log_root}/Predictor/BidCostPredictor && " .
    "mkdir -p \${log_root}/Predictor/BidCostConfig && " .
    "mkdir -p \${log_root}/Predictor/BidCostConfigTmp && " .
    "mkdir -p \${log_root}/Predictor/CTRTrivialConfig && " .
    "mkdir -p \${log_root}/Predictor/CTRTrivialConfigTmp && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ " .
      "rm -f \${workspace_root}/run/bid_cost_predictor.pid && " .
      "BidCostPredictor service start " .
        "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BidCostPredictorConfig.json " .
        ">\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}bid_cost_predictor.out 2>&1 & " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "BidCostPredictor service stop " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BidCostPredictorConfig.json " .
      ">\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}bid_cost_predictor.out 2>&1 ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "BidCostPredictor service status " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BidCostPredictorConfig.json " .
      ">\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}bid_cost_predictor.out 2>&1 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);
  if ($res != 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

1;