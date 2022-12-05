#!/usr/bin/perl -w

use strict;
use warnings;
use DateTime;
use File::Find;
use Path::Iterator::Rule;
use Text::CSV_XS;
#use Tie::Array::Sorted;
use List::Util qw[min max minstr];
use List::BinarySearch::XS qw( :all );
use List::BinarySearch qw(binsearch_range);
use Class::Struct;

use Args;

struct(TargetAction => [action_ids => '$', timeout => '$', type => '$']);

my $def_folder = '.';

my $imp_folder;
my $click_folder;
my $action_folder;
my $processed_bid_folder;

my $target_action_config = {};

sub load_target_action_config
{
  my ($target_action_config_file) = @_;
  my %res;

  my $csv = Text::CSV_XS->new({ sep_char => ',', eol => $/ });

  open AC_FILE, $target_action_config_file || die "Can't open '$target_action_config_file'";
  while(<AC_FILE>)
  {
    my $line = $_;
    chomp $line;
    $csv->parse($line);
    my @arr = $csv->fields();

    if(scalar(@arr) > 1)
    {
      my $cc_id = $arr[0];
      my $action_type = $arr[1];
      my @action_ids = split('\|', $arr[2]);
      my $action_timeout = $arr[3];
      $res{$cc_id} = new TargetAction(
        action_ids => \@action_ids,
        timeout => length($action_timeout) > 0 ? $action_timeout : undef,
        type => $action_type);
    }
  }

  return \%res;
}

# convert %F %T to epoch timestamp
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
  my ($file) = @_;

  if($file =~ m|^.*/?[^/]+_(\d{4})(\d{2})(\d{2})(\d{2})(\d{4}-\d{6}-\d{6})?[.]csv$|)
  {
    return DateTime->new(
      year => $1,
      month => $2,
      day => $3,
      hour => $4);
  }

  return undef;
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
    my $dt = get_file_dt($file);

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

  my $csv = Text::CSV_XS->new({ sep_char => ',', eol => $/ });

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

        if(scalar(@arr) > 1)
        {
          my $time = shift @arr;
          my $reqid = shift @arr;
          $clicks{$reqid} = exists($clicks{$reqid}) && length($clicks{$reqid}) > 0 ?
            minstr($time, $clicks{$reqid}) :
            $time;
        }
      }

      close RCLICK_FILE;
    }
  }

  # load actions
  my %unsorted_actions;

  # Timestamp,Device,IP Address,UID,URL,Action ID,Order ID,Order Value
  foreach my $action_file(@action_files)
  {
    if(length($action_file) > 0  && open RACTION_FILE, $action_file)
    {
      my $head = <RACTION_FILE>;

      while(<RACTION_FILE>)
      {
        my $line = $_;
        chomp $line;
        $csv->parse($line);
        my @arr = $csv->fields();

        if(scalar(@arr) > 1)
        {
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
  9) Tag ID
  10)External Tag ID
  11)Campaign Creative ID
  12)Geo Channels
  13)User Channels
  14)Impression Channels
  15)Bid Price
  16)Bid Floor
  17)Algorithm ID
  18)Size ID
  19)Colo ID
  20)Predicted CTR
  21)Campaign Frequency
  22)CR Algorithm ID
  23)Predicted CR
