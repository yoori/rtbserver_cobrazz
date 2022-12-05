package SoftTriggerMatchingTest;

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


  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      keyword_list => "Test1 Test2",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("Channel/01", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 2,
      keyword_list => "Test15 Test16 Test17 Test18",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("Channel/02", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 3,
      keyword_list => "Test17 Test19 Test20 Test16 Test21",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("Channel/03", $bp->page_key());

}

1;
