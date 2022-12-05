package NonRecursiveExistingToken;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $publisher = $ns->create(Publisher => { name => "Publisher" });

  my $advertiser = $ns->create(Account => {
    name => "Adv",
    role_id => DB::Defaults::instance()->advertiser_role });

  my $template = $ns->create(Template => {
    name => 'Template',
    template_file => 'UnitTests/banner_img_clk.html',
    flags => 0 });
  
  my $keyword = make_autotest_name($ns, "keyword");

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    account_id => $advertiser,
    name => 'Channel',
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 'Campaign1',
    account_id => $advertiser,
    campaigncreativegroup_cpm => 10,
    creative_template_id => $template,
    site_links => [{ site_id =>  $publisher->{site_id} }],
    channel_id => $channel });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'Campaign2',
    account_id => $advertiser,
    campaigncreativegroup_cpm => 20,
    creative_template_id => $template,
    creative_adimage_value =>
      DB::Options->blank(
       type => 'String',
       recursive_tokens => undef,
       sort_order => 1,
       value => '##ADIMAGE-SERVER##'),
    site_links => [{ site_id =>  $publisher->{site_id} }],
    channel_id => $channel });

  $ns->output("KEYWORD", $keyword);
  $ns->output("CC1", $campaign1->{cc_id});
  $ns->output("CC2", $campaign2->{cc_id});
  $ns->output("TAG", $publisher->{tag_id});
}

1;
