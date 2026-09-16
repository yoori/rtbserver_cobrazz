package AdServer::LogProcessing::Telegraf;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/Telegraf.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "test -x /opt/foros/server/bin/telegraf || " .
      "{ echo 'foros-telegraf is not installed' >&2; exit 1; } && " .
    "mkdir -p \${workspace_root}/run && " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi && " .
    "{ setsid -f /opt/foros/server/bin/telegraf " .
      "--config \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/Telegraf.conf " .
      "--pidfile $pid_file " .
      "> \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}Telegraf.out 2>&1 < /dev/null; }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file);
}

sub is_alive
{
  my ($host, $descr) = @_;
  my $command =
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && kill -0 \$pid 2>/dev/null";
  return AdServer::Functions::execute_command($host, $descr, $command);
}

1;
