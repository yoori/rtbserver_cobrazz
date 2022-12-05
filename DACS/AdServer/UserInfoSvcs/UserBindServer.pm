package AdServer::UserInfoSvcs::UserBindServer;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $user_bind_server_port = "user_bind_server_port";

sub start
{
  my ($host, $descr) = @_;

  my $command =
   "mkdir -p \${log_root}/UserBindServer && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_1 && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_1/Intermediate && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_2 && " .
   "mkdir -p \${log_root}/UserBindServer/In/UserBindOp_2/Intermediate && " .
   "mkdir -p \${log_root}/UserBindServer/Out/UserBindOp_1 && " .
   "mkdir -p \${log_root}/UserBindServer/Out/UserBindOp_2 && " .
   "ulimit -n 16384 && " .
   "export MALLOC_ARENA_MAX=1 && " .
   "{ ".
     #"valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes UserBindServer " .
     "UserBindServer " .
       "\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/UserBindServer.xml > " .
       "\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}UserBindServer.out 2>&1 & ".
   "}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_stop(
    $host,
    $user_bind_server_port,
    $descr);
}

sub is_alive
{
  my ($host, $descr) = @_;

  return AdServer::Functions::process_control_is_alive(
    $host,
    $user_bind_server_port,
    $descr);
}

1;
