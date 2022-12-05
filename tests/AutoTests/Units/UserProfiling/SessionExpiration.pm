
package SessionExpiration;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $url = "http://" . make_autotest_name($ns, 1) . ".com";

  my $account = $ns->create(Account =>
    { name => 1,
      role_id => DB::Defaults::instance()->advertiser_role });
  
  my $channel = $ns->create(DB::BehavioralChannel->blank(
      account_id => $account,
      name => 1,
      url_list => $url,
      behavioral_parameters => 
        [ DB::BehavioralChannel::BehavioralParameter->blank(
           trigger_type => "U",
           minimum_visits => 10,
           time_to => 10*60) ] ));

  $ns->output("Ref", $url);
  $ns->output("Channel", $channel->{channel_id});
  $ns->output("BP", $channel->url_key());
}

1;
