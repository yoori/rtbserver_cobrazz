
package CampaignBudgetTest;

use strict;
use warnings;
use POSIX qw(strftime ceil);
use Time::HiRes qw(gettimeofday);

use DB::Defaults;
use DB::Util;

use constant DAY => 24 * 60 * 60;

sub init
{
  my ($self, $ns) = @_;

  # 3 CCG for campaign with fixed daily budget limit
  my $publisher = $ns->create(Publisher => { name => 'Pub'});
  $ns->output("Tag", $publisher->{tag_id});

  my $advertiser = $ns->create(Account => {
    name => 'Adv',
    role_id => DB::Defaults::instance()->advertiser_role() });

  my $daily_budget = 2;
  my $keyword_cpm = make_autotest_name($ns, "FixedCPM");
  my $cpm_daily_campaign = $ns->create(DisplayCampaign => {
    campaign_name => 'FixedDailyBudget',
    name => 'FixedCPM',
    account_id => $advertiser,
    campaign_daily_budget => $daily_budget,
    campaign_delivery_pacing => 'F',
    campaigncreativegroup_cpm => 1000,
    site_links => [{ site_id =>  $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      account_id => $advertiser,
      name => "FixedCPM",
      keyword_list => $keyword_cpm,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ])
    });

  $ns->output("FixedCPM/CCG", $cpm_daily_campaign->{ccg_id});
  $ns->output("FixedCPM/CC", $cpm_daily_campaign->{cc_id});
  $ns->output("FixedCPM/Campaign", $cpm_daily_campaign->{campaign_id});
  $ns->output("FixedCPM/Keyword", $keyword_cpm);
  $ns->output("FixedDailyBudget/DailyBudget", $daily_budget);


  my $keyword_cpc = make_autotest_name($ns, "FixedCPC");

  my $cpc_daily_campaign = $ns->create(DisplayCampaign => {
    name => 'FixedCPC',
    campaign_id => $cpm_daily_campaign->{campaign_id},
    campaigncreativegroup_cpc => 1.0, # price for 1 click
    campaigncreativegroup_cpm => 0,
    site_links => [{ site_id =>  $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      account_id => $advertiser,
      name => "FixedCPC",
      keyword_list => $keyword_cpc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ])
    });

  $ns->output("FixedCPC/CCG", $cpc_daily_campaign->{ccg_id});
  $ns->output("FixedCPC/Campaign", $cpc_daily_campaign->{campaign_id});
  $ns->output("FixedCPC/CC", $cpc_daily_campaign->{cc_id});
  $ns->output("FixedCPC/Keyword", $keyword_cpc);

  my $never_matched = $ns->create(DisplayCampaign => {
    name => 'NeverMatched',
    campaign_id => $cpm_daily_campaign->{campaign_id},
    campaigncreativegroup_cpm => 1,
    #cc_id => undef,
    #creative_id => undef,
    site_links => [{ site_id =>  $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      account_id => $advertiser,
      name => "NeverMatched",
      keyword_list => make_autotest_name($ns, "NeverMatched"),
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ])
  });

  $ns->output("NeverMatched/CCG", $never_matched->{ccg_id});

  # 1 CCG for campaign with dynamic budget limit
  my $now = [ Time::HiRes::gettimeofday() ];
  $ns->output("TODAY", POSIX::strftime("%F 00:00:00", gmtime($now->[0])));
  my $date_end =
    POSIX::strftime("%F 00:00:00", gmtime($now->[0] + 9 * DAY));

  foreach my $case_name ("Dynamic1", "Dynamic2")
  {
    my $keyword_dynamic = make_autotest_name($ns, $case_name);
    my $cpm = 500;
    my $budget = 10;
    my $cpm_dynamic_campaign = $ns->create(DisplayCampaign => {
      name => $case_name,
      account_id => $advertiser,
      campaign_budget => $budget,
      campaign_delivery_pacing => 'D',
      # 10 days to end
      campaign_date_end =>
        DB::Entity::Oracle->sql("to_date('$date_end', 'YYYY-MM-DD HH24:MI:SS')"),
      campaigncreativegroup_cpm => $cpm, # price for 1000 impressions
      site_links => [{ site_id =>  $publisher->{site_id} }],
      channel_id => DB::BehavioralChannel->blank(
        account_id => $advertiser,
        name => $case_name,
        keyword_list => $keyword_dynamic,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]),
      });

    $ns->output("$case_name/Keyword", $keyword_dynamic);
    $ns->output("$case_name/CC", $cpm_dynamic_campaign->{cc_id});
    $ns->output("$case_name/CCG", $cpm_dynamic_campaign->{ccg_id});
    $ns->output("$case_name/Budget", $budget);
    $ns->output("$case_name/Campaign", $cpm_dynamic_campaign->{campaign_id});
    $ns->output("$case_name/DateEnd", $date_end);
    $ns->output("$case_name/ImpRevenue", $cpm / 1000);
  }
}

1;
