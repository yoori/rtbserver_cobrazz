
package AbsentProfileTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;
      
  my $kwd1 = make_autotest_name($ns, "KWD1");
  my $kwd2 = make_autotest_name($ns, "KWD2");
  
  my $account = $ns->create(Account =>
                            { name => 1,
                              role_id => DB::Defaults::instance()->advertiser_role });
  
  # Session channels

  my $bps1 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "S1",
        keyword_list => $kwd1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 2,
            time_to => 7200) ]));

  my $bps2 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "S2",
        keyword_list => $kwd2,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 7200) ]));

  
  $ns->output("BP/S1", $bps1->page_key());
  $ns->output("Channel/S1", $bps1->channel_id);
  $ns->output("BP/S2", $bps2->page_key());
  $ns->output("Channel/S2", $bps2->channel_id);
  
  # History+today channels
  my $bpht1 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "HT1",
        keyword_list => $kwd1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 2,
            time_to => 86400) ]));

  my $bpht2 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "HT2",
        keyword_list => $kwd2,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_to => 86400) ]));
  
  $ns->output("BP/HT1", $bpht1->page_key());
  $ns->output("Channel/HT1", $bpht1->channel_id);
  $ns->output("BP/HT2", $bpht2->page_key());
  $ns->output("Channel/HT2", $bpht2->channel_id);
  
  # History channels
  my $bph1 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "H1",
        keyword_list => $kwd1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 2,
            time_from => 86400,
            time_to => 259200) ]));

  my $bph2 = $ns->create(
      DB::BehavioralChannel->blank(
        name => "H2",
        keyword_list => $kwd1,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 1,
            time_from => 86400,
            time_to => 259200) ]));

  my $stmt = $ns->pq_dbh->prepare("SELECT Max(Colo_id) From Colocation");
  $stmt->execute;
  my @result = $stmt->fetchrow_array;
  $stmt->finish;
  
  $ns->output("BP/H1", $bph1);
  $ns->output("Channel/H1", $bph1->channel_id);
  $ns->output("BP/H2", $bph2);
  $ns->output("Channel/H2", $bph2->channel_id);
  
  $ns->output("ValidColo", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("AbsentColo", $result[0] + 10000);
  $ns->output("Keyword1", $kwd1);
  $ns->output("Keyword2", $kwd2);
}

1;
