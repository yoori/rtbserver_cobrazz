package AdServer::CampaignSvcs::CampaignServer;

use strict;
use AdServer::Functions;
use AdServer::Path;


my $pid_file = "\${workspace_root}/run/CampaignServer.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/CampaignServer && " .
    "mkdir -p \${log_root}/CampaignServer/Out/ColoUpdateStat && " .
    "mkdir -p \${log_root}/CampaignServer/Out/ColoUpdateStat_ && " .
    "mkdir -p \${workspace_root}/run && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "CampaignServer") . " && " .
    "ulimit -n 4096 && " .
    "{ setsid -f \${VALGRIND_PREFIX} CampaignServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/CampaignServerConfig.xml " .
      "> \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}CampaignServer.out 2>&1 < /dev/null ; } ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "CampaignServer");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "CampaignServer"));
}

sub db_status
{
  my ($host, $descr) = @_;
  return is_alive @_;
}

1;
