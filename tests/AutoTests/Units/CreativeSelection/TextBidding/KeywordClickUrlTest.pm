
package KeywordClickUrlTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword1 = make_autotest_name($ns, "keyword_phrase_1");
  my $keyword2 = make_autotest_name($ns, "keyword_phrase_2");
  my $keyword3 = make_autotest_name($ns, "keyword_phrase_3");
  my $keyword4 = make_autotest_name($ns, "keyword_phrase_4");
  $ns->output("Keyword1", $keyword1, "kw");
  $ns->output("Keyword2", $keyword2, "kw");
  $ns->output("Keyword3", $keyword3, "kw");
  $ns->output("Keyword4", $keyword4, "kw");

  my $click_url1 = 'http://www.yahoo.com/';
  $ns->output("click_url1", $click_url1, "click_url1");

  my $click_url2 = 'http://www.altavista.com/';
  $ns->output("click_url2", $click_url2, "click_url2");

  my $click_url3 = 'http://www.clickurl3.com/';
  $ns->output("click_url3", $click_url3, "click_url3");

  my $click_url4 = 'http://www.clickurl4.com/';
  $ns->output("click_url4", $click_url4, "click_url4");

  my $size_id = $ns->create(CreativeSize =>
                             { name => 1,
                               max_text_creatives => 2 });

  my $foros_min_margin = get_config($ns->pq_dbh, 'OIX_MIN_MARGIN') / 100.00;
  my $foros_min_fixed_margin = get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN');
  my $min_cpm = 1.0 + $foros_min_fixed_margin;

  my $campaign = $ns->create(TextAdvertisingCampaign =>
                                 { name => 1,
                                   size_id => $size_id,
                                   template_id => DB::Defaults::instance()->text_template,
                                   original_keyword => $keyword1,
                                   max_cpc_bid => 2 * $min_cpm / 10,
                                   ccgkeyword_click_url => $click_url1,
                                   creative_crclick_value => $click_url2,
                                   site_links => [{name => 1}] });

  my $ccg_keyword_id2 = $ns->create(CCGKeyword =>
                                    { name => 2,
                                      ccg_id => $campaign->{ccg_id},
                                      original_keyword => $keyword2,
                                      max_cpc_bid => 2 * $min_cpm / 10});

  my $campaign3 = $ns->create(TextAdvertisingCampaign =>
                                 { name => 3,
                                   size_id => $size_id,
                                   template_id => DB::Defaults::instance()->text_template,
                                   account_id => $campaign->{account_id},
                                   original_keyword => $keyword3,
                                   max_cpc_bid => 2 * $min_cpm / 10,
                                   ccgkeyword_click_url => undef,
                                   creative_crclick_value => $click_url3.'##KEYWORD##',
                                   site_links => [{site_id => $campaign->{Site}[0]->{site_id}}] });

  my $campaign4 = $ns->create(TextAdvertisingCampaign =>
                                 { name => 4,
                                   size_id => $size_id,
                                   template_id => DB::Defaults::instance()->text_template,
                                   account_id => $campaign->{account_id},
                                   original_keyword => $keyword4,
                                   max_cpc_bid => 2 * $min_cpm / 10,
                                   ccgkeyword_click_url => undef,
                                   creative_crclick_value => $click_url4.'##RANDOM##',
                                   site_links => [{site_id => $campaign->{Site}[0]->{site_id}}] });

  my $tag_id = $ns->create(PricedTag =>
                           { name => 1,
                             site_id => $campaign->{Site}[0]->{site_id},
                             size_id => $size_id,
                             cpm => $min_cpm / (1 + $foros_min_margin)  });
  $ns->output("Tag", $tag_id, "tid");
}

1;
