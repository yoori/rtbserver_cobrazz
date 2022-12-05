package FreqCapMergingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "keyword");

  $ns->output("KEYWORD", $keyword);

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 1,
    behavioralchannel_keyword_list => $keyword,
    campaign_freq_cap_id => DB::FreqCap->blank(
      window_length => 5,
      window_count => 1),
    site_links => [{name => 1}] });

  my $bp = $ns->create(
     DB::BehavioralChannel::BehavioralParameter->blank(
       channel_id => $campaign1->{channel_id},
       trigger_type => "P",
       time_to => 120 ));

  $ns->output("Channel", $campaign1->{channel_id});

  $ns->output("CC", $campaign1->{cc_id});

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign1->{Site}[0]->{site_id} });

  $ns->output("Tag", $tag_id, "tid");
}

1;
