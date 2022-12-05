
package ApplyDeliveryThresholdTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $cpm = 100;
  my $daily_budget = 12;

  # CCG with fixed daily budget limit
  my $keyword = make_autotest_name($ns, "Keyword");
  my $publisher = $ns->create(Publisher => { name => 'PUB'});

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'ADT',
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => $cpm, # price for 1000 impressions
    campaign_daily_budget => $daily_budget,
    campaign_delivery_pacing => 'F',
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $campaign->{channel_id} ));

  $ns->output("KEYWORD", $keyword);
  $ns->output("ATR_CCID", $campaign->{cc_id});
  $ns->output("ATR_CCGID", $campaign->{ccg_id});
  $ns->output("ATR_TID", $publisher->{tag_id});
  $ns->output("ATR_BUDGET", $daily_budget);
}

1;
