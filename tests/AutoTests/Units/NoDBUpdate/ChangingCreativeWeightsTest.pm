package ChangingCreativeWeightsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;
  # Remove this comment and insert your code here.

  my $keyword = make_autotest_name($ns, "Keyword");
  $ns->output("KEYWORD", $keyword);
  my $publisher = $ns->create(PubAccount => { name => 'Pub'});

  # Campaign with 2 creatives
  my $ccw_campaign = $ns->create(DisplayCampaign => {
    name => 'CCW',
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => 1, 
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES |
      DB::Campaign::CREATIVE_OPTIMIZATION,
    site_links => [{name => 'CCW', account_id => $publisher->{account_id}}] });
    
  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $ccw_campaign->{channel_id} ));
    
  my $ccw_tag = $ns->create(PricedTag => {
    name => 'CCW',
    site_id => $ccw_campaign->{Site}[0]->{site_id} 
    });

  my $creative = $ns->create(Creative => {
    name => 'CCW-2',
    account_id => $ccw_campaign->{account_id} });

  my $cc = $ns->create(CampaignCreative => {
    ccg_id => $ccw_campaign->{ccg_id},
    creative_id => $creative });

  $ns->output("CCW_CCGID", $ccw_campaign->{ccg_id});  
  $ns->output("CCW_CCID1", $ccw_campaign->{cc_id});
  $ns->output("CCW_CCID2", $cc);
  $ns->output("CCW_TID", $ccw_tag);
}

1;
