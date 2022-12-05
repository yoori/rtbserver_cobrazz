
package ExactKeywordMatching;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $account,
    search_list => "nonexactmatch",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        time_to => 60) ]));

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => 2,
    account_id => $account,
    search_list => 
     "[exactmatch2 exactmatch1]\n" .
     "[exactmatch3]\n" .
     "[exactmatch4]\n" .
     "[exactmatch5]\n" .
     "[exactmatch5 exactmatch6]\n" .
     "[exactmatch7]\n" .
     "exactmatch7",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        time_to => 60) ]));

  $ns->output("Channel1S", $channel1->search_key());
  $ns->output("Channel2S", $channel2->search_key());
  $ns->output("Channel1", $channel1);
  $ns->output("Channel2", $channel2);
  output_channel_triggers($ns, "ChannelTrigger/2", $channel2, 'S');

  $ns->output("SEARCH1", "exactmatch2 exactmatch1");
  $ns->output("SEARCH2", "exactmatch1 exactmatch2");
  $ns->output("SEARCH3", "exactmatch2 exactmatch1 nonexactmatch");
  $ns->output("SEARCH4", "nonexactmatch exactmatch2 exactmatch1");
  $ns->output("SEARCH5", "exactmatch2 nonexactmatch exactmatch1");
  $ns->output("SEARCH6", "exactmatch3 exactmatch3");
  $ns->output("SEARCH7", "exactmatch3 exactmatch4");
  $ns->output("SEARCH8", "exactmatch5");
  $ns->output("SEARCH9", "exactmatch5 exactmatch6");
  $ns->output("SEARCH10", "exactmatch7 dummy");
}

1;
