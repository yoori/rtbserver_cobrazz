
package AccountBudgetTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "Keyword");
  $ns->output("KEYWORD", $keyword);
  my $publisher = $ns->create(PubAccount => { name => 'Pub'});
    
  my $cpm = 501;
  my $account_credit = 1;
  my $zero_balance_campaign = $ns->create(DisplayCampaign => {
    name => 'ZeroBalance',
    advertiser_prepaid_amount => $account_credit,
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => $cpm, # price for 1000 impressions
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 'ZeroBalance', account_id => $publisher->{account_id}}] });
    
  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $zero_balance_campaign->{channel_id} ));
    
  my $zero_balance_tag = $ns->create(PricedTag => {
    name => 'ZeroBalance',
    site_id => $zero_balance_campaign->{Site}[0]->{site_id} 
    });
    
  $ns->output("ZEROBALANCE_CCID", $zero_balance_campaign->{cc_id});
  $ns->output("ZEROBALANCE_CCGID", $zero_balance_campaign->{ccg_id});
  $ns->output("ZEROBALANCE_TID", $zero_balance_tag);
  $ns->output("ZEROBALANCE_CPM", $cpm);
  $ns->output("ZEROBALANCE_ACCOUNT_CREDIT",  $account_credit);
}

1;
