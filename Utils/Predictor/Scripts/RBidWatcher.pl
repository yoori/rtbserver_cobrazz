
use strict;
use warnings;
use DateTime;
use File::Find;
use Path::Iterator::Rule;
use Text::CSV;
#use Tie::Array::Sorted;
use List::Util qw[min max minstr];
use List::BinarySearch::XS qw( :all );
use List::BinarySearch qw(binsearch_range);
use Class::Struct;

use Args;

struct(TargetAction => [action_ids => '$', timeout => '$', type => '$']);

my $def_folder = '.';

my $bid_folder;
my $imp_folder;
my $click_folder;
my $action_folder;
my $processed_bid_folder;

my $target_action_config = {};

sub load_target_action_config
{
  my ($target_action_config_file) = @_;
  my %res;

  my $csv = Text::CSV->new({ sep_char => ',', eol => $/ });

  open AC_FILE, $target_action_config_file || die "Can't open '$target_action_config_file'";
  while(<AC_FILE>)
  {
    my $line = $_;
    chomp $line;
    $csv->parse($line);

    my @arr = $csv->fields();
    my $cc_id = $arr[0];
    my $action_type = $arr[1];
    my @action_ids = split('\|', $arr[2]);
    my $action_timeout = $arr[3];
    $res{$cc_id} = new TargetAction(
      action_ids => \@action_ids,
      timeout => length($action_timeout) > 0 ? $action_timeout : undef,
      type => $action_type);
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
  my ($bid_files, $imp_files_ptr, $click_files_ptr, $action_files_ptr) = @_;
  my @imp_files = @$imp_files_ptr;
  my @click_files = @$click_files_ptr;
  my @action_files = @$action_files_ptr;
  my %imps;
  my %clicks;

  my $csv = Text::CSV->new({ sep_char => ',', eol => $/ });

  # load imps
  # Timestamp,Request ID,UID,Win Price
  foreach my $imp_file(@imp_files)
  {
    if(length($imp_file) > 0 && open RIMP_FILE, $imp_file)
    {
      my $head = <RIMP_FILE>;

      while(<RIMP_FILE>)
      {
        my $line = $_;
        chomp $line;
        if($csv->parse($line))
        {
          my @arr = $csv->fields();
          my $time = shift @arr;
          my $reqid = shift @arr;
          my $uid = shift @arr;
          my $win_price = shift @arr;

          my $rewrite = 1;
          if(exists($imps{$reqid}))
          {
            $csv->parse($imps{$reqid});
            my @old_arr = $csv->fields();
            my $old_time = shift @old_arr;
            my $old_reqid = shift @old_arr;
            my $old_uid = shift @old_arr;
            if(defined($old_uid) && length($old_uid) > 0 &&
              !(defined($uid) && length($uid) > 0))
            {
              $rewrite = undef;
            }
            elsif($old_time < $time)
            {
              $rewrite = undef;
            }
          }

          if($rewrite)
          {
            $imps{$reqid} = $line;
          }
        }
        else
        {
          die "Line could not be parsed: $line\n";
        }
      }

      close RIMP_FILE;
    }
  }

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
    if(length($action_file) > 0  && open RACTION_FILE, $action_file)
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
  foreach my $bid_file(@$bid_files)
  {
    $bid_file =~ m|^(.*/)?([^/]+_\d{4}\d{2}\d{2}\d{2})(\d{4}-\d{6}-\d{6})?[.]csv$|;

    #print "MERGE INTO '$res_bid_file' <= bid: $bid_file, imps: " . join(',', @imp_files) . ", clicks: " . join(',', @click_files) . "\n";

    my %res_files;

    open RBID_FILE, $bid_file or die $!;

    my $head = <RBID_FILE>;

    while(<RBID_FILE>)
    {
      my $line = $_;
      chomp $line;
      $csv->parse($line);

      my @arr = $csv->fields();
      my $time = shift @arr;
      my $timestamp = to_ts($time);
      my $reqid = shift @arr;
      # read 21 rbid fields
      my @other_bid_fields = splice(@arr, 0, 21);
      my $cc_id = $other_bid_fields[8];
      my $imp_uid = '';

      my @imp_fields = ('', '', '');
      my @click_fields = (0,'');

      my $skip = 1;

      if(exists($imps{$reqid}))
      {
        # Timestamp,Request ID,UID,Win Price,Clicked,ClickTimestamp
        $csv->parse($imps{$reqid});
        my @imp_arr = $csv->fields();

        my $imp_timestamp = shift @imp_arr;
        shift @imp_arr; # reqid
        $imp_uid = shift @imp_arr;
        my $imp_win_price = shift @imp_arr;
        @imp_fields = ($imp_timestamp , $imp_uid , $imp_win_price);
        $skip = undef;
      }

      my $link_action;

      if(exists($clicks{$reqid}))
      {
        @click_fields = (1, $clicks{$reqid});
        $skip = undef;
        $link_action = 1;
        #print "to find action for '$cc_id'\n";
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

      if(!defined($skip))
      {
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
            print $res_fh "Target Action,Target Action Timestamp,Clicked,Click Timestamp,Timestamp,Request ID,Global Request ID,Device,IP Address," .
              "HID,UID,URL,Tag ID,External Tag ID,Campaign Creative ID,Geo Channels,User Channels," .
              "Impression Channels,Bid Price,Bid Floor,Algorithm ID,Size ID,Colo ID," .
              "Predicted CTR,Campaign Frequency,CR Algorithm ID,Predicted CR,".
              # imp
              "Impression Timestamp,Impression UID,Win Price\n";
          }

          $res_files{$target_action_type} = $res_fh;
        }
        else
        {
          $res_fh = $res_files{$target_action_type};
        }

        my @res_row = (@target_action_fields, @click_fields, $time, $reqid, @other_bid_fields, @imp_fields);
        $csv->print($res_fh, \@res_row);
      }
    }

    foreach my $key(keys(%res_files))
    {
      close($res_files{$key});
    }

    close RBID_FILE;

    rename($bid_file, $bid_file . ".unlink"); # unlink source
  }
}

