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
   "if test -e $pid_file; then " .
     "pid=`cat $pid_file`; " .
     "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
   "fi && " .
   "{ " .
   AdServer::Functions::thread_affinity_env(
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindServerConfig.xml",
     "UserBindServerConfig") .
   "setsid -f \${CONTROL_CPU_AFFINITY} \${VALGRIND_PREFIX} UserBindServer " .
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindServer.xml > " .
     "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserBindServer.out 2>&1 < /dev/null ; " .
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && " .
    "kill -0 \$pid 2>/dev/null || exit 1; " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

1;
