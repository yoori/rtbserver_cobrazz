package AdServer::CampaignSvcs::CampaignServer;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;


my $service_port = "campaign_server_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/CampaignServer && " .
    "mkdir -p \${log_root}/CampaignServer/Out/ColoUpdateStat && " .
    "mkdir -p \${log_root}/CampaignServer/Out/ColoUpdateStat_ && " .
    "ulimit -n 4096 && " .
    "{ \${VALGRIND_PREFIX} CampaignServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/CampaignServerConfig.xml " .
      "> \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}CampaignServer.out 2>&1 & } ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $service_port);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $service_port);
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
