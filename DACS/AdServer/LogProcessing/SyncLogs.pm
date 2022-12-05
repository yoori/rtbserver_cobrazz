package AdServer::LogProcessing::SyncLogs;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $sync_logs_port = "sync_logs_port";

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
    "ulimit -n 4096 && " .
    "{ \${VALGRIND_PREFIX} SyncLogs " .
    "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/SyncLogsConfig.xml" .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}SyncLogs.out 2>&1 & } ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $sync_logs_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $sync_logs_port,
    $descr);
}

1;
