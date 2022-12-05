package AdServer::Predictor::Merger;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor/ResearchLogs/PRImpression && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ rm -f \${workspace_root}/run/predictor_merger.pid && ".
    "PredictorMerger.pl " .
      "--config-file-path=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/PredictorMergerConfig.xml " .
      "--work-dir=\${workspace_root}/${AdServer::Path::OUT_FILE_BASE} " .
      "--pid-file-path=\${workspace_root}/run/predictor_merger.pid start " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}PredictorMerger.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "PredictorMerger.pl " .
    "--config-file-path=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/PredictorMergerConfig.xml " .
    "--work-dir=\${workspace_root}/${AdServer::Path::OUT_FILE_BASE} " .
    "--pid-file-path=\${workspace_root}/run/predictor_merger.pid stop " .
    " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}PredictorMerger.out 2>&1 &";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/run/predictor_merger.pid || exit 1 && " . 
    "kill -0 \`cat \${workspace_root}/run/predictor_merger.pid\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
