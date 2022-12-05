package AdServer::UserInfoSvcs::UserInfoManagerController;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $user_info_manager_controller_port = "user_info_manager_controller_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/UserInfoManagerController && " .
       "{ ".
         "\${VALGRIND_PREFIX} UserInfoManagerController " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserInfoManagerControllerConfig.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserInfoManagerController.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $user_info_manager_controller_port,
    $descr);
}

sub park
{
  stop(@_);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $user_info_manager_controller_port,
    $descr);
}

1;
