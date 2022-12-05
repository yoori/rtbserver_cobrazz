package WalledGardenTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_campaign 
{
  my ($self, $ns, $opt, $params, $site_links) = @_;

  my $marketplace = exists $params->{marketplace}?
      $params->{marketplace}: $opt;

  my $keyword = make_autotest_name($ns, "kwd-$opt");
    $ns->output("KW-$opt", $keyword);

  my $campaign = $ns->create(DisplayCampaign => {
      name => $opt,
      account_id => $params->{agency_account},
      behavioralchannel_keyword_list => $keyword,
      cpm => $params->{cpm},
      cpc => $params->{cpc},
      campaigncreativegroup_ctr =>
          defined $params->{cpc}? 0.1: undef,
      creative_crclick_value => "www.dread.com",
      marketplace => $marketplace,
      site_links => $site_links });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  $ns->output("CC-$opt", $campaign->{cc_id});
  
  if (defined $params->{cpm})
  {
    $ns->output("CC-CPM-$opt", $params->{cpm});
  }
  if (defined $params->{cpc})
  {
    $ns->output("CC-CPC-$opt", $params->{cpc});
  }
}

sub create_publisher
{
  my ($self, $ns, $opt, $params, $site_links) = @_;
  
  my $marketplace = exists $params->{marketplace}?
    $params->{marketplace}: $opt;

  my $cpm = defined $params->{cpm} ?
     $params->{cpm} / 2: 
        defined $params->{cpc}? DB::Defaults::instance()->cpm / 2: 0;

  my $publisher = $ns->create(Publisher => {
    name => $opt,
    account_id => $params->{pub_account},
    pricedtag_cpm => $cpm,
    pricedtag_marketplace => $marketplace });
    
  push(@$site_links, {site_id => $publisher->{site_id}});

  $ns->output("TAG-$opt", $publisher->{tag_id});

  $ns->output("TAG-CPM-$opt",  $cpm);

  return $publisher;
}
  
sub init
{
  my ($self, $ns) = @_;

  my $agency_acc = $ns->create(Account => {
    name => 'agency',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $agency_acc_2 = $ns->create(Account => {
    name => 'agency2',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $agency_acc_non_wg = $ns->create(Account => {
      name => 'agency non wg',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $pub_acc = 
    $ns->create(PubAccount => { 
      name => "publisher" });

  my $pub_acc_2 = 
    $ns->create(PubAccount => { 
      name => "publisher2" });

  my $pub_acc_non_wg = 
    $ns->create(PubAccount => { 
      name => "publisher non wg" });

  my $cpc = 100;

  my %wg_options = (
    'WG' => { 
      cpc => $cpc,
      agency_account => $agency_acc,
      pub_account => $pub_acc
    }, 
    'OIX' => { 
      cpm => DB::Defaults::instance()->cpm + 1,
      agency_account => $agency_acc,
      pub_account => $pub_acc
    }, 
    'ALL' => { 
      cpm => DB::Defaults::instance()->cpm - 1,
      agency_account => $agency_acc,
      pub_account => $pub_acc
    },  
    'NON-WG' => { 
      cpm => DB::Defaults::instance()->cpm + 2,
      agency_account => $agency_acc_non_wg,
      pub_account => $pub_acc_non_wg,
      marketplace => undef
    });

  my @site_links = ();

  while (my ($opt, $params) = each %wg_options)
  {
    $self->create_publisher($ns, $opt, $params, \@site_links);
  }

  my $pub_wg_2 = $self->create_publisher(
    $ns, 'WG-2',
    { pub_account => $pub_acc_2 ,
      marketplace => 'WG',
      cpm => undef}, \@site_links);

  my $tag_all_2 = $ns->create(PricedTag => {
    name => 'TAG-ALL-2',
    site_id => $pub_wg_2->{site_id},
    marketplace => 'ALL'});

  $ns->output("TAG-ALL-2", $tag_all_2);

  my $tag_foros_2 = $ns->create(PricedTag => {
    name => 'TAG-OIX-2',
    site_id => $pub_wg_2->{site_id},
    marketplace => 'OIX'});
  
  $ns->output("TAG-OIX-2", $tag_foros_2);

    
  while (my ($opt, $params) = each %wg_options)
  {
    $self->create_campaign($ns, $opt, $params, \@site_links);
  }

  $self->create_campaign(
    $ns, 
    "NON-WG-2",
    { cpm => DB::Defaults::instance()->cpm - 2,
      agency_account => $agency_acc_non_wg,
      marketplace => undef } , \@site_links);


  $self->create_campaign(
    $ns, 
    "WG-2",
    { cpm => DB::Defaults::instance()->cpm / 4.0,
      agency_account => $agency_acc,
      marketplace => 'WG' } , \@site_links);

  $self->create_campaign(
    $ns, 
    "ALL-2",
    { cpc => $cpc + 10,
      agency_account => $agency_acc,
      marketplace => 'ALL' } , \@site_links);

  $ns->create(WalledGarden => {
    pub_account_id => $pub_acc->{account_id},
    agency_account_id => $agency_acc->{account_id},
    pub_marketplace => 'WG',
    agency_marketplace => 'WG'});

  $ns->create(WalledGarden => {
    pub_account_id => $pub_acc_2->{account_id},
    agency_account_id => $agency_acc_2->{account_id},
    pub_marketplace => 'WG',
    agency_marketplace => 'WG'});
}

1;
