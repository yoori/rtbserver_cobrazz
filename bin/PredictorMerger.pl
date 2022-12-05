#!/usr/bin/perl

# Requirenments:
# * perl-Path-Iterator-Rule
# * perl-List-BinarySearch-XS
# * perl-List-BinarySearch
# * perl-Proc-Daemon
# * perl-Log-Dispatch

use warnings;
use strict;

=head1 NAME

PredictorMerger.pl - scipt for making performance test configuration file

=head1 SYNOPSIS

  PredictorMerger.pl OPTIONS start|stop|status

=head1 OPTIONS

=over

=item C<--pid-file-path working directory>

S<Daemon working directory>

B<Required.>

=item C<--config-file-path configuration path>

S<Configuration file path>

=back

=cut

use Pod::Usage;
use Cwd;
use Getopt::Long qw(:config gnu_getopt bundling);
use Proc::Daemon;
use Cwd;
use XML::Twig;
use File::Spec::Functions;
use DateTime;
use File::Find;
use Path::Iterator::Rule;
use Text::CSV;
use List::Util qw[min max minstr];
use List::BinarySearch::XS qw( :all );
use List::BinarySearch qw(binsearch_range);
use Class::Struct;
use Log::Dispatch;
use File::Basename;
use File::Spec;
use File::Path qw(make_path);
use POSIX qw(strftime);
use Time::HiRes qw(time);
use Fcntl qw(:flock SEEK_END LOCK_EX LOCK_UN);

struct(TargetAction => [
  action_ids => '$', 
  timeout => '$', 
  type => '$'] );


use constant DEFAULT_MAX_TIMEOUT => 3600;
use constant DEFAULT_FROM => -1;
use constant DEFAULT_IMPRESSION_TO => -6;
use constant DEFAULT_ACTION_TO => 5;
use constant DEFAULT_CLICK_TO => 4;
use constant DEFAULT_SLEEP_TIMEOUT => 300;
use constant DEFAULT_SLEEP => 2;
use constant DEFAULLT_DAYS_TO_KEEP => 90;

my $work_dir = getcwd();
my $pid_file_name = 'predictor_merger.pid';
my $config;
my ($config_file, $pid_file);
my ($imp_config, $click_config, $action_config);
my $target_action_config = {};

my $click_folder;
my $action_folder;
my $processed_bid_folder;
my $continue = 1;

my %options = (
  "config-file-path|c=s" => \$config_file,
  "pid-file-path|l=s" => \$pid_file,
);

if (! GetOptions(%options))
{
  pod2usage(1);
}

if (scalar(@ARGV) != 1)
{
  print "You have to define operation: start, stop or status !\n";
  pod2usage(1);
}

if (defined($pid_file))
{
  ($pid_file_name, $work_dir) = fileparse($pid_file);
}
else
{
   $pid_file = catfile($work_dir, $pid_file_name);
}

my $sub = sub { 
  my %p = @_; 
  my $t = time;
  my $time = strftime "%a %d %b %Y %H:%M:%S", localtime $t;
  $time .= sprintf(".%03d", ($t-int($t))*1000);
  $time .= strftime " %Z", localtime $t;
  return $time . 
    " [" . uc($p{level}) . "] " . 
      $p{message}; };

sub get_log_level {
  my ($log_level) = @_;
  my @levels = (
    'emergency', #EMERGENCY 
    'alert',     # ALERT
    'critical',  # CRITICAL
    'error',     # ERROR
    'warning',   # WARNING
    'notice',    # NOTICE
    'info',      # INFO
    'debug',     # DEBUG
    'debug'      # TRACE
  );

  if (defined $log_level and $levels[$log_level])
  {
    return $levels[$log_level];
  }
  return 'debug';
}

my $logger = Log::Dispatch->new(
      callbacks => $sub,
      outputs => [
          [ 'Screen', min_level => 'debug', newline => 1, ],
          [ 'Screen', min_level => 'warning', stderr => 1, newline => 1 ],
      ],
  );

sub DEBUG { $logger->debug(@_); }
sub ERROR { $logger->error(@_); }
sub TRACE { $logger->debug(@_); }
sub WARN  { $logger->warn(@_); }
sub INFO  { $logger->info(@_); }


#=== Time utils ===

# convert %F %T to epoch timestamp

sub lock_file {
    my ($fh) = @_;
    flock($fh, LOCK_EX) or die "Cannot lock file - $!";
    # and, in case someone appended while we were waiting...
    seek($fh, 0, SEEK_END) or die "Cannot seek to end - $!";
}
sub unlock_file {
    my ($fh) = @_;
    flock($fh, LOCK_UN) or die "Cannot unlock file - $!";
}

