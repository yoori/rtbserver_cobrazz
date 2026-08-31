#!/usr/bin/perl -w

use strict;

my $usage = "Usage: aiserver.pl host service function\n";
@ARGV == 3 or die $usage;

my ($host, $service, $command) = @ARGV;
eval "require $service" or die "Can not load '$service': $@";

my $function = $service . '::' . $command;
my $description;
my $result;
{
  no strict 'refs';
  $result = $function->($host, \$description);
}

if (!defined($result) || $result != 1)
{
  print STDERR $description, "\n" if defined($description);
  exit 1;
}

exit 0;
