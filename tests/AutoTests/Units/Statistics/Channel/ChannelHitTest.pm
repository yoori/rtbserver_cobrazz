package ChannelHitTest;

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

  my $kw1 = make_autotest_name($ns, "test1");
  my $kw2 = make_autotest_name($ns, "test2");
  my $kw3 = make_autotest_name($ns, "test3");
  my $url1 = make_autotest_name($ns, "testurl1");
  my $only_page = make_autotest_name($ns, "page");
  my $only_search = make_autotest_name($ns, "search");

  my $keywords = join("\n", $kw1, $kw2, $kw3);
  my $urls = $url1;

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'B',
    account_id => $acc,
    keyword_list => "$keywords\n$only_page",
    search_list => "$keywords\n$only_search",
    url_list => $urls,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U')
      ]
    ));


  $ns->output("KW1", $kw1);
  $ns->output("KW2", $kw2);
  $ns->output("KW3", $kw3);
  $ns->output("ONLYPAGE", $only_page);
  $ns->output("ONLYSEARCH", $only_search);
  $ns->output("URL1", $url1);

  $ns->output("CH1", $channel);
  $ns->output("Tag", DB::Defaults::instance()->tag());
  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("NonDefaultColo", DB::Defaults::instance()->ads_isp->{colo_id});
}

1;
