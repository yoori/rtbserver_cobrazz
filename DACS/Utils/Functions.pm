package Utils::Functions;

use LWP::Simple;
use strict;
use warnings;

my $sys_mutex : shared;

sub safe_system
{
  lock($sys_mutex);
  return system(@_);
}

sub safe_system_for_output
{
  lock($sys_mutex);

  open(SSO, '-|', @_) or die "safe_system_for_output: can't open pipe.";
  my $output = "";

  $output = <SSO>;
  close(SSO);
  my $ret = $?;

  return ($ret, defined $output ? $output : "");
}

sub probe_http
{
  my ($url, $answer) = @_;
  my $content;
  unless (defined($content = get($url)) and ($content eq $answer))
  {
    return 0;
  }
  return 1;
}

sub make_corba_ref
{
  my ($host, $port, $service_name) = @_;
  return 'corbaloc:iiop:' . $host . ':' . $port . '/' . $service_name;
}

sub init_environment
{
  defined($ENV{"config_root"}) or
    die "Not defined environment variable config_root";

  my $environment_dir = $ENV{'config_root'};

  return $environment_dir;
}

sub process_control_any
{
  my ($host, $port_var, $current_config_dir, $doing, $descr) = @_;

  my $environment_dir = init_environment();

  my $ref = make_corba_ref(
       $host,
       "\${".$port_var."}",
       "ProcessControl");

  my $command =
    ". $environment_dir/environment.sh && " .
    "test \${workspace_root} || " .
      "{ echo \"Stop: Variable workspace_root isn't defined on $host\" && exit 1 ; } && " .
    "if [ -f  \$config_root/$current_config_dir/fe.sh ]; then . \$config_root/$current_config_dir/fe.sh 2>/dev/null; fi && " .
    "if [ -f \$config_root/$current_config_dir/be.sh ]; then . \$config_root/$current_config_dir/be.sh 2>/dev/null; fi && " .
    "if [ -f \$config_root/$current_config_dir/pbe.sh ]; then . \$config_root/$current_config_dir/pbe.sh 2>/dev/null; fi && " .
    "if [ -f \$config_root/$current_config_dir/ccluster.sh ]; then . \$config_root/$current_config_dir/ccluster.sh  2>/dev/null; fi && " .
    "test \${$port_var} || " .
      "{ echo \"Stop: Variable $port_var isn't defined on $host\" && exit 1 ; } && " .
    "ProbeObj $doing $ref";
  return safe_system($command);
}

sub process_control_stop
{
  my ($host, $port_var, $current_config_dir, $descr) = @_;
  my $res = process_control_any(
    $host, $port_var, $current_config_dir, "-shutdown", $descr);

  if ($res)
  {
    return 0;
  }
  return 1;
}

sub process_control_is_alive
{
  my ($host, $port_var, $current_config_dir, $descr) = @_;
  my $ret = process_control_any(
    $host, $port_var, $current_config_dir, "", $descr);

  if($ret == -1)
  {
    $$descr =  "failed to run ProbeObj: $!";
    return undef;
  }
  else
  {
    my $probe_ret = ($? >> 8);
    if ($probe_ret == 1)
    {
      return 0;
    }
    elsif($probe_ret != 0)
    {
      $$descr =  "The command ProbeObj exited with code: " . $probe_ret;
      return undef;
    }
  }
  return 1;
}

sub ssh_invoke
{
  my ($host, $command, $ssh_identity) = @_;
  my $status;

  $status = safe_system("ssh",  $host, "-i", $ssh_identity, $command);
  return $status;
}

1;
