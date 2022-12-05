package AdServer::ChannelSvcs::DictionaryProvider;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $service_port = "dictionary_provider_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "mkdir -p \${log_root}/DictionaryProvider && " .
       "{ ".
         "\${VALGRIND_PREFIX} DictionaryProvider " .
           "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/DictionaryProvider.xml > " .
           "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}DictionaryProvider.out 2>&1 & ".
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

1;
