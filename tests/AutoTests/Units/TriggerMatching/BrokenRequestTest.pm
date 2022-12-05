
package BrokenRequestTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword1 = make_autotest_name($ns, "audi");
  my $keyword2 = make_autotest_name($ns, "renault");
  my $keyword3 = make_autotest_name($ns, "citroen");
  my $keyword4 = "broken encoding";

  my $domain = make_autotest_name($ns, "domain");
  my $url1   = $domain . ".ru";
  my $url2   = $domain . ".fr";
  my $url3   = "www.google.ru";

  my $channel1 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      keyword_list => 
        $keyword1 . "\n" .
        $keyword2 . "\n" . $keyword3,
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 60) ]));

  my $channel2 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 2,
      url_list => $url1 . "\n" . $url2,
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U') ]));

  my $channel3 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 3,
      url_list => $url3,
      keyword_list => $keyword4,
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U'),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'S',
          time_to => 60), ]));

  $ns->output("Channel1", $channel1->page_key());
  $ns->output("Channel2", $channel2->url_key());
  $ns->output("Channel3_1", $channel3->url_key());
  $ns->output("Channel3_2", $channel3->search_key());
  
  $ns->output("REF1", $url1);
  $ns->output("KWD1", "\"p," . $keyword2);
  $ns->output("REF2", "www.mail:infobox.ru");
  $ns->output("KWD2", $keyword2);
  $ns->output("SEARCH3", "broken%%20encoding");
}

1;
