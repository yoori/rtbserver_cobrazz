
package BaiduTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub base
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('BASE');

  my $publisher = 
    $ns->create(Publisher => { 
      name => "Publisher",
      account_id => DB::Defaults::instance()->baidu_account,
      pricedtag_size_id => DB::Defaults::instance()->size });

  my $url = make_autotest_name($ns, "url") . ".baidu.com";

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    channel_id=>
      DB::BehavioralChannel->blank(
        name => 'Channel',
        url_list => $url,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U') ]),,
    size_id => DB::Defaults::instance()->size,
    campaigncreativegroup_country_code => 'GB',
    campaigncreativegroup_cpm => 100,
    site_links => [
      { site_id => $publisher->{site_id} }] });

  $ns->output("ACCOUNT", DB::Defaults::instance()->baidu_account);
  $ns->output("CC", $campaign->{cc_id});
  $ns->output("CCG", $campaign->{ccg_id});
  $ns->output("CREATIVE", get_baidu_creative($campaign->{Creative}));
  $ns->output("ADV", $campaign->{account_id});
  $ns->output("URL", $url);
}

sub init {
  my ($self, $ns) = @_;

  $self->base($ns);

  $ns->output("SIZE", DB::Defaults::instance()->size->{protocol_name});  
}

1;
