package AdServer::ChannelSvcs::ChannelServer;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $service_port = "channel_server_port";

sub start
{
  my ($host, $descr) = @_;

  #  
  my $command =
       "mkdir -p \${log_root}/ChannelServer && " .
       "mkdir -p \${log_root}/ChannelServer/Out/ColoUpdateStat && " .
       "mkdir -p \${log_root}/ChannelServer/Out/ColoUpdateStat_ && " .
       "{ ".
         "\${VALGRIND_PREFIX} ChannelServer " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ChannelServer.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ChannelServer.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $service_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $service_port,
    $descr);
}

sub db_status
{
  my ($host, $descr) = @_;
  my $status = is_alive @_;
  if($status == 1)
  {
    Utils::Functions::process_control_any(
      $host,
      $service_port, 
      $host . "/" . ${AdServer::Path::ADSERVER_CURRENTENV_DIR},
      "-control DB -1",
      $descr);
  }
  return $status;
}

1;
