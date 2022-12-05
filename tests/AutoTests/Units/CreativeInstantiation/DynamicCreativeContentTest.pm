package DynamicCreativeContentTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;
  my $file_path = $ns->options()->{'local_data_path'};
  my $file_name = make_autotest_name($ns, "DynamicCreativeContent");
  my $file = "/UnitTests/$file_name";

  my $template_file_name = $file;
  my $template_file_name1 =  $file_path . "DynamicCreativeContentTemplateFile1";
  my $template_file_name2 =  $file_path . "DynamicCreativeContentTemplateFile2";
  my $template_file_name3 =  $file_path . "DynamicCreativeContentTemplateFile3";
  $ns->output("TemplateFile", $template_file_name);
  $ns->output("DynamicCreativeContentTemplateFile1", $template_file_name1);
  $ns->output("DynamicCreativeContentTemplateFile2", $template_file_name2);
  $ns->output("DynamicCreativeContentTemplateFile3", $template_file_name3);

  my $my_kww = make_autotest_name($ns, "/key");
  $ns->output("Keyword", $my_kww);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher" });

  $ns->output("Tag", $publisher->{tag_id});

  my $template_id = $ns->create(Template => {
    name => 'Template',
    template_file => $template_file_name,
    template_file_type => 'T' });

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'DisplayCampaign',
    account_id => $advertiser,
    site_links => [{site_id => $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      account_id => $advertiser,
      name => 'Channel',
      keyword_list => $my_kww,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    creative_template_id => $template_id });

  $ns->output("CC", $campaign->{cc_id});

}

1;
