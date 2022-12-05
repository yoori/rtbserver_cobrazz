
package ProfilesContentCleanupTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $account =  $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type });

  my $keyword = make_autotest_name($ns, "kwd");

  my $channel = $ns->create(
      DB::BehavioralChannel->blank(
        name => 1,
        channel_type => 'K',
        keyword_list => $keyword,
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            time_to => 180 * 24 * 60 * 60),
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'S',
            time_to => 180 * 24 * 60 * 60) ]));

  $ns->output("CHANNEL", $channel->channel_id(), "keyword channel");

  my $campaign = $ns->create(TextAdvertisingCampaign => {
    name => 1,
    size_id => DB::Defaults::instance()->size(),
    template_id => DB::Defaults::instance()->text_template,
    account_id => $account,
    original_keyword => $keyword,
    ccgkeyword_channel_id => $channel->channel_id(),
    max_cpc_bid => 1,
    site_links => [{ name => 1 }] });

  my $tag = $ns->create(PricedTag => {
    name => "Tag",
    site_id => $campaign->{Site}[0]->{site_id}});

  $ns->output("KEYWORD", $keyword, "keyword for adv request");
  $ns->output("CCID", $campaign->{cc_id}, "ccid");
  $ns->output("CCGID", $campaign->{ccg_id}, "ccgid");
  $ns->output("TAG", $tag, "tag id");
}

1;
