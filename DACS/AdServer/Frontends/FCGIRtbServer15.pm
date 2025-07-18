package AdServer::Frontends::FCGIRtbServer15;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $fcgi_rtbserver15_port = "fcgi_rtbserver15_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/FCGIRtbServer15 && ".
    "ulimit -n 64000 && " .
    "ulimit -s 100000 && " .
    "export MALLOC_ARENA_MAX=4 && " .
    "{ FCGIServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGIRtbServer15Config.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}FCGIRtbServer15.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $fcgi_rtbserver15_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $fcgi_rtbserver15_port,
    $verbose);
}

1;
