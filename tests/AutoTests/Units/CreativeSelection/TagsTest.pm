package TagsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  # Publishers

  my $publisher1 = $ns->create(Publisher => {
    name => 'Publisher-1',
    pricedtag_cpm => 5});

  my $publisher2 = $ns->create(Publisher => {
    name => 'Publisher-2',
    pricedtag_size_id => DB::Defaults::instance()->size_300x250(),
    pricedtag_cpm => 5});

  my $tag3 = $ns->create(PricedTag => {
    name => 3,
    site_id => $publisher1->{site_id},
    size_id => DB::Defaults::instance()->size_300x250(),
    cpm => 5 });

  my $tag_pricing1 = $ns->create(TagPricing => {
    tag_id => $publisher1->{tag_id},
    country_code => "RU",
    cpm => 10 });

  my $tag_pricing2 = $ns->create(TagPricing => {
    tag_id =>  $publisher1->{tag_id},
    country_code => "US",
    cpm => 20 });

  $ns->create(TagPricing => {
    tag_id => $publisher2->{tag_id},
    country_code => "US",
    cpm => 10 });

  $ns->create(TagPricing => {
    tag_id => $publisher2->{tag_id},
    country_code => "RU",
    cpm => 50 });

  # Campaigns

  my $advertiser = $ns->create(Advertiser => { name => 1 });

  my $creative1 = $ns->create(Creative => { 
     name => "Creative1",
     account_id => $advertiser });

  my $creative2 = $ns->create(Creative => {
    name => 'Creative2',
    account_id => $advertiser,
    size_id => DB::Defaults::instance()->size_300x250() });

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      keyword_list => "twelve_of_october",
      account_id => $advertiser,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 1,
    account_id => $advertiser,
    channel_id => $bp->channel_id,
    campaigncreativegroup_cpm => 40,
    cc_id => undef,
    creative_id => undef,
    campaigncreativegroup_country_code => "US",
    site_links => 
      [ { site_id => $publisher1->{site_id}},
        { site_id => $publisher2->{site_id}} ]});

  my $cc1_1 = $ns->create(CampaignCreative => {
    ccg_id => $campaign1->{ccg_id},
    creative_id => $creative1 });

  my $cc1_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign1->{ccg_id},
    creative_id => $creative2 });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 2,
    channel_id => $bp->channel_id,
    campaign_id => $campaign1->{campaign_id},
    cc_id => undef,
    creative_id => undef,
    campaigncreativegroup_cpm => 40,
    campaigncreativegroup_country_code => "RU",
    site_links => 
      [ { site_id => $publisher1->{site_id}},
        { site_id => $publisher2->{site_id}} ]});

  my $cc2_1 = $ns->create(CampaignCreative => {
    ccg_id => $campaign2->{ccg_id},
    creative_id => $creative1 });

  my $cc2_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign2->{ccg_id},
    creative_id => $creative2 });

  my $campaign3 = $ns->create(DisplayCampaign => {
    name => 3,
    channel_id => $bp->channel_id,
    campaign_id => $campaign1->{campaign_id},
    campaigncreativegroup_cpm => 40,
    campaigncreativegroup_country_code => "KR",
    cc_id => undef,
    creative_id => undef,
    site_links => 
      [ { site_id => $publisher1->{site_id}},
        { site_id => $publisher2->{site_id}} ]});

  my $cc3_1 = $ns->create(CampaignCreative => {
    ccg_id => $campaign3->{ccg_id},
    creative_id => $creative1 });

  my $cc3_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign3->{ccg_id},
    creative_id => $creative2 });


  $ns->output("CCID/1", $cc1_1);
  $ns->output("CCID/2", $cc1_2);
  $ns->output("CCID/3", $cc2_1);
  $ns->output("CCID/4", $cc2_2);
  $ns->output("CCID/5", $cc3_1);
  $ns->output("CCID/6", $cc3_2);

  $ns->output("TagsTest/01", "twelve_of_october");
  $ns->output("Tag Id/1", $publisher1->{tag_id});
  $ns->output("Tag Id/2", $publisher2->{tag_id});
  $ns->output("Tag Id/3", $tag3);
  $ns->output("SiteRate Id/1", $tag_pricing1->site_rate_id);
  $ns->output("SiteRate Id/2", $tag_pricing2->site_rate_id);
  # Default TagPricing for tag_id1
  $ns->output(
    "SiteRate Id/1/Default", 
     $publisher1->{Tags}->{tag_pricing_id}->{site_rate_id});
  $ns->output(
    "SiteRate Id/5", 
    $tag3->{tag_pricing_id}->{site_rate_id});

}

1;
