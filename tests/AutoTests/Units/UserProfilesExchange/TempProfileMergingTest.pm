

package TempProfileMergingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;
  $ns->output("Colo/2", DB::Defaults::instance()->remote_isp->{colo_id});
  $ns->output("Colo/1", DB::Defaults::instance()->isp->{colo_id});
  
  $ns->output("COLO_EXCHANGE_TIMEOUT", 300);
  
  my $account = $ns->create(Account =>
                            { name => 1,
                              role_id => DB::Defaults::instance()->advertiser_role });
  
  
  my $kwdS1 = make_autotest_name($ns, "S1");
  my $kwdS2 = make_autotest_name($ns, "S2");
  my $kwdHT1 = make_autotest_name($ns, "HT1");
  my $kwdHT2 = make_autotest_name($ns, "HT2");
  $ns->output("KeywordS1", $kwdS1);
  $ns->output("KeywordS2", $kwdS2);
  $ns->output("KeywordHT1", $kwdHT1);
  $ns->output("KeywordHT2", $kwdHT2);

  # Session channels

  my $bps1 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "S1",
        keyword_list => $kwdS1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 36000) ]));

  my $bps2 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "S2",
        keyword_list => $kwdS2,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 36000) ]));

  # History+today channels
  my $bpht1 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "HT1",
        keyword_list => $kwdHT1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 172800) ]));

  my $bpht2 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "HT2",
        keyword_list => $kwdHT2,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 172800) ]));

  $ns->output("Channel/S1", $bps1->channel_id);
  $ns->output("Channel/S2", $bps2->channel_id);
  $ns->output("Channel/HT1", $bpht1->channel_id);
  $ns->output("Channel/HT2", $bpht2->channel_id);
}

1;