sub process_bid_file_group
{
  my ($bid_file_arr, $max_timeout) = @_;

  my @bid_files = @$bid_file_arr;
  my $min_bid_dt;
  my $max_bid_dt;

  foreach my $bid_file(@bid_files)
  {
    my $cur_dt = get_file_dt($bid_file);

    if(!defined($cur_dt))
    {
      die "Can't determine DateTime for '" . $bid_file . "'";
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

  # find Imp files
  my $imp_time_from = $min_bid_dt->clone();
  $imp_time_from->add(hours => -1);
  my $imp_time_to = $max_bid_dt->clone();
  $imp_time_to->add(hours => +2);
  my @imp_files = find_files($imp_folder, 'RImpression', $imp_time_from, $imp_time_to);

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
    "  Bid files:" . join(',', @bid_files) . "\n" .
    "  Imp files:" . join(',', @imp_files) . "\n" .
    "  Click files:" . join(',', @click_files) . "\n" .
    "  Action files:" . join(',', @action_files) . "\n";

  #
  link_bid_files(\@bid_files, \@imp_files, \@click_files, \@action_files);
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

  $bid_folder = exists($args{"bid-folder"}) ? $args{"bid-folder"} : $folder;
  $imp_folder = exists($args{"imp-folder"}) ? $args{"imp-folder"} : $folder;
  $click_folder = exists($args{"click-folder"}) ? $args{"click-folder"} : $folder;
  $action_folder = exists($args{"action-folder"}) ? $args{"action-folder"} : $folder;
  $processed_bid_folder = exists($args{"res-folder"}) ? $args{"res-folder"} : $folder;

  # eval max timeout
  my $max_timeout;

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
    # load target action config
    if(exists($args{'config'}))
    {
      $target_action_config = load_target_action_config($args{'config'});
    }

    #
    my $now = DateTime->now();
    my $cmp_time = $now->clone();
    # process only files saved more then 6 hours ago
    $cmp_time->add(hours => -6);

    # find Rbid files without .processed < 6 hours from now
    print "to find RBid files\n";
    my $bid_files = find_grouped_files($bid_folder, 'RBid', undef, $cmp_time);

    #print "process files: " . join(',', @files) . "\n";

    print "to process RBid files\n";
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
