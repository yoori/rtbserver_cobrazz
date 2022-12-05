
package TanxTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub base {
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('BASE');

  my $size = $ns->create(CreativeSize => {
    name => 'Size',
    width => 99,
    height => 69 });

  my $publisher = 
    $ns->create(Publisher => { 
      name => "Publisher",
      account_id => DB::Defaults::instance()->tanx_account,
      pricedtag_size_id => $size });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    channel_id => undef,
    size_id => $size,
    campaigncreativegroup_country_code => 'GB',
    campaigncreativegroup_cpm => 100,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON,
    site_links => [
      { site_id => $publisher->{site_id} }] });

  $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->display_template(),
      size_id => $size,
      app_format_id => DB::Defaults::instance()->html_format });

  $ns->output("CCG", $campaign->{ccg_id});
  $ns->output("CC", $campaign->{cc_id});
  $ns->output("CREATIVE",
     get_tanx_creative($campaign->{Creative}));
  $ns->output("SIZE", $size->{protocol_name});
}

sub init {
  my ($self, $ns) = @_;

  $ns->output("TANX/ACCOUNT", DB::Defaults::instance()->tanx_account);

  $self->base($ns);
}


1;
