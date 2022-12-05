package AdServer::ChannelSearchSvcs::ChannelSearchService;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $channel_search_service_port = "channel_search_service_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/ChannelSearchService && " .
       "{ ".
         "\${VALGRIND_PREFIX} ChannelSearchService " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ChannelSearchServiceConfig.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ChannelSearchService.out 2>&1 & ".
       "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $channel_search_service_port);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $channel_search_service_port);
}

1;
