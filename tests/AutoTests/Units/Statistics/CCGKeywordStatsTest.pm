package CCGKeywordStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant EUROPE_COPENHAGEN_TIMEZONE_ID => 13; # GMT + 01:00
use constant AMERICA_SAO_PAULO_TIMEZONE_ID => 24; # GMT - 02:00

use constant ADV_EXCHANGE_RATE => 23.41;
use constant TAG_CPM => 10;

# use specific country code for 
sub init_case
{
  my ($ns, $case_name, $size, $advertisers_properties, $calc_amounts) = @_;

  $calc_amounts = !defined($calc_amounts) ? 1 : $calc_amounts;

  my $site_id;

  my $site = $ns->create(Site => {
    name => "$case_name" });

  my $tag = $ns->create(PricedTag => {
    name => "$case_name",
    site_id => $site,
    size_id => $size,
    cpm => TAG_CPM,
    adjustment => 1.0 });

  my $adv_props_size = scalar(@$advertisers_properties);
  my $prev_click_sys_revenue = 0;
  my $sum_ecpm = 0; # without ctr & 1000 multiply (see calc_ta_top_acpc)
  my $i = 1;

  for(my $i = scalar(@$advertisers_properties); $i > 0; --$i)
  {
    my $adv_props = $advertisers_properties->[$i - 1];

    my $keyword = make_autotest_name($ns, "$case_name/Keyword/$i");

    my $currency = defined($adv_props->{currency}) ?
      $adv_props->{currency} : DB::Defaults::instance()->currency();

    my $exchange_rate = $currency->{rate};

    # max_cpc_bid in advertiser currency
    my $max_cpc_bid = money_upscale(
      ((defined($adv_props->{bid}) ? $adv_props->{bid} : 1.3) *
        TAG_CPM) * $exchange_rate / (DB::Defaults::default_ctr() * 1000));

    my $advertiser = $ns->create(Account => {
      name => $case_name . $i,
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type,
      currency_id => $currency,
      timezone_id => defined($adv_props->{timezone}) ?
        $adv_props->{timezone} : DB::Defaults::instance()->timezone() });
      
    my $campaign = $ns->create(TextAdvertisingCampaign => {
      name => $case_name . $i,
      account_id => $advertiser,
      template_id => DB::Defaults::instance()->text_template,
      size_id => $size, # ???
      original_keyword => $keyword,
      max_cpc_bid => $max_cpc_bid,
      ccgkeyword_ctr => DB::Defaults::default_ctr(),
      ccgkeyword_tow => 1.0,
      site_links => [{ site_id => $site }] });

    $ns->output("Keyword$i/${case_name}", $keyword, "Request Keyword");
    $ns->output("CC$i/${case_name}", $campaign->{cc_id}, "CcId");
    $ns->output("CCGKeywordId$i/${case_name}",
      $campaign->{ccg_keyword_id}, "ccg_keyword_id");

    if($calc_amounts > 0)
    {
      my $click_amount = (
        ($i eq 1) && scalar(@$advertisers_properties) > 1 ?
          money_upscale(
            calc_ta_top_acpc(
              $max_cpc_bid / $exchange_rate, # top cpc in sys currency
              $prev_click_sys_revenue, # under top cpc in sys currency
              TAG_CPM, # min ecpm
              $sum_ecpm, # sum ecpm
              DB::Defaults::default_ctr() * 1000) * $exchange_rate) :
          $max_cpc_bid);

      $sum_ecpm += $max_cpc_bid / $exchange_rate;
      $prev_click_sys_revenue = $max_cpc_bid / $exchange_rate;

      $ns->output("ClickAmount$i/${case_name}", $click_amount, "Click revenue");
    }
  }

  $ns->output("Tid/${case_name}", $tag, "Tag id");
}

sub init
{
  my ($self, $ns) = @_;

  # size
  my $size = $ns->create(CreativeSize => {
    name => 'MaxTwoCreatives',
    max_text_creatives => 2});

  my $currency =  $ns->create(Currency => {
    rate => ADV_EXCHANGE_RATE,
    fraction_digits => 2 });

  init_case($ns, 'SystemCurrency', $size, [ {} ] );
  init_case($ns, 'AdvCurrency', $size, [ { currency => $currency } ] );
  init_case($ns, 'MixedCurrency', $size, [ { bid => 1 }, { currency => $currency, bid => 0.2 } ] );

  # campaigns will be matched separatly without revenue check
  init_case($ns,
   'AdvTimezone',
    $size,
    [ { timezone => EUROPE_COPENHAGEN_TIMEZONE_ID, bid => 1.2 },
      { timezone => AMERICA_SAO_PAULO_TIMEZONE_ID, bid => 1.2 }
    ],
    0);
}

1;
