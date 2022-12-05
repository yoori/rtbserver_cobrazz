package AdServer::Frontends::FCGIRtbServer2;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $fcgi_rtbserver2_port = "fcgi_rtbserver2_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/FCGIRtbServer2 && ".
    "ulimit -n 64000 && " .
    "ulimit -s 100000 && " .
    "export MALLOC_ARENA_MAX=4 && " .
    "{ FCGIServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGIRtbServer2Config.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}FCGIRtbServer2.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $fcgi_rtbserver2_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $fcgi_rtbserver2_port,
    $verbose);
}

1;
