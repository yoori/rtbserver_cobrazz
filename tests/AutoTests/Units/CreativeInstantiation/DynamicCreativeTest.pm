
package DynamicCreativeTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_campaign
{
  my ($self, $ns, $name, $creative_path, $account) = @_;

  my $kwd = make_autotest_name($ns, $name);

  my $size = $ns->create(CreativeSize => {
      name => 'Size-'  . $name 
     });

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'Publisher-' . $name,
       size_id => $size });

  my $template = $ns->create(Template => {
    name => 'Template-' . $name,
    template_file => 'UnitTests/DynamicCreativeTest.html',
    size_id => $size,
    flags => 0 });

  my $creative = $ns->create(Creative => {
    name => 'Creative-' . $name,
    account_id => $account,
    size_id => $size,
    imagetitle_value => 'Image',
    crhtml_value => $creative_path,
    template_id => $template });
    
  my $campaign = $ns->create(DisplayCampaign => {
    name => "Campaign-" . $name,
    account_id => $account,
    behavioralchannel_keyword_list => $kwd,
    campaigncreativegroup_cpm => 300,
    creative_id => $creative,
    site_links => 
      [{ site_id => $publisher->{site_id} }] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  $ns->output($name . "/CC", $campaign->{cc_id});
  $ns->output($name . "/TAG", $publisher->{tag_id});
  $ns->output($name . "/KWD", $kwd);
}

sub init {
  my ($self, $ns) = @_;

  my $agency = $ns->create(Account =>
    { name => 'Agency',
      text_adserving => 'M',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $single_advertiser = 
     $ns->create(Account => { 
       name => 'Advertiser-single',
       role_id => DB::Defaults::instance()->advertiser_role });

  my $agency_advertiser = 
     $ns->create(Account => { 
       name => 'Advertiser-Agency',
       agency_account_id => $agency,
       text_adserving => undef,
       role_id => DB::Defaults::instance()->advertiser_role });

  $self->create_campaign(
    $ns, 'Unexisting', 
    '/DynamicCreative/unexistingtoken.html',
    $single_advertiser);

  $self->create_campaign(
    $ns, 'Incomplete', 
    '/DynamicCreative/incompletetoken.html',
    $single_advertiser);

  $self->create_campaign(
    $ns, 'NoTokens', 
    '/DynamicCreative/random.html', 
     $single_advertiser);

  $self->create_campaign(
    $ns, 'SingleAdvertisier', 
    '/DynamicCreative/simple.html', 
     $single_advertiser);

  $self->create_campaign(
    $ns, 'AgencyAccount', 
    '/DynamicCreative/simple.html', 
    $agency_advertiser);

  $ns->output("Agency", $agency);
  $ns->output("AgencyAdvertiser", $agency_advertiser);
  $ns->output("SingleAdvertiser", $single_advertiser);
}

1;
