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
    "rm -f '$pid_file' && " .
    "{ " .
      "RImpressionStatUploader.py " .
        "-c \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/ClickhouseUploaderConfig.json " .
        " >\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}ClickhouseUploader.out 2>&1 & " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 0 && " .
    "kill `cat $pid_file`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e $pid_file || exit 1 && " .
    "kill -0 \`cat $pid_file\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
