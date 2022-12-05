#!/usr/bin/perl

use strict;
use warnings;
#use diagnostics;

use File::Find::Rule ();

my $usage = qq(
  Clear log files according policies.\n
  Usage: $0 <params>\n
    params:\n
      -conf <file with configuration>  Specify config file\n
      -s    Single check, clean and exit\n
      -h    Show this message\n
  );

use Cwd 'abs_path';
my $initial_path = abs_path('./');

my $config_file = 'cleanup.pm';
my $single_check = 0;

  for (my $i = 0; $i < @ARGV; ++$i)
  {
    if ($ARGV[$i] eq '-conf')
    {
      die $usage unless ($i + 1 < @ARGV);
      $i++;
      $config_file = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '--help' or $ARGV[$i] eq '-h')
    {
      print $usage;
      exit 1;
    }
    elsif ($ARGV[$i] eq '-s' or $ARGV[$i] eq '--single')
    {
      $single_check = 1;
    }
  }

  if(!defined($config_file))
  {
    die "error: -conf parameter is not defined.";
  }

  eval('require $config_file');
  if ($@)
  {
    die "failed to load, because : $@";
  }

  my $pid_file = $::cleanup::SERVICE_CONFIG{'pid_file'};
  my $check_period = $cleanup::SERVICE_CONFIG{'check_period'};
  open(PID_FILE, ">" . $pid_file . '_') || die "Can't open pid file: $pid_file";
  print PID_FILE $$;
  close PID_FILE;
  rename $pid_file . '_', $pid_file;


sub check_and_remove_files
{
  my $currenttime = shift(@_);
  my $url = shift(@_);
  my $idx = rindex $url, '/';
  my $folder = substr $url, 0, $idx;

  our $mask = '^' . (substr $url, $idx + 1) . '\..*\z';
  our $timeout = $cleanup::CLEANUP_CONFIG{$url}->{"time"};
  our $max_size = $cleanup::CLEANUP_CONFIG{$url}->{"size"};

  my $process_file_rule = File::Find::Rule->new;
  $process_file_rule->maxdepth(1);
  my @log_files =
    $process_file_rule->relative->file->name(qr/$mask/)->in("$folder");

  @log_files = sort {$b cmp $a} @log_files;
  my $size_accumulator = 0;

  my $remove_index = 0;
  foreach my $file(@log_files)
  {
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime);

    ($dev,$ino,$mode,$nlink,$uid,$gid,
     $rdev,$size,$atime,$mtime,$ctime) = lstat $folder . '/' . $file;
    if ($currenttime - $mtime > $timeout * 60)
    {
      last; # rest is older files only
    }
    $size_accumulator += $size;
    if ($size_accumulator > $max_size)
    {
      last;
    }
    $remove_index++;
  }

  splice @log_files, 0, $remove_index;
  foreach my $file(@log_files)
  {
    unlink($folder . '/' . $file);
  }
}

sub pid_file_cleanup
{
  system "cd $initial_path; rm -f $::cleanup::SERVICE_CONFIG{'pid_file'}";
  system "cd $initial_path; rm -f $::cleanup::SERVICE_CONFIG{'pid_file'}_";
  print STDOUT "\nCleanupLogs stopped\n";
  exit 0;
};

$SIG{TERM} = $SIG{INT} = $SIG{QUIT} = \&pid_file_cleanup;

sub main()
{
  do
  {
    foreach my $folder(keys %cleanup::CLEANUP_CONFIG)
    {
      check_and_remove_files(time(), $folder);
    }
  }
  while (!$single_check and sleep($check_period));
  pid_file_cleanup();
}

  main();

0;
