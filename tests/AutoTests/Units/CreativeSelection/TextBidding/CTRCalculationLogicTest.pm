package CTRCalculationLogicTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $country = $ns->create(Country => {
    country_code => "WF",  # Wallis and Futuna
    low_channel_threshold => 0,
    high_channel_threshold => 0 });

  my $size = $ns->create(CreativeSize => {
    name => 1,
    max_text_creatives => 1 });

  my $tag_cpm = 4.0;

  my $publisher1 = $ns->create(Publisher => { 
    name => "Publisher1",
    pubaccount_country_code => $country->{country_code},
    pricedtag_size_id => $size,
    pricedtag_cpm => $tag_cpm });

  my $publisher2 = $ns->create(Publisher => { 
    name => "Publisher2",
    pubaccount_country_code => $country->{country_code},
    pricedtag_size_id => $size,
    pricedtag_cpm => $tag_cpm });

  my $keyword1 = make_autotest_name($ns, "keyword1");
  my $keyword2 = make_autotest_name($ns, "keyword2");
  my $keyword3 = make_autotest_name($ns, "keyword3");

  my $tac1 = $ns->create(TextAdvertisingCampaign => {
    name => 1,
    size_id => $size,
    country_code => $country->{country_code},
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword1,
    max_cpc_bid => 1.0 * $tag_cpm,
    ccgkeyword_tow => undef,
    ccgkeyword_ctr => undef,
    site_links => [{site_id => $publisher1->{site_id}}] });

  my $tac2 = $ns->create(TextAdvertisingCampaign => {
    name => 2,
    size_id => $size,
    country_code => $country->{country_code},
    campaigncreativegroup_campaign_id => $tac1->{campaign_id},
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword2,
    max_cpc_bid => 1.0 * $tag_cpm,
    ccgkeyword_tow => undef,
    ccgkeyword_ctr => undef,
    site_links => [{site_id => $publisher2->{site_id}}] });

  my $ccg_keyword3 = $ns->create(CCGKeyword => {
    name => 3,
    ccg_id => $tac2->{ccg_id},
    channel_id => $tac1->{CCGKeyword}->channel_id,
    original_keyword => $keyword1,
    max_cpc_bid => 1.0 * $tag_cpm,
    click_url => 'http://test.com',
    tow => undef,
    ctr => undef });

  my $ccg_keyword4 = $ns->create(CCGKeyword => {
    name => 4,
    ccg_id => $tac2->{ccg_id},
    original_keyword => $keyword3,
    max_cpc_bid => 1.0 * $tag_cpm,
    click_url => 'http://test.com',
    tow => undef,
    ctr => undef
    });

  $ns->output("Keyword1", $keyword1);
  $ns->output("Keyword2", $keyword2);
  $ns->output("Keyword3", $keyword3);
  $ns->output("CCG1", $tac1->{ccg_id});
  $ns->output("CCG2", $tac2->{ccg_id});
  $ns->output("CC1", $tac1->{cc_id});
  $ns->output("CC2", $tac2->{cc_id});
  $ns->output("CCGKeyword1", $tac1->{ccg_keyword_id});
  $ns->output("CCGKeyword2", $tac2->{ccg_keyword_id});
  $ns->output("CCGKeyword3", $ccg_keyword3);
  $ns->output("CCGKeyword4", $ccg_keyword4);
  $ns->output("Tag1", $publisher1->{tag_id});
  $ns->output("Tag2", $publisher2->{tag_id});
  $ns->output("Country", lc($country->{country_code}));
  $ns->output("COUNTRYCODE", $country->{country_code});
}

1;

