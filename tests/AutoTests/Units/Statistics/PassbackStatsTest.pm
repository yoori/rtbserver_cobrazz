package PassbackStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $url = "http://" . make_autotest_name($ns, "1") . ".com/";
  my $empty_cc_keyword = make_autotest_name($ns, "EmptyCCKeyword");
  my $ad_keyword = make_autotest_name($ns, "AdKeyword");

  my $pub_acc = $ns->create(PubAccount => {
    name => "pub-adsc-4704",
    flags => DB::Account::TEST });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_keyword_list => $ad_keyword,
    site_links => [
      {name => 1},
      {name => 'adsc-4704', account_id => $pub_acc} ]
    });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $campaign->{channel_id} ));

  my $tag0 = $ns->create(PricedTag => {
    name => '0',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag1 = $ns->create(PricedTag => {
    name => '1',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag2 = $ns->create(PricedTag => {
    name => '2',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag3 = $ns->create(PricedTag => {
    name => '3',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag4 = $ns->create(PricedTag => {
    name => '4',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag5 = $ns->create(PricedTag => {
    name => '5',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag6 = $ns->create(PricedTag => {
    name => '6',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag7 = $ns->create(PricedTag => {
    name => '7',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $tag8 = $ns->create(PricedTag => {
    name => '8',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  # ADSC-4704
  my $tag9 = $ns->create(PricedTag => {
    name => '9',
    site_id => $campaign->{Site}[1]->{site_id},
    cpm => 1 });

  my $tag10 = $ns->create(PricedTag => {
    name => '10',
    site_id => $campaign->{Site}[0]->{site_id},
    cpm => 1 });

  my $stmt = $ns->pq_dbh->prepare("SELECT Max(Colo_id) From Colocation");
  $stmt->execute;
  my @absent_colo = $stmt->fetchrow_array;
  $stmt->finish;

  $ns->output("ABSENT_COLO", $absent_colo[0] + 1000);

  $stmt = $ns->pq_dbh->prepare("SELECT Max(tag_id) From Tags");
  $stmt->execute;
  my @absent_tag = $stmt->fetchrow_array;
  $stmt->finish;

  $ns->output("ABSENT_TAG", $absent_tag[0] + 1000);

  $ns->output("COLO", DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("REDIRECT_URL", lc($url));
  $ns->output("TAG_0", $tag0);
  $ns->output("TAG_1", $tag1);
  $ns->output("TAG_2", $tag2);
  $ns->output("TAG_3", $tag3);
  $ns->output("TAG_4", $tag4);
  $ns->output("TAG_5", $tag5);
  $ns->output("TAG_6", $tag6);
  $ns->output("TAG_7", $tag7);
  $ns->output("TAG_8", $tag8);
  $ns->output("TAG_9", $tag9);
  $ns->output("TAG_10", $tag10);
  $ns->output("AD_KEYWORD", $ad_keyword);
  $ns->output("EMPTY_CC_KEYWORD", $empty_cc_keyword);

  $ns->output("COLO_2", DB::Defaults::instance()->test_isp->{colo_id});
}

1;
