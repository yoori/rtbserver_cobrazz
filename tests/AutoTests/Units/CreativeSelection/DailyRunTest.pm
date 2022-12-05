
package DailyRunTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant {
  MONDAY => 0,
  TUESDAY => 1,
  WEDNESDAY => 2,
  THURSDAY => 3,
  FRIDAY => 4,
  SATURDAY => 5,
  SUNDAY => 6,
  YDAY => 7,
  ISDST => 8,
};

use constant WEEKDAYS => (
  MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY);

sub DailyRunTest_create_testcase
{
  my ($self, $ns, $site, $timezone, $ccg_intervals, $campaign_intervals, $name) = @_;

  $name = $timezone unless defined $name;

  my $advertiser = $ns->create(Account => {
    name => $name,
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type,
    timezone_id => DB::TimeZone->blank(tzname => $timezone) });

  my $campaign = $ns->create(DisplayCampaign => {
    name => $name,
    account_id => $advertiser,
    site_links => [{site_id => $site}],
    campaigncreativegroup_flags => 8 | DB::Campaign::INCLUDE_SPECIFIC_SITES, # use CCGSchedule
    channel_id => DB::BehavioralChannel->blank(
      account_id => $advertiser,
      name => $name,
      keyword_list => $name,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ])
     });

  sub to_minutes
  {
    my ($day, $time) = @_;
    return $day * 24 * 60 + $1 * 60 + $2 if defined $time and $time =~ /^(\d{2}):(\d{2})$/;
  }

  foreach my $interval (@$ccg_intervals)
  {
    my @days = defined $interval->[0]? ($interval->[0]) : WEEKDAYS;
    for my $day (@days)
    {
      $ns->create(CCGSchedule => {
        ccg_id => $campaign->{ccg_id},
        time_from => to_minutes($day, $interval->[1]),
        time_to => to_minutes($day, $interval->[2]) });
    }
  }

  foreach my $interval (@$campaign_intervals)
  {
    my @days = defined $interval->[0]? ($interval->[0]) : WEEKDAYS;
    for my $day (@days)
    {
      $ns->create(CampaignSchedule => {
        campaign_id => $campaign->{campaign_id},
        time_from => to_minutes($day, $interval->[1]),
        time_to => to_minutes($day, $interval->[2]) });
    }
  }

  $ns->output("CC/$name", $campaign->{cc_id});
}

sub init
{
  my ($self, $ns) = @_;
  
  my $site = $ns->create(Site => { name => 1 });
  
  my $tag = $ns->create(PricedTag => {
    name => 1,
    site_id => $site });

  $ns->output("Tag", $tag);

  my @moscow = (
    [MONDAY, '00:00' , '01:59'],
    [TUESDAY, '04:00', '05:59'],
    [WEDNESDAY, '08:00', '09:59'],
    [THURSDAY, '12:00', '13:59'],
    [FRIDAY, '16:00', '17:59'],
    [SATURDAY, '20:00', '21:59'],
    [SUNDAY, '23:00',  '23:59' ]
    );
  
  $self->DailyRunTest_create_testcase(
    $ns, $site, "Europe/Moscow", [], \@moscow);
  
  my @tokyo = (
    [MONDAY, '05:30', '05:59'],
    [TUESDAY, '08:00', '09:59'],
    [SATURDAY, '11:00', '11:29']
    );
  
  $self->DailyRunTest_create_testcase(
    $ns, $site, "Asia/Tokyo", \@tokyo, []);
  
  $self->DailyRunTest_create_testcase(
    $ns, $site, "America/Los_Angeles", [], [], "America/Los_Angeles-any");
  
  my @los_angeles_ccg = (
    [undef, '07:00', '14:29'],
    [undef, '15:00', '16:59']
    );

  my @los_angeles_campaign = (
    [undef, '16:30', '17:29'],
    [undef, '21:00', '22:59']
    );
  
  $self->DailyRunTest_create_testcase(
    $ns, $site, "America/Los_Angeles", 
    \@los_angeles_ccg, \@los_angeles_campaign);
}

1;
