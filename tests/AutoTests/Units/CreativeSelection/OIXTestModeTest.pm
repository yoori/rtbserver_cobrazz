package OIXTestModeTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_campaign_creative_pack
{
  my ($ns, $name, $action, $keyword, $acc_flag, $cost) = @_;

  my $account = $ns->create(Account => {
    name => "a-$name",
    role_id => DB::Defaults::instance()->advertiser_role,
    flags => $acc_flag });

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => $name,
    account_id => $account,
    keyword_list => "$keyword-d",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 86400)]));

  my $d_campaign = $ns->create(DisplayCampaign => {
    name => "$name-d",
    account_id => $account,
    channel_id => $channel->channel_id(),
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpc => 0,
    campaigncreativegroup_cpa => $cost, # CPA
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_action_id => $action,
    campaigncreativegroup_flags => 0, # disabled site specific
    campaign_flags => 0});

  my $t_campaign = $ns->create(TextAdvertisingCampaign => {
    name => "$name-t",
    # size have two linked template files (with imp track & without):
    template_id => DB::Defaults::instance()->text_template,
    size_id => DB::Defaults::instance()->size(),
    account_id => $account,
    original_keyword => "$keyword-t",
    campaign_flags => 0,
    campaigncreativegroup_flags => 0, # disabled site specific
    max_cpc_bid => $cost });

  return ($d_campaign->{cc_id}, $t_campaign->{cc_id},
          $channel->channel_id(), $t_campaign->{CCGKeyword}->{channel_id});
}

