
package UrlTriggerTypeTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my @triggerlists = ("internet.cnews.ru\n" .
                        "internet.cnews.fr",
                        "www.internet.cnews.ru/internet-reclama/index.shtml\n" .
                        "www.internet.cnews.fr/internet-reclama/index.shtml",
                        "www.internet.cnews.ru/internet-reclama/index.shtml?1\n" .
                        "www.internet.cnews.fr/internet-reclama/index.shtml?1",
                        "cnews.ru\n" . 
                        "cnews.fr",
                        "internet.cnews.ru/internet-reclama\n" .
                        "internet.cnews.fr/internet-reclama",
                        "internet.cnews.ru/internet-reclama/index.shtml\n" .
                        "internet.cnews.fr/internet-reclama/index.shtml",
                        "internet.cnews.ru/internet-reclama/index.shtml?1\n" .
                        "internet.cnews.fr/internet-reclama/index.shtml?1",
                        "www.internet.cnews.ru\n" .
                        "www.internet.cnews.fr",
                        "www.internet.cnews.ru/internet-reclama\n" .
                        "www.internet.cnews.fr/internet-reclama",
                        "cnews.fr/internet-reclama\n" .
                        "cnews.ru/internet-reclama",
                        "www.mapinter.net/eng");

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $index = 0;

  foreach my $trigger (@triggerlists)
  {
    my $channel = $ns->create(
      DB::BehavioralChannel->blank(
        name => $index,
        url_list => $trigger,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'U') ]));

    $index++;
    $ns->output("Channel" . $index, $channel->url_key());
  }

  $ns->output("REF1", "http://www.internet.cnews.fr/internet-reclama/index.shtml?1");
  $ns->output("REF2", "http://www.somesite.com/index.shtml?1=http://www.mapinter.net");
  $ns->output("REF3", "http://www.internet.cnews.fr/internet-reclama-123");
  $ns->output("REF4", "http://internet.cnews.fr/internet");
  $ns->output("REF5", "http://internet.cnews.fr?internet-reclama");
  $ns->output("REF6", "http://internet.cnews.fr/internet-reclama?index.shtml");
  $ns->output("REF7", "http://internet.cnews.fr/internet-reclama/index.shtmlAAA");
}

1;
