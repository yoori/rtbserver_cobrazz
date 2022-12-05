
package NoAdvNoTrackTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  print_NoAdvNoTrack($ns);

  my $not_stored_kwd = make_autotest_name($ns, "not_store_in_profile_1");
  
  my $account = $ns->create(
    Account => { 
      name => 1,
      role_id => DB::Defaults::instance()->advertiser_role });

  my $channel1 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      account_id =>  $account,
      keyword_list => 
        "robbery\n" .
        "crime\n" .
        "unbanned_2",
      url_list => 
        "alcoholnews.org\n" .
        "alcoholism.about.com\n" .
        "www.tobacco.org/ads\n" .
        "www.smokin4free.com/parliament.html",
      url_kwd_list =>
        "bannedAdvURLKeyword\n" .
        "bannedTrackURLKeyword",
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          time_to => 3 * 60 * 60),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "S",
          time_to => 3 * 60 * 60),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "R",
          time_to => 3 * 60 * 60),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "U",
          time_to => 3 * 60 * 60)] ));


  my $channel2 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 2,
      account_id =>  $account,
      keyword_list => 
          "drugs\n" .
          "crime\n" .
          "murder\n" .
          "robbery\n" .
          "unbanned_2",
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          time_to => 24 * 60 * 60),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "S",
          time_to => 24 * 60 * 60)] ));

  my $channel3 = $ns->create(
    DB::BehavioralChannel->blank( 
      name => 3,
      account_id =>  $account,
      keyword_list => 
          "drugs\n" .
          "robbery\n",
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 3,
          time_from => 24 * 60 * 60,
          time_to => 2* 24 * 60 * 60)] ));

  my $channel4 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 4,
      account_id =>  $account,
      keyword_list => 
        "drugs\n" .
        "crime\n" .
        "robbery\n",
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P")] ));

  my $channel5 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 5,
      account_id =>  $account,
      keyword_list => $not_stored_kwd,
      url_kwd_list => $not_stored_kwd,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "S",
          time_to => 3 * 60 * 60),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "R",
          time_to => 3 * 60 * 60),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          time_to => 3 * 60 * 60)] ));

  my $expression_channel = $ns->create(
    ExpressionChannel => { 
      name => "Expr",
      account_id => $account,
      expression => 
        $channel1->channel_id . "|" . 
        $channel2->channel_id . "|" . 
        $channel3->channel_id . "|" .
        $channel4->channel_id . "|" .
        $channel5->channel_id });

  my $publisher = $ns->create(
    Publisher => { name => "Publisher" });
  
  my $campaign = $ns->create(
    DisplayCampaign => { 
      name => 'Campaign',
      channel_id =>  $expression_channel->{channel_id},
      campaigncreativegroup_cpm => 1000,
      site_links => [{ site_id =>  $publisher->{site_id} }] });

  my $passback_url = 'https://jira.corp.foros.com/browse/ADSC-5272';

  my $passback_tag = $ns->create(
    PricedTag => { 
      name => "Passback",
      site_id => $publisher->{site_id},
      passback => $passback_url });

  $ns->output("Channel1_1", $channel1->page_key());
  $ns->output("Channel1_2", $channel1->url_key());
  $ns->output("Channel1_3", $channel1->search_key());
  $ns->output("Channel1_4", $channel1->url_kwd_key());
  $ns->output("Channel2_1", $channel2->page_key());
  $ns->output("Channel2_2", $channel2->search_key());
  $ns->output("Channel3_1", $channel3->page_key());
  $ns->output("Channel4_1", $channel4->page_key());
  $ns->output("Channel5_1", $channel5->page_key());
  $ns->output("Channel5_2", $channel5->search_key());
  $ns->output("Channel5_3", $channel5->url_kwd_key());

  $ns->output("Channel1", $channel1);
  $ns->output("Channel2", $channel2);
  $ns->output("Channel3", $channel3);
  $ns->output("Channel4", $channel4);
  $ns->output("ChannelNotProfile", $channel5);

  $ns->output("Tag", $publisher->{tag_id});
  $ns->output("PassbackTag", $passback_tag->{tag_id});
  $ns->output("PassbackURL", $passback_url);
  $ns->output("CC", $campaign->{cc_id});

  $ns->output("not_store_kwd", $not_stored_kwd);
}

1;
