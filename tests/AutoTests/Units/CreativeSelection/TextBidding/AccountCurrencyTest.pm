package AccountCurrencyTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword1 = make_autotest_name($ns, "phrase_1");

  $ns->output("Keyword1", $keyword1, "kw");

  my $size_id = $ns->create(CreativeSize => {
    name => 1,
    max_text_creatives => 4 });

  my $site_id = $ns->create(Site => { name => 1 });

  my $tag_cpm = 4.0;

  $ns->output("min_cpm", $tag_cpm);

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $site_id,
    size_id => $size_id,
    cpm => $tag_cpm });

  $ns->output("Tag1", $tag_id, "tid");

  my @exchange_rate = (1, 20, 30, 5);

  my $cur_id1 = $ns->create(Currency => { rate => $exchange_rate[1] });

  my $cur_id2 = $ns->create(Currency => { rate => $exchange_rate[2] });

  my $cur_id3 = $ns->create(Currency => { rate => $exchange_rate[3] });

  my @currency_id = (DB::Defaults::instance()->currency(), $cur_id1, $cur_id2, $cur_id3);
  my @max_cpc_bid = (0.03, 0.035, 0.12, 0.04);
    
  my $sum_cpm = 0.0;
  for (my $i = 0; $i < 4; ++$i)
  {
    my $cpm = $max_cpc_bid[$i] * $tag_cpm;
    my $bid = $cpm * $exchange_rate[$i];

    my $campaign = $ns->create(TextAdvertisingCampaign => {
      name => $i + 1,
      size_id => $size_id,
      template_id => DB::Defaults::instance()->text_template,
      advertiser_currency_id => $currency_id[$i],
      original_keyword => $keyword1,
      max_cpc_bid => $bid,
      site_links => [{site_id => $site_id}] });

    $ns->output("CC".($i + 1), $campaign->{cc_id});

    if ($i != 2)
    {
      $ns->output("actual_cpc".($i + 1), 
        money_upscale($bid));
      $sum_cpm = $sum_cpm + $cpm;
    }
  }  

  my $actual_cpc3 = calc_ta_top_acpc(
    $max_cpc_bid[2] * $tag_cpm,
    $max_cpc_bid[3] * $tag_cpm,
    $tag_cpm,
    $sum_cpm);

  # https://confluence.ocslab.com/display/TDOCDRAFT/REQ-2849+Drop+to+the+Floor+Fix+2
  # 1 cent for eCPM => 0.1 cent for CPC (eCPM  / CTR / 1000, CTR = 0.01 )
  my $top_cpc_correction = 0.001;

  $ns->output("actual_cpc3", 
    money_upscale(($actual_cpc3 + $top_cpc_correction) * $exchange_rate[2]));
}

1;
