package AdServer::LogProcessing::SyncLogsServer;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/synclogsserver.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/SyncLogsServer && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ rm -f \${workspace_root}/run/synclogsserver.pid && ".
    "/usr/bin/rsync --daemon --config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/synclogs_server.conf " .
    " 2>>\${workspace_root}/run/synclogsserver.run 1>&2 & } ";

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
