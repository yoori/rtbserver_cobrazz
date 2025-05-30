package AdServer::Frontends::FCGITrackServer4;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $fcgi_trackserver4_port = "fcgi_trackserver4_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/FCGITrackServer4 && ".
    "ulimit -n 64000 && " .
    "ulimit -s 100000 && " .
    "export MALLOC_ARENA_MAX=4 && " .
    #"{ valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes FCGIServer " .
    "{ FCGIServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGITrackServer4Config.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}FCGITrackServer4.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $fcgi_trackserver4_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $fcgi_trackserver4_port,
    $verbose);
}

1;
