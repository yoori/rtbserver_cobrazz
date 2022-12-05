
package UserInfoExchangerFunctionalityTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;
  
  my $acc = $ns->create(Account =>
                        { name => 1,
                          role_id => DB::Defaults::instance()->advertiser_role });
  
  my $key1 = make_autotest_name($ns, "S");
  my $key2 = make_autotest_name($ns, "HT");
  my $key3 = make_autotest_name($ns, "H");
  my $keyMark1 = make_autotest_name($ns, "Marker1"); # key for "marker" channel#1
  my $keyMark2 = make_autotest_name($ns, "Marker2"); # key for "marker" channel#2

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "S",
      keyword_list => $key1,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          minimum_visits => 3,
          time_to => 7200) ]));
  
  $ns->output("BP/S", $bp->page_key());
  $ns->output("CH/S", $bp->channel_id);
  $ns->output("KeyS", $key1);

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "HT",
      keyword_list => $key2,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          minimum_visits => 3,
          time_to => 259200) ]));
  
  $ns->output("BP/HT", $bp->page_key());
  $ns->output("CH/HT", $bp->channel_id);
  $ns->output("KeyHT", $key2);

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "H",
      keyword_list => $key3,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          minimum_visits => 3,
          time_from => 86400,
          time_to => 259200) ]));
  
  $ns->output("BP/H", $bp->page_key());
  $ns->output("CH/H", $bp->channel_id);
  $ns->output("KeyH", $key3);

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "Marker1",
      keyword_list => $keyMark1,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          minimum_visits => 1,
          time_to => 7200) ]));
  
  $ns->output("BP/Marker1", $bp->page_key());
  $ns->output("CH/Marker1", $bp->channel_id);
  $ns->output("KeyMarker1", $keyMark1);

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "Marker2",
      keyword_list => $keyMark2,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          minimum_visits => 1,
          time_to => 259200) ]));
  
  $ns->output("BP/Marker2", $bp->page_key());
  $ns->output("CH/Marker2", $bp->channel_id);
  $ns->output("KeyMarker2", $keyMark2);

  $ns->output("COLO_EXCHANGE_TIMEOUT", 500);

  $ns->output("COLO1", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("COLO2", DB::Defaults::instance()->remote_isp->{colo_id});
}

1;
