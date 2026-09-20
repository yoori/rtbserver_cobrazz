package AdServer::LogProcessing::StatReceiver;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/StatReceiver.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/StatReceiver/Out/ExtStat && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ rm -f \${workspace_root}/run/StatReceiver.pid && ".
    "/usr/bin/rsync --daemon --config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/StatReceiver.conf " .
    " 2>>\${workspace_root}/run/StatReceiver.out 1>&2 & } ";

  $command = AdServer::Functions::pidfile_start_guard($pid_file, "/usr/bin/rsync") .
    " && " . $command;

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "/usr/bin/rsync");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "/usr/bin/rsync"));
}

1;
