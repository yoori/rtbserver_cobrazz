
package InactiveCCGTest;

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

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'IC',
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => 1, 
    campaigncreativegroup_status => 'I', 
    campaigncreativegroup_display_status_id => DISPLAY_STATUS_INACTIVE, 
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 'IC', account_id => $publisher->{account_id}}] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id =>  $campaign->{channel_id} ));

  my $tag = $ns->create(PricedTag => {
    name => 'IC',
    site_id => $campaign->{Site}[0]->{site_id} });

  $ns->output("IC_CCGID", $campaign->{ccg_id});
  $ns->output("IC_TID", $tag);
}

1;
