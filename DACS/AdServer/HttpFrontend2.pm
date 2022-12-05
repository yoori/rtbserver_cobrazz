package AdServer::HttpFrontend2;

use strict;
use Errno;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/tmp \${log_root} && " .
    "mkdir -p \${workspace_root}/log/nginx2 && " .
    "ulimit -n 64000 && " .
    "/usr/sbin/nginx -c \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf2/nginx.conf " .
      "-p \${workspace_root}/log/nginx2/ " .
      ">> \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}nginx2.out 2>&1";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  Utils::Functions::init_environment();

  my $command =
    "/usr/sbin/nginx -c \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf2/nginx.conf -s stop " .
    " >> \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}nginx2.out 2>&1";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $environment_dir = Utils::Functions::init_environment();

  my $command =
    "source $environment_dir/environment.sh && " .
    "pid=`cat \${workspace_root}/run/nginx2.pid 2>/dev/null`; test -n \"\$pid\" && test \"`ps -p \$pid -o comm= 2>/dev/null`\" = nginx";

  my $ret = Utils::Functions::safe_system($command);

  return $ret == 0;
}

1;
