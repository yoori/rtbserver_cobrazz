package AdServer::Frontends::ProfilingServer;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $profiling_server_port = "profiling_server_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/ProfilingServer && " .
    "ulimit -n 16000 && " .
    "export MALLOC_ARENA_MAX=1 && " .
    "{ ProfilingServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ProfilingServerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ProfilingServer.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $profiling_server_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $profiling_server_port,
    $verbose);
}

1;
