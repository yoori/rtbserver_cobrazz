
package PublisherAndAdvertiserTracking;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $pub_track = 
    "http://img.yandex.net/i/1.gif?r=##RANDOM##";

  my $adv_track_imp = 
    "http://img.yandex.net/i/imp.gif?r=##RANDOM##";

  my $adv_track_no_imp = 
    "http://img.yandex.net/i/no_imp.gif?r=##RANDOM##";

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    flags => 0,
    cpm => 0,
    pricedtag_publ_tag_track_pixel_value => 
      $pub_track } );

  my $test_publisher = $ns->create(Publisher => {
    name => "PublisherTest",
    pubaccount_flags => DB::Account::TEST,
    cpm => 0,
    pricedtag_publ_tag_track_pixel_value => 
      $pub_track } );

  my $advertiser = $ns->create(Account => {
    name => "Adv",
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type } );

  my $templateNoImp = $ns->create(Template => { 
    name => 'TemplateNoImp',
    template_file => 'UnitTests/PublisherAndAdvertiserTracking.html',
    app_format_id =>  DB::Defaults::instance()->app_format_no_track,
    flags => 0,
    template_file_type => 'T'});

  my $templateImp = $ns->create(Template => { 
    name => 'TemplateImp',
    template_file => 'UnitTests/PublisherAndAdvertiserTracking.html',
    app_format_id =>  DB::Defaults::instance()->app_format_track,
    flags => DB::TemplateFile::PIXEL_TRACKING,
    template_file_type => 'T'});

  my $keyword = make_autotest_name($ns, "Keyword");

  my $channel = DB::BehavioralChannel->blank(
    name => "Channel",
    account_id => $advertiser,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]);

  my $campaignNoImp = $ns->create(DisplayCampaign => {
    name => 'CampaignNoImp',
    account_id => $advertiser,
    campaigncreativegroup_cpm => 20,
    creative_template_id => $templateNoImp,
    creative_cradvtrackpixel_value => $adv_track_no_imp,
    creative_publ_tag_track_pixel_value => undef,
    site_links => [
      { site_id => $publisher->{site_id} },
      { site_id => $test_publisher->{site_id} }],
    channel_id => $channel });

  my $campaignImp  = $ns->create(DisplayCampaign => {
    name => 'CampaignImp',
    account_id => $advertiser,
    campaigncreativegroup_cpm => 20,
    creative_template_id => $templateImp,
    creative_cradvtrackpixel_value => $adv_track_imp,
    creative_publ_tag_track_pixel_value => undef,
    site_links => [
      { site_id => $publisher->{site_id} },
      { site_id => $test_publisher->{site_id} }],
    channel_id => $channel });

  $ns->output("KEYWORD", $keyword);
  $ns->output("CCIMP", $campaignImp->{cc_id});
  $ns->output("CCNOIMP", $campaignNoImp->{cc_id});
  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("TAGTEST", $test_publisher->{tag_id});
  $ns->output("COLOTEST", DB::Defaults::instance()->test_isp->{colo_id});
}

1;
