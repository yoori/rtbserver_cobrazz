package AdServer::LogProcessing::YaMetrikaUploader;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "rm -f \${workspace_root}/run/ya_metrika_uploader.pid && " .
    "{ " .
      "YaMetrikaUploader.py " .
        "--config=\${config_root}/${AdServer::Path::JSON_FILE_BASE}$host/YaMetrikaUploaderConfig.json " .
        "--pid-file=\${workspace_root}/run/ya_metrika_uploader.pid " .
        " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}YaMetrikaUploader.out 2>&1 & " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test \${workspace_root}/run/ya_metrika_uploader.pid || exit 0 && " .
    "kill -TERM `cat \${workspace_root}/run/ya_metrika_uploader.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/run/ya_metrika_uploader.pid || exit 1 && " . 
    "kill -0 \`cat \${workspace_root}/run/ya_metrika_uploader.pid\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;

