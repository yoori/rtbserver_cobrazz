
package OpenRTBTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub simple_case
{
  my ($self, $namespace) = @_;  

  my $ns = $namespace->sub_namespace('SIMPLE');

  my $cpm = 100;

  my $keyword = make_autotest_name($ns, "KWD");

  my $advertiser = 
    $ns->create(Advertiser => { 
      name => "Advertiser"});

  my $channel = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 ) ]));

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    channel_id => $channel,
    size_id => $self->{size},
    campaigncreativegroup_country_code => 'RU',
    campaigncreativegroup_cpm => $cpm,
    site_links => [
      { site_id => $self->{publisher}->{site_id} }] });
 
  $ns->output("KWD", $keyword); 
  $ns->output("CCG", $campaign->{ccg_id});
  $ns->output("CAMPAIGN", $campaign->{campaign_id});
  $ns->output("CC", $campaign->{cc_id});
  $ns->output("CREATIVE", $campaign->{creative_id});
  $ns->output("CPM", $cpm);
}

sub auctions_lost
{
  my ($self, $namespace) = @_;  

  my $ns = $namespace->sub_namespace('AUCTIONSLOST');

  my $advertiser = 
    $ns->create(Advertiser => { 
      name => "Advertiser",
      currency_id => DB::Defaults::instance()->openrtb_account()->currency_id });

  my $keyword = make_autotest_name($ns, "KWD");

  my $channel = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 ) ]));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 'Campaign1',
    account_id => $advertiser,
    channel_id => $channel,
    size_id => $self->{size},
    campaigncreativegroup_country_code => 'RU',
    campaigncreativegroup_cpm => 100,
    site_links => [
      { site_id => $self->{publisher}->{site_id} }] });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'Campaign2',
    account_id => $advertiser,
    channel_id => $channel,
    size_id => $self->{size},
    campaigncreativegroup_country_code => 'RU',
    campaigncreativegroup_cpm => 110.5,
    site_links => [
      { site_id => $self->{publisher}->{site_id} }] });

  $ns->output("KWD", $keyword); 
  $ns->output("CCG1", $campaign1->{ccg_id});
  $ns->output("CC1", $campaign1->{cc_id});
  $ns->output("CCG2", $campaign2->{ccg_id});
  $ns->output("CC2", $campaign2->{cc_id});
}

sub init {
  my ($self, $ns) = @_;

  $self->{size} = DB::Defaults::instance()->size;

  $self->{publisher} = 
    $ns->create(Publisher => { 
      name => "Publisher",
      account_id => DB::Defaults::instance()->openrtb_account,
      pricedtag_size_id => $self->{size} });


  $self->simple_case($ns);
  $self->auctions_lost($ns);

  $ns->output("OPENRTB/ACCOUNT", DB::Defaults::instance()->openrtb_account);
  $ns->output("SIZE", $self->{size}->{protocol_name});
  $ns->output("CURRENCY", DB::Defaults::instance()->openrtb_currency()->currency_code);
  $ns->output("RATE", DB::Defaults::instance()->openrtb_currency()->rate);
  
}

1;