=cut

  # res format :
  #   <Clicked>,<Click Timestamp>,<Target Action>,<Target Action Timestamp>,<...Bid Fields>,<Impression Timestamp>,<Impression UID>,<Win Price>
  foreach my $imp_file(@imp_files)
  {
    $imp_file =~ m|^(.*/)?([^/]+_\d{4}\d{2}\d{2}\d{2})(\d{4}-\d{6}-\d{6})?[.]csv$|;

    #print "MERGE INTO '$res_bid_file' <= bid: $bid_file, imps: " . join(',', @imp_files) . ", clicks: " . join(',', @click_files) . "\n";

    my %res_files;

    open RBID_FILE, $imp_file or die $!;

    my $head = <RBID_FILE>;

    while(<RBID_FILE>)
    {
      my $line = $_;
      chomp $line;
      $csv->parse($line);
      my @arr = $csv->fields();

      if(scalar(@arr) > 1)
      {
        my $time = shift @arr;
        my $timestamp = to_ts($time);
        my $reqid = shift @arr;
        # read 25 rbid fields
        my @other_bid_fields = splice(@arr, 0, 25);
        my $imp_uid = $other_bid_fields[4];
        my $cc_id = $other_bid_fields[11];

        #print "CC_ID: $cc_id\n";

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
          #if(defined($target_action))
          #{
          #  print "link action, imp_uid=$imp_uid,action_id=" . $target_action->action_id() . "\n";
          #}

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

                #print "to find action in [" . to_ft($timestamp - 2) . "," . to_ft($timestamp + $target_action->timeout()) . "]:";
                #foreach my $act(@$action_timestamps)
                #{
                #  print " " . to_ft($act);
                #}
                #print "; low = " . (defined($low) ? $low : 'undef') . ", high = " . (defined($high) ? $high : 'undef') . "\n";

                if(defined($low))
                {
                  if($low <= $high)
                  {
                    @target_action_fields = (1, to_ft($action_timestamps->[$low]));
                    last;
                  }
                  #else
                  #{
                  #  die "unexpected result of search: low=$low, high=$high, intvl=[" .
                  #     ($timestamp - 2) . "," . ($timestamp + $target_action->timeout()) . "], arr=[" . join(',', @cc) . "]";
                  #}
                }
              }
            }
          }
        }

        # print
        my $res_fh;

        my $target_action_type = defined($target_action) ? $target_action->type() : '';

        if(!exists($res_files{$target_action_type}))
        {
          my $res_bid_file = $processed_bid_folder . "/P" . $target_action_type . $2 . ".csv";
          my $write_head = ( ! -e $res_bid_file ) ? 1 : undef;

          open($res_fh, '>>', $res_bid_file) or die "Can't open '$res_bid_file': $!";

          if($write_head)
          {
            print $res_fh "Target Action,#Target Action Timestamp,Clicked,#Click Timestamp," .
              "Timestamp,#RequestID,#GlobalRequestID,Device,#IPAddress,#HID,#UID," .
              "Link,Publisher,Tag,ETag,Campaign,Group,CCID,GeoCh," .
              "UserCh,#ImpCh,#BidPrice,#BidFloor," .
              "#AlgorithmID,SizeID,Colo,#PredictedCTR," .
              "Campaign_Freq,#CRAlgorithmID,#PredictedCR,#WinPrice\n";
          }

          $res_files{$target_action_type} = $res_fh;
        }
        else
        {
          $res_fh = $res_files{$target_action_type};
        }

        my @res_row = (@target_action_fields, @click_fields, $time, $reqid, @other_bid_fields);

        $csv->print($res_fh, \@res_row);
      }
    }

    foreach my $key(keys(%res_files))
    {
      close($res_files{$key});
    }

    close RBID_FILE;

    rename($imp_file, $imp_file . ".unlink"); # unlink source
  }
}

sub process_bid_file_group
{
  my ($imp_file_arr, $max_timeout) = @_;

  my @imp_files = @$imp_file_arr;
  my $min_bid_dt;
  my $max_bid_dt;

  foreach my $imp_file(@imp_files)
  {
    my $cur_dt = get_file_dt($imp_file);

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
  $click_time_from->add(hours => -1);
  my $click_time_to = $max_bid_dt->clone();
  $click_time_to->add(hours => +4);
  my @click_files = find_files($click_folder, 'RClick', $click_time_from, $click_time_to);

  # find Action files
  my $action_time_from = $min_bid_dt->clone();
  $action_time_from->add(hours => -1);
  my $action_time_to = $max_bid_dt->clone();
  $action_time_to->add(hours => $max_timeout / 3600 + 5);
  my @action_files = find_files($action_folder, 'RAction', $action_time_from, $action_time_to);

  print "Process:\n" .
    "  Imp files:" . join(',', @imp_files) . "\n" .
    "  Click files:" . join(',', @click_files) . "\n" .
    "  Action files:" . join(',', @action_files) . "\n";

  #
  link_bid_files(\@imp_files, \@click_files, \@action_files);
}

sub main
{
  my ($argv) = @_;
  my $args_ptr = Args::parse(@$argv);
  my %args = %$args_ptr;

  my $folder = '.';

  if(exists($args{"folder"}))
  {
    $folder = $args{"folder"};
  }

  $imp_folder = exists($args{"imp-folder"}) ? $args{"imp-folder"} : $folder;
  $click_folder = exists($args{"click-folder"}) ? $args{"click-folder"} : $folder;
  $action_folder = exists($args{"action-folder"}) ? $args{"action-folder"} : $folder;

  $processed_bid_folder = exists($args{"res-folder"}) ? $args{"res-folder"} : $folder;

  # load target action config
  if(exists($args{'config'}))
  {
    $target_action_config = load_target_action_config($args{'config'});
  }

  # eval max timeout
  my $max_timeout = 3600;

  foreach my $key(keys %$target_action_config)
  {
    if(!defined($max_timeout))
    {
      $max_timeout = $target_action_config->{$key}->timeout();
    }
    else
    {
      $max_timeout = max($max_timeout, $target_action_config->{$key}->timeout());
    }
  }

  while(1)
  {
    #
    my $now = DateTime->now();
    my $cmp_time = $now->clone();
    # process only files saved more then 6 hours ago
    $cmp_time->add(hours => -6);

    # find RImpression files without .processed < 6 hours from now
    print "to find RImpression files\n";
    my $bid_files = find_grouped_files($imp_folder, 'RImpression', undef, $cmp_time);

    print "to process RImpression files\n";
    foreach my $bid_file_group_key(sort keys %$bid_files)
    {
      process_bid_file_group($bid_files->{$bid_file_group_key}, $max_timeout);
    }

    #TODO: remove expired files (>24 hours ago)
    print "to sleep\n";

    #last;
    sleep(600);
  }
}

main(\@ARGV);
