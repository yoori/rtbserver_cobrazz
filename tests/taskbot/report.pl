#! /usr/bin/perl
# -*- cperl -*-
#
use warnings;
use strict;

=head1 NAME

report.pl - generate report during taskbot run.

=head1 SYNOPSIS

  report.pl CONFIG REPORT [ ADDITIONAL ARGS ]

=head2 OPTIONS

=over

=item C<--task=TASK>

C<TASK> is the task ID to link the report to.

=back

=head2 ENVIRONMENT

Environment variables I<TASKBOT_PARENT_PID> and I<TASKBOT_SHARE>
should be set by taskbot.

=cut


use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %option;
if (! GetOptions(\%option, qw(task=i))
    || @ARGV < 2 || ! defined $ENV{TASKBOT_PARENT_PID}) {
    pod2usage(1);
}

my $config_file = shift @ARGV;
my $report_file = shift @ARGV;


use FindBin;
use lib "$FindBin::Bin/lib", $ENV{TASKBOT_SHARE};

use Taskbot::Report;

my $report = new Taskbot::Report($config_file, \@ARGV, $option{task});

my $config = do $config_file;
while (my ($k, $v) = each %{$config->{environment}}) 
{
  if (defined $v) {
    $ENV{$k} = $v;
  } else {
    delete $ENV{$k};
  }
}

$report->generate($report_file);
