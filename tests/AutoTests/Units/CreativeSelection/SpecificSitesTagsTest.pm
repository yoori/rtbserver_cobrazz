package SpecificSitesTagsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword1 = make_autotest_name($ns, "Keyword1");

  my $site1 = $ns->create(Site => { name => 1});
  my $site2 = $ns->create(Site => { name => 2});

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_keyword_list => $keyword1,
    campaign_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{site_id => $site1}]});

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign1->{channel_id},
      trigger_type => "P" ));

  my $keyword2 = make_autotest_name($ns, "Keyword2");

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 2,
    behavioralchannel_keyword_list => $keyword2,
    campaign_flags => 0,
    campaigncreativegroup_flags => 0});

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign2->{channel_id},
      trigger_type => "P" ));


  $ns->output("KEYWORD1", $keyword1);
  $ns->output("KEYWORD2", $keyword2);
  
  $ns->output("CC Id/1", $campaign1->{cc_id}, "CC Id/1");
  $ns->output("CC Id/2", $campaign2->{cc_id}, "CC Id/2");
  
  my $tag_pricing = get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN')/10;
  
  my $tag_id1 = $ns->create(PricedTag =>
                            { name => 1,
                              site_id => $site1,
                              cpm => $tag_pricing });
  
  $ns->output("Tag Id/1", $tag_id1, "Tag Id/1");
  
  my $tag_id2 = $ns->create(PricedTag =>
                            { name => 2,
                              site_id => $site2,
                              cpm => $tag_pricing });
  
  $ns->output("Tag Id/2", $tag_id2, "Tag Id/2");
}

1;
