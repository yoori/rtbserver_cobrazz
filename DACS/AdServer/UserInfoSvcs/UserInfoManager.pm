package AdServer::UserInfoSvcs::UserInfoManager;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $user_info_manager_port = "user_info_manager_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
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
   "ulimit -n 16000 && " .
   "export MALLOC_ARENA_MAX=2 && " .
   "{ ".
     "\${VALGRIND_PREFIX} UserInfoManager " .
       "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserInfoManagerConfig.xml > " .
       "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserInfoManager.out 2>&1 & ".
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $user_info_manager_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $user_info_manager_port,
    $descr);
}

1;
