package DB::Util;

use warnings;
use strict;
use DB::EntitiesImpl;
require DB::Entities::BehavioralChannel;
use POSIX qw(strftime mktime);
use Time::Local;

use vars qw(@ISA @EXPORT);

use DB::Defaults qw(:constants);

use Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(get_config money_upscale money_downscale
             calc_min_cpm min max calc_ta_top_acpc make_autotest_name
             print_NoAdvNoTrack create_behavioral_parameters check_threshold_channel
             calc_ctr_pq sync get_tz_ofset output_channel_triggers get_tanx_creative
             get_baidu_creative);

sub create_behavioral_parameters {
    my ($ns, $acc, $format, $defaults, @triggers) = @_;

    my $i = 0;
    my @channels = ();
    while (my ($trigger_type, $trigger_list) = splice(@triggers, 0, 2)) {
        my $name = sprintf($format, ++$i);

        my $trigger_list_key =
          $trigger_type eq "U" ? 'url_list' : 'keyword_list';

        my %args = %$defaults;
        $args{trigger_type} = $trigger_type;

        my $bp = DB::BehavioralChannel::BehavioralParameter->blank(%args);

        my $ch = $ns->create(
          DB::BehavioralChannel->blank(
            name => $name,
            account_id => $acc,
            $trigger_list_key => $trigger_list,
            behavioral_parameters => [ $bp ]));

        push(@channels, $ch);

        $ns->output($name, $ch->channel_id() . $bp->{trigger_type});

        $ns->output("CHof".$name, $ch->channel_id());
    }
    return @channels;
}

sub get_config ($$) {
    my ($dbh, $name) = @_;

    my $stmt = $dbh->prepare(q{
      SELECT PARAM_VALUE from ADSCONFIG WHERE PARAM_NAME=?
      });

    $stmt->execute($name);
    my $res = $stmt->fetchrow_arrayref;
    return $res->[0] if $res;
    die "Error: Get param from ADSCONFIG { $name }";
}

sub money_upscale {
    my ($money, $fraction) = @_;

    use POSIX qw(ceil);
    use constant EPSILON => 10 ** -10;

    $fraction = DB::Currency::DEFAULT_FRACTION_DIGITS
      unless defined $fraction;
    my $scale = 10 ** $fraction;
    $money = ceil($money * $scale - EPSILON) / $scale;

    return $money;
}

sub money_downscale {
    my ($money, $fraction) = @_;

    use POSIX qw(floor);
    use constant EPSILON => 10 ** -10;

    $fraction = DB::Currency::DEFAULT_FRACTION_DIGITS
      unless defined $fraction;
    my $scale = 10 ** $fraction;
    $money = floor($money * $scale - EPSILON) / $scale;

    return $money;
}

sub min {
    my ($l, $r) = @_;
    if ($l < $r)
    {
        return $l;
    }
    return $r;
}

sub max {
    my ($l, $r) = @_;
    if ($l > $r)
    {
        return $l;
    }
    return $r;
}

sub calc_ta_top_acpc
{
  my ($top_cpc, $under_top_cpc, $min_ecpm, $sum_cpm, $ctr_in) = @_;
  my $ctr = GLOBAL_CTR * 1000;
  if ($ctr_in)
  {
    $ctr = $ctr_in;
  }

  my $actual_ecpm = min(money_upscale($top_cpc*$ctr),
    max(money_upscale($under_top_cpc*$ctr),
      $min_ecpm - money_upscale($sum_cpm*$ctr)));
  return money_upscale($actual_ecpm/$ctr);
}

sub make_autotest_name {
  my ($ns, $name) = @_;
  (my $autotest_name = $ns->namespace . $name) =~ s/\W//g;
  return $autotest_name;
}

sub print_NoAdvNoTrack
{
  my ($ns) = @_;

  $ns->output("no_track_words", 
    DB::Defaults::instance()->no_track_channel->
      keyword_channel_triggers()->[0]->{original_trigger});
  $ns->output("no_adv_words",
    DB::Defaults::instance()->no_adv_channel->
      keyword_channel_triggers()->[0]->{original_trigger});
  $ns->output("no_track_urls", 
    DB::Defaults::instance()->no_track_channel->
      url_channel_triggers()->[0]->{original_trigger});
  $ns->output("no_adv_urls", 
    DB::Defaults::instance()->no_adv_channel->
      url_channel_triggers()->[0]->{original_trigger});
  $ns->output("no_track_url_words", 
    DB::Defaults::instance()->no_track_channel->
      url_kwd_channel_triggers()->[0]->{original_trigger});
  $ns->output("no_adv_url_words",
    DB::Defaults::instance()->no_adv_channel->
      url_kwd_channel_triggers()->[0]->{original_trigger});

}

sub unwrap {
    my ($obj) = @_;
    use Scalar::Util qw(blessed);
    if (blessed($obj))
    {
        return $obj->{$obj->_id()};
    }
    return $obj;
}

sub calc_ctr_pq  {
  my $dbh  = $_[0]->_pq_dbh();
  return if !$dbh;
  my @ctr_functions = 
    qw(init pub_tag_adjustments
       keyword_targeted_text_groups
       keyword_targeted_text_groups_tow
       cpc_groups);
  for my $f (@ctr_functions) {
    $dbh->do("select ctr." . $f . "();");
  }
}

sub get_tz_ofset {
  my ($ns, $tz_name) = @_;  
  my $stmt = $ns->pq_dbh->prepare_cached(q|
    select now() - now() at time zone 'GMT' at time zone tzname from 
    timezone where tzname = ?|, undef, 1);
  $stmt->execute($tz_name);
  my @result_row = $stmt->fetchrow_array() or
     die "Invalid timezone name '$tz_name'";
  return $1 + $2 / 60 if $result_row[0] =~ /^.*(\d{2}):(\d{2}):(\d{2}).*$/;
}

sub output_channel_triggers
{
  my ($ns, $prefix, $channel, $type) = @_;

  $type = 'P' unless defined $type;

  my $i = 0;

  my $triggers = $type eq 'P'? 
    $channel->keyword_channel_triggers():
      $type eq 'S'? $channel->search_channel_triggers():
        $type eq 'R'? $channel->url_kwd_channel_triggers():
          $channel->url_channel_triggers();

  for my $trigger (@$triggers)
  {
    $ns->output("$prefix/" . ++$i, 
      ($type eq 'P'? $channel->page_key():
       $type eq 'S'? $channel->search_key():
       $type eq 'R'? $channel->url_kwd_key():
         $channel->url_key())  . " :: " . 
           $trigger->{channel_trigger_id}); 
  }
}

sub get_tanx_creative {
  my $creative = shift;

  my ($year, $month, $day, $hour, $min, $sec, $nsec) = 
    ($creative->{version} =~ 
       m!(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})\.(\d+)!);

  my $time = timegm ($sec, $min, $hour, $day, $month - 1, $year);

  $creative->{creative_id} . "-" . 
    strftime("%y%m%d%H%M%S", gmtime($time));
}

sub get_baidu_creative {
  my $creative = shift;

  my ($year, $month, $day, $hour, $min, $sec, $nsec) = 
    ($creative->{version} =~ 
       m!(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})\.(\d+)!);

  my $time = timegm ($sec, $min, $hour, $day, $month - 1, $year);

  ($creative->{creative_id} << 32) + $time;
}

1;
