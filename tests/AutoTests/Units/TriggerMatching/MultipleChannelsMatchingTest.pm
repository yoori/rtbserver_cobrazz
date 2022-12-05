package MultipleChannelsMatchingTest;

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

  $ns->output("Tags/Default", DB::Defaults::instance()->tag);

  my $bp = $ns->create(
      DB::BehavioralChannel->blank(
        name => 1,
        keyword_list => "Test10",
        account_id => $acc,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P') ]));

  $ns->output("MultipleChannelsMatchingTest/01", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 2,
      keyword_list => "Test11",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("MultipleChannelsMatchingTest/02", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 3,
      keyword_list => "Test12",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("MultipleChannelsMatchingTest/03", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 4,
      url_list => "Test12",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U') ]));

  $ns->output("MultipleChannelsMatchingTest/04", $bp->url_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 5,
      keyword_list => "Test13",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("MultipleChannelsMatchingTest/05", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 8,
      keyword_list => "Test14",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("MultipleChannelsMatchingTest/08", $bp->page_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 9,
      keyword_list => "Test14",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("MultipleChannelsMatchingTest/09", $bp->page_key());
}

1;
