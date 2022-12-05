#v2

package SubAgentTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub init {
  my ($self, $ns) = @_;

  my $size = $ns->create(CreativeSize =>
    { name => "size",
      max_text_creatives => 1 });
  
  my $publisher = $ns->create(Publisher => {
    name => "pub",
    pricedtag_cpm => 1,
    size_id => $size });
  my $site = $publisher->{site_id};

  my $adv_acc = $ns->create(Account => {
    name => "adv",
    role_id => DB::Defaults::instance()->advertiser_role });

  my $app_format = $ns->create(AppFormat => { 
    name => 'html', 
    mime_type => 'text/html' });

  my $template = $ns->create(Template =>
    { name => "template",
      size_id => $size,
      template_file => 'UnitTests/monitoring.html',
      app_format_id => $app_format,
      flags => 0,
      template_file_type => 'T'});

  my $keyword = "Channel Server Monitoring";
  
  my $urls = '';
  for (my $i = 0; $i < 20; ++$i)
  {
      $urls = $urls . "channel.server$i.com/check/me/please\n";
  }
  $urls = $urls . "channel.server20.com/check/me/please";

  my $channelp = $ns->create(DB::BehavioralChannel->blank(
    name => 'channelp',
    account_id => $adv_acc,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P" )] ));

  my $channelu = $ns->create(DB::BehavioralChannel->blank(
    name => 'channelu',
    account_id => $adv_acc,
    url_list => $urls,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "U" )] ));

  my $channele = $ns->create(DB::ExpressionChannel->blank(
    name => 'channele',
    account_id => $adv_acc,
    expression => $channelp->channel_id . "&" . $channelu->channel_id));

  
  my $dc = $ns->create(DisplayCampaign => {
    name => "campaign",
    account_id => $adv_acc,
    template_id => $template,
    size_id => $size,
    creative_adhtml_value => '<p>Channel Server is running</p>',
    campaigncreativegroup_cpm => 30,
    channel_id => $channele->channel_id,
    site_links => [{ site_id => $site }]});

  $ns->output("CC", $dc->{cc_id}, "ccid");
  $ns->output("CCG", $dc->{ccg_id}, "ccgid");
  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("KEYWORD", $keyword);
  $ns->output("URL", "channel.server20.com/check/me/please");
}

1;
