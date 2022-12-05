package CampaignCreativeUpdate;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub add_creative
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("ADDCREATIVE");
 
  my $campaign = $ns->create(DisplayCampaign => { 
    name => 'Display' });

  my $template = $ns->create(Template => {
    name => 'Template',
    template_file => 'UnitTests/banner_img_clk.html',
    flags => 0 });

  $ns->output('Name', make_autotest_name($ns, ''));
  $ns->output('TemplateName', $template->{name});
  $ns->output('Template', $template);
  $ns->output('Account', $campaign->{account_id});
  $ns->output('CCG', $campaign->{ccg_id});
  $ns->output('Size', DB::Defaults::instance()->size_300x250());
  $ns->output('SizeName', DB::Defaults::instance()->size_300x250()->{name});
}

sub remove_creative
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("REMOVECREATIVE");

  my $campaign = $ns->create(DisplayCampaign => { 
    name => 'Display' });

  $ns->output("CC", $campaign->{cc_id});
}

sub update_creative
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("UPDATECREATIVE");

  my $template = $ns->create(Template => {
    name => 'Template',
    template_file => 'UnitTests/banner_img_clk.html',
    flags => 0 });

  my $campaign = $ns->create(DisplayCampaign => { 
    name => 'Display',
    size_id => DB::Defaults::instance()->size(),
    template_id => $template });

  $ns->output("CC", $campaign->{cc_id});
  $ns->output("Creative", $campaign->{creative_id});
  $ns->output("Size", DB::Defaults::instance()->size());
  $ns->output("SizeName", DB::Defaults::instance()->size()->{name});
  $ns->output("NewSize", DB::Defaults::instance()->size_300x250());
  $ns->output("NewSizeName", DB::Defaults::instance()->size_300x250()->{name});
  $ns->output("Template", $template);
  $ns->output("TemplateName", $template->{name});
  $ns->output("NewTemplateName", make_autotest_name($ns, 'NewName'));
}

sub update_option_value
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("UPDATEOPTION");

  my $acc_type_id = $ns->create(AccountType => { 
    name => "AccType",
    adv_exclusions => 'S',
    account_role_id => DB::Defaults::instance()->publisher_role });

  my $publisher = $ns->create(Publisher => {
    name => 'Publisher',
    account_type_id => $acc_type_id,
    pricedtag_cpm => 1 });

  my $keyword = make_autotest_name($ns, "ADSC-5428");

  my $campaign = $ns->create(TextAdvertisingCampaign => { 
    name => 'TA-UpdateOptionValue',
    size_id => DB::Defaults::instance()->size(),
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword,
    campaign_flags => 0,
    campaigncreativegroup_flags => 0,
    max_cpc_bid => 100 });

  my $option = $ns->create(Options => { 
    token => "HEADLINE",
    option_group_id => DB::Defaults::instance()->text_option_group,
    template_id => DB::Defaults::instance()->text_template  });

  my $v_category = $ns->create(CreativeCategory => { 
    name => "V",
    cct_id => 0 });

  my $c_category = $ns->create(CreativeCategory => { 
    name => "C",
    cct_id => 1 });

  $ns->create(CreativeCategory_Creative => { 
    creative_category_id => $v_category,
    creative_id => $campaign->{creative_id} });

  $ns->create(CreativeCategory_Creative => { 
    creative_category_id => $c_category,
    creative_id => $campaign->{creative_id} });

  my $exclusion_tag = make_autotest_name($ns, "tag");

  my $category = $ns->create(CreativeCategory => { 
    name => $exclusion_tag,
    cct_id => 2 }); # tag type

  $ns->create(SiteCreativeCategoryExclusion => { 
    site_id => $publisher->{site_id},
    creative_category_id => $category });

  $ns->create(CreativeOptionValue => { 
    creative_id => $campaign->{creative_id},
    option_id => $option,
    value => 'CampaignCreativeUpdate' });

  $ns->output("Creative", $campaign->{creative_id});
  $ns->output("CC", $campaign->{cc_id});
  $ns->output("Keyword", $keyword);
  $ns->output("Tag", $publisher->{tag_id});
  $ns->output("ExclusionTag", $exclusion_tag);
  $ns->output('Option', $option);
  $ns->output('Category', $category);
}

sub init
{
  my ($self, $ns) = @_;
  
  $self->add_creative($ns);
  $self->remove_creative($ns);
  $self->update_creative($ns);
  $self->update_option_value($ns);
  
}

1;
