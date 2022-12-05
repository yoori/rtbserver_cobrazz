
package BannedChAdReqProfDisabling;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  print_NoAdvNoTrack($ns);

  my $behav_channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id =>  $account,
    keyword_list => "robbery\n" .
      "crime\n" .
      "murder\n" .
      "unbanned_2",
    url_list => "alcoholnews.org\n" .
      "alcoholism.about.com\n" .
      "www.tobacco.org/ads\n" .
      "www.smokin4free.com/parliament.html",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P" , time_to => 3 * 60 * 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U", time_to => 3 * 60 * 60)]));

  my $publisher = $ns->create(Publisher => { name => "Publisher" });

  my $campaign  = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    channel_id =>  $behav_channel1->channel_id(),
    campaigncreativegroup_cpm => 1000,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->output("Channel1", $behav_channel1->channel_id());
  $ns->output("Channel1_1", $behav_channel1->page_key());
  $ns->output("Channel1_2", $behav_channel1->url_key());

  $ns->output("Tag", $publisher->{tag_id});
  $ns->output("CC", $campaign->{cc_id});

  $ns->output("KWD1", "robbery");
  $ns->output("REF1", "http://alcoholnews.org");

  $ns->output("KWD2_1", "murder");
  $ns->output("KWD2_2", "crime");
  $ns->output("REF2", "google.com/search?hl=en&q=crime&btnG=Search");
}

1;
