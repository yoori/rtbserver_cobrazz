package AdServer::Frontends::FCGIRtbServer;

use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/FCGIRtbServer.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    AdServer::Functions::prepare_ram_log_dirs("FCGIRtbServer", "Geo") . " && " .
    "mkdir -p \${log_root}/FCGIRtbServer/Out/Geo \${workspace_root}/run && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "FCGIServer") . " && " .
    "ulimit -s 100000 && " .
    "ulimit -n 256000 && " .
    "export MALLOC_CONF=narenas:64,background_thread:true,dirty_decay_ms:5000,muzzy_decay_ms:5000 && " .
    AdServer::Functions::thread_affinity_env(
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGIRtbServerConfig.xml",
      "FCGIServerConfig",
      "FCGIRtbServer") .
    "setsid -f \${CONTROL_CPU_AFFINITY} \${VALGRIND_PREFIX} FCGIServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGIRtbServerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}FCGIRtbServer.out 2>&1 < /dev/null";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "FCGIServer");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "FCGIServer"));
}

1;
