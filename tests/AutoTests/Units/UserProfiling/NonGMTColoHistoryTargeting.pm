
package NonGMTColoHistoryTargeting;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub make_channel
{
  my ($self, $ns, $prefix, $account, $params) = @_;

  my $url = "http://" . make_autotest_name($ns, $prefix) . ".com";
  
  my $channel = $ns->create(DB::BehavioralChannel->blank(
      account_id => $account,
      name => $prefix,
      url_list => $url,
      behavioral_parameters => 
        [ DB::BehavioralChannel::BehavioralParameter->blank(%$params) ] ));

  $ns->output($prefix . "Ref", $url);
  $ns->output($prefix . "Channel", $channel->{channel_id});
  $ns->output($prefix . "BP", $channel->url_key());
}

sub init {
  my ($self, $ns) = @_;

  $ns->output("TZColo", DB::Defaults::instance()->remote_isp->{colo_id});

  $ns->output("TZName", 
    DB::Defaults::instance()->remote_isp->{Account}->{timezone_id}->{tzname});

  my $account = $ns->create(Account =>
    { name => 1,
      role_id => DB::Defaults::instance()->advertiser_role });

  $self->make_channel(
    $ns,
    "H1",
    $account,
    { trigger_type => "U",
      minimum_visits => 2,
      time_from => 86400,
      time_to => 172800 });

  $self->make_channel(
    $ns,
    "H2",
    $account,
    { trigger_type => "U",
      minimum_visits => 2,
      time_from => 86400,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "H3",
    $account,
    { trigger_type => "U",
      minimum_visits => 3,
      time_from => 86400,
      time_to => 172800 });
}

1;
