package AdServer::UserInfoSvcs::UserInfoExchanger;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $user_info_exchanger_port = "user_info_exchanger_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${workspace_root}/cache/UsersExchange && " .
       "mkdir -p \${log_root}/UserInfoExchanger && " .
       "{ ".
         "\${VALGRIND_PREFIX} UserInfoExchanger " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserInfoExchanger.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserInfoExchanger.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  AdServer::Functions::process_control_stop(
    $host,
    $user_info_exchanger_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $user_info_exchanger_port,
    $descr);
}

1;
