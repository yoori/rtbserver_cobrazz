#!/usr/bin/perl

use threads;
use Utils::Functions;
use AdServer::Functions;

my $count = @ARGV;
if ($count<2)
{
  die "ProbeObjGroup.pl: wrong count of arguments, $count<2\n";
}

my $config_file = shift(@ARGV);
my $doing = join(' ', @ARGV);

eval('require "' . $config_file . '"') or
  die "ProbeObjGroup.pl: Error: can not load $config_file file.\n";


my $ret_value = 0;
my $environment_dir = Utils::Functions::init_environment();

sub run_for
{
  my ($service_name, $probe_ref) = @_;

  my $command =
    "source $environment_dir/environment.sh && " .
    "test \${workspace_root} || " .
    "{ echo \"Stop: Variable workspace_root isn't defined on $host\" && exit 1 ; } && " .
    "ProbeObj $doing $probe_ref 1>/dev/null";
  my $ret_value = 0;
  my $host;
  if( $probe_ref =~ m/corbaloc:iiop:(.*?):\d+\/ProcessControl/)
  {
    $host = $1;
  }
  else
  {
    die "ERROR: ProbeObjGroup.pl: Bad host name in $probe_ref\n";
  }
  my $res = AdServer::Functions::execute_command($host, "", $command);
  if ($res != 1)
  {
    print "WARNING: can't execute command \'$command\' for $service_name at $host\n";
    return (1);
  }
  return (0);
}

my @services, @thr_ids;

if(defined $channel_server) { push(@services, ['ChannelServer', $channel_server]);}
if(defined $campaign_server) { push(@services, ['CampaignServer', $campaign_server]);}
if(defined $log_generalizer) { push(@services, ['LogGeneralizer', $log_generalizer]);}

foreach $thr_args (@services)
{
  my $pid = threads->create('run_for', @$thr_args);
  if($pid)
  {
    push(@thr_ids, $pid);
  }
  else
  {
    die 'Can not create thread';
  }
}
foreach $pid (@thr_ids)
{
  my ($status) = $pid->join();
  $ret_value += $status;
}

exit $ret_value;

