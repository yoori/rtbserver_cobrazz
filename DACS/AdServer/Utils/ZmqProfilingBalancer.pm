package AdServer::Utils::ZmqProfilingBalancer;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $zmq_profiling_balancer_port = "zmq_profiling_balancer_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/ZmqProfilingBalancer && ".
    "ulimit -n 16000 && " .
    "{ ZmqBalancer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ZmqProfilingBalancerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ZmqProfilingBalancer.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $zmq_profiling_balancer_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $zmq_profiling_balancer_port,
    $verbose);
}

1;
