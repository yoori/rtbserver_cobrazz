package CombinedHardSoftMatchingTest;

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

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $acc,
    keyword_list => "\"Test3 Test4\" Test5",
    behavioral_parameters => [ DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P") ]));

  $ns->output("CombinedHardSoftMatchingTest/01",
    $ch->page_key(), "type = P, content = \"Test3 Test4\" Test5");
}

1;
