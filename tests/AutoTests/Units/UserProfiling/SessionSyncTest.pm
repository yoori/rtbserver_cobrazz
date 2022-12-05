package SessionSyncTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = "Test27";
  my $min_visits = 99;
  my $repeat_count = (1000/$min_visits)*20;

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    keyword_list => $keyword,
    account_id => $acc,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        minimum_visits => $min_visits,
        time_to => 600) ]));

  $ns->output("SessionSyncTest/01",
    $ch->page_key(),
    "Minimum_visits = $min_visits, from = 0, to = 600, type = P, content = $keyword");
  $ns->output("SessionSyncTest/KeyWord", $keyword, "KeyWord");
  $ns->output("SessionSyncTest/MinimumVisits", $min_visits, "MinimumVisits");
  $ns->output("SessionSyncTest/RepeatCount", $repeat_count, "RepeatCount");
}

1;
