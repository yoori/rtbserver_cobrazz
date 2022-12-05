
package ChannelInventoryEstimCommon;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_channel
{
  my ($self, $ns, $prefix, $account, $params) = @_;

  my $url = "http://" . make_autotest_name($ns, "url$prefix") . ".com";
  my $keyword = make_autotest_name($ns, "keyword$prefix");

  $ns->output("URL$prefix", $url);
  $ns->output("KW$prefix", $keyword);

  my $adv_channel = $ns->create(DB::BehavioralChannel->blank(
      name => "Adv-" . $prefix,
      account_id => $account,
      keyword_list => $keyword,
      url_list => $url, 
      behavioral_parameters => $params));

  $ns->output("AdvChannel$prefix", $adv_channel);
  $ns->output("AdvBPU$prefix", $adv_channel->channel_id . 'U');
  $ns->output("AdvBPP$prefix", $adv_channel->channel_id . 'P');
  $ns->output("AdvBPS$prefix", $adv_channel->channel_id . 'S');
}

1;
