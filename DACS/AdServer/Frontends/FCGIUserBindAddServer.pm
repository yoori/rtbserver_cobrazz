package AdServer::Frontends::FCGIUserBindAddServer;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $fcgi_userbindaddserver_port = "fcgi_userbindaddserver_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/FCGIUserBindAddServer && ".
    "ulimit -s 100000 && " .
    "ulimit -n 32000 && " .
    "export MALLOC_ARENA_MAX=4 && " .
    #"{ valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes FCGIServer " .
    "{ FCGIServer " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/FCGIUserBindAddServerConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}FCGIUserBindAddServer.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $fcgi_userbindaddserver_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $fcgi_userbindaddserver_port,
    $verbose);
}

1;
