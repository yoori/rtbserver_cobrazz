package StatsHourlyLoggingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

# cpa must be great for concurent with tag showing cost (0.02) if system level AR is low (0.01 by default)
use constant CPA_VALUE => 1000;

sub init_case
{
  my ($ns, $name_prefix, $agency,
      $tag_cur, $colo_cur) = @_;


  my $acc_tag = $ns->create(PubAccount => {
    name => "pub$name_prefix",
    country_code => "GN",
    commission => 0.2,
    currency_id => $tag_cur });

  my $isp = $ns->create(Account => {
    name => "isp$name_prefix",
    role_id => DB::Defaults::instance()->publisher_role,
    country_code => "GN",
    currency_id => $colo_cur });

  my $campaign = $ns->create(DisplayCampaign => {
    name => $name_prefix,
    advertiser_agency_account_id => $agency,
    advertiser_commission => 0.1,
    campaign_commission => 0.3,
    behavioralchannel_keyword_list => $name_prefix,
    campaigncreativegroup_cpm => 11,
    site_links => [{name => $name_prefix, account_id => $acc_tag }] });

  my $bp = $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $campaign->{channel_id} ));

  $ns->output("CC Id/$name_prefix", $campaign->{cc_id}, "cc_id");

  my $tag_id = $ns->create(PricedTag => {
    name => $name_prefix,
    site_id => $campaign->{Site}[0]->{site_id},
    country_code => undef,
    cpm => 7 });

  $ns->output("Tag Id/$name_prefix", $tag_id, "tag_id");

  my $colo_id = $ns->create(Colocation => {
    name => $name_prefix,
    account_id => $isp,
    revenue_share => 0.3 });
  $ns->output("Colo/$name_prefix", $colo_id, "colo_id");
}

sub init
{
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type});

  my $keyword = make_autotest_name($ns, "keyword");

  my $common_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'CommonChannel',
    account_id => $advertiser,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 'NoTrack',
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    site_links => [{name => 1}],
    creative_crclick_value => "www.dread.com",
    channel_id => $common_channel
    });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'ActionsWithoutImps',
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 2}],
    channel_id => $common_channel
    });

  my $template_id = $ns->create(Template => {
    name => 3,
    template_file => 'UnitTests/banner_img_clk.html',
    flags => 0 });

  $ns->create(TemplateFile => {
    template_id => $template_id,
    template_file => 'UnitTests/img_clk.foros-ui',
    flags => 0, # REVIEW (see app format)
    app_format_id => DB::Defaults::instance()->app_format_track,
    template_type => 'T' });

  my $campaign3 = $ns->create(DisplayCampaign => {
    name => 3,
    account_id => $advertiser,
    campaigncreativegroup_cpm => 100,
    creative_template_id => $template_id,
    site_links => [{name => 3}],
    channel_id => $common_channel
    });

  my $campaign4 = $ns->create(DisplayCampaign => {
    name => 4,
    account_id => $advertiser,
    campaigncreativegroup_cpm => 100,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 4}],
    channel_id => $common_channel
    });

  my $campaign5 = $ns->create(DisplayCampaign => {
    name => 5,
    account_id => $advertiser,
    campaigncreativegroup_cpm => 100,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 5}],
    channel_id => $common_channel
    });


  my $campaign6 = $ns->create(DisplayCampaign => {
    name => 6,
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    site_links => [{name => 6}],
    creative_crclick_value => "www.dread.com",
    channel_id => $common_channel
    });

  $ns->output("StatsHourlyLoggingTest/Colo", DB::Defaults::instance()->ads_isp->{colo_id});

  $ns->output("CC Id/1", $campaign1->{cc_id});
  $ns->output("CC Id/2", $campaign2->{cc_id});
  $ns->output("CC Id/3", $campaign3->{cc_id});
  $ns->output("CC Id/4", $campaign4->{cc_id});
  $ns->output("CC Id/5", $campaign5->{cc_id});
  $ns->output("CC Id/6", $campaign6->{cc_id});

  $ns->output("Keyword", $keyword);

  my $tag_id1 = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign1->{Site}[0]->{site_id},
    country_code => "GB",
    cpm => 10 });
  $ns->output("Tag Id/1", $tag_id1);

  my $tag_id2 = $ns->create(PricedTag => {
    name => 2,
    site_id => $campaign2->{Site}[0]->{site_id},
    country_code => "GB",
    cpm => 20 });
  $ns->output("Tag Id/2", $tag_id2);

  my $tag_id3 = $ns->create(PricedTag => {
    name => 3,
    site_id => $campaign3->{Site}[0]->{site_id},
    country_code => "GB",
    cpm => 20 });
  $ns->output("Tag Id/3", $tag_id3);

  my $tag_id4 = $ns->create(PricedTag => {
    name => 4,
    site_id => $campaign4->{Site}[0]->{site_id},
    country_code => "GB",
    cpm => 20 });
  $ns->output("Tag Id/4", $tag_id4);

  my $tag_id5 = $ns->create(PricedTag => {
    name => 5,
    site_id => $campaign5->{Site}[0]->{site_id},
    country_code => "GB",
    cpm => 20 });
  $ns->output("Tag Id/5", $tag_id5);

  my $tag_id6 = $ns->create(PricedTag => {
    name => 6,
    site_id => $campaign6->{Site}[0]->{site_id},
    country_code => "GB",
    cpm => 20 });
  $ns->output("Tag Id/6", $tag_id6);

  my $isp = $ns->create(Isp => {
    name => "ISP",
    account_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account->{account_id} });

  $ns->output("Colo Id/2", $isp->{colo_id});

  # ADSC-532
  my $tag_cur = $ns->create(Currency => { rate => 2 });
  my $colo_cur = $ns->create(Currency => { rate => 3 });

  init_case($ns, "TC1", undef, $tag_cur, $colo_cur);

  # ADSC-2759
  init_case($ns, "TC2", DB::Defaults::instance()->agency_gross(), $tag_cur, $colo_cur);

  $ns->output("RequestsCount", 1000);
}

1;

