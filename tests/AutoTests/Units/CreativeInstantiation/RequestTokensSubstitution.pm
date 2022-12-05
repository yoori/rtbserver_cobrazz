

package RequestTokensSubstitution;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_campaign
{
  my ($self, $ns, $account, $name, $template_option_value, $size_option_value) = @_;

  my $kwd = make_autotest_name($ns, $name);

  my $publisher = $ns->create(Publisher => { 
    name => "Publisher-$name",
    size_id => $self->{size} });

  my $campaign = $ns->create(DisplayCampaign => {
    name => "Campaign-$name",
    account_id => $account,
    creative_template_id => $self->{template},
    size_id => $self->{size},
    behavioralchannel_keyword_list => $kwd,
    campaigncreativegroup_cpm => 300,
    site_links => 
      [{ site_id => $publisher->{site_id} }] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  $ns->create(CreativeOptionValue => {
    creative_id => $campaign->{creative_id},
    option_id => $self->{template_option},
    value => $template_option_value }) if $template_option_value;

  $ns->create(CreativeOptionValue => {
    creative_id => $campaign->{creative_id},
    option_id => $self->{size_option}, 
    value => $size_option_value }) if $size_option_value;

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

  $self->{size} = $ns->create(CreativeSize => {
    name => 'Size'});

  $self->{template} = $ns->create(Template => {
    name => 'Template',
    template_file => 'UnitTests/RequestTokensSubstitution',
    flags => 0,
    size_id => $self->{size} });

  $self->{template_option} = $ns->create(Options => {
    name => 'Dynamic file option for template',
    token => 'DCRADVTMP_FILE',
    template_id => $self->{template},
    type => 'Dynamic File',
    option_group_id =>
      $ns->create(DB::OptionGroup->blank(
        name => "Template-" . $self->{template}->{template_id} . "-h",
        template_id => $self->{template},
        type => 'Hidden' )) });

  $self->{size_option} = $ns->create(Options => {
    name => 'Dynamic file option for size',
    token => 'DCRADVSZ_FILE',
    size_id => $self->{size},
    type => 'Dynamic File',
    option_group_id =>
      $ns->create(DB::OptionGroup->blank(
        name => "Size-" . $self->{size}->{size_id} . "-h",
        size_id => $self->{size}, 
        type => 'Hidden' )) });

  $self->create_campaign(
    $ns, $single_advertiser, 'SingleAdvertisier-EmptyTokens');

  $self->create_campaign(
    $ns, $agency_advertiser, 'AgencyAccount-EmptyTokens');

  my $dfile1 = '/DynamicCreative/simple.html';
  my $dfile2 = '/DynamicCreative/random.html';

  $self->create_campaign(
    $ns, $single_advertiser, 'SingleAdvertisier-DefaultTokens', 
    $dfile1, $dfile2);

  $self->create_campaign(
    $ns, $agency_advertiser, 'AgencyAccount-DefaultTokens', 
    $dfile1, $dfile2);

  $ns->output("Agency", $agency);
  $ns->output("AgencyAdvertiser", $agency_advertiser);
  $ns->output("SingleAdvertiser", $single_advertiser);
  $ns->output("DynamicFile1", $dfile1);
  $ns->output("DynamicFile2", $dfile2);
}

1;
