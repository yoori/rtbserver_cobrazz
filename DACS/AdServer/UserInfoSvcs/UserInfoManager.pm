package AdServer::UserInfoSvcs::UserInfoManager;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/UserInfoManager.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${workspace_root}/run && " .
   "mkdir -p \${log_root}/UserInfoManager/In/UserOp_1 && " .
   "mkdir -p \${log_root}/UserInfoManager/In/UserOp_1/Intermediate && " .
   "mkdir -p \${log_root}/UserInfoManager/In/UserOp_2 && " .
   "mkdir -p \${log_root}/UserInfoManager/In/UserOp_2/Intermediate && " .
   "mkdir -p \${log_root}/UserInfoManager/Out/ChannelCountStat && " .
   "mkdir -p \${log_root}/UserInfoManager/Out/ChannelCountStat_ && " .
   "mkdir -p \${log_root}/UserInfoManager/Out/UserOp_1 && " .
   "mkdir -p \${log_root}/UserInfoManager/Out/UserOp_2 && " .
   "mkdir -p \${log_root}/UserInfoManager/In/ExternalUserOp && " .
   "mkdir -p \${log_root}/UserInfoManager/In/ExternalUserOp/Intermediate && " .
   "mkdir -p \${log_root}/UserInfoManager/Out/ExternalUserOp && " .
   "if test -e $pid_file; then " .
     "pid=`cat $pid_file`; " .
     "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
   "fi && " .
   "ulimit -n 16000 && " .
   "export MALLOC_ARENA_MAX=2 && " .
   "{ ".
     "\${VALGRIND_PREFIX} UserInfoManager " .
       "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserInfoManagerConfig.xml > " .
       "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserInfoManager.out 2>&1 < /dev/null & ".
   "}";

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
