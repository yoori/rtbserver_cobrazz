package AdServer::Predictor::CTRResearchModelGenerator;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $pid_file = "\${workspace_root}/run/CTRResearchModelGenerator.pid";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/run && " .
    "mkdir -p \${workspace_root}/CTRResearchModelGenerator && " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi && " .
    "{ " .
      "setsid -f CTRResearchModelGenerator.py " .
        "--config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/" .
        "CTRResearchModelGeneratorConfig.json " .
        " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}" .
        "CTRResearchModelGenerator.out 2>&1 < /dev/null ; " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::stop_by_pidfile($host, $descr, $pid_file);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::execute_command(
    $host,
    $descr,
    "test -e $pid_file || exit 1 && " .
    "pid=`cat $pid_file` && " .
    "kill -0 \$pid 2>/dev/null || exit 1; exit 0");
}

1;
