package AdServer::LogProcessing::STunnel;

use strict;
use Utils::Functions;
use AdServer::Functions;

my $pid_file = "\${workspace_root}/run/stunnel.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "{ " .
         "mkdir -p \${workspace_root}/log/STunnelClient && " .
         "/usr/bin/stunnel \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/stunnel_client.conf 2>&1 " .
         "| RotateLog --size 100 --time 1440 --cron 00:00 \${workspace_root}/log/STunnelClient/STunnelClient.log; " .
       "} ";

  $command = AdServer::Functions::pidfile_start_guard($pid_file, "/usr/bin/stunnel") .
    " && " . $command;

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "/usr/bin/stunnel");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "/usr/bin/stunnel"));
}

1;
