
package CCGBudgetTest;

use strict;
use warnings;
use POSIX qw(strftime ceil);
use Time::HiRes qw(gettimeofday);

use DB::Defaults;
use DB::Util;

use constant MINUTE => 60;
use constant HOUR => 60 * MINUTE;
use constant DAY => 24 * HOUR;
use constant DEFAULT_BUDGET => 10_000_000;


sub creative_revenue 
{
  my ($self, $params) = @_;
  if (defined $params->{cpm})
  {
    return $params->{cpm} / 1000;
  }
  if (defined $params->{cpc})
  {
    return $params->{cpc};
  }
  if (defined $params->{cpa})
  {
    return $params->{cpa};
  }
  return 0;
}

sub channel_revenue
{
  my ($self, $params) = @_;
  if (defined $params->{channel_rate})
  {
    return $params->{channel_rate} / 1000;
  }
  return 0;
}

sub request_revenue
{
  my ($self, $params) = @_;
  return 
    $self->channel_revenue($params) +
    $self->creative_revenue($params);
}

sub budget
{
  my ($self, $params) = @_;
  foreach my $budget_fld (qw(budget ccg_daily_budget))
  {
    if (defined $params->{$budget_fld})
    {
      return $params->{$budget_fld};
    }
  }
}

sub create_campaign
{
  my ($self, $ns, $prefix, $params) = @_;

  my $keyword = make_autotest_name($ns, $prefix);

  my $tzname = defined $params->{timezone} ? $params->{timezone} : 'GMT';

  my $account = defined $params->{account}
    ? $params->{account}
    : $ns->create(Account => {
        name => "Account$prefix",
        role_id => DB::Defaults::instance()->advertiser_role(),
        timezone_id => DB::TimeZone->blank(tzname => $tzname),
        currency_id => defined $params->{exchange_rate}
          ? $ns->create(Currency => { rate => $params->{exchange_rate} })
          : DB::Defaults::instance()->currency() } );

  $ns->output("$prefix/TIMEZONE", $tzname);

  $ns->output("$prefix/TZDATE",
    strftime("%d-%m-%Y", gmtime(time_in_tz($ns, $self->{now}, $tzname))));

  my $sites = [];
  foreach my $site (ref $params->{publishers} eq 'ARRAY'
                      ? @{$params->{publishers}}
                      : $params->{publishers})
  {
    push @$sites, { site_id => $site->{site_id} };
  }

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign' . $prefix,
    account_id => $account,
    campaigncreativegroup_cpm => $params->{cpm},
    campaigncreativegroup_cpc => $params->{cpc},
    campaigncreativegroup_cpa => $params->{cpa},
    campaigncreativegroup_budget =>
        defined $params->{budget}? 
          $params->{budget}: DEFAULT_BUDGET,
    campaigncreativegroup_daily_budget => 
       $params->{ccg_daily_budget},
    campaigncreativegroup_delivery_pacing => 
       defined $params->{ccg_delivery_pacing}? 
          $params->{ccg_delivery_pacing} : 'F',
    campaigncreativegroup_date_end =>
      defined $params->{ccg_date_end}?
        DB::Entity::Oracle->sql(
         "to_date('$params->{ccg_date_end}', " .
           "'YYYY-MM-DD HH24:MI:SS')"): undef,
    site_links => $sites,
    targeting_channel_id => undef,
    channel_id => DB::BehavioralChannel->blank(
      account_id => defined $params->{channel_rate}?
        $ns->create(Account => {
          name => "CMP$prefix",
          role_id => DB::Defaults::instance()->cmp_role()}):
        $account,
      name => "Channel" . $prefix,
      keyword_list => $keyword,
      cpm => $params->{channel_rate},
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P") ])
    });

  $ns->output($prefix . "/KWD", $keyword);
  $ns->output($prefix . "/CCG", $campaign->{ccg_id});
  $ns->output($prefix . "/CC", $campaign->{cc_id});
  $ns->output($prefix . "/Revenue", $self->request_revenue($params));
  $ns->output($prefix . "/Budget", $self->budget($params));
  if (defined $params->{ccg_date_end})
  {
    $ns->output(
      $prefix . "/DateEnd", 
      $params->{ccg_date_end});   
  }
}

sub time_in_tz
{
  my ($ns, $utc, $tzname) = @_;

  my ($tz_offset) = $ns->pq_dbh->selectrow_array(q[
    select now() - now() at time zone 'GMT' at time zone tzname from 
    timezone where tzname = ?;
    ], undef, $tzname);

  chop $tz_offset;

  my ($sign, $hours, $minutes, $secs) = 
    $tz_offset =~ m/^\s*(\+|-)?([0-9]+):([0-9]+):([0-9]+)\s*$/;

  my $offset = $hours * HOUR + $minutes * MINUTE;

  $offset = 0 - $offset if defined $sign && $sign eq "-";

  return $utc + $offset;
}

