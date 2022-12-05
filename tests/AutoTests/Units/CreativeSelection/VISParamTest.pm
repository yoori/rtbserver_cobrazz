#v2

package VISParamTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub case01_vis_param_tests {
  my ($self, $ns, $name, $country, $country_code) = @_;

  my $pub_acc = $ns->create(PubAccount => {
    name => "acc-pub-" . $name,
    passback_below_fold => "Y",
    country_code => $country->country_code});
  my $publisher = $ns->create(Publisher => {
    name => "pub-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc });
  my $site = $publisher->{site_id};

  my $adv_acc = $ns->create(Account => {
    name => "adv-" . $name,
    country_code => $country->country_code,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = make_autotest_name($ns, "Keyword" . $name);

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel-' . $name,
    account_id => $adv_acc,
    keyword_list => $keyword,
    country_code => $country->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $dc = $ns->create(DisplayCampaign => {
    name => "campaign-" . $name,
    account_id => $adv_acc,
    campaigncreativegroup_cpm => 30,
    campaigncreativegroup_country_code => $country->country_code,
    channel_id => $channel->channel_id,
    site_links => [{ site_id => $site }]});

  $ns->output("CC01", $dc->{cc_id}, "ccid");
  $ns->output("TAG01", $publisher->{tag_id});
  $ns->output("KEYWORD01", $keyword);
  $ns->output("COUNTRY01", $country_code, "ccid");
}

sub case02_ad_selection_based_on_visibility_filter {
  my ($self, $ns, $name, $country1, $country_code1, 
      $country2, $country_code2, 
      $country4, $country_code4) = @_;

  my $pub_acc1 = $ns->create(PubAccount => {
    name => "acc-pub-1-" . $name,
    passback_below_fold => "Y",
    country_code => $country1->country_code});
  my $publisher1 = $ns->create(Publisher => {
    name => "pub-1-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc1 });
  my $site1 = $publisher1->{site_id};

  my $pub_acc2 = $ns->create(PubAccount => {
    name => "acc-pub-2-" . $name,
    passback_below_fold => "Y",
    country_code => $country2->country_code});
  my $publisher2 = $ns->create(Publisher => {
    name => "pub-2-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc2 });
  my $site2 = $publisher2->{site_id};

  my $pub_acc3 = $ns->create(PubAccount => {
    name => "acc-pub-3-" . $name,
    passback_below_fold => "N",
    country_code => $country2->country_code});
  my $publisher3 = $ns->create(Publisher => {
    name => "pub-3-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc3 });
  my $site3 = $publisher3->{site_id};

  my $pub_acc4 = $ns->create(PubAccount => {
    name => "acc-pub-4-" . $name,
    passback_below_fold => "Y",
    country_code => $country4->country_code});
  my $publisher4 = $ns->create(Publisher => {
    name => "pub-4-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc4 });
  my $site4 = $publisher4->{site_id};

  my $adv_acc1 = $ns->create(Account => {
    name => "adv-1-" . $name,
    country_code => $country1->country_code,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $adv_acc2 = $ns->create(Account => {
    name => "adv-2-" . $name,
    country_code => $country2->country_code,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $adv_acc4 = $ns->create(Account => {
    name => "adv-4-" . $name,
    country_code => $country4->country_code,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword1 = make_autotest_name($ns, "Keyword1" . $name);
  my $keyword2 = make_autotest_name($ns, "Keyword2" . $name);
  my $keyword4 = make_autotest_name($ns, "Keyword4" . $name);

  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel-1-' . $name,
    account_id => $adv_acc1,
    keyword_list => $keyword1,
    country_code => $country1->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel-2-' . $name,
    account_id => $adv_acc2,
    keyword_list => $keyword2,
    country_code => $country2->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channel4 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel-4-' . $name,
    account_id => $adv_acc4,
    keyword_list => $keyword4,
    country_code => $country4->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $dc1 = $ns->create(DisplayCampaign => {
    name => "campaign-1-" . $name,
    account_id => $adv_acc1,
    campaigncreativegroup_cpm => 30,
    campaigncreativegroup_country_code => $country1->country_code,
    channel_id => $channel1->channel_id,
    site_links => [{ site_id => $site1 }]});

  my $dc2 = $ns->create(DisplayCampaign => {
    name => "campaign-2-" . $name,
    account_id => $adv_acc2,
    campaigncreativegroup_cpm => 30,
    campaigncreativegroup_country_code => $country2->country_code,
    channel_id => $channel2->channel_id,
    site_links => [{ site_id => $site2 },
                   { site_id => $site3 }]});

  my $dc4 = $ns->create(DisplayCampaign => {
    name => "campaign-4-" . $name,
    account_id => $adv_acc4,
    campaigncreativegroup_cpm => 30,
    campaigncreativegroup_country_code => $country4->country_code,
    channel_id => $channel4->channel_id,
    site_links => [{ site_id => $site4 }]});

  $ns->output("CC02_1", $dc1->{cc_id}, "ccid");
  $ns->output("CC02_2", $dc2->{cc_id}, "ccid");
  $ns->output("CC02_3", $dc4->{cc_id}, "ccid");
  $ns->output("TAG02_1", $publisher1->{tag_id});
  $ns->output("TAG02_2", $publisher2->{tag_id});
  $ns->output("TAG02_3", $publisher3->{tag_id});
  $ns->output("TAG02_4", $publisher4->{tag_id});
  $ns->output("KEYWORD02_1", $keyword1);
  $ns->output("KEYWORD02_2", $keyword2);
  $ns->output("KEYWORD02_3", $keyword4);
  $ns->output("COUNTRY02_1", $country_code1, "ccid");
  $ns->output("COUNTRY02_2", $country_code2, "ccid");
  $ns->output("COUNTRY02_3", $country_code4, "ccid");
}

sub case03_publisher_inventory_mode {
  my ($self, $ns, $name, $country, $country_code) = @_;

  my $passback_url = "http://visparamtest_" . $name . ".com/";

  my $pub_acc1 = $ns->create(PubAccount => {
    name => "acc-pub-1-" . $name,
    passback_below_fold => "Y",
    country_code => $country->country_code});
  my $publisher1 = $ns->create(Publisher => {
    name => "pub-1-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc1 });
  my $site1 = $publisher1->{site_id};
  my $inv_tag1 = $ns->create(PricedTag => {
    name => "inv-1-" . $name,
    site_id => $site1,
    flags => DB::Tags::INVENORY_ESTIMATION,
    cpm => 0.1,
    passback => $passback_url });

  my $pub_acc2 = $ns->create(PubAccount => {
    name => "acc-pub-2-" . $name,
    passback_below_fold => "N",
    country_code => $country->country_code});
  my $publisher2 = $ns->create(Publisher => {
    name => "pub-2-" . $name,
    pricedtag_cpm => 1,
    account_id => $pub_acc2 });
  my $site2 = $publisher2->{site_id};
  my $inv_tag2 = $ns->create(PricedTag => {
    name => "inv-2-" . $name,
    site_id => $site2,
    flags => DB::Tags::INVENORY_ESTIMATION,
    cpm => 0.1,
    passback => $passback_url });

  my $adv_acc = $ns->create(Account => {
    name => "adv-" . $name,
    country_code => $country->country_code,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = make_autotest_name($ns, "Keyword" . $name);

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'channel-' . $name,
    account_id => $adv_acc,
    keyword_list => $keyword,
    country_code => $country->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $dc = $ns->create(DisplayCampaign => {
    name => "campaign-" . $name,
    account_id => $adv_acc,
    campaigncreativegroup_cpm => 30,
    campaigncreativegroup_country_code => $country->country_code,
    channel_id => $channel->channel_id,
    site_links => [{ site_id => $site1 },
                   { site_id => $site2 }]});

  $ns->output("CC03", $dc->{cc_id}, "ccid");
  $ns->output("TAG03_1", $inv_tag1);
  $ns->output("TAG03_2", $inv_tag2);
  $ns->output("KEYWORD03", $keyword);
  $ns->output("COUNTRY03", $country_code, "ccid");
}

sub init {
  my ($self, $ns) = @_;

  my $country_code1 = "vi";
  my $country_code2 = "vu";
  my $country_code3 = "ve";

  my $country1 = $ns->create(Country => {
      country_code => "VI",  # US Virgin Islands
      min_tag_visibility => 0,
      low_channel_threshold => 0,
      high_channel_threshold => 0});

  my $country2 = $ns->create(Country => {
    country_code => "VU",  # Vanuatu
    min_tag_visibility => 50,
    low_channel_threshold => 0,
    high_channel_threshold => 0});

  my $country3 = $ns->create(Country => {
    country_code => "VE",  # Venezuela
    min_tag_visibility => 100,
    low_channel_threshold => 0,
    high_channel_threshold => 0});

  case01_vis_param_tests($self, $ns, "visparam",
     $country2, $country_code2);
  case02_ad_selection_based_on_visibility_filter($self, $ns, "adselection",
     $country1, $country_code1, $country2, $country_code2, $country3, $country_code3);
  case03_publisher_inventory_mode($self, $ns, "inventorymode",
     $country2, $country_code2);
}

1;
