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
    AdServer::Functions::pidfile_start_guard($pid_file, "BillingServer") . " && " .
    "setsid -f \${CONTROL_CPU_AFFINITY} \${VALGRIND_PREFIX} BillingServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BillingServer.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}BillingServer.out 2>&1 < /dev/null";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "BillingServer");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "BillingServer"));
}

1;