sub init
{
  my ($self, $ns) = @_;

  my $now = [ Time::HiRes::gettimeofday() ];
  $self->{now} = $now->[0]; # trim usec

  my $publisher1 = $ns->create(
    Publisher => { name => 'Pub1' });

  my $publisher2 = $ns->create(
    Publisher => { name => 'Pub2' });

  my $publisher3 = $ns->create(
    Publisher => { name => 'Pub3' });

  my $advertiser = $ns->create(Account => {
    name => 'Adv',
    role_id => 
      DB::Defaults::instance()->advertiser_role() });

  my $cpm = 1000;

  $self->create_campaign(
     $ns, "FixedDailyBudget",
     { cpm => $cpm,
       ccg_daily_budget => 2 * $cpm / 1000, # reached by two impressions
       publishers => [ $publisher1, $publisher2, $publisher3 ],
       account => $advertiser });

  foreach my $case ({name => "DynamicDailyBudget1", budget => 10},
                    {name => "DynamicDailyBudget2", budget => 30},
                    {name => "DynamicDailyBudget3", budget => 22})
  {
    $self->create_campaign($ns, $case->{name},
      { cpm => $cpm,
        budget => $case->{budget} * $cpm / 1000,
        ccg_delivery_pacing => 'D',
        # 10 days to end
        ccg_date_end => strftime("%F 00:00:00", gmtime($now->[0] + 9 * DAY)),
        publishers => [ $publisher1, $publisher2, $publisher3 ],
        account => $advertiser });
  }

  $self->create_campaign($ns, "ChannelRate",
    { cpm => 800,
      channel_rate => 200,
      ccg_daily_budget => 2,
      publishers => [ $publisher1, $publisher2 ],
      account => $advertiser });

  my $gross_advertiser = $ns->create(Account => {
    name => 'GrossAdv',
    commission => 0.1,
    agency_account_id => 
        DB::Defaults::instance()->agency_gross(),
    role_id => 
      DB::Defaults::instance()->advertiser_role() });

  $self->create_campaign(
     $ns, "GrossAdvertiser",
     { cpc => 1,
       ccg_daily_budget => 1,
       publishers => [ $publisher1, $publisher2 ],
       account => $gross_advertiser });

  my $net_advertiser = $ns->create(Account => {
    name => 'NetAdv',
    commission => 0.5,
    role_id => 
      DB::Defaults::instance()->advertiser_role() });

  $self->create_campaign(
     $ns, "NetAdvertiser",
     { cpc => 1,
       ccg_daily_budget => 2,
       publishers => [ $publisher1, $publisher2 ],
       account => $net_advertiser });

  $self->create_campaign(
     $ns, "TZAdvertiser",
     { cpm => 1000,
       ccg_daily_budget => 2,
       publishers => [ $publisher1, $publisher2 ],
       timezone => 'America/Sao_Paulo' });  

  my ($tz_positive_offset, $tz_negative_offset) =
    ('Europe/Istanbul', 'America/Sao_Paulo');

  my ($time_in_pos_tz, $time_in_neg_tz) = 
    (time_in_tz($ns, $now->[0], $tz_positive_offset),
     time_in_tz($ns, $now->[0], $tz_negative_offset));

  $ns->output("PositiveTZDate", strftime("%d-%m-%Y", gmtime($time_in_pos_tz)));
  $ns->output("NegativeTZDate", strftime("%d-%m-%Y", gmtime($time_in_neg_tz)));
  $ns->output("GMTDate", strftime("%d-%m-%Y", gmtime($now->[0])));

  foreach my $case ("DDBudgetPosTZ1", "DDBudgetPosTZ2")
  {
    $self->create_campaign($ns, $case,
      { cpm => $cpm,
        budget => 30 * $cpm / 1000,
        ccg_delivery_pacing => 'D',
        timezone => $tz_positive_offset,
        # 2 days in GMT, but 3 days in Istanbul
        ccg_date_end => strftime("%F 01:00:00", gmtime($time_in_pos_tz + 2 * DAY)),
        publishers => [ $publisher1, $publisher2, $publisher3 ] });
  }

  foreach my $case ("DDBudgetNegTZ", "DDBudgetNegTZMarker")
  {
    $self->create_campaign($ns, $case,
      { cpm => $cpm,
        budget => 30 * $cpm / 1000,
        ccg_delivery_pacing => 'D',
        timezone => $tz_negative_offset,
        # 3 days in GMT, but 2 days in Sao Paulo
        ccg_date_end => strftime("%F 23:00:00", gmtime($time_in_neg_tz + 1 * DAY)),
        publishers => [ $publisher1, $publisher2, $publisher3 ],
        exchange_rate => 2 });
  }

  $ns->output("Tid1", $publisher1->{tag_id});
  $ns->output("Tid2", $publisher2->{tag_id});
  $ns->output("Tid3", $publisher3->{tag_id});
}

1;