sub to_ts
{
  my ($ft) = @_;

  if($ft =~ m|^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})$|)
  {
    return DateTime->new(
      year => $1,
      month => $2,
      day => $3,
      hour => $4,
      minute => $5,
      second => $6)->epoch();
  }

  return undef;
}

sub to_ft
{
  my ($epoch) = @_;
  return DateTime->from_epoch(epoch => $epoch)->strftime('%F %T');
}

sub get_file_dt
{
  my ($file, $ext) = @_;

  if($file =~ m|^.*/?[^/]+_(\d{4})(\d{2})(\d{2})(\d{2})(\d{4}-\d{6}-\d{6})?[.]$ext$|)
  {
    return DateTime->new(
      year => $1,
      month => $2,
      day => $3,
      hour => $4);
  }

  return undef;
}

#=== Log processing ===

sub remove_obsolete_processed_logs
{
  my ($dir, $prefix, $days_to_keep) = @_;
  my $rule = Path::Iterator::Rule->new;
  my $last_actual_date = DateTime->now();
  $last_actual_date->subtract(days => $days_to_keep);

  $rule->name($prefix . "_*.unlink");
  my $it = $rule->iter($dir);
  my $i = 0;
  while(my $file = $it->())
  {
    my $dt = get_file_dt($file, "csv.unlink");
    if (defined($dt) && $dt < $last_actual_date) {
       unlink($file);
       $i++;
    }
  }
  return $i;
}

sub find_files
{
  my ($dir, $prefix, $dt_from, $dt_to) = @_;

  my @res_files;

  my $rule = Path::Iterator::Rule->new;
  $rule->name($prefix . "_*.csv");

  my $it = $rule->iter($dir);
  while(my $file = $it->())
  {
    my $dt = get_file_dt($file, 'csv');

    if(defined($dt) &&
      (!defined($dt_from) || DateTime->compare($dt_from, $dt) < 0) &&
      DateTime->compare($dt, $dt_to) <= 0)
    {
      push(@res_files, $file);
    }
  }

  return @res_files;
}

sub find_grouped_files
{
  my ($dir, $prefix, $dt_from, $dt_to) = @_;
  my @res_files = find_files($dir, $prefix, $dt_from, $dt_to);
  my %res_grouped_files;
  foreach my $file(@res_files)
  {
    if($file =~ m|^.*/?[^/]+_(\d{4}\d{2}\d{2}\d{2})(\d{4}-\d{6}-\d{6})?[.]csv$|)
    {
      my $group_name = $1;
      if(!exists($res_grouped_files{$group_name}))
      {
        my @new_arr;
        $res_grouped_files{$group_name} = \@new_arr;
      }
      push(@{$res_grouped_files{$group_name}}, $file);
    }
  }
  return \%res_grouped_files;
}

sub link_bid_files
{
  my ($imp_files_ptr, $click_files_ptr, $action_files_ptr) = @_;
  my @imp_files = @$imp_files_ptr;
  my @click_files = @$click_files_ptr;
  my @action_files = @$action_files_ptr;
  my %clicks;

  my $csv = Text::CSV->new({ sep_char => ',', eol => $/ });

  # load clicks
  # Timestamp,Request ID,URL
  foreach my $click_file(@click_files)
  {
    if(length($click_file) > 0  && open RCLICK_FILE, $click_file)
    {
      # Timestamp,Request ID,URL
      my $head = <RCLICK_FILE>;

      while(<RCLICK_FILE>)
      {
        my $line = $_;
        chomp $line;
        $csv->parse($line);
        my @arr = $csv->fields();
        my $time = shift @arr;
        my $reqid = shift @arr;
        $clicks{$reqid} = exists($clicks{$reqid}) && length($clicks{$reqid}) > 0 ?
          minstr($time, $clicks{$reqid}) :
          $time;
      }

      close RCLICK_FILE;
    }
  }

  # load actions
  my %unsorted_actions;

  # Timestamp,Device,IP Address,UID,URL,Action ID,Order ID,Order Value
  foreach my $action_file(@action_files)
  {
    if(length($action_file) > 0  && open RACTION_FILE, "<:encoding(utf8)", $action_file)
    {
      my $head = <RACTION_FILE>;

      while(<RACTION_FILE>)
      {
        my $line = $_;
        chomp $line;
        $csv->parse($line);
        my @arr = $csv->fields();
        my $time = $arr[0];
        my $timestamp = to_ts($time);
        my $user_id = $arr[3];
        my $action_id = $arr[5];

        my $key = $user_id . '/' . $action_id;
        if(!exists($unsorted_actions{$key}))
        {
          $unsorted_actions{$key} = [];
        }

        push(@{$unsorted_actions{$key}}, $timestamp);
      }

      close RACTION_FILE;
    }
  }

  # post process actions
  my %actions;

  foreach my $key(keys %unsorted_actions)
  {
    my @sorted_arr = sort { $a <=> $b } @{$unsorted_actions{$key}};
    $actions{$key} = \@sorted_arr;
  }

  # read bid file
=comment
  1) Timestamp
  2) Request ID
  3) Global Request ID
  4) Device
  5) IP Address
  6) HID
  7) UID
  8) URL
  9) Publisher ID
  10) Tag ID
  11)External Tag ID
  12)Campaign ID
  13)CCG ID
  14)Campaign Creative ID
  15)Geo Channels
  16)User Channels
  17)Impression Channels
  18)Bid Price
  19)Bid Floor
  20)Algorithm ID
  21)Size ID
  22)Colo ID
  23)Predicted CTR
  24)Campaign Frequency
  25)CR Algorithm ID
  26)Predicted CR
  27)Win Price
  28)Viewability
