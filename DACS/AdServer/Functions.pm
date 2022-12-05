package AdServer::Functions;

use Utils::Functions;
use AdServer::Path;

sub process_control_stop
{
  my ($host, $port_var, $descr) = @_;
  Utils::Functions::process_control_stop(
    $host,
    $port_var,
    $host . "/" . ${AdServer::Path::ADSERVER_CURRENTENV_DIR},
    $descr);
}

sub process_control_is_alive
{
  my ($host, $port_var, $descr) = @_;
  return Utils::Functions::process_control_is_alive(
    $host,
    $port_var,
    $host . "/" . ${AdServer::Path::ADSERVER_CURRENTENV_DIR},
    $descr);
}

sub execute_command
{
  my ($host, $descr, $exe_command) = @_;
  my $environment_dir = Utils::Functions::init_environment();

  my $command = 
      "source $environment_dir/environment.sh && " .
      "test \${workspace_root}  || " .
        "{ echo \"Variable workspace_root isn't defined on $host\" && exit -1 ; } && " .  
      "test \${config_root} || " .
        "{ echo \"Variable config_root isn't defined on $host\" && exit -1 ; } && " .
      "test \${log_root} || " .
         "{ echo \"variable log_root isn't defined on $host\" && exit -1 ; } && " .
      "mkdir -p \${workspace_root}/${AdServer::Path::OUT_FILE_BASE} && " .
      "cd \${workspace_root}/${AdServer::Path::OUT_FILE_BASE} && " .
      "$exe_command ";

  my $res =  Utils::Functions::safe_system($command);
  if($res)
  {
    return 0;
  }
  else
  {
    if($res>>8 == 0)
    {
      return 1;
    }
    else
    {
      return $res>>8;
    }
  }
}

1;
