package AdServer::UserInfoSvcs::UserBindController;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $user_bind_server_port = "user_bind_controller_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/UserBindController && " .
   "{ ".
     "\${VALGRIND_PREFIX} UserBindController " .
       "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindController.xml > " .
       "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserBindController.out 2>&1 & ".
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $user_bind_server_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $user_bind_server_port,
    $descr);
}

1;
