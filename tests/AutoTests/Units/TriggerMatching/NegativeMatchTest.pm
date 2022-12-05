
package NegativeMatchTest;

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

  my @channels;

  $channels[0] = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $account,
    keyword_list => "squid\n" .
      "-squid-ocean\n" . 
      "-giant squid\n" . 
      "-ocean\n" . 
      "-\"mega squid\"\n" . 
      "small squid",
    url_list => "-ocean.com\n" .
      "-squids-online.com",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U") ]));

  $channels[1] = $ns->create(DB::BehavioralChannel->blank(
    name => 2,
    account_id => $account,
    keyword_list => "squid\n" .
      "squid-ocean",
    url_list => "ocean.com\n" .
      "squids-online.com",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U") ]));

  $channels[2] = $ns->create(DB::BehavioralChannel->blank(
    name => 3,
    account_id => $account,
    keyword_list => "-tiguar zoo\n" .
      "tiguar car",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 60) ]));


  $channels[3] = $ns->create(DB::BehavioralChannel->blank(
    name => 4,
    account_id => $account,
    url_list => "jaguar.com\n" .
      "-zoo.com",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U") ]));

  $channels[4] = $ns->create(DB::BehavioralChannel->blank(
    name => 5,
    account_id => $account,
    keyword_list => "-minus4 -escape4\n" .
      "minus4 \\-escape4",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 60) ]));

  $channels[5] = $ns->create(DB::BehavioralChannel->blank(
    name => 6,
    account_id => $account,
    keyword_list => "exactmatchnotnegative\n" .
      "-[exactmatchnegative1]\n" .
      "-[\" exacTmatchnegative3\" exactmatchnegative2]",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        time_to => 60) ]));

  $channels[6] = $ns->create(DB::BehavioralChannel->blank(
    name => 7,
    account_id => $account,
    keyword_list => "SplitPageSearch\n" .
      "-SplitPageSearchNeg1",
    search_list => "SplitPageSearch\n" .
      "-SplitPageSearchNeg2",
    url_kwd_list => "SplitPageSearch\n" .
      "-SplitPageSearchNeg3",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "R",
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        time_to => 60) ]));


  my $expression_channel = $ns->create(ExpressionChannel => {
    name => "ExprCCG",
    account_id => $account,
    expression =>
      $channels[0]->channel_id() . "|" . 
      $channels[1]->channel_id() . "|" . 
      $channels[2]->channel_id()  . "|" .
      $channels[3]->channel_id()  . "|" .
      $channels[4]->channel_id() . "|" .
      $channels[5]->channel_id() . "|" .
      $channels[6]->channel_id() });

  my $publisher = $ns->create(Publisher => { name => "Publisher" });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    channel_id =>  $expression_channel->{channel_id},
    campaigncreativegroup_cpm => 1000,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->output("REF1", "jaguar.com");
  $ns->output("KWD1", "squid,tiguar car");
  $ns->output("REF2", "google.com");
  $ns->output("FT2", "giant squid");
  $ns->output("REF3", "google.com/search?hl=en&q=giant+squid&btnG=Search");
  $ns->output("KWD3", "squid");
  $ns->output("REF4", "ocean.com");
  $ns->output("KWD4", "squid");
  $ns->output("KWD5", "squid-ocean");
  $ns->output("REF5", "squids-online.com");
  $ns->output("KWD6", "mega squid");
  $ns->output("FT7", "tiguar zoo,tiguar car");
  $ns->output("REF8", "zoo.com");
  $ns->output("KWD9", "minus4 \\-escape4");

  $ns->output("KWD10", "exactmatchnotnegative");
  $ns->output("SEARCH10", "exactmatchnegative1");
  $ns->output("SEARCH11", "exactmatchnotnegative exactmatchnegative1");
  $ns->output("KWD12", "exactmatchnotnegative");
  $ns->output("SEARCH12", "Exactmatchnegative3 exactMatchnegative2");
  $ns->output("KWD13", "exactmatchnotnegative");
  $ns->output("SEARCH13", "exactmatchnegative2 exactmatchnegative3");

  $ns->output("KWD14", "SplitPageSearchNeg1");
  $ns->output("SEARCH14", "SplitPageSearch");
  $ns->output("KWD15", "SplitPageSearch");
  $ns->output("SEARCH15", "SplitPageSearchNeg2");
  $ns->output("SEARCH16", "SplitPageSearch,SplitPageSearchNeg1");
  $ns->output("SEARCH17", "SplitPageSearchNeg3");
  $ns->output("KWD17", "SplitPageSearch,SplitPageSearchNeg2");
  $ns->output("KWD18", "SplitPageSearch,SplitPageSearchNeg3");
  
  $ns->output("Channel1_1", $channels[0]->page_key());
  $ns->output("Channel1_2", $channels[0]->url_key());
  $ns->output("Channel1_3", $channels[0]->search_key());

  $ns->output("Channel2_1", $channels[1]->page_key());
  $ns->output("Channel2_2", $channels[1]->url_key());

  $ns->output("Channel3_1", $channels[2]->page_key());

  $ns->output("Channel4_1", $channels[3]->url_key());
  $ns->output("Channel5_1", $channels[4]->page_key());

  $ns->output("Channel6_1", $channels[5]->page_key());
  $ns->output("Channel6_2", $channels[5]->search_key());

  $ns->output("Channel7_1", $channels[6]->page_key());
  $ns->output("Channel7_2", $channels[6]->search_key());
  $ns->output("Channel7_3", $channels[6]->url_kwd_key());

  $ns->output("Tag", $publisher->{tag_id});
  $ns->output("CC", $campaign->{cc_id});

  $ns->output("NegativeTriggersList", 100, "list of channel_triggers_id's with negative keywords");
  foreach my $channel (@channels)
  {
    my @triggers;
    push @triggers, @{$channel->keyword_channel_triggers()} if $channel->keyword_channel_triggers();
    push @triggers, @{$channel->search_channel_triggers()} if $channel->search_channel_triggers();
    push @triggers, @{$channel->url_channel_triggers()} if $channel->url_channel_triggers();
    foreach my $trigger (@triggers)
    {
      $ns->output($trigger->original_trigger(),
        $trigger->channel_trigger_id(),
        $trigger->trigger_type()) if $trigger->negative() eq 'Y';
    }
  }
  $ns->output("NegativeTriggersListEnd", 100);

  $ns->output("MarkerTrigger",
    $channels[6]->keyword_channel_triggers()->[0]->channel_trigger_id());
}

1;
