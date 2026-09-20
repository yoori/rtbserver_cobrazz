package AdServer::UserInfoSvcs::UserBindServer;

use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/UserBindServer.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/UserBindServer \${workspace_root}/run && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_1 && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_1/Intermediate && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_2 && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_2/Intermediate && " .
   "mkdir -p \${log_root}/UserBindServer/Out/UserBindOp_1 && " .
   "mkdir -p \${log_root}/UserBindServer/Out/UserBindOp_2 && " .
   "ulimit -n 16384 && " .
   "export MALLOC_CONF=narenas:64,background_thread:true,dirty_decay_ms:5000,muzzy_decay_ms:5000 && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "UserBindServer") . " && " .
   "{ " .
   AdServer::Functions::thread_affinity_env(
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindServer.xml",
     "UserBindServerConfig",
     "UserBindServer") .
   "setsid -f \${CONTROL_CPU_AFFINITY} \${VALGRIND_PREFIX} UserBindServer " .
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindServer.xml > " .
     "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserBindServer.out 2>&1 < /dev/null ; " .
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "UserBindServer");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "UserBindServer"));
}

1;
