package AdServer::UserInfoSvcs::UserBindController2;

use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/UserBindController2.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/UserBindController2 \${workspace_root}/run && " .
   "rm -f $pid_file && " .
   "{ ".
     "\${VALGRIND_PREFIX} UserBindController2 " .
       "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindController2.xml > " .
       "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserBindController2.out 2>&1 & " .
     "echo \$! > $pid_file; ".
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 0 && " .
    "kill -9 `cat $pid_file` 2>/dev/null || true; " .
    "rm -f $pid_file";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "kill -0 `cat $pid_file` 2>/dev/null || exit 1 && " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

1;
