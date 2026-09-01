package AICluster::SegmentModelViewer;

use strict;
use AICluster::Functions;

my $pid_file = "\${workspace_root}/run/SegmentModelViewer.pid";

sub start
{
  my ($host, $description) = @_;
  my $config_file = "\${config_root}/$host/SegmentModelViewerConfig.json";
  my $command =
    "mkdir -p \${workspace_root}/run; " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi; " .
    "setsid -f SegmentModelViewer.py --config=\"$config_file\" " .
      "> \${workspace_root}/run/SegmentModelViewer.out 2>&1 < /dev/null; " .
    "for i in `seq 1 50`; do " .
      "test -e $pid_file && pid=`cat $pid_file` && " .
        "kill -0 \$pid 2>/dev/null && exit 0; " .
      "sleep 0.1; " .
    "done; " .
    "exit 1";
  return AICluster::Functions::execute_command($command);
}

sub stop
{
  my ($host, $description) = @_;
  return AICluster::Functions::stop_by_pidfile($pid_file);
}

sub is_alive
{
  my ($host, $description) = @_;
  return AICluster::Functions::execute_command(
    "test -e $pid_file || exit 1; " .
    "pid=`cat $pid_file`; " .
    "kill -0 \$pid 2>/dev/null");
}

1;
