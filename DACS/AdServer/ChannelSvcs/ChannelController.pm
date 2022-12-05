package AdServer::ChannelSvcs::ChannelController;

use strict;
use Utils::Functions;
use AdServer::Functions;

my $channel_controller_port = "channel_controller_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/ChannelController && " .
       "{ ".
         "\${VALGRIND_PREFIX} ChannelController " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ChannelController.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ChannelController.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  AdServer::Functions::process_control_stop(
    $host,
    $channel_controller_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $channel_controller_port,
    $descr);
}

1;
