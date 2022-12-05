package RefererUrlParsingTest;

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

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'C1',
    account_id => $acc,
    url_list =>
      "http://auction1.taobao.com/auction/today_top5.htm\nhttp://www.ukaka.com/website/y/165.php",
    behavioral_parameters => [ DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "U") ]));

  $ns->output("RefererUrlParsingTest/C/01", $ch->url_key());
}

1;
