package ActionRequestsTests;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => "isp",
    role_id => DB::Defaults::instance()->isp_role });

  my $advertiser = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  # REVIEW : possible, exact cpm is excess for this test
  my $foros_min_margin = get_config($ns->pq_dbh, 'OIX_MIN_MARGIN') / 100.00;
  my $foros_min_fixed_margin = get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN');
  my $min_cpm = 1.0 + $foros_min_fixed_margin;

  my $keyword1 = make_autotest_name($ns, "kw1");
  $ns->output("Keyword1", $keyword1);
  my $keyword2 = make_autotest_name($ns, "kw2");
  $ns->output("Keyword2", $keyword2);

  my $action_id1 = $ns->create(Action => {
    name => 1,
    url => "http:\\www.yahoo.com",
    account_id => $advertiser});
  $ns->output("Action1", $action_id1);

  my $action_id2 = $ns->create(Action => {
    name => 2,
    url => "http:\\www.google.com",
    account_id => $advertiser});
  $ns->output("Action2", $action_id2);

  my $cpa = 100;

  my $campaign = $ns->create(DisplayCampaign => {
    name => '1',
    account_id => $advertiser,
    behavioralchannel_keyword_list => $keyword1,
    campaign_flags => 0,
    campaigncreativegroup_cpa =>  $cpa,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_action_id => $action_id1,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 1}] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  $ns->output("Campaign1", $campaign->{campaign_id});

  my $tag_price = $min_cpm / (1 + $foros_min_margin);

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => $tag_price });

  $ns->output("Tag1", $tag_id);

  $ns->output("CCG1", $campaign->{ccg_id});
  $ns->output("CC1", $campaign->{cc_id});
  $ns->output("COLO", DB::Defaults::instance()->ads_isp->{colo_id});
}

1;
