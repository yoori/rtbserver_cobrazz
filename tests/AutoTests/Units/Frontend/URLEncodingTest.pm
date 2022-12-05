#v2

package URLEncodingTest;

use strict;
use warnings;
use encoding 'utf8';
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $url1 = "http://" . make_autotest_name($ns, "1") . ".com/";
  my $url2 = "http://" . qq[\xd0\xbf\xd1\x80\xd0\xb5\xd0\xb7\xd0\xb8] . 
      qq[\xd0\xb4\xd0\xb5\xd0\xbd\xd1\x82] . "." . qq[\xd1\x80\xd1\x84\x2f];
  my $url3 = "www." . make_autotest_name($ns, "3") . ".com/zet?x=y";
  my $url3_exp = "www." . make_autotest_name($ns, "3") . ".com%2Fzet%3Fx%3Dy";
  my $url4 = "http://" . make_autotest_name($ns, "4") . ".com/zet?x=y";
  my $keyword1 = make_autotest_name($ns, "keyword1");
  my $keyword2 = make_autotest_name($ns, "keyword2");

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type});

  my $publisher1 = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_cpm => 1 });

  my $publisher2 = $ns->create(Publisher => {
    name => "Publisher-EnPassback",
    pricedtag_cpm => 1,
    pricedtag_passback => $url1});

  my $publisher3 = $ns->create(Publisher => {
    name => "Publisher-RuPassback",
    pricedtag_cpm => 1,
    pricedtag_passback => $url2 });

  my $min_cpm = 1.0;

  my $size_id = $ns->create(CreativeSize =>
                             { name => 'Click',
                               max_text_creatives => 2 });

  my $publisher4 = $ns->create(Publisher => {
    name => "Publisher-Click1",
    size_id => $size_id });

  my $publisher5 = $ns->create(Publisher => {
    name => "Publisher-Click2" });

  my $campaign1 = $ns->create(TextAdvertisingCampaign => {
    name => 'Click1',
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    account_id => $advertiser,
    original_keyword => $keyword1,
    max_cpc_bid => 2 * $min_cpm / 10,
    ccgkeyword_click_url => $url2,
    site_links => 
      [ { site_id =>  $publisher4->{site_id} }] });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'Click2',
    account_id => $advertiser,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_cpc => 50, 
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel-Clicl2',
      account_id => $advertiser,
      keyword_list => $keyword2,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    site_links => 
      [ { site_id =>  $publisher5->{site_id} }] });

  $ns->output("KEYWORD1", $keyword1);
  $ns->output("KEYWORD2", $keyword2);
  $ns->output("PASSBACK_URL1", $url1);
  $ns->output("PASSBACK_URL1_EXP", lc($url1));
  Encode::_utf8_on($url2);
  $ns->output("PASSBACK_URL2", $url2);
  $ns->output("PASSBACK_URL2_IDNA_EXP", 
              "http://xn--d1abbgf6aiiy.xn--p1ai/");
  $ns->output("PASSBACK_URL3", $url3);
  $ns->output("PASSBACK_URL3_EXP", $url3_exp);
  $ns->output("PASSBACK_URL4", $url4);
  $ns->output("PASSBACK_URL4_EXP", lc($url4));
  $ns->output("CRCLICK_DEFAULT", 'http://www.autotest.com/');
  $ns->output("SIMPLE_TAG", $publisher1->{tag_id});
  $ns->output("ENPASSBACK_TAG", $publisher2->{tag_id});
  $ns->output("RUPASSBACK_TAG", $publisher3->{tag_id});
  $ns->output("CLICK1_TAG", $publisher4->{tag_id});
  $ns->output("CLICK2_TAG", $publisher5->{tag_id});
}

1;
