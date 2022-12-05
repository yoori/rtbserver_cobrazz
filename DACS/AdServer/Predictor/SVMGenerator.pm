package AdServer::Predictor::SVMGenerator;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $svm_generator_port = "svm_generator_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/Predictor && ".
    "mkdir -p \${log_root}/Predictor/ResearchLogs/LibSVM && ".
    "{ CTRPredictorSVMGenerator " .
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/CTRPredictorSVMGeneratorConfig.xml " .
      " > \${workspace_root}/${AdServer::Path::OUT_FILE_BASE}CTRPredictorSVMGenerator.out 2>&1 & }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $svm_generator_port,
    $verbose);
}

sub is_alive
{
  my ($host, $verbose) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $svm_generator_port,
    $verbose);
}

1;
