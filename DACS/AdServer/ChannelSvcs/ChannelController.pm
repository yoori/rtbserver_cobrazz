package AdServer::ChannelSvcs::ChannelController;

use strict;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/ChannelController.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/ChannelController \${workspace_root}/run && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "ChannelController") . " && " .
    "{ " .
      "setsid -f \${VALGRIND_PREFIX} ChannelController " .
        "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ChannelController.xml > " .
        "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ChannelController.out 2>&1 < /dev/null ; " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file, "ChannelController");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "ChannelController"));
}

1;
