package AdServer::UserInfoSvcs::UserOperationGenerator;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $user_operation_generator_port = "user_operation_generator_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/UserOperationGenerator/Out/UserOp && " .
   "mkdir -p \${log_root}/UserOperationGenerator/Out/UserOp_ && " .
   "mkdir -p \${log_root}/UserOperationGenerator/Snapshot && " .
   "mkdir -p \${log_root}/UserOperationGenerator/Temp && " .
   "mkdir -p \${log_root}/UserOperationGenerator/In/Snapshot && " .
   "find \${log_root}/UserOperationGenerator/Snapshot/ -name '.\$tmp_*' -type f -delete && " .
   "{ ".
     "\${VALGRIND_PREFIX} UserOperationGenerator " .
       "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserOperationGenerator.xml > " .
       "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserOperationGenerator.out 2>&1 & ".
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $user_operation_generator_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $user_operation_generator_port,
    $descr);
}

1;
