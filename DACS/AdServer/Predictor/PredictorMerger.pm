package AdServer::Predictor::Merger;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/predictor_merger.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor/ResearchLogs/PRImpression && " .
    "mkdir -p \${log_root}/Predictor/Merger && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ rm -f \${workspace_root}/run/predictor_merger.pid && ".
    "PredictorMerger.pl " .
      "--config-file-path=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/PredictorMergerConfig.xml " .
      "--work-dir=\${workspace_root}/${AdServer::Path::OUT_FILE_BASE} " .
      "--pid-file-path=\${workspace_root}/run/predictor_merger.pid start " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}PredictorMerger.out 2>&1 < /dev/null & }";

  $command = AdServer::Functions::pidfile_start_guard($pid_file, "PredictorMerger.pl") .
    " && " . $command;

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "PredictorMerger.pl");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "PredictorMerger.pl"));
}

1;
