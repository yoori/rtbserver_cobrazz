package AdServer::LogProcessing::CleanupLogs;

use strict;
use warnings;
use Utils::Functions;
use AdServer::Functions;

my $pid_file = "\${workspace_root}/run/cleanup_logs.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/CleanupLogs && "
    ."date  +'%a %d %m %Y %H:%M:%S:%N : start logs cleanup' "
    .">> \${workspace_root}/log/CleanupLogs/CleanupLogs.log && "
    ."{ CleanupLogs.pl -conf \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/cleanup_logs.conf "
    .">> \${workspace_root}/log/CleanupLogs/CleanupLogs.log 2>&1 < /dev/null &}";

  $command = AdServer::Functions::pidfile_start_guard($pid_file, "CleanupLogs.pl") .
    " && " . $command;

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "CleanupLogs.pl");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "CleanupLogs.pl"));
}

1;
