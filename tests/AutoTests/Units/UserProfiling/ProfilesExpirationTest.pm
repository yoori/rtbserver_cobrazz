
package ProfilesExpirationTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account =>
    { name => "acc",
      role_id => DB::Defaults::instance()->advertiser_role});

  # History + Today channels
  my $keyword_ht = make_autotest_name($ns, "keyword-ht");
  $ns->output("HT/Keyword", $keyword_ht, "keyword for matching HT channels");

  my $channel_ht1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'ht1',
    account_id => $advertiser,
    keyword_list => $keyword_ht,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 259200)]));


  my $channel_ht2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'ht2',
    account_id => $advertiser,
    keyword_list => $keyword_ht,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 86400)]));

  $ns->output("HT/Channel1", $channel_ht1->channel_id(), "0-3 days history channel");
  $ns->output("HT/Channel2", $channel_ht2->channel_id(), "0-1 days history channel");

  # History channels
  my $keyword_h = make_autotest_name($ns, "keyword-h");
  $ns->output("H/Keyword", $keyword_h, "keyword for matching H channels");

  my $channel_h1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'h1',
    account_id => $advertiser,
    keyword_list => $keyword_h,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_from => 259200,
        time_to => 345600)]));

  my $channel_h2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'h2',
    account_id => $advertiser,
    keyword_list => $keyword_h,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_from => 172800,
        time_to => 259200)]));

  my $channel_h3 = $ns->create(DB::BehavioralChannel->blank(
    name => 'h3',
    account_id => $advertiser,
    keyword_list => $keyword_h,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_from => 86400,
        time_to => 172800)]));

  $ns->output("H/Channel1", $channel_h1->channel_id(), "3-4 days history channel");
  $ns->output("H/Channel2", $channel_h2->channel_id(), "2-3 days history channel");
  $ns->output("H/Channel3", $channel_h3->channel_id(), "1-2 days history channel");

  # Session channels
  my $keyword_s = make_autotest_name($ns, "keyword-s");
  $ns->output("S/Keyword", $keyword_s, "keyword for matching S channels");

  my $channel_s1 = $ns->create(DB::BehavioralChannel->blank(
    name => 's1',
    account_id => $advertiser,
    keyword_list => $keyword_s,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 10800)]));

  $ns->output("S/Channel1", $channel_s1->channel_id(), "0-3 hours session channel");

  # ADSC-5643
  my $freq_cap = $ns->create(FreqCap =>
                             { life_count => 4 });

  my $keyword_2 = make_autotest_name($ns, "adsc-5643");

  my $channel3 = $ns->create(DB::BehavioralChannel->blank(
    name => "adsc-5643",
    account_id => $advertiser,
    keyword_list => $keyword_2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 259200)]));

  my $campaign = $ns->create(DisplayCampaign => {
    name => "adsc-5643",
    account_id => $advertiser,
    channel_id => $channel3->channel_id(),
    campaign_freq_cap_id => $freq_cap,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{ name => "adsc-5643" }] });

  my $tag = $ns->create(PricedTag =>
                        { name => "adsc-5643",
                          site_id => $campaign->{Site}[0]->{site_id} });

  my $stmt = $ns->pq_dbh->prepare("SELECT Max(Colo_id) From Colocation");
  $stmt->execute;
  my @absent_colo = $stmt->fetchrow_array;
  $stmt->finish;

  $ns->output("ABSENT_COLO", $absent_colo[0] + 1000);

  $ns->output("ADSC-5643/KEYWORD", $keyword_2);
  $ns->output("ADSC-5643/CHANNEL", $channel3->channel_id());
  $ns->output("ADSC-5643/CC_ID", $campaign->{cc_id});
  $ns->output("ADSC-5643/TAG", $tag);
  $ns->output("ADSC-5643/FCAP", $freq_cap);
}

1;
