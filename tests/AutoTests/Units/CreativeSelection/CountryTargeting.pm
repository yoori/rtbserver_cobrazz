#v2

package CountryTargeting;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $pub_acc_lu = $ns->create(PubAccount => {
    name => "acc-pub-lu",
    country_code => DB::Defaults::instance()->test_country_1->country_code});
  my $publisher_lu = $ns->create(Publisher => {
    name => "pub-lu",
    pricedtag_cpm => 1,
    account_id => $pub_acc_lu });
  $ns->output("TAG-LU", $publisher_lu->{tag_id});
  my $site_lu = $publisher_lu->{site_id};

  my $pub_acc_ug = $ns->create(PubAccount => {
    name => "acc-pub-ug",
    country_code => DB::Defaults::instance()->test_country_2->country_code});
  my $publisher_ug = $ns->create(Publisher => {
    name => "pub-ug",
    pricedtag_cpm => 1,
    account_id => $pub_acc_ug });
  $ns->output("TAG-UG", $publisher_ug->{tag_id});
  my $site_ug = $publisher_ug->{site_id};
  my $tag_ug2 = $ns->create(PricedTag => {
    name => 'pub-ug2',
    site_id => $site_ug,
    flags => DB::Tags::INVENORY_ESTIMATION,
    cpm => 1 });
  $ns->output("TAG-UG2", $tag_ug2);

  my $adv_acc_lu = $ns->create(Account => {
    name => "adv-lu",
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $adv_acc_agency_ug = $ns->create(Account => {
    name => "agency-ug",
    country_code => DB::Defaults::instance()->test_country_2->country_code,
    text_adserving => 'M',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $adv_acc_ug1 = $ns->create(Account => {
    name => "ag-ug-1",
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    agency_account_id => $adv_acc_agency_ug, 
    text_adserving => undef,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $adv_acc_ug2 = $ns->create(Account => {
    name => "ag-ug-2",
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    agency_account_id => $adv_acc_agency_ug,
    text_adserving => undef,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword_dc_lu  = make_autotest_name($ns, "DCLU");
  my $keyword_dc_ug1 = make_autotest_name($ns, "DCUG1");
  my $keyword_dc_ug2 = make_autotest_name($ns, "DCUG2");
  my $keyword_tc_lu  = make_autotest_name($ns, "TCLU");
  my $keyword_tc_ug1 = make_autotest_name($ns, "TCUG1");
  my $keyword_tc_ug2 = make_autotest_name($ns, "TCUG2");

  $ns->output("KEYWORD-DC-LU", $keyword_dc_lu, "keyword for adv request");
  $ns->output("KEYWORD-DC-UG1", $keyword_dc_ug1, "keyword for adv request");
  $ns->output("KEYWORD-DC-UG2", $keyword_dc_ug2, "keyword for adv request");
  $ns->output("KEYWORD-TC-LU", $keyword_tc_lu, "keyword for adv request");
  $ns->output("KEYWORD-TC-UG1", $keyword_tc_ug1, "keyword for adv request");
  $ns->output("KEYWORD-TC-UG2", $keyword_tc_ug2, "keyword for adv request");

  my $channel_disp_lu = $ns->create(DB::BehavioralChannel->blank(
    name => 'DispChannel-LU',
    account_id => $adv_acc_lu,
    keyword_list => $keyword_dc_lu,
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channel_disp_ug1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'DispChannel-UG1',
    account_id => $adv_acc_ug1,
    keyword_list => $keyword_dc_ug1,
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channel_disp_ug2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'DispChannel-UG2',
    account_id => $adv_acc_lu,
    keyword_list => $keyword_dc_ug2,
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $dc_lu = $ns->create(DisplayCampaign => {
    name => "display-campaign-lu",
    account_id => $adv_acc_lu,
    campaigncreativegroup_cpm => 30,
    campaigncreativegroup_country_code => DB::Defaults::instance()->test_country_1->country_code,
    channel_id => $channel_disp_lu->channel_id,
    site_links => [{ site_id => $site_lu },
                   { site_id => $site_ug }
                   ]});
  $ns->output("DC-LU", $dc_lu->{cc_id}, "ccid");

  my $dc_ug1 = $ns->create(DisplayCampaign => {
    name => "display-campaign-ug1",
    account_id => $adv_acc_ug1,
    campaigncreativegroup_cpm => 20,
    campaigncreativegroup_country_code => DB::Defaults::instance()->test_country_1->country_code,
    channel_id => $channel_disp_ug1->channel_id,
    site_links => [{ site_id => $site_lu},
                   { site_id => $site_ug}
                   ]});
  $ns->output("DC-UG1", $dc_ug1->{cc_id}, "ccid");

  my $dc_ug2 = $ns->create(DisplayCampaign => {
    name => "display-campaign-ug2",
    account_id => $adv_acc_ug2,
    campaigncreativegroup_cpm => 10,
    campaigncreativegroup_country_code => DB::Defaults::instance()->test_country_1->country_code,
    channel_id => $channel_disp_ug2->channel_id,
    site_links => [{ site_id => $site_lu},
                   { site_id => $site_ug}
                   ]});
  $ns->output("DC-UG2", $dc_ug2->{cc_id}, "ccid");

  my $channel_text_lu = $ns->create(DB::BehavioralChannel->blank(
    name => 'TextChannel-LU',
    account_id => $adv_acc_lu,
    keyword_list => $keyword_tc_lu,
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    channel_type => 'K',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channel_text_ug1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'TextChannel-UG1',
    account_id => $adv_acc_ug1,
    keyword_list => $keyword_tc_ug1,
    channel_type => 'K',
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channel_text_ug2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'TextChannel-UG2',
    account_id => $adv_acc_lu,
    keyword_list => $keyword_tc_ug2,
    channel_type => 'K',
    country_code => DB::Defaults::instance()->test_country_1->country_code,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $size = $ns->create(CreativeSize => {
    name => "Text",
    max_text_creatives => 2 });

  my $tc_lu =
      $ns->create(TextAdvertisingCampaign => {
        name => "TextCampaign-LU",
        size_id => $size,
        template_id =>  DB::Defaults::instance()->text_template,
        account_id =>  $adv_acc_lu,
        #country_code => 'LU',
        campaigncreativegroup_country_code => DB::Defaults::instance()->test_country_1->country_code,
        original_keyword => $keyword_tc_lu,
        ccgkeyword_channel_id => $channel_text_lu,
        ctr => 0.1,
        max_cpc_bid => 10,
        site_links => [{ site_id => $site_lu},
                       { site_id => $site_ug}] });
  $ns->output("TC-LU", $tc_lu->{cc_id}, "ccid");

  my $tc_ug1 =
      $ns->create(ChannelTargetedTACampaign => {
        name => "TextCampaign-UG1",
        size_id => $size,
        template_id =>  DB::Defaults::instance()->text_template,
        account_id =>  $adv_acc_ug1,
        #country_code => 'UG',
        campaigncreativegroup_country_code => DB::Defaults::instance()->test_country_1->country_code,
        channel_id => $channel_text_ug1,
        cpm => 10,
        site_links => [{ site_id => $site_lu},
                       { site_id => $site_ug}] });
  $ns->output("TC-UG1", $tc_ug1->{cc_id}, "ccid");

  my $tc_ug2 =
      $ns->create(TextAdvertisingCampaign => {
        name => "TextCampaign-UG2",
        size_id => $size,
        template_id =>  DB::Defaults::instance()->text_template,
        account_id =>  $adv_acc_ug2,
        #country_code => 'UG',
        campaigncreativegroup_country_code => DB::Defaults::instance()->test_country_1->country_code,
        original_keyword => $keyword_tc_ug2,
        ccgkeyword_channel_id => $channel_text_ug2,
        ctr => 0.1,
        max_cpc_bid => 10,
        site_links => [{ site_id => $site_lu},
                       { site_id => $site_ug}] });
  $ns->output("TC-UG2", $tc_ug2->{cc_id}, "ccid");
}

1;
