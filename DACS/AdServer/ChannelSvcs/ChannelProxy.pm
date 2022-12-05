package AdServer::ChannelSvcs::ChannelProxy;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $channel_proxy_port = "channel_proxy_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/ChannelProxy && " .
       "mkdir -p \${log_root}/ChannelProxy/Out/ColoUpdateStat && " .
       "mkdir -p \${log_root}/ChannelProxy/Out/ColoUpdateStat_ && " .
       "{ ".
         "\${VALGRIND_PREFIX} ChannelProxy " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ChannelProxy.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ChannelProxy.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $channel_proxy_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $channel_proxy_port,
    $descr);
}

1;
