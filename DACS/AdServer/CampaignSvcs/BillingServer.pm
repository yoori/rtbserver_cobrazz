package AdServer::CampaignSvcs::BillingServer;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $billing_server_port = "billing_server_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/BillingServer/In/BillOperation && " .
    "mkdir -p \${cache_root}/BillingServer && " .
    "{ BillingServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/BillingServer.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}BillingServer.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $billing_server_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $billing_server_port,
    $verbose);
}

1;
