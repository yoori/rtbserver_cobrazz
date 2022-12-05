package ChannelTargetingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;
  my $flags = DB::Campaign::INCLUDE_SPECIFIC_SITES;

  my $foros_min_margin = (1.00 * get_config($ns->pq_dbh, 'OIX_MIN_MARGIN'))/100.0;
  my $foros_min_fixed_margin = 1.00 * get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN');

  my $min_cpm  = $foros_min_fixed_margin + 1.0;
  my $tag_cpm  = $min_cpm*(1.00 + $foros_min_margin);

  $ns->output("min_cpm", $min_cpm, "min cpm");

  my $size  = $ns->create(CreativeSize => {
    name => 1,
    max_text_creatives => 2});

  ######
  my $site1 = $ns->create(Site => { name => 1 });

  my $tag1 = $ns->create(PricedTag => {
    name => 1,
    site_id => $site1,
    size_id => $size,
    cpm => $tag_cpm });

  $ns->output("Tag1", $tag1, "tid");

  my $keyword1 = make_autotest_name($ns, "Keyword1");
  $ns->output("Key1", $keyword1, "Key");

  my $campaign1 = $ns->create(ChannelTargetedTACampaign => {
    name => 1,
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    behavioralchannel_keyword_list => $keyword1,
    campaigncreativegroup_cpm =>  (2.5*$min_cpm),
    site_links => [{site_id => $site1}] });

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank(
    channel_id => $campaign1->{channel_id},
    trigger_type => "P"));

  $ns->output("CC1", $campaign1->{cc_id});

  my $campaign2 = $ns->create(ChannelTargetedTACampaign => {
    name => 2,
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    behavioralchannel_keyword_list => $keyword1,
    campaigncreativegroup_cpm =>  (2*$min_cpm),
    site_links => [{site_id => $site1}] });

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank(
    channel_id => $campaign2->{channel_id},
    trigger_type => "P"));

  $ns->output("CC2", $campaign2->{cc_id});

  ######
  my $site2 = $ns->create(Site => { name => 2 });

  my $tag2 = $ns->create(PricedTag => {
    name => 2,
    site_id => $site2,
    size_id => $size,
    cpm => $tag_cpm });

  $ns->output("Tag2", $tag2, "tid");

  my $keyword2 = make_autotest_name($ns, "-keyword2");
  $ns->output("Key2", $keyword2, "Key");

  my $campaign3 = $ns->create(TextAdvertisingCampaign => {
    name => 3,
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword2,
    max_cpc_bid => 3.5*$min_cpm/10,
    site_links => [{site_id => $site2}] });

  $ns->output("CC3", $campaign3->{cc_id});

  my $campaign4 = $ns->create(ChannelTargetedTACampaign => {
    name => 4,
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    behavioralchannel_keyword_list => $keyword2,
    campaigncreativegroup_cpm =>  (3*$min_cpm),
    site_links => [{site_id => $site2}] });

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank(
    channel_id => $campaign4->{channel_id},
    trigger_type => "P"));

  $ns->output("CC4", $campaign4->{cc_id});
}

1;