=cut

  # res format :
  #   <Clicked>,<Click Timestamp>,<Target Action>,<Target Action Timestamp>,<...Bid Fields>
  foreach my $imp_file(@imp_files)
  {
    $imp_file =~ m|^(.*/)?([^/]+_\d{4}\d{2}\d{2}\d{2})(\d{4}-\d{6}-\d{6})?[.]csv$|;

    my %res_files;

    open RBID_FILE, "<:encoding(utf8)", $imp_file or die $!;

    my $head = <RBID_FILE>;

    my $line_i =0;

    while(1)
    {
      #my $line = $_;
      #chomp $line;
      my $arr_ref = $csv->getline(*RBID_FILE);
      if(!defined($arr_ref))
      {
        last; 
      }
      $line_i++;
      my @arr = @$arr_ref;
      #die "'$imp_file' line $line_i doesn't contain values '$line'" if not grep {$_} @arr;
      my $time = shift @arr;

      my $timestamp = to_ts($time);
      my $reqid = shift @arr;

      # read 26 rbid fields
      my @other_bid_fields = splice(@arr, 0, 26);
      my $imp_uid = $other_bid_fields[4];
      my $campaign_id = $other_bid_fields[9];
      my $cc_id = $other_bid_fields[11];

      my @click_fields = (0,'');

      my $link_action;

      if(exists($clicks{$reqid}))
      {
        @click_fields = (1, $clicks{$reqid});
        $link_action = 1;
      }

      my $target_action = undef;
      my @target_action_fields = (0,'');

      if(exists($target_action_config->{$cc_id}))
      {
        $target_action = $target_action_config->{$cc_id};
      }

      if(defined($link_action))
      {
        if(!defined($target_action))
        {
          @target_action_fields = @click_fields;
        }
        elsif(defined($imp_uid) && length($imp_uid) > 0)
        {
          my $action_ids = $target_action->action_ids();
          foreach my $action_id(@$action_ids)
          {
            if(exists($actions{$imp_uid . '/' . $action_id}))
            {
              my $action_timestamps = $actions{$imp_uid . '/' . $action_id};
              my( $low, $high ) = binsearch_range(
                sub { $a <=> $b },
                $timestamp - 2,
                $timestamp + $target_action->timeout(),
                @$action_timestamps);

              if(defined($low))
              {
                if($low <= $high)
                {
                  @target_action_fields = (1, to_ft($action_timestamps->[$low]));
                  last;
                }
              }
            }
          }
        }
      }

      my $res_fh;

      my $target_action_type = defined($target_action) ? $target_action->type() : '';
      my $target_file_key = $target_action_type . $campaign_id;
      
      if(!exists($res_files{$target_file_key}))
      {
        my $res_bid_folder = File::Spec->join(
          $config->att('output_path'), $campaign_id);
        my $res_bid_file = File::Spec->join(
          $res_bid_folder, "/P" . $target_action_type . $2 . ".csv");
        my $write_head = ( ! -e $res_bid_file ) ? 1 : undef;

        if (! -d $res_bid_folder) 
        {
            make_path($res_bid_folder);
        }  

        open($res_fh, '>>', $res_bid_file) or die "Can't open '$res_bid_file': $!";
        
        if($write_head)
        {
        
          lock_file($res_fh);
          print $res_fh "#Target Action,#Target Action Timestamp,label,#Click Timestamp," .
            "Timestamp,#RequestID,#GlobalRequestID,Device,#IPAddress,#HID,#UID," .
            "Link,Publisher,Tag,ETag,Campaign,Group,CCID,GeoCh," .
            "UserCh,#ImpCh,#BidPrice,#BidFloor," .
            "#AlgorithmID,SizeID,Colo,#PredictedCTR," .
            "Campaign_Freq,#CRAlgorithmID,#PredictedCR,#WinPrice,#Viewability\n";
          unlock_file($res_fh);
        }

        $res_files{$target_file_key} = $res_fh;
      }
      else
      {
        $res_fh = $res_files{$target_file_key};
      }

      {
         lock_file($res_fh);
         my @res_row = (@target_action_fields, @click_fields, $time, $reqid, @other_bid_fields);
         $csv->print($res_fh, \@res_row);
         unlock_file($res_fh);
      }
    }

    foreach my $key(keys(%res_files))
    {
      close($res_files{$key});
    }

    close RBID_FILE;

    rename($imp_file, $imp_file . ".unlink"); # unlink source

    if ( ! $continue )
    {
      last;
    }
  }
}

