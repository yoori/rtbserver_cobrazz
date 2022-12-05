package TextAdAutoCategories;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $foros_min_margin = get_config($ns->pq_dbh, 'OIX_MIN_MARGIN') / 100.00;
  my $foros_min_fixed_margin = get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN');

  my $tag_cpm = 4.0;

  my $size = $ns->create(CreativeSize => {
    name => 1,
    max_text_creatives => 10 });

  my $acc_type = $ns->create(AccountType => {
    name => "pub",
    account_role_id => DB::Defaults::instance()->publisher_role,
    adv_exclusions => 'S',
    flags => 0 });

  my $acc = $ns->create(PubAccount => {
    name => 'Pub-1',
    account_type_id => $acc_type });

  my $site = $ns->create(Site => {
    name => 1,
    account_id => $acc });

  my $tag = $ns->create(PricedTag => {
    name => 1,
    site_id => $site,
    size_id => $size,
    cpm => $tag_cpm });

  $ns->output("Tag1", $tag);

  my $keyword1 = make_autotest_name($ns, "keyword1");
  $ns->output("Keyword1", $keyword1);

  # Note on leading space: below test name prefix is prepended to
  # CreativeCategory.name.  We use leading space to make it a separate
  # word.  For @co array below the prefix is also added, This extra
  # first word doesn't affect the test, but it's nice to remember
  # about it.
  my $creative_category1 = $ns->create(CreativeCategory => {
    cct_id => DB::CreativeCategoryType->blank(name => 'Tags'),
    name => " a b c" });

  $ns->create(SiteCreativeCategoryExclusion => {
    site_id => $site,
    creative_category_id => $creative_category1,
    approval => "R" });

  my @co = (
    HEADLINE1 => " a b c",
    HEADLINE => " a b c d",
    HEADLINE => " a b d",
    DESCRIPTION1 => "   a   b   c  ",
    DESCRIPTION1 => " a b",
    DESCRIPTION2 => " A B C D",
    DESCRIPTION2 => " a b cd",
    DISPLAY_URL => " a   B c",
    DISPLAY_URL => " b a c",
    );

  my %cpm_arr = (
    0 => money_upscale(0.8 * $tag_cpm),
    2 => money_upscale(0.6 * $tag_cpm),
    4 => money_upscale(0.4 * $tag_cpm),
    6 => money_upscale(0.2 * $tag_cpm),
    8 => money_upscale(0.1 * $tag_cpm),
    9 => money_upscale(0.01 * $tag_cpm),
    );

  my $total_creatives = 0;
  my $sum_cpm = 0.0;

  for (my $i = 0; $i < 10; ++$i)
  {
    my $cpm = (1.0 - 0.1 * $i) * $tag_cpm;
    if (exists $cpm_arr{$i})
    {
      $cpm = $cpm_arr{$i};
    }

    my $campaign = $ns->create(TextAdvertisingCampaign => {
      name => $i + 1,
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      original_keyword => $keyword1,
      max_cpc_bid => $cpm,
      site_links => [{site_id => $site}] });

    if (exists $cpm_arr{$i})
    {
      ++$total_creatives;
      $ns->output("CC$total_creatives", $campaign->{cc_id});
      if($i != 0)
      {
        $ns->output("ActualCPC$total_creatives", $cpm);
      }
      $sum_cpm = $sum_cpm + $cpm;
    }

    my ($t, $v) = @co[$i * 2, $i * 2 + 1];

    next unless defined $t;

    my $option_group = DB::Defaults::instance()->text_option_group;

    my $co = $ns->create(Options => {
      token => $t,
      option_group_id => $option_group,
      template_id =>  DB::Defaults::instance()->text_template,
      creative_id => $campaign->{creative_id},
      value => $ns->namespace . "-" . $v });

  }

  my $actual_cpc1 = calc_ta_top_acpc($cpm_arr{0}, $cpm_arr{2}, $tag_cpm, $sum_cpm) + 0.01;
  $ns->output("ActualCPC1", $actual_cpc1);
  $ns->output("TotalCreatives", $total_creatives);
}

1;
