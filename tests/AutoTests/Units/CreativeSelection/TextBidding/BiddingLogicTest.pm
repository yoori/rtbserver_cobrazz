package BiddingLogicTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => "Advertizer",
    role_id => DB::Defaults::instance()->advertiser_role });

  my $tag_cpm1 = 2.0;
  my $tag_cpm2 = $tag_cpm1 * 0.5;

  $ns->output("min_cpm1", $tag_cpm1);
  $ns->output("min_cpm2", $tag_cpm2);

  my $keyword1 = make_autotest_name($ns, "keyword_01");
  my $keyword2 = make_autotest_name($ns, "keyword_02");
  my $keyword3 = make_autotest_name($ns, "keyword_03");
  my $keyword4 = make_autotest_name($ns, "keyword_04");
  my $keyword5 = make_autotest_name($ns, "keyword_05");
  my $keyword6 = make_autotest_name($ns, "keyword_06");
  my $keyword7 = make_autotest_name($ns, "keyword_07");
  my $keyword8 = make_autotest_name($ns, "keyword_08");

  $ns->output("Key01", $keyword1);
  $ns->output("Key02", $keyword2);
  $ns->output("Key03", $keyword3);
  $ns->output("Key04", $keyword4);
  $ns->output("Key05", $keyword5);
  $ns->output("Key06", $keyword6);
  $ns->output("Key07", $keyword7);
  $ns->output("Key08", $keyword8);

  my $size_id = $ns->create(CreativeSize => {
    name => 1 ,
    max_text_creatives => 2});

  my $size_id3  = $ns->create(CreativeSize => {
    name => 3 ,
    max_text_creatives => 3});

  my $publisher1 = $ns->create(Publisher => {
    name => 'Pub1',
    pricedtag_size_id => $size_id,
    pricedtag_cpm => $tag_cpm1 });

  my $publisher2 = $ns->create(Publisher => {
    name => 'Pub2',
    pricedtag_size_id => $size_id,
    pricedtag_cpm => $tag_cpm2 });

  my $publisherNoMargin = $ns->create(Publisher => {
    name => "NoMargin",
    pubaccount_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account,
    pricedtag_cpm => 0 });

  $ns->output("NoMarginColo", 
     DB::Defaults::instance()->no_margin_isp->{colo_id});

  my $tag3 = $ns->create(PricedTag => {
      name => 3,
      site_id => $publisher1->{site_id},
      size_id => $size_id3,
      cpm => $tag_cpm1 });

  my $campaign1 = $ns->create(TextAdvertisingCampaign => {
    name => 1,
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword1,
    max_cpc_bid => (0.02*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}} ] });

  my $ccg_keyword1_2 = $ns->create(CCGKeyword => {
    name => "1_2",
    ccg_id => $campaign1->{ccg_id},
    original_keyword => $keyword2,
    max_cpc_bid => (0.2*$tag_cpm1),
    click_url => 'http://test.com' });

  my $ccg_keyword1_3 = $ns->create(CCGKeyword => {
    name => "1_3",
    ccg_id => $campaign1->{ccg_id},
    original_keyword => $keyword3,
    max_cpc_bid => (0.15*$tag_cpm1),
    click_url => 'http://test.com' });


  $ns->output("CC01", $campaign1->{cc_id});

  my $campaign2 = $ns->create(TextAdvertisingCampaign => {
    name => 2,
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    ccgkeyword_channel_id => $campaign1->{CCGKeyword}->channel_id,
    original_keyword => $keyword1,
    max_cpc_bid => (0.03*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC02", $campaign2->{cc_id});

  my $campaign3 = $ns->create(TextAdvertisingCampaign => {
    name => 3,
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    ccgkeyword_channel_id => $campaign1->{CCGKeyword}->channel_id,
    original_keyword => $keyword1,
    max_cpc_bid => (0.11*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}}, 
      {site_id => $publisher2->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC03", $campaign3->{cc_id});

  my $campaign4 = $ns->create(TextAdvertisingCampaign => {
    name => 4,
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    account_id => $campaign3->{account_id},
    ccgkeyword_channel_id => $ccg_keyword1_3->{channel_id},
    original_keyword => $keyword3,
    max_cpc_bid => (0.09*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}}, 
      {site_id => $publisher2->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  my $ccg_keyword4_2 = $ns->create(CCGKeyword => {
    name => "4_2",
    ccg_id => $campaign4->{ccg_id},
    original_keyword => $keyword4,
    max_cpc_bid => (0.03*$tag_cpm1),
    click_url => 'http://test.com' });

  my $ccg_keyword4_3 = $ns->create(CCGKeyword => {
    name => "4_3",
    ccg_id => $campaign4->{ccg_id},
    original_keyword => $keyword5,
    max_cpc_bid => (0.21*$tag_cpm1),
    click_url => 'http://test.com' });

  $ns->output("CC04", $campaign4->{cc_id});

  my $campaign5 = $ns->create(TextAdvertisingCampaign => {
    name => 5,
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    advertiser_name => 4,
    ccgkeyword_channel_id => $ccg_keyword1_3->channel_id,
    original_keyword => $keyword3,
    max_cpc_bid => (0.04*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->create(CCGKeyword => { 
    ccg_id => $campaign5->{ccg_id},
    channel_id => $ccg_keyword4_2->{channel_id},
    original_keyword => $keyword4,
    max_cpc_bid => (0.11*$tag_cpm1),
    click_url => 'http://test.com' });

  $ns->create(CCGKeyword => {
    ccg_id => $campaign5->{ccg_id},
    channel_id => $ccg_keyword4_3->{channel_id},
    original_keyword => $keyword5,
    max_cpc_bid => (0.09*$tag_cpm1),
    click_url => 'http://test.com' });

  $ns->output("CC05", $campaign5->{cc_id});

  my $acc6 = $ns->create(Account => {
    name => 6,
    text_adserving => 'A',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $flags = DB::Campaign::INCLUDE_SPECIFIC_SITES;

  # Display company
  my $display_campaign = $ns->create(DisplayCampaign => { 
    name => "DisplayCampaign",
    account_id => $campaign1->{account_id},
    channel_id => $ccg_keyword4_3->{channel_id},
    campaign_flags => $flags,
    campaigncreativegroup_cpm => 2.0*$tag_cpm1,
    campaigncreativegroup_flags => $flags,
    creative_size_id => $size_id,
    creative_template_id => DB::Defaults::instance()->text_template,
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}],
  });

  $ns->output("Channel01", $ccg_keyword4_3->{channel_id});
  $ns->output("CC06", $display_campaign->{cc_id});

  # Filtering by campaign (M) case
  my $campaign5_1 = $ns->create(TextAdvertisingCampaign => {
    name => "5-1",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    advertiser_text_adserving => 'M',
    original_keyword => $keyword6,
    max_cpc_bid => (0.90*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  my $creative_id5_2 = $ns->create(Creative => {
    name => "5-2",
    account_id => $campaign5_1->{account_id},
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template });

  my $cc_id5_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign5_1->{ccg_id},
    creative_id => $creative_id5_2 });

  my $campaign5_3 = $ns->create(TextAdvertisingCampaign => {
    name => "5-3",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    account_id => $campaign5_1->{account_id},
    ccgkeyword_channel_id => $campaign5_1->{CCGKeyword}->channel_id,
    original_keyword => $keyword6,
    max_cpc_bid => (0.80*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC05_1", $campaign5_1->{cc_id});
  $ns->output("CC05_2", $cc_id5_2);
  $ns->output("CC05_3", $campaign5_3->{cc_id});

  # Filtering by advertiser (A) case
  my $campaign6_1 = $ns->create(TextAdvertisingCampaign => {
    name => "6-1",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    advertiser_account_type_id => undef,
    advertiser_text_adserving => undef,
    advertiser_agency_account_id => $acc6,
    original_keyword => $keyword7,
    max_cpc_bid => (0.80 * $tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC06_1", $campaign6_1->{cc_id});

  my $campaign6_2 = $ns->create(TextAdvertisingCampaign => { 
    name => "6-2",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    advertiser_account_type_id => undef,
    advertiser_agency_account_id => $acc6,
    advertiser_text_adserving => undef,
    ccgkeyword_channel_id => $campaign6_1->{CCGKeyword}->channel_id,
    original_keyword => $keyword7,
    max_cpc_bid => (0.85*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });
  
  $ns->output("CC06_2", $campaign6_2->{cc_id});

  my $campaign6_3 = $ns->create(TextAdvertisingCampaign => { 
    name => "6-3",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    account_id => $campaign6_2->{account_id},
    ccgkeyword_channel_id => $campaign6_1->{CCGKeyword}->channel_id,
    original_keyword => $keyword7,
    max_cpc_bid => (0.82*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC06_3", $campaign6_3->{cc_id});

  # Filtering by account (O) case
  my $campaign7_1 = $ns->create(TextAdvertisingCampaign => { 
    name => "7-1",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    advertiser_text_adserving => 'O',
    original_keyword => $keyword8,
    max_cpc_bid => (0.90*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC07_1", $campaign7_1->{cc_id});

  my $campaign7_2 = $ns->create(TextAdvertisingCampaign => { 
    name => "7-2",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    advertiser_text_adserving => 'O',
    ccgkeyword_channel_id => $campaign7_1->{CCGKeyword}->channel_id,
    original_keyword => $keyword8,
    max_cpc_bid => (0.85*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC07_2", $campaign7_2->{cc_id});

  my $campaign7_3 = $ns->create(TextAdvertisingCampaign => { 
    name => "7-3",
    size_id => $size_id3,
    template_id => DB::Defaults::instance()->text_template,
    account_id => $campaign7_2->{account_id},
    ccgkeyword_channel_id => $campaign7_1->{CCGKeyword}->channel_id,
    original_keyword => $keyword8,
    max_cpc_bid => (0.80*$tag_cpm1),
    site_links => [
      {site_id => $publisher1->{site_id}},
      {site_id => $publisherNoMargin->{site_id}}] });

  $ns->output("CC07_3", $campaign7_3->{cc_id});

  $ns->output("Tag01", $publisher1->{tag_id});
  $ns->output("Tag02", $publisher2->{tag_id});
  $ns->output("Tag03", $tag3);
  $ns->output("TagNoMargin", $publisherNoMargin->{tag_id});

}

1;

