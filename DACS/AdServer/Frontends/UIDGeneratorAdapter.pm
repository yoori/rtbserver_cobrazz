package AdServer::Frontends::UIDGeneratorAdapter;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $uid_generator_adapter_port = "uid_generator_adapter_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/UIDGeneratorAdapter && ".
    "mkdir -p \${log_root}/UIDGeneratorAdapter/Out/KeywordHitStat && " .
    "ulimit -n 12288 && " .
    #"{ valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes UIDGeneratorAdapter " .
    "{ UIDGeneratorAdapter " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UIDGeneratorAdapterConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UIDGeneratorAdapter.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $uid_generator_adapter_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $uid_generator_adapter_port,
    $verbose);
}

1;
