package AdServer::PredictorSvcs::SegmentLoader;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Segmentloader && " .
    "mkdir -p \${workspace_root}run/Utils/GPT && " .
    "cp -r \${server_root}/Utils/GPT \${workspace_root}run/Utils/GPT && " .
   "{ " .
     "rm -f \${workspace_root}run/SegmentLoader.pid && ".
     "\${VALGRIND_PREFIX} \${server_root}/PredictorSvcs/SegmentLoader/Segmentloader.py " .
     "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/SegmentLoader.conf" .
     " > \${log_root}/Segmentloader/SegmentLoader.out 2>&1 & " .
   "} ";
  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test \${workspace_root}/run/SegmentLoader.pid || exit 0 &&".
    "kill `cat \${workspace_root}/run/SegmentLoader.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/run/SegmentLoader.pid || exit 1 && " .
    "kill -0 \`cat \${workspace_root}/run/SegmentLoader.pid\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
