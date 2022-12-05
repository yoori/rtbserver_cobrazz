package SiteChannelStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword1 = "keyword1";
  $ns->output("KW1", $keyword1);

  my $url1 = "url1";
  $ns->output("URL1", $url1);

  my $advertiser = $ns->create(Account => {
  name => 'Advertiser',
  role_id => DB::Defaults::instance()->advertiser_role});

  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $advertiser,
    keyword_list => $keyword1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P")]));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 1,
    channel_id => $channel1->channel_id(),
    campaigncreativegroup_cpm => 200,
    campaigncreativegroup_cpc => 20,
    site_links => [{name => 1}] });

  $ns->output("BP1", $channel1->page_key());
  $ns->output("CH1", $channel1->channel_id());

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => 2,
    account_id => $advertiser,
    url_list => $url1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U")]));
    
  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 2,
    account_id => $advertiser,
    channel_id => $channel2->channel_id(),
    campaigncreativegroup_cpm => 100,
    campaigncreativegroup_cpc => 10,
    site_links => [{site_id => $campaign1->{Site}[0]->{site_id}}] });

  $ns->output("BP2", $channel2->url_key());
  $ns->output("CH2", $channel2->channel_id());

  my $ch3 = $ns->create(DB::BehavioralChannel->blank(
    name => 3,
    account_id => $campaign1->{account_id},
    keyword_list => $keyword1,
    url_list => $url1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P'),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'U') ]));

  $ns->output("CCG1", $campaign1->{ccg_id});
  $ns->output("CCG2", $campaign2->{ccg_id});

  $ns->output("CC1", $campaign1->{cc_id});
  $ns->output("CC2", $campaign2->{cc_id});

  $ns->output("CH3", $ch3);
  $ns->output("BP31", $ch3->page_key());
  $ns->output("BP32", $ch3->url_key());

  my $tag1 = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign1->{Site}[0]->{site_id},
    cpm => 50 });

  $ns->output("TID1", $tag1);
}

1;
