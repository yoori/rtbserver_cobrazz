package CountryTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

# for showing ad publisher and advertiser country must be equals: REQ-2215

sub init
{
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "CountryTest-1");
  $ns->output("KEYWORD", $keyword, "keyword for adv request");

  my $publisher = $ns->create(PubAccount => {
    name => "Publisher",
    country_code => DB::Defaults::instance()->test_country_1->{country_code} });

  my $site = $ns->create(Site => {
    name => "Site",
    account_id => $publisher});

  my $tag = $ns->create(PricedTag => {
    name => "Tag",
    site_id => $site});

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_keyword_list => $keyword,
    country_code => DB::Defaults::instance()->test_country_1->{country_code},
    site_links => [{ site_id => $site }] });

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank(
    channel_id => $campaign->{channel_id},
    trigger_type => "P"));

  $ns->output("CC", $campaign->{cc_id}, "ccid");
  $ns->output("TAG", $tag->tag_id(), "tid");
}

1;

