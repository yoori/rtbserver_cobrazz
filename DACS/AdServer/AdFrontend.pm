package AdServer::AdFrontend;

use strict;
use Errno;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

my $ad_frontend_port = "ad_frontend_port";

sub start
{
  my ($host, $descr) = @_;

  my $command = 
      "\${config_root}/$host/http/bin/apachectl -host $host start ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  Utils::Functions::init_environment();

  my $command = 
    "\${config_root}/$host/http/bin/apachectl -host $host graceful-stop";
  
  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $environment_dir = Utils::Functions::init_environment();

  my $command = 
    ". $environment_dir/environment.sh && " .
    "test \${workspace_root} || " .
      "{ echo \"Stop: Variable workspace_root isn't defined on $host\" && exit 1 ; } && " .
    "if [ -f  \$config_root/$host/${AdServer::Path::ADSERVER_CURRENTENV_DIR}/fe.sh ]; then . \$config_root/$host/${AdServer::Path::ADSERVER_CURRENTENV_DIR}/fe.sh 2>/dev/null; fi && " .
    "test \${$ad_frontend_port} || " .
    "{ echo \"Is Alive: Variable $ad_frontend_port isn't defined on $host\" && exit -1 ; } && " .
    "echo \${$ad_frontend_port}";

  my ($res, $command_output) =
    Utils::Functions::safe_system_for_output($command);

  if($res)  
  {
    $$descr =  "returned error code: $res, comment: " . $command_output;
    undef;
  }
 
  my $adfe_port = 0;

  if($command_output =~ m/^(\d*)$/)
  {
    $adfe_port = $1;
  }
  else
  {
    return 0;
  }

  my $url = 'http://' . $host . ':' . $adfe_port . '/sysmon';  
  my $ret = Utils::Functions::probe_http($url, "system_monitor");

  if($! == 111)
  {
    # ignore connection refused error
    $@ = "";
    $! = 0;
    $ret = 0;
  }

  return $ret;
}

1;
