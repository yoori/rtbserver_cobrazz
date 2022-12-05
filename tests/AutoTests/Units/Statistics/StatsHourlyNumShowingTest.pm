
package StatsHourlyNumShowingTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_campaigns
{
  my ($self, $args) = @_;

  my $keyword = make_autotest_name($self->{ns_}, "Keyword");

  my $index = 0;

  foreach my $cpc_bid (@{$args->{cpc_bids}})
  {

    my $campaign = $self->{ns_}->create(
      TextAdvertisingCampaign => {
        name => ++$index,
        size_id => $args->{size_id},
        template_id => DB::Defaults::instance()->text_template,
        original_keyword => $keyword,
        max_cpc_bid => $cpc_bid,
        campaigncreativegroup_cpm => 
          defined $args->{ccg_cpm}? 
            $args->{ccg_cpm}: 0.0001,
        ccgkeyword_ctr => DB::Defaults::default_ctr(),
        ccgkeyword_tow => 1,
        site_links => [
          {site_id => $args->{site_id}}] });

     $self->{ns_}->output("CC/" . $index, $campaign->{cc_id});
  }
  $self->{ns_}->output("KWD", $keyword);
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;  }
  $self->{ns_} = $ns->sub_namespace($prefix);
  $self->{prefix_} = $prefix;
  $self->create_campaigns($args);
}

1;

package StatsHourlyNumShowingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  # Common
  my $foros_min_margin = get_config($ns->pq_dbh, 'OIX_MIN_MARGIN') / 100.00;
  my $foros_min_fixed_margin = 1.00 * get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN');

  my $tag_cpm = 1;

  my $size =  $ns->create(CreativeSize => {
    name => 'Size',
    max_text_creatives => 2});

  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->text_template,
    size_id => $size,
    template_file => 'UnitTests/textad.xsl',
    flags => 0,
    app_format_id => DB::Defaults::instance()->app_format_no_track,
    template_type => 'X'});

  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->text_template,
    size_id => $size,
    template_file => 'UnitTests/textad.xsl',
    flags => 1,
    app_format_id => DB::Defaults::instance()->app_format_track,
    template_type => 'X'});

  my $publisher = $ns->create( Publisher => {
    name => 'Publisher',
    size_id => $size,
    pricedtag_cpm => $tag_cpm,
    pricedtag_adjustment => 1 });

  my $exchange_rate = 20;
  my $currency = $ns->create(Currency => 
    { rate => $exchange_rate });

  my $publisher_non_sys_currency = $ns->create( Publisher => {
    name => 'PublisherNonSystemCurrency',
    pubaccount_currency_id => $currency,
    size_id => $size,
    pricedtag_cpm => $tag_cpm * $exchange_rate,
    pricedtag_adjustment => 1 });

  # Test cases
 StatsHourlyNumShowingTest::TestCase->new(
    $ns, 'OneShown',
    { size_id => $size,
      site_id => $publisher->{site_id},
      cpc_bids => [0.21 * $tag_cpm] });

 StatsHourlyNumShowingTest::TestCase->new(
    $ns, 'TwoShown',
    { size_id => $size,
      site_id => $publisher->{site_id},
      cpc_bids => [ 
        0.21 * $tag_cpm,
        0.19 * $tag_cpm ] });

 StatsHourlyNumShowingTest::TestCase->new(
    $ns, 'NonSystemCurrency',
    { size_id => $size,
      site_id => $publisher_non_sys_currency->{site_id},
      cpc_bids => [ 
        0.21 * $tag_cpm,
        0.19 * $tag_cpm ] });

 StatsHourlyNumShowingTest::TestCase->new(
    $ns, 'ImprTrack',
    { size_id => $size,
      site_id => $publisher->{site_id},
      ccg_cpm => 40,
      cpc_bids => [ 
        0.19 * $tag_cpm,
        0.17 * $tag_cpm ] });

  $ns->output("TID", $publisher->{tag_id});
  $ns->output("PUB_CPM", $tag_cpm);
  $ns->output("TIDCURRENCY", $publisher_non_sys_currency->{tag_id});
  $ns->output("EXCHANGE_RATE", $exchange_rate);
}

1;
