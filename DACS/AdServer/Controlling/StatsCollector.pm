package AdServer::Controlling::StatsCollector;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $stats_collector_port = "stats_collector_port";

sub start
{
  my ($host, $descr) = @_;

  Utils::Functions::init_environment();

  my $command =
       "mkdir -p \${log_root}/StatsCollector && " .
       "{ ".
         "\${VALGRIND_PREFIX} StatsCollector " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/StatsCollector.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}StatsCollector.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $stats_collector_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $stats_collector_port,
    $descr);
}

1;
