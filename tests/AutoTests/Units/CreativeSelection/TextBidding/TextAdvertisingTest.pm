
package TextAdvertisingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
    my ($self, $ns) = @_;
    my $keyword1 = make_autotest_name($ns, "phrase_1");
    my $keyword2 = make_autotest_name($ns, "phrase_2");

    $ns->output("Keyword1", $keyword1, "kw");
    $ns->output("Keyword2", $keyword2, "kw");

    my $size_id = $ns->create(CreativeSize =>
                              { name => 1,
                                max_text_creatives => 5 });

    my $site_id = $ns->create(Site =>
                              { name => 1});

    my $tag_cpm = 10.0;

    my $tag_id1 = $ns->create(PricedTag =>
                              { name => 1,
                                site_id => $site_id,
                                size_id => $size_id,
                                cpm => $tag_cpm });
    $ns->output("Tag1", $tag_id1, "tid");

    for (my $i = 0; $i < 5; ++$i) {
      my $max_cpc_bid = (8 - $i) / 2 * $tag_cpm / 10;
      #  3.5 3 2.5 2
      my $campaign = $ns->create(TextAdvertisingCampaign =>
                                 { name => $i + 1,
                                   size_id => $size_id,
                                   template_id => DB::Defaults::instance()->text_template,
                                   original_keyword => $keyword1,
                                   max_cpc_bid => $max_cpc_bid,
                                   site_links => [{site_id => $site_id}] });

      $ns->output("CC".($i + 1), $campaign->{cc_id}, "cc_id");

      if ($i > 0) {
          my $actualECPMbid = $max_cpc_bid*100;
          $ns->output("actual_cpc".($i + 1), money_upscale($actualECPMbid) / 100, "actual_cpc");
      }
    }
    $ns->output("actual_cpc1", money_upscale((7 / 2) * $tag_cpm / 10), "actual_cpc");

    my $campaign = $ns->create(TextAdvertisingCampaign =>
                               { name => 6,
                                 size_id => $size_id,
                                 template_id => DB::Defaults::instance()->text_template,
                                 original_keyword => $keyword2,
                                 max_cpc_bid => 2.25 * $tag_cpm / 10,
                                 site_links => [{site_id => $site_id}] });

    $ns->output("CC6", $campaign->{cc_id}, "cc_id");
    $ns->output("actual_cpc6", money_upscale(22.5 * $tag_cpm) / 100, "actual_cpc");
}

1;
