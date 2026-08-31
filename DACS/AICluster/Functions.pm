package AICluster::Functions;

use strict;

sub execute_command
{
  my ($command) = @_;
  my $result = system('/bin/bash', '-c', $command);
  return $result == 0 ? 1 : 0;
}

sub stop_by_pidfile
{
  my ($pid_file) = @_;
  my $command =
    "test -e $pid_file || exit 0; " .
    "pid=`cat $pid_file`; " .
    "kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 0; }; " .
    "kill -TERM \$pid 2>/dev/null || true; " .
    "for i in `seq 1 60`; do " .
      "test -e $pid_file || exit 0; " .
      "kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 0; }; " .
      "sleep 1; " .
    "done; " .
    "exit 1";
  return execute_command($command);
}

1;
