package AdServer::UserInfoSvcs::UserInfoController;

use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/UserInfoController.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/UserInfoController \${workspace_root}/run && " .
   "if test -e $pid_file; then " .
     "pid=`cat $pid_file`; " .
     "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
   "fi && " .
   "\${VALGRIND_PREFIX} UserInfoController " .
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserInfoController.xml > " .
     "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserInfoController.out 2>&1 < /dev/null &";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 0 && " .
    "pid=`cat $pid_file` && " .
    "{ kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 0; }; } && " .
    "kill -TERM \$pid 2>/dev/null || true; " .
    "for i in `seq 1 60`; do " .
      "test -e $pid_file || exit 0; " .
      "kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 0; }; " .
      "sleep 1; " .
    "done; " .
    "kill -9 \$pid 2>/dev/null || true; " .
    "test -e $pid_file && rm -f $pid_file || true";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && " .
    "kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 1; } && " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

1;
