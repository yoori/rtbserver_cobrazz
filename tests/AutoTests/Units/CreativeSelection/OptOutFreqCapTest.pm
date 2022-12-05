package OptOutFreqCapTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  # 1. opted-out user send advertizing request for campaign with
  #    disabled frequency caps (campaign have no link with the FreqCaps
  #    table (Freq_cap_id=null in tables Campaign, CampaignCreative and
  #    CampaignCreativeGroup)) & action tracking (CPA > 0).
  # 2. test that server return creative.
  # 3. opted-out user send advertizing request for campaign with
  #    disabled frequency caps (campaign have reference to frequency
  #    caps with period=0) & enabled action tracking.
  # 4. test that server return creative and debug-info field
  #    action_pixel_url is empty.

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_url_list => "http://www.OptOutFreqCapTest.com",
    campaign_freq_cap_id => undef,
    campaigncreativegroup_freq_cap_id => undef,
    campaigncreativegroup_cpa => undef,
    campaigncreative_freq_cap_id => undef,
    site_links => [{name => 1}] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign1->{channel_id},
      trigger_type => "U" ));

  $ns->output("CC-1", $campaign1->{cc_id}, "cc_id");

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign1->{Site}[0]->{site_id} });

  $ns->output("Tag-1", $tag_id, "tid");

  # Campaign#2
  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 2,
    account_id => $campaign1->{account_id},
    channel_id => $campaign1->{channel_id},
    creative_id => $campaign1->{creative_id},
    campaign_flags => 0,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 2}]
    });

  $ns->output("CC-2", $campaign2->{cc_id}, "cc_id");

  $tag_id = $ns->create(PricedTag => {
    name => 2,
    site_id => $campaign2->{Site}[0]->{site_id} });

  $ns->output("Tag-2", $tag_id, "tid");


  # Campaign#3
  my $campaign3 = $ns->create(DisplayCampaign => {
    name => 3,
    account_id => $campaign1->{account_id},
    channel_id => $campaign1->{channel_id},
    creative_id => $campaign1->{creative_id},
    campaign_flags => 0,
    campaign_freq_cap_id => 
      DB::FreqCap->blank(period => 1),
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 3}] });

  $tag_id = $ns->create(PricedTag => {
    name => 3,
    site_id => $campaign3->{Site}[0]->{site_id} });

  $ns->output("Tag-3", $tag_id, "tid");
  $ns->output("Colo", DB::Defaults::instance()->ads_isp->{colo_id});
}

1;
