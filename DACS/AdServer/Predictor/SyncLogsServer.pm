package AdServer::Predictor::SyncLogsServer;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor/CTRConfig && " .
    "mkdir -p \${log_root}/Predictor/ConvConfig && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/ResearchBid && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/ResearchImpression && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/ResearchClick && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/ResearchAction && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/ResearchWebStat && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/ResearchProf && " .
    "mkdir -p \${log_root}/Predictor/ResearchLogs/BidCostStat && " .
    "mkdir -p \${log_root}/Predictor/Backup/ResearchBid && " .
    "mkdir -p \${log_root}/Predictor/Backup/ResearchImpression && " .
    "mkdir -p \${log_root}/Predictor/Backup/ResearchClick && " .
    "mkdir -p \${log_root}/Predictor/Backup/ResearchAction && " .
    "mkdir -p \${log_root}/Predictor/Backup/ResearchWebStat && " .
    "mkdir -p \${log_root}/Predictor/Backup/ResearchProf && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ rm -f \${workspace_root}/run/predictor_synclogsserver.pid && ".
    "/usr/bin/rsync --daemon --config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/predictor_synclogs_server.conf " .
    " 2>>\${workspace_root}/run/predictor_synclogsserver.run 1>&2 & } ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test \${workspace_root}/run/predictor_synclogsserver.pid || exit 0 &&".
    "kill `cat \${workspace_root}/run/predictor_synclogsserver.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/run/predictor_synclogsserver.pid || exit 1 && " . 
    "kill -0 \`cat \${workspace_root}/run/predictor_synclogsserver.pid\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
