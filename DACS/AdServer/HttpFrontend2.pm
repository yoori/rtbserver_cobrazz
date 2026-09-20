package AdServer::HttpFrontend2;

use strict;
use Errno;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/nginx2.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/tmp \${log_root} && " .
    "mkdir -p \${workspace_root}/log/nginx2 && " .
    "ulimit -n 1024000 && " .
    "/usr/sbin/nginx -c \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf2/nginx.conf " .
      "-p \${workspace_root}/log/nginx2/ " .
      ">> \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}nginx2.out 2>&1";

  $command = AdServer::Functions::pidfile_start_guard($pid_file, "/usr/sbin/nginx") .
    " && " . $command;

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "/usr/sbin/nginx");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "/usr/sbin/nginx"));
}

1;
