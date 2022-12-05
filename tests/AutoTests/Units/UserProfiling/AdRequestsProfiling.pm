
package AdRequestsProfiling;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $url1 = "http://" . make_autotest_name($ns, 1) . ".com";
  my $url2 = "http://" . make_autotest_name($ns, 2) . ".com";
  my $keyword1 = make_autotest_name($ns, 1);
  my $keyword2 = make_autotest_name($ns, 2);
  my $keyword3 = make_autotest_name($ns, 3);
  my $keyword4 = make_autotest_name($ns, 4);
  my $keyword5 = make_autotest_name($ns, 5);

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $publisher = $ns->create(Publisher => {
    name => "Publisher"});

  
  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    account_id => $account,
    name => 1,
    keyword_list => $keyword1,
    url_list => $url1,
    behavioral_parameters => 
      [ DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "U",
          minimum_visits => 1,
          time_to => 8*60*60 ),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 2,
          time_to => 3*24*60*60 )] ));

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    account_id => $account,
    name => 2,
    keyword_list => $keyword2,
    url_list => $url2,
    behavioral_parameters => 
      [ DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "U",
          minimum_visits => 3,
          time_to => 3*24*60*60 ),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 1,
          time_from => 1*24*60*60,
          time_to => 3*24*60*60 )] ));

  my $channel3 = $ns->create(DB::BehavioralChannel->blank(
    account_id => $account,
    name => 3,
    keyword_list => $keyword3,
    behavioral_parameters => 
      [ DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 1,
          time_to => 3*24*60*60 )] ));

  my $channel4 = $ns->create(DB::BehavioralChannel->blank(
    account_id => $account,
    name => 4,
    keyword_list => $keyword4  . "\n" . $keyword5,
    behavioral_parameters => 
      [ DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 2,
          time_to => 2*24*60*60 )] ));

  my $channel5 = $ns->create(DB::BehavioralChannel->blank(
    account_id => $account,
    name => 5,
    keyword_list => $keyword4,
    behavioral_parameters => 
      [ DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 1,
          time_to => 20*60 )] ));

  my $expression = $ns->create(DB::ExpressionChannel->blank(
    name => "Expression",
    account_id => $account,
    expression => 
      $channel1->channel_id . "|" . 
      $channel2->channel_id . "|" . 
      $channel3->channel_id));

  my $campaign = $ns->create(DisplayCampaign => {
    name => "adsc-5643",
    account_id => $account,
    channel_id => $expression->channel_id(),
    campaigncreativegroup_cpm => 1,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->output("Ref1", $url1);
  $ns->output("Ref2", $url2);
  $ns->output("Keyword1", $keyword1);
  $ns->output("Keyword2", $keyword2);
  $ns->output("Keyword3", $keyword3);
  $ns->output("Keyword4", $keyword4);
  $ns->output("Keyword5", $keyword5);

  $ns->output("BP1P", $channel1->page_key());
  $ns->output("BP2P", $channel2->page_key());
  $ns->output("BP3P", $channel3->page_key());
  $ns->output("BP4P", $channel4->page_key());
  $ns->output("BP5P", $channel5->page_key());
  $ns->output("BP1U", $channel1->url_key());
  $ns->output("BP2U", $channel2->url_key());

  $ns->output("Channel1", $channel1);
  $ns->output("Channel2", $channel2);
  $ns->output("Channel3", $channel3);
  $ns->output("Channel4", $channel4);
  $ns->output("Channel5", $channel5);

  $ns->output("CC", $campaign->{cc_id});
  $ns->output("Tag", $publisher->{tag_id});
}

1;
