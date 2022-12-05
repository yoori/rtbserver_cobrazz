package CampaignStartupTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant COUNTRY_CODE => 'AD';

sub init
{
  my ($self, $ns) = @_;

  # Make sure that we have now campaign with this country code in
  # any other tests (user created campaigns)
  my $country = $ns->create(Country => {
    country_code => COUNTRY_CODE,  # Andorra
    low_channel_threshold => 0,
    high_channel_threshold => 0 });

  my $keyword = make_autotest_name($ns, "Keyword");

  my $cpc = 10;

  my $advertiser = $ns->create(Account => {
    name => 'Adv',
    country_code => COUNTRY_CODE,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $publisher = $ns->create(PubAccount => {
    name => "Pub",
    country_code => COUNTRY_CODE} );

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    country_code => $country->{country_code},
    campaigncreativegroup_rate_type => 'CPC',
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpc => $cpc,
    campaigncreativegroup_cpa => 0,
    campaigncreativegroup_status => 'D',
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    site_links => [{ name => 1, account_id => $publisher }] });

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign->{Site}[0]->{site_id} });

  $ns->output("CCGID", $campaign->{ccg_id});
  $ns->output("CCID", $campaign->{cc_id});
  $ns->output("CAMPAIGNID", $campaign->{campaign_id});
  $ns->output("KEYWORD", $keyword);
  $ns->output("CCID", $campaign->{cc_id});
  $ns->output("TID", $tag_id);
  $ns->output("CPC", $cpc);
  $ns->output("Country", lc($country->{country_code}));
  $ns->output("COUNTRYCODE", $country->{country_code});
}

1;
