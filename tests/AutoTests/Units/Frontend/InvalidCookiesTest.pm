package InvalidCookiesTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "key_phrase_1");

  my $advertiser = $ns->create(Account => {
    name => 'Adv',
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type});

  my $template = $ns->create(Template => {
    name => 1,
    size_id => DB::Defaults::instance()->size(),
    template_file => 'UnitTests/banner_img_clk.html',
    flags => DB::TemplateFile::PIXEL_TRACKING,
    app_format_id => DB::Defaults::instance()->app_format_track,
    template_file_type => 'T' });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    account_id => $advertiser,
    campaigncreativegroup_cpa => 1, # expected adv action url
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 1}],
    template_id => $template,
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ])
    });

  $ns->output("Keyword/01", $keyword, "keyword for request");

  $ns->output("CCID/01", $campaign->{cc_id}, "Campaign creative id");

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign->{Site}[0]->{site_id} });

  my $passback_tag = $ns->create(PricedTag => {
    name => 'PassbackTag',
    site_id => $campaign->{Site}[0]->{site_id},
    passback => 'https://jira.corp.foros.com/browse/ADSC-3949'});

  $ns->output("Tag/01", $tag_id, "tid");
  $ns->output("PassbackTag", $passback_tag, "tid with defined passback");

  $ns->output("PassbackURL", $passback_tag->{passback}, "passback url for redirect");
}

1;
