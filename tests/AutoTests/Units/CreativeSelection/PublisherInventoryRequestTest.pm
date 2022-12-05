package PublisherInventoryRequestTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

# for showing ad publisher and advertiser country must be equals: REQ-2215

sub init
{
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "kwd");

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_keyword_list => $keyword,
    site_links => [{ name => "1" }] });

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank(
    channel_id => $campaign->{channel_id},
    trigger_type => "P"));

  my $passback_url = "https://confluence.ocslab.com";
  $ns->output("PASSBACK", $passback_url, "");

  # Tags
  my $tag_inv = $ns->create(PricedTag => {
    name => 'inv',
    site_id => $campaign->{Site}[0]->{site_id},
    flags => DB::Tags::INVENORY_ESTIMATION,
    passback => $passback_url });

  my $tag_no_inv = $ns->create(PricedTag => {
    name => 'noinv',
    site_id => $campaign->{Site}[0]->{site_id} });

  $ns->output("KEYWORD", $keyword, "keyword for adv request");
  $ns->output("CC", $campaign->{cc_id}, "ccid");
  $ns->output("TAG_INV", $tag_inv);
  $ns->output("TAG_NO_INV", $tag_no_inv);
}

1;