sub process_bid_file_group
{
  my ($imp_file_arr, $max_timeout, $imp_time_to) = @_;

  my @imp_files = @$imp_file_arr;
  my $min_bid_dt;
  my $max_bid_dt;

  foreach my $imp_file(@imp_files)
  {
    my $cur_dt = get_file_dt($imp_file, 'csv');

    if(!defined($cur_dt))
    {
      die "Can't determine DateTime for '" . $imp_file . "'";
    }

    if(!defined($min_bid_dt))
    {
      $min_bid_dt = $cur_dt;
      $max_bid_dt = $min_bid_dt;
    }
    else
    {
      $min_bid_dt = min($min_bid_dt, $cur_dt);
      $max_bid_dt = max($max_bid_dt, $cur_dt);
    }
  }

  # find Click files
  my $click_time_from = $min_bid_dt->clone();
  $click_time_from->add(hours => 
    $click_config->att('from') || DEFAULT_FROM);

  my $click_time_to = $max_bid_dt->clone();
  $click_time_to->add(hours => 
    $click_config->att('to') || DEFAULT_CLICK_TO);

  my $click_path = $click_config->att('path');

  TRACE("Process click path '$click_path' from '$click_time_from' to '$click_time_to'");
  my @click_files = find_files($click_path, 'RClick', $click_time_from, $click_time_to);

  # find Action files
  my $action_time_from = $min_bid_dt->clone();
  $action_time_from->add(hours => 
    $action_config->att('from') || DEFAULT_FROM );

  my $action_time_to = $max_bid_dt->clone();
  $action_time_to->add(hours => $max_timeout / 3600 + (
    $action_config->att('to') || DEFAULT_ACTION_TO));

  my $action_path = $action_config->att('path');

  TRACE("Process action path '$action_path' from '$action_time_from' to '$action_time_to'");
  my @action_files = find_files($action_config->att('path'), 'RAction', $action_time_from, $action_time_to);

  DEBUG("Bid files processed:\n" .
    "  Imp files  to '$imp_time_to' count: " . scalar @imp_files . "\n" .
    "  Click files from '$click_time_from' to '$click_time_to' count: " . 
        scalar @click_files . "\n" .
    "  Action files from '$action_time_from' to '$action_time_to' count: " .
        scalar @action_files . "\n");

  link_bid_files(\@imp_files, \@click_files, \@action_files);
}

