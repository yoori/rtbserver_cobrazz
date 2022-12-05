
package UnexistingTokenInCreativeOption;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $publisher = $ns->create(Publisher => { name => "Publisher" });

  my $template = $ns->create(Template => {
    name => 'Template',
    template_file => 'UnitTests/test.html',
    flags => 0 });
  
  my $keyword = make_autotest_name($ns, "keyword");

  my $campaign1  = $ns->create(DisplayCampaign => {
    name => 'Campaign1',
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => 50,
    creative_template_id => $template,
    creative_imagetitle_value => '##FUDJIN##',
    campaigncreative_weight => 1,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign1->{channel_id},
      trigger_type => "P" ));

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'Campaign2',
    channel_id => $campaign1->{channel_id},
    campaigncreativegroup_cpm => 10,
    creative_template_id => $template,
    creative_imagetitle_value => undef,
    campaigncreative_weight => 1,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  my $option = $ns->create(Options => { 
     token => 'IMAGETITLE',
     option_group_id => 
         $template->{option_group_id},
     template_id => $template });

  $ns->output("KEYWORD", $keyword);
  $ns->output("CC1", $campaign1->{cc_id});
  $ns->output("CREATIVE1", $campaign1->{creative_id});
  $ns->output("CC2", $campaign2->{cc_id});
  $ns->output("CREATIVE2", $campaign2->{creative_id});
  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("OPTION", $option);
}

1;
