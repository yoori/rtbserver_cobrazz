package AdServer::Predictor::SegmentUploader;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor/SegmentUploader/uploader_log && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ " .
      "SegmentUploader.py --config " .
        "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/SegmentUploader.json 2>&1 & " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}


sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/segment_uploader.pid || exit 1 && " .
    "kill -0 \`cat \${workspace_root}/segment_uploader.pid\` 2>/dev/null || exit 1 && " .
    "kill -9 \`cat \${workspace_root}/segment_uploader.pid\` 2>/dev/null && exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/segment_uploader.pid || exit 1 && " .
    "kill -0 \`cat \${workspace_root}/segment_uploader.pid\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
