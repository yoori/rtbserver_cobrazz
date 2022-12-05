package BadReferersProcessingTest;

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

  $ns->output("Tags/Default", DB::Defaults::instance()->tag, "tid");

  my $behav_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "U",
    url_list => "U01",
    account_id => $acc,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "U") ]));

  $ns->output("Channel/U/01", $behav_channel->url_key(), "type = U, content = \"U01\"");

  $behav_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "P",
    keyword_list => "P_01",
    account_id => $acc,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P")
      ]));

  $ns->output("Channel/P/01", $behav_channel->page_key(), "type = P, content = \"P_01\"");
}

1;
