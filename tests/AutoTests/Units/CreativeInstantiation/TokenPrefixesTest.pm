package TokenPrefixesTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $test_name = "TokenPrefixesTest";

  my $template_file_name_simple = "/UnitTests/TokenPrefixesSimpleTest"; # simple template

  my $template_id_simple = $ns->create(Template => {
    name => 1,
    template_file => $template_file_name_simple });

  my $creative_category = $ns->create(CreativeCategory => {
    name => 1 });

  my $my_kww1 = make_autotest_name($ns, "key_phrase_1");

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    template_id => $template_id_simple,
    behavioralchannel_keyword_list => $my_kww1,
    site_links => [{name => 1}] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  $ns->create(CreativeCategory_Creative => {
    creative_category_id => $creative_category,
    creative_id => $campaign->{creative_id} });

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign->{Site}[0]->{site_id} });

  # register options
  my $option_value = "\nпри.:/<>01abc &%?";

  my $option = $ns->create(Options =>
    { token => uc($test_name) . "_OPTION_1",
      option_group_id =>  $template_id_simple->{option_group_id},
      template_id => $template_id_simple });
      
  $ns->create(CreativeOptionValue => {
    option_id => $option,
    creative_id => $campaign->{creative_id},
    value => $option_value}) if $option_value;

  # expected body
  my $orig = $option_value;
  my $js = "\\x0aпри.:\\x2f\\x3c\\x3e01abc &%?";
  my $js_ucode = "\\u000a\\u043f\\u0440\\u0438\\u002e\\u003a\\u002f\\u003c\\u003e01abc\\u0020\\u0026\\u0025\\u003f";
  my $mime_url = "%0A%D0%BF%D1%80%D0%B8.%3A%2F%3C%3E01abc+%26%25%3F";
  my $xml = "&#xa;&#x43f;&#x440;&#x438;.:/&lt;&gt;01abc &amp;%?";
  my $expected_body1 = $orig . "\n" . 
    $js . "\n" . 
    $js_ucode . "\n" . 
    $mime_url . "\n" . 
    $xml . "\n";
  Encode::_utf8_on($expected_body1);

  $ns->output("$test_name/TemplateFile/Simple", $template_file_name_simple, "Template file");
  $ns->output("Keyword/01", $my_kww1, "keyword for request");
  $ns->output("Tag/01", $tag_id, "");
  $ns->output("Expected Body/01", $expected_body1, "Text expected in body");
}

1;
