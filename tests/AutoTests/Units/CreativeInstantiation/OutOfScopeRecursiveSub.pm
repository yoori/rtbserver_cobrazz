
package OutOfScopeRecursiveSub;

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
    template_file => 'UnitTests/test.html',
    flags => 0 });
  
  my $keyword = make_autotest_name($ns, "keyword");

  my $campaign1  = $ns->create(DisplayCampaign => {
    name => 'Campaign1',
    account_id => $advertiser,
    creative_template_id => $template,
    creative_imagetitle_value => undef ,
    site_links => [{ site_id =>  $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ])
    });

  my $creative2 = $ns->create(Creative => {
    name => 1,
    account_id => $advertiser,
    template_id => $template,
    imagetitle_value => '##RANDOM##'});

  my $cc_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign1->{ccg_id},
    creative_id => $creative2,
    weight => 1000 });
  
  $ns->output("KEYWORD", $keyword);
  $ns->output("CC1", $campaign1->{cc_id});
  $ns->output("CC2", $cc_2);
  $ns->output("TAG", $publisher->{tag_id});
}

1;
