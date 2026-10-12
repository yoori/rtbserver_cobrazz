package AdServer::RequestInfoSvcs::UrlInfoStorage;

use strict;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/UrlInfoStorage.pid";

sub start
{
  my ($host, $descr) = @_;
  my $command =
    "mkdir -p \${log_root}/UrlInfoStorage \${workspace_root}/run && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "UrlInfoStorage") . " && " .
    "{ setsid -f \${VALGRIND_PREFIX} UrlInfoStorage " .
    "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UrlInfoStorage.xml > " .
    "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UrlInfoStorage.out 2>&1 < /dev/null ; }";
  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "UrlInfoStorage");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "UrlInfoStorage"));
}

1;
