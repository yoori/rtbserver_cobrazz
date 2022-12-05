package XsltCreativeTemplateValidation;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub load_XsltTemplateValidation_data
{
  my ($ns, $file, $sfx) = @_;
  # create creative with templates, template files

  my $template_file_name = $file;
  $ns->output("TemplateFile/$sfx", $template_file_name, "Template file $sfx");

  my $template_id = $ns->create(Template => {
    name => $sfx,
    template_file => $template_file_name,
    template_file_type => 'X' });

  my $my_kww = make_autotest_name($ns, "/key/$sfx");

  my $campaign = $ns->create(DisplayCampaign =>
                             { name => $sfx,
                               behavioralchannel_keyword_list => $my_kww,
                               creative_template_id => $template_id,
                               site_links => [{name => $sfx}] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  my $creative_category  = $ns->create(CreativeCategory =>
                                       { name => $sfx });

  $ns->create(CreativeCategory_Creative =>
              { creative_category_id => $creative_category,
                creative_id => $campaign->{creative_id} });

  $ns->output("Keyword/$sfx", $my_kww, "keyword for request");

  $ns->output("Cid/$sfx", $campaign->{cc_id}, "");

  my $tag_id     = $ns->create(PricedTag =>
                               { name => $sfx,
                                 site_id => $campaign->{Site}[0]->{site_id} });

  $ns->output("Tag/$sfx", $tag_id, "");

}

sub init
{
  my ($self, $ns) = @_;

  # create account, advertiser, campaign ..... creative template file

  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlValid_empty.xsl", "01");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlInvalid_absent.xsl", "02");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlInvalid_foros_call.xsl", "03");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlValid_foros_call.xsl", "04");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlInvalid_xml.xsl", "05");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlInvalid_namespace.xsl", "06");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlInvalid_unknown_func.xsl", "07");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlValid_include.xsl", "08");
  load_XsltTemplateValidation_data($ns,"/UnitTests/XmlInvalid_include.xsl", "09");
}

1;
