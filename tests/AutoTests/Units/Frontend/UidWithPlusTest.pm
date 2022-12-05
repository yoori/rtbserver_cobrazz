package UidWithPlusTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  $ns->output("UidWithPlusTest/Colo", 
    DB::Defaults::instance()->ads_isp->{colo_id});

  my $keyword = make_autotest_name($ns, "keyword");
  $ns->output("KEYWORD", $keyword);

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_cpa => 40, # expect action tracking url
    campaigncreativegroup_ar => 0.01,
    creative_crclick_value => "www.dread.com",
    site_links => [{name => 1}] });

  my $bp = $ns->create(
     DB::BehavioralChannel::BehavioralParameter->blank(
       channel_id => $campaign->{channel_id},
       trigger_type => "P" ));

  my $tag_id1 = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign->{Site}[0]->{site_id},
    country_code => DB::Defaults::instance()->test_country_1->{country_code},
    cpm => 20 });

  $ns->output("Tag Id/01", $tag_id1);

  $ns->output("RequestsCount", 10000, "Sets max ad requests count. ".
    "If value equals zero, it means that requests count is unbounded: " .
    "be carefull in this case - test may cycle.");
}

1;
