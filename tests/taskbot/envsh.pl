#! /usr/bin/perl
# -*- cperl -*-
#
use warnings;
use strict;

=head1 NAME

envsh.pl - run shell script in the configured environment

=head1 SYNOPSIS

  envsh.pl [ OPTIONS ] CONFIG SCRIPT

=head2 OPTIONS

All options are passed in environment variable I<TASKBOT_OPTIONS>.

=cut

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %option;
if (@ARGV < 2) {
    pod2usage(1);
}

my ($config_file, $script) = splice(@ARGV, 0, 2);
my @taskbot_options = @ARGV;

my $config = do $config_file;
unless ($config) {
    die "While reading $config_file: $!" if ($!);
    die "While compiling $config_file: $@" if ($@);
    die "While executing $config_file: returned '$config'";
}

while (my ($k, $v) = each %{$config->{environment}}) {
    $ENV{$k} = $v;
}
unless ($config_file =~ m|^/|) {
    use Cwd;
    $config_file = join('/', cwd, $config_file);
}

use FindBin;
$ENV{TASKBOT} = "$FindBin::Bin/$FindBin::Script";
$ENV{TASKBOT_OPTIONS} = "@taskbot_options";
$ENV{TASKBOT_CONFIG} = $config_file;

exec($config->{sh}, $script, @taskbot_options);
die "exec failed: $!";
