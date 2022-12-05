#!/usr/bin/perl -w

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
#   if($prefix eq 'RImpression')
#   {
#     print "DC: $file\n";
#   }

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
    if($file =~ m|^.*/?[^/]+_(\d{4}\d{2}\d{2}\d{2})(\d{4}-\d{6}-\d{6})?[.]csv([.]unlink)?$|)
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
  my ($bid_files, $imp_files_ptr) = @_;
  my @imp_files = @$imp_files_ptr;
  my %imps;

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
  #   <...Bid Fields>,<Win Price>
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
        @imp_fields = ($imp_win_price);
        $skip = undef;
      }

      $other_bid_fields[4] = $imp_uid;

      if(!defined($skip))
      {
        # print
        my $res_bid_file = "RImp/RImpression.csv";
        my $write_head = ( ! -e $res_bid_file ) ? 1 : undef;

        open(my $res_fh, '>>', $res_bid_file) or die "Can't open '$res_bid_file': $!";

        if($write_head)
        {
          print $res_fh "Timestamp,#RequestID,#GlobalRequestID,Device,#IPAddress,#HID,#UID," .
            "URL,Publisher,Tag,ETag,Campaign,Group,CCID,GeoCh," .
            "UserCh,#ImpCh,#BidPrice,#BidFloor," .
            "#AlgorithmID,SizeID,Colo,#PredictedCTR," .
            "Campaign_Freq,#CRAlgorithmID,#PredictedCR,#WinPrice\n";
        }

        my @res_row = ($time, $reqid, @other_bid_fields, @imp_fields);
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
  $imp_time_from->add(hours => -100000);
  my $imp_time_to = $max_bid_dt->clone();
  $imp_time_to->add(hours => +100000);
  my @imp_files = find_files($imp_folder, 'RImpression', $imp_time_from, $imp_time_to);

  print "Process:\n" .
    "  Bid files:" . join(',', @bid_files) . "\n" .
    "  Imp files:" . join(',', @imp_files) . " (from $imp_folder)\n";

  #
  link_bid_files(\@bid_files, \@imp_files);
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

  while(1)
  {
    # eval max timeout
    my $max_timeout = 360000000;

    #
    my $now = DateTime->now();
    my $cmp_time = $now->clone();
    # process only files saved more then 6 hours ago
    $cmp_time->add(hours => -6);

    # find Rbid files without .processed < 6 hours from now
    print "to find RBid files\n";
    my $bid_files = find_grouped_files($bid_folder, 'RBid', undef, $cmp_time);

    #print "process files: " . join(',', @files) . "\n";

    my @all_bid_files;
    foreach my $bid_file_group_key(sort keys %$bid_files)
    {
      push(@all_bid_files, @{$bid_files->{$bid_file_group_key}});
    }

    process_bid_file_group(\@all_bid_files, $max_timeout);

#   print "to process RBid files\n";
#   foreach my $bid_file_group_key(sort keys %$bid_files)
#   {
#     process_bid_file_group($bid_files->{$bid_file_group_key}, $max_timeout);
#   }
  }
}

main(\@ARGV);
