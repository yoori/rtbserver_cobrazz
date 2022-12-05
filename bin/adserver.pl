#!/usr/bin/perl -w

use threads;
use threads::shared;

sub do_command;
sub do_service;

my $usage = "Usage: adserver.pl host service function\n";

(@ARGV == 3) or die $usage;

my $host = $ARGV[0];
my $service = $ARGV[1];
my $command = $ARGV[2];

my $ret_value =  do_command($host, $service, $command); 

if(defined($ret_value) && $ret_value==1)
{
  exit(0);
}
else
{
  exit(1);
}

sub do_command
{
  my ($host, $service_name, $command) = @_;
  if (!eval("require $service_name"))
  {
    die("Error: can not load '" . $service_name .  "' service module. ", $@);
  }
  my $func_name = $service_name . "::" . $command;
  my $descr;
   return $func_name->("$host", \$descr);
}

1;