sub create_overlap_channels
{
  my ($self, $ns) = @_;

  my $keyword1 = make_autotest_name($ns, "overlaps1");
  my $keyword2 = make_autotest_name($ns, "overlaps2");

  my $internal_account = $ns->create(Account => {
    name => 'Internal-Overlap-Account1',
      internal_account_id => undef, 
      role_id => DB::Defaults::instance()->internal_role,
      account_type_id => DB::Defaults::instance()->internal_type });

  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Overlap-Channel1',
    account_id => $internal_account,
    flags => 
      DB::CMPChannelBase::AUTO_QA | DB::CMPChannelBase::OVERLAP,
    keyword_list => $keyword1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60)]));

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Overlap-Channel2',
    account_id => DB::Defaults::instance()->internal_account,                                                         
    flags => 
      DB::CMPChannelBase::AUTO_QA | DB::CMPChannelBase::OVERLAP,
    keyword_list => $keyword1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60)]));

  my $channel3 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Overlap-Channel3',
    account_id => DB::Defaults::instance()->internal_account,
    flags => 
      DB::CMPChannelBase::AUTO_QA | DB::CMPChannelBase::OVERLAP,
    keyword_list => $keyword2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60)]));

  my $channel4 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Overlap-Channel4',
    account_id => DB::Defaults::instance()->internal_account,
    flags => 
      DB::CMPChannelBase::AUTO_QA | DB::CMPChannelBase::OVERLAP,
    keyword_list => $keyword2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60)]));

  $ns->output("OVERLAP-KWD1", $keyword1);
  $ns->output("OVERLAP-KWD2", $keyword2);
  $ns->output("OVERLAP-CHANNEL1", $channel1);
  $ns->output("OVERLAP-CHANNEL2", $channel2);
  $ns->output("OVERLAP-CHANNEL3", $channel3);
  $ns->output("OVERLAP-CHANNEL4", $channel4);
}

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => "adv",
    role_id => DB::Defaults::instance()->advertiser_role });

  my $action = $ns->create(Action => {
    name => "custom",
    url => "http://www.ebay.com",
    account_id => $acc});

  $ns->output("CustomAction", $action);

  # Test publisher
  my $passback_url = "http://forostestmode.com/";
  $ns->output("ORIGINAL_URL", $passback_url, "original passback url");

  my $test_publisher = $ns->create(PubAccount => {
    name => "p-test",
    flags => DB::Account::TEST });
  my $test_site = $ns->create(Site => {
    name => "test",
    account_id => $test_publisher });
  my $test_tag = $ns->create(PricedTag => {
    name => "test",
    site_id => $test_site,
    cpm => 0.1,
    passback => $passback_url });

  # Non test publisher
  my $non_test_publisher = $ns->create(PubAccount => {
    name => "p-non-test",
    flags => 0 });
  my $non_test_site = $ns->create(Site => {
    name => "non-test",
    account_id => $non_test_publisher });
  my $non_test_tag = $ns->create(PricedTag => {
    name => "non-test",
    site_id => $non_test_site,
    cpm => 0.1,
    passback => $passback_url });
  my $expensive_tag = $ns->create(PricedTag => {
    name => "exp-tag",
    site_id => $non_test_site,
    cpm => 10000,
    passback => $passback_url });

  my $key1 = make_autotest_name($ns, "non-test");
  my $key2 = make_autotest_name($ns, "test");

  my ($d_cc1, $t_cc1, $channel_1, $k_channel_1) = create_campaign_creative_pack(
    $ns, "non-test", $action, $key1, 0, 1);

  my ($d_cc2, $t_cc2, $channel_2, $k_channel_2) = create_campaign_creative_pack(
    $ns, "test", $action, $key2, DB::Account::TEST, 100);

  my $ctx_keyword = make_autotest_name($ns, "ctx");
  my $ctx_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'ctx',
    account_id => $acc,
    keyword_list => $ctx_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P")]));

  $ns->output("CTX/CHANNEL", $ctx_channel->channel_id(), "context channel");
  $ns->output("CTX/KEYWORD", $ctx_keyword, "keyword for matching context channel");

  my $halo_trigger = make_autotest_name($ns, "halo-trigger");
  $ns->output("HALO-TRIGGER", $halo_trigger, "trigger that doesn't match any channel");

  $ns->output("DISP/CC-NON-TEST",  $d_cc1, "display non test creative");
  $ns->output("DISP/CC-TEST",  $d_cc2, "display test creative");

  $ns->output("TEXT/CC-NON-TEST",  $t_cc1, "text non test creative");
  $ns->output("TEXT/CC-TEST",  $t_cc2, "text test creative");

  $ns->output("TAG-NON-TEST", $non_test_tag, "non test tag");
  $ns->output("TAG-TEST", $test_tag, "test tag");
  $ns->output("TAG-EXP", $expensive_tag, "expensive non test tag");

  $ns->output("KW-NON-TEST", $key1, "keyword base for non test campaign");
  $ns->output("KW-TEST", $key2, "keyword base for test campaign");

  $ns->output("CHANNEL-NON-TEST", $channel_1, "channel for non test cc pack");
  $ns->output("CHANNEL-TEST", $channel_2, "channel for test cc pack");

  $ns->output("K-CHANNEL-NON-TEST", $k_channel_1, "keyword channel for non test cc pack");
  $ns->output("K-CHANNEL-TEST", $k_channel_2, "keyword channel for test cc pack");

  $ns->output("SITE-NON-TEST", $non_test_site, "non test site");
  $ns->output("SITE-TEST", $test_site, "test site");

  my $non_test_colo = $ns->create(Isp => {
    name => "non-test",
    flags => 0,
    colocation_revenue_share => 0.1 });

  my $test_colo = $ns->create(Isp => { 
      name => "test",
      account_flags => DB::Account::TEST,
      colocation_revenue_share => 0.1});

  $ns->output("COLO-NON-TEST", $non_test_colo->{colo_id});
  $ns->output("COLO-TEST", $test_colo->{colo_id});
  $ns->output("COLO-DEFAULT", DB::Defaults::instance()->isp->{colo_id});

  # ADSC-5526
  my $pub_inv_keyword = make_autotest_name($ns, "pubinv");
  $ns->output("PUBINV/KW", $pub_inv_keyword);
  my $cpm = 1;
  my $pub_inv_cmp = $ns->create(DisplayCampaign => {
    name => 'pubInv',
    advertiser_flags => DB::Account::TEST,
    campaigncreativegroup_cpm => $cpm, # works relative margin
    campaigncreativegroup_flags => 0,
    campaign_flags => 0,
    channel_id => DB::BehavioralChannel->blank(
      name => 'pubInv',
      account_id => $acc,
      keyword_list => $pub_inv_keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ])
  });

  my $inv_test_tag = $ns->create(PricedTag => {
    name => "inv",
    site_id => $test_site,
    flags => DB::Tags::INVENORY_ESTIMATION,
    cpm => 10,
    passback => $passback_url });

  $ns->output("PUBINV/TAG", $inv_test_tag, "test tag in inventory estimation mode");
  $ns->output("PUBINV/CC", $pub_inv_cmp->{cc_id});
  use POSIX qw(floor);
  $ns->output("PUBINV/THRESHOLD_CPM", 
    money_upscale($cpm * 100, 8));

  # ADSC-6708
  $self->create_overlap_channels($ns);
}

1;
