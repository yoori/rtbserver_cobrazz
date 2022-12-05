package AdServer::Frontends::FCGITrackServer;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $fcgi_trackserver_port = "fcgi_trackserver_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/FCGITrackServer && ".
    "ulimit -n 64000 && " .
    "ulimit -s 100000 && " .
    "export MALLOC_ARENA_MAX=4 && " .
    #"{ valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes FCGIServer " .
    "{ FCGIServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGITrackServerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}FCGITrackServer.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $fcgi_trackserver_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $fcgi_trackserver_port,
    $verbose);
}

1;
