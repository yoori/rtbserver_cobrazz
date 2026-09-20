package AdServer::LogProcessing::ClickhouseUploader;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/ClickhouseUploader.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/run && " .
    "mkdir -p \${workspace_root}/log/ClickhouseUploader/Error && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "ClickhouseStatUploader.py") . " && " .
    "{ " .
      "setsid -f ClickhouseStatUploader.py " .
        "-c \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ClickhouseUploaderConfig.json " .
        " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ClickhouseUploader.out 2>&1 < /dev/null ; " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Functions::stop_by_pidfile(
    $host, $descr, $pid_file, "ClickhouseStatUploader.py");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "ClickhouseStatUploader.py"));
}

1;
