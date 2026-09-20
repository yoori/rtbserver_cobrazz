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
    AdServer::Functions::pidfile_start_guard($pid_file, "CTRResearchModelGenerator.py") . " && " .
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
  return AdServer::Functions::stop_by_pidfile(
    $host, $descr, $pid_file, "CTRResearchModelGenerator.py");
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Functions::execute_command(
    $host, $descr, AdServer::Functions::pidfile_is_alive($pid_file, "CTRResearchModelGenerator.py"));
}

1;
