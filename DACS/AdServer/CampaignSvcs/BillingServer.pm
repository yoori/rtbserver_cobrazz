package AdServer::CampaignSvcs::BillingServer;

use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/BillingServer.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/BillingServer/In/BillOperation && " .
    "mkdir -p \${cache_root}/BillingServer && " .
    "export MALLOC_CONF=narenas:64,background_thread:true,dirty_decay_ms:5000,muzzy_decay_ms:5000 && " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi && " .
    "setsid -f \${CONTROL_CPU_AFFINITY} \${VALGRIND_PREFIX} BillingServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BillingServer.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}BillingServer.out 2>&1 < /dev/null";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && " .
    "kill -0 \$pid 2>/dev/null || exit 1; " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $verbose, $command);
}

1;
