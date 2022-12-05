
package StatProfilesExpirationTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account => {
    name => "acc",
    role_id => DB::Defaults::instance()->advertiser_role});

  my $action = $ns->create(Action =>
    { name => "adsc-5643",
      url => "http:\\www.ebay.com",
      account_id => $advertiser});

  my $keyword_d = make_autotest_name($ns, "adsc-5643-d");
  my $url = "http://" . make_autotest_name($ns, "adsc-5643") . ".com";

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "adsc-5643",
    account_id => $advertiser,
    keyword_list => $keyword_d,
    url_list => $url,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P")]));

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $channel->channel_id(),
      trigger_type => "U"));

  my $campaign = $ns->create(DisplayCampaign => {
    name => "adsc-5643-d",
    account_id => $advertiser,
    channel_id => $channel->channel_id(),
    campaigncreativegroup_cpa => 1, # required for action profile creating
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_rate_type => 'CPA',
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_action_id => $action,
    site_links => [{ name => "adsc-5643" }] });

  my $keyword_t = make_autotest_name($ns, "adsc-5643-t");

  my $text_campaign = $ns->create(TextAdvertisingCampaign => {
    name => "adsc-5643-t",
    template_id => DB::Defaults::instance()->text_template,
    size_id => DB::Defaults::instance()->size(),
    account_id => $advertiser,
    original_keyword => $keyword_t,
    max_cpc_bid => 100,
    site_links => [{ site_id => $campaign->{Site}[0]->{site_id} }] });

  my $tag = $ns->create(PricedTag =>
    { name => "adsc-5643",
      site_id => $campaign->{Site}[0]->{site_id},
      passback => "www.statprofilesexpirationtest.com" });

  $ns->output("KEYWORD/DISPLAY", $keyword_d);
  $ns->output("KEYWORD/TEXT", $keyword_t);
  $ns->output("URL", $url);
  $ns->output("CHANNEL", $channel->page_key());
  $ns->output("K-CHANNEL", $text_campaign->{CCGKeyword}->{channel_id}->page_key());
  $ns->output("CC/DISPLAY", $campaign->{cc_id});
  $ns->output("CC/TEXT", $text_campaign->{cc_id});
  $ns->output("TAG", $tag);
  $ns->output("ACTION", $action);
}

1;
