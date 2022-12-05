
package DisputingInvoice;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $publisher = $ns->create(Publisher => {
    name => 'Publisher' });

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type,
    prepaid_amount => 3});

  my $keyword = make_autotest_name($ns, "Keyword");

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    account_id => $advertiser,
    channel_id => 
      DB::BehavioralChannel->blank(
        account_id => $advertiser,
        name => 'Channel',
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]),
    campaigncreativegroup_cpm => 1000,
    campaigncreativegroup_budget => 100000,
    site_links => [{site_id => $publisher->{site_id}}] });

  $ns->output("CC", $campaign->{cc_id});
  $ns->output("CCG", $campaign->{ccg_id});
  $ns->output("ACCOUNT", $campaign->{account_id});
  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("KWD", $keyword);
}

1;
