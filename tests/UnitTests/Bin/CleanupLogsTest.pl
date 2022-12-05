#!/usr/bin/perl

use strict;
use warnings;
use diagnostics;

use POSIX qw( strftime );
use File::Find::Rule ();

my $server_path = defined($ENV{'server_root'}) ?
  $ENV{'server_root'} : $ENV{'TEST_TOP_SRC_DIR'};

if (!defined $server_path)
{
  $server_path = '../../../';
}

if (!defined($ENV{'TEST_TMP_DIR'}))
{
  $ENV{'TEST_TMP_DIR'} = $server_path . 'build/test/tmp';
}

my $tmp_path = $ENV{'TEST_TMP_DIR'};
  system "mkdir -p $tmp_path";

my $logs_cleaner =
  "PERL5LIB=\$PERL5LIB:$server_path/CMS/Plugin/lib " .
  "$server_path/bin/CleanupLogs.pl -s " .
  "-conf $server_path/tests/UnitTests/Bin/cleanup_test.pm >/dev/null";

eval('require("$server_path/tests/UnitTests/Bin/cleanup_test.pm")');
  if ($@)
  {
    print "Failed to load, because : $@";
    exit -1;
  }

# Create 20 files with monotonic increase modification time and growth size.
# Steps: dT = 10 minutes, size += 1 byte.
# Below, we will apply different constraints to generated file grid
# For example: all files allowed by size, half files allowed by time, etc.
# We will combine constrains and compare filtered grid with STANDARD results.
# In general, test control 12 cases
sub make_files
{
  my ($timestamp, $path) = @_;
  $path = $tmp_path . $path;
  system "rm -rf $path ; mkdir -p $path";
  my $dT = 600; # 10 minutes step
  my $content = '';

  my @files_list;

  for (my $i = 0; $i < 20; ++$i)
  {
    my $file_name = "clean_test.log.$timestamp";
    my $full_file_name = "$path/$file_name";
    open(FILE, ">" . $full_file_name);
    print FILE $content;
    close FILE;
    my $time2str = POSIX::strftime("%Y%m%d%H%M", localtime $timestamp);
    my $touch = "touch -t " . $time2str . " $full_file_name";
    system($touch);
    $timestamp += $dT;
    $content .= 'o';
    push @files_list, $file_name;
  }
  open(FILE, ">" . "$path/clean_test.log");
  print FILE $content;
  close FILE;
  return @files_list;
}

sub generate_test_case_files
{
  my ($folder, $timeshift) = @_;
  my $end = rindex $folder, '/';
  my $begin = rindex $folder, '/', $end - 1;
  my $test_name = substr $folder, $begin, $end - $begin;

  my $logs_ttl = $cleanup::CLEANUP_CONFIG{$folder}{"time"} * 60; # seconds
  return make_files(time() - $logs_ttl - $timeshift, $test_name);
}

# compare two list values
sub aeq
{
  my(@a) = splice(@_,0,shift);
  my(@b) = splice(@_,0,shift);
  return 0 unless @a == @b; # same len?
  while (@a)
  {
    return 0 if pop(@a) ne pop(@b);
  }
  return 1;
}

sub check_test_case
{
  my ($folder, $standard_idx, $gf_ref) = @_;
  my @generated_files = @$gf_ref;
  my $ret = 0;

  my $expectation =
    $cleanup::CLEANUP_CONFIG{$folder}{"standard"}[$standard_idx];
# Prepare expected files list
  if ($expectation eq 'zero')
  {
    @generated_files = ();
  }
  elsif ($expectation eq 'one')
  {
    @generated_files = splice @generated_files, $#generated_files;
  }
  elsif ($expectation eq 'half')
  {
    @generated_files = splice @generated_files, ($#generated_files + 1) / 2;
  }

# find files and compare with standard expectation
  my $idx = rindex($folder, '/');
  my $mask = '^' . (substr $folder, $idx + 1) . '.*\z';
  $folder = substr $folder, 0, $idx;
  $idx = rindex($folder, '/') + 1;
  my $test_name = substr $folder, $idx, length($folder) - $idx;
  print "Checking $test_name.\n";

  my $process_file_rule = File::Find::Rule->new;
  my @found_files = sort
     $process_file_rule->relative->file->name(qr/$mask/)->in("$folder");
  my $main_file = shift @found_files;

  if ($main_file ne 'clean_test.log')
  {
    print STDERR "$test_name: Removed main.log file\n";
    ++$ret;
  }

  if (aeq($#found_files + 1, @found_files,
          $#generated_files + 1, @generated_files) == 0)
  {
    require Data::Dumper;
      print STDERR "The found_files is:\n",
        Data::Dumper::Dumper( @found_files ), "\n";
      print STDERR "The generated_files is:\n",
        Data::Dumper::Dumper( @generated_files ), "\n";
    ++$ret;
  }
  return $ret;
}

sub cleanup
{
  foreach my $folder (keys %cleanup::CLEANUP_CONFIG)
  {
    my $end = rindex $folder, '/';
    my $begin = rindex $folder, '/', $end - 1;
    my $test_name = substr $folder, $begin, $end - $begin;
    system "rm -rf $tmp_path$test_name";
  }
}


# MAIN
  print "$0 started..\n";

  my $ret = 0;

# under 10 minutes and to left only one fresh file at prelast step of cycle
  my $timestep = 5500;
# - 60: Give 1 minute to script execution, allow delay for CleanupLogs.pl.
  my $checking_service_delay = -60;
# Cycle test cases: all, half, one, zero - files left after cleanup.

  for (my $timeshift = $checking_service_delay;
       $timeshift <= $timestep * 3 + $checking_service_delay;
       $timeshift += $timestep)
  {
    print "Shift time of fresh files\n";
    my %tests_files;
    foreach my $folder(keys %cleanup::CLEANUP_CONFIG)
    {
      my @files_list = generate_test_case_files($folder, $timeshift);
      $tests_files{$folder} = \@files_list;
    }

# Run CleanupLogs.pl
    system "$logs_cleaner";
    if ($? == -1)
    {
      print "failed to execute $logs_cleaner: $!\n";
    }
    elsif ($? & 127)
    {
      printf "$logs_cleaner died with signal %d, %s coredump\n",
        ($? & 127),  ($? & 128) ? 'with' : 'without';
    }
    my $service_ret = $? >> 8;
    if ($service_ret)
    {
      print STDERR "Shell command execute failed: " .
        "$logs_cleaner, return $service_ret\n";
      exit -1;
    }

# Examine results and compare with expectations.
    foreach my $folder(keys %tests_files)
    {
      $ret += check_test_case($folder,
        ($timeshift - $checking_service_delay) / $timestep,
         $tests_files{$folder});
    }
  }

  if ($ret == 0)
  {
    print "Test SUCCESS.\n";
    cleanup();
  }
  else
  {
    die "Test detected $ret errors.\n";
  }

0;
