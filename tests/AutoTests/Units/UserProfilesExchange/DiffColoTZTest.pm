
package DiffColoTZTest;

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
 
  my $tz_ofset = get_tz_ofset($ns, 
    DB::Defaults::instance()->remote_isp->{Account}->{timezone_id}->{tzname});

  $ns->output("TZOfset", $tz_ofset);

  my $account = $ns->create(Account =>
                            { name => 1,
                              role_id => DB::Defaults::instance()->advertiser_role });
  

  my $kwdS = make_autotest_name($ns, "S");
  my $kwdHT1 = make_autotest_name($ns, "HT1");
  my $kwdHT2 = make_autotest_name($ns, "HT2");
  my $kwdC = make_autotest_name($ns, "C");
  $ns->output("KeywordS", $kwdS);
  $ns->output("KeywordHT1", $kwdHT1);
  $ns->output("KeywordHT2", $kwdHT2);
  $ns->output("KeywordC", $kwdC);

  # Session channel (for Test 5.1)
  my $bps = $ns->create(
      DB::BehavioralChannel->blank(
        name => "S",
        keyword_list => $kwdS,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 28800) ]));


  # History+today channels

  # (for Test 5.1)
  my $bpht1 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "HT1",
        keyword_list => $kwdHT1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 2,
            time_to => 172800) ]));

  $ns->output("BP/S", $bps->page_key());
  $ns->output("Channel/S", $bps->channel_id);
  $ns->output("BP/HT1", $bpht1->page_key());
  $ns->output("Channel/HT1", $bpht1->channel_id);


  # (for Test 5.2)
  my $bpht2 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "HT2",
        keyword_list => $kwdHT2,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 3,
            time_to => 172800) ]));

  # (for Test 5.2)
  my $bpc = $ns->create(
      DB::BehavioralChannel->blank(
        name => "C",
        keyword_list => $kwdC,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 259200) ]));

  $ns->output("BP/HT2", $bpht2->page_key());
  $ns->output("Channel/HT2", $bpht2->channel_id);
  $ns->output("BP/C", $bpc->page_key());
  $ns->output("Channel/C", $bpc->channel_id);
}

1;
