package AdServer::UserInfoSvcs::UserInfoExchangerProxy;

use AdServer::Functions;

my $user_info_exchanger_port = "user_info_exchanger_proxy_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/UserInfoExchangerProxy && " .
       "{ ".
         "\${VALGRIND_PREFIX} UserInfoExchangerProxy " .
           "\${workspace_root}/AdServer/UserInfoExchangerProxy.xml > " .
           "\${workspace_root}/UserInfoExchangerProxy.out 2>&1 & ".
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

sub park
{
  stop(@_);
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
