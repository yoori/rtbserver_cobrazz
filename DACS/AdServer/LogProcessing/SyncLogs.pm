package AdServer::LogProcessing::SyncLogs;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/SyncLogs.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/SyncLogs && " .
    "mkdir -p \${data_root}/tags && " .
    "mkdir -p \${data_root}/Creatives && " .
    "mkdir -p \${data_root}/Templates && " .
    "mkdir -p \${data_root}/Publ && " .
    "mkdir -p \${data_root}/WebwiseDiscover/common && " .
    "mkdir -p \${data_root}/WebwiseDiscover/templates && " .
    "mkdir -p \${data_root}/Discover/Customizations && " .
    "mkdir -p \${workspace_root}/run && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "SyncLogs") . " && " .
    "ulimit -n 4096 && " .
    "{ \${VALGRIND_PREFIX} SyncLogs " .
    "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/SyncLogsConfig.xml" .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}SyncLogs.out 2>&1 < /dev/null & } ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "SyncLogs");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "SyncLogs"));
}

1;