#=== Main cycle ===
sub work_()
{
  my $max_timeout = $config->att('max_timeout') || DEFAULT_MAX_TIMEOUT;
  # load target action config
  if(defined($config->att('target_action_config')))
  {
    $target_action_config = 
      load_target_action_config($config->att('target_action_config'));
  }

  foreach my $key(keys %$target_action_config)
  {
    $max_timeout = max($max_timeout, $target_action_config->{$key}->timeout());
  }

  my $now = DateTime->now();
  my $cmp_time = $now->clone();
  # process only files saved more then Impression.to hours ago
  $cmp_time->add(hours => $imp_config->att('to') || DEFAULT_IMPRESSION_TO );

  # find RImpression files without .processed < Impression.to hours from now
  my $imp_path = $imp_config->att('path');
  my $bid_files = find_grouped_files($imp_config->att('path'), 'RImpression', undef, $cmp_time);
  my @bid_files_keys = sort keys %$bid_files;
  foreach my $bid_file_group_key(@bid_files_keys)
  {
    # process grouped by hour files
    process_bid_file_group($bid_files->{$bid_file_group_key}, $max_timeout, $cmp_time);
    if (!$continue)
    {
      last;
    }
  }

  my $bid_files_count = @bid_files_keys;
  DEBUG "Impression path '$imp_path' to '$cmp_time': $bid_files_count processed" ;

  # Remove obsolete processed files
  my $days_to_keep = $config->att('log_days_to_keep') || DEFAULLT_DAYS_TO_KEEP;
  my $impression_removed = 
    remove_obsolete_processed_logs($imp_config->att('path'), 'RImpression', $days_to_keep);
  my $click_removed = 
    remove_obsolete_processed_logs($click_config->att('path'), 'RClick', $days_to_keep);
  my $action_removed = 
    remove_obsolete_processed_logs($action_config->att('path'), 'RAction', $days_to_keep);
  if ($impression_removed || $click_removed || $action_removed)
  {
      DEBUG("Remove obsolete processed logs (older than $days_to_keep days):\n" . 
      "  Imp files count: $impression_removed\n" .
      "  Click files count: $click_removed\n" .
      "  Action files count: $action_removed\n");
  }
}

#=== Daemon operations ===
sub stop
{
  my ($daemon, $pid, $pid_file) = @_;
  if ($pid) {
    print "Stopping pid $pid...\n";
    if ($daemon->Kill_Daemon($pid_file, 15)) {
      print "Successfully stopped.\n";
    } else {
      print "Could not find $pid.  Was it running?\n";
    }
  } else {
    print "Not running, nothing to stop.\n";
  }
}

sub status
{
  my ($pid) = @_;
  if ($pid) {
    print "Running with pid $pid.\n";
  } else {
    print "Not running.\n";
  }
}

sub shutdown
{
  $continue = 0;
}

sub run
{
  my ($daemon, $pid, $pid_file) = @_;
  if (!$pid)
  {
    print "Starting daemon: ";

    $daemon->Init({
      work_dir => $work_dir,
      pid_file => $pid_file_name});

    $SIG{INT} = \&shutdown;
    $SIG{TERM} = \&shutdown;
    $SIG{__DIE__} = sub { ERROR("Caught error: @_"); exit(1); };
   
    my $twig = XML::Twig->new;
    eval { $twig->parsefile($config_file) } or 
      die "Invalid xml config: $@";
    
    $config = $twig->root;

    $imp_config = $config->first_child('cfg:Impression') or
      die "Invalid xml config: impression config is undefined";
    $click_config = $config->first_child('cfg:Click') or
      die "Invalid xml config: click config is undefined";
    $action_config = $config->first_child('cfg:Action') or
      die "Invalid xml config: action config is undefined";
    my $sleep_timeout = 
      $config->att('sleep_timeout') || DEFAULT_SLEEP_TIMEOUT;
    
    # Initialize logger
    my $logger_config = $config->first_child('cfg:Logger') or
      die "Invalid xml config: logger config is undefined";

    my @outputs;
    my $config_log_level = $logger_config->att('log_level');
    my $log_name = $logger_config->att('filename');
    for my $suff ($logger_config->children('cfg:Suffix'))
    {
      my $log_level = $suff->att('max_log_level');
      my $suffix = $suff->att('name');
      push(@outputs, ['File', mode => 'append', newline => 1,
        min_level => get_log_level(
          min($config_log_level, $log_level)),
          filename => $log_name . $suffix ]);
    }

    $logger = Log::Dispatch->new(
       callbacks => $sub,
       outputs => \@outputs);

    INFO("Start merger");

    while ($continue)
    {
      work_();
      my $start_wait_time = time;
      while ($continue && (time - $start_wait_time < $sleep_timeout))
      {
        sleep DEFAULT_SLEEP;
      }
    }

    INFO("Merger finished");
    unlink $pid_file;
  }
  else
  {
      print "Already Running with pid $pid\n";
  }
}

my $operation = $ARGV[0];
my $daemon = Proc::Daemon->new;
my $pid = $daemon->Status($pid_file);

if ($operation eq 'start')
{
  run($daemon, $pid, $pid_file);
}
elsif ($operation eq 'stop')
{
  stop($daemon, $pid, $pid_file);
}
elsif ($operation eq 'status')
{
  status($pid);
}
else
{
  print "Unknown operation: $operation!\n";
}
