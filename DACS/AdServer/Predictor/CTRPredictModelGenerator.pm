package AdServer::Predictor::CTRPredictModelGenerator;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start_objective
{
  my ($host, $descr, $model_name, $objective) = @_;
  my $pid_file = "\${workspace_root}/run/${model_name}PredictModelGenerator.pid";

  my $command =
    "mkdir -p \${workspace_root}/run && " .
    "mkdir -p \${workspace_root}/${model_name}PredictModelGenerator && " .
    AdServer::Functions::pidfile_start_guard($pid_file, "CTRPredictModelGenerator.py") . " && " .
    "{ " .
      "setsid -f CTRPredictModelGenerator.py " .
        "--config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/" .
          "${model_name}PredictModelGeneratorConfig.json " .
        "--objective=$objective " .
        " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}" .
          "${model_name}PredictModelGenerator.out 2>&1 < /dev/null ; " .
    "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop_objective
{
  my ($host, $descr, $model_name) = @_;
  my $pid_file = "\${workspace_root}/run/${model_name}PredictModelGenerator.pid";

  return AdServer::Functions::stop_by_pidfile(
    $host, $descr, $pid_file, "CTRPredictModelGenerator.py");
}

sub is_alive_objective
{
  my ($host, $descr, $model_name) = @_;
  my $pid_file = "\${workspace_root}/run/${model_name}PredictModelGenerator.pid";

  return AdServer::Functions::execute_command(
    $host,
    $descr,
    AdServer::Functions::pidfile_is_alive($pid_file, "CTRPredictModelGenerator.py"));
}

sub start
{
  my ($host, $descr) = @_;
  return start_objective($host, $descr, 'CTR', 'ctr');
}

sub stop
{
  my ($host, $descr) = @_;
  return stop_objective($host, $descr, 'CTR');
}

sub is_alive
{
  my ($host, $descr) = @_;
  return is_alive_objective($host, $descr, 'CTR');
}

1;
