#v2

package AverageUsersCost;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my ($cpm, $cpc, $cpa) = (1000, 2, 3);

  my @cases = (
    { name => 'DEFCUR',
      rate => 1 },
    { name => 'NONDEFCUR',
      rate => 2 }
  );

  foreach my $case (@cases)
  {
    my $currency_id = $case->{rate} eq 1 ?
      DB::Defaults::instance()->currency() :
      $ns->create(Currency => { rate => $case->{rate} });
    $case->{account} = $ns->create(Account => {
      name => $case->{name},
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type,
      currency_id => $currency_id });
  }

  my $request_keyword = make_autotest_name($ns, "request");

  my $request_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "request",
    account_id => $cases[0]->{account},
    keyword_list => $request_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => 'P' ) ]
    ));

  $ns->output("REQUEST/KEYWORD", $request_keyword);
  $ns->output("REQUEST/CHANNEL", $request_channel->channel_id());

  foreach my $case (@cases)
  {
    # CPM creative
    my $campaign = $ns->create(DisplayCampaign => {
      name => $case->{name},
      account_id => $case->{account},
      channel_id => $request_channel->{channel_id},
      campaigncreativegroup_name => "$case->{name}-CPM",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => $cpm,
      site_links => [ { name => "$case->{name}-CPM" } ] });

    my $tag_cpm = $ns->create(PricedTag => {
      name => "$case->{name}-CPM",
      site_id => $campaign->{Site}[0]->{site_id} });

    $ns->output("$case->{name}/CPM/CCID", $campaign->{cc_id});
    $ns->output("$case->{name}/CPM/TAG", $tag_cpm);
    $ns->output("$case->{name}/CPM/REVENUE",
                money_upscale($cpm / 1000  / $case->{rate}),
                "revenue for 1 impression");

    # CPC creative

    my $publisher_cpc = $ns->create(Publisher => {
      name => "$case->{name}-CPC"});

    my $campaign_cpc = $ns->create(DisplayCampaign => {
      name => "$case->{name}-CPC",
      campaign_id => $campaign->{campaign_id},
      creative_id => $campaign->{creative_id},
      campaigncreativegroup_channel_id => $request_channel->{channel_id},
      campaigncreativegroup_cpc => $cpc,
      site_links => [ { site_id => $publisher_cpc->{site_id} } ]  });

    $ns->output("$case->{name}/CPC/CCID", $campaign_cpc->{cc_id});
    $ns->output("$case->{name}/CPC/TAG", $publisher_cpc->{tag_id});
    $ns->output("$case->{name}/CPC/REVENUE",
                money_upscale($cpc / $case->{rate}),
                "revenue for 1 click");

    # CPA creative
    my $publisher_cpa = $ns->create(Publisher => {
      name => "$case->{name}-CPA"});

    my $campaign_cpa = $ns->create(DisplayCampaign => {
      name => "$case->{name}-CPA",
      campaign_id => $campaign->{campaign_id},
      creative_id => $campaign->{creative_id},
      campaigncreativegroup_channel_id => $request_channel->{channel_id},
      campaigncreativegroup_cpa => $cpa,
      campaigncreativegroup_ar => 0.01,
      site_links => [ { site_id => $publisher_cpa->{site_id} } ] });

    $ns->output("$case->{name}/CPA/CCID", $campaign_cpa->{cc_id});
    $ns->output("$case->{name}/CPA/TAG", $publisher_cpa->{tag_id});
    $ns->output("$case->{name}/CPA/REVENUE",
                money_upscale($cpa / $case->{rate}),
                "revenue for 1 action");
  }

  # Text advertising
  my @bids = (1, 4, 5);

  my $size = $ns->create(CreativeSize => {
    name => 1,
    max_text_creatives => (scalar @bids) });

  my $ta_site = $ns->create(Site => {
    name => 'ta'});

  my $ta_tag = $ns->create(PricedTag => {
    name => 'ta',
    site_id => $ta_site,
    size_id => $size });

  $ns->output("TEXT/TAG", $ta_tag);

  my $k_channel = $ns->create(
      DB::BehavioralChannel->blank(
        name => 'ta',
        channel_type => 'K',
        keyword_list => $request_keyword,
        account_id => $cases[0]->{account},
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            minimum_visits => 2,
            time_from => 0,
            time_to => 3600 ) ]));

  $ns->output("REQUEST/K-CHANNEL", $k_channel->channel_id());

  foreach my $i (@bids)
  {
    my $text_campaign = $ns->create(TextAdvertisingCampaign => {
      name => "ta$i",
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      original_keyword => $request_keyword,
      ccgkeyword_channel_id => $k_channel->channel_id(),
      max_cpc_bid => $i,
      site_links => [{ site_id => $ta_site }] });

    $ns->output("TEXT/CCID#$i", $text_campaign->{cc_id});
  }
  my $revenue = 0;
  my @sorted_bids = sort {$a <=> $b} @bids;
  pop @sorted_bids;
  push @sorted_bids, $sorted_bids[-1];
  map {$revenue += $_} @sorted_bids;
  # +0.01 for top eCPM correction (REQ-2849)
  $revenue += 0.01;
  $ns->output("TEXT/REVENUE",
              money_upscale($revenue));
  

  # Test channels
  foreach my $channel (qw(TEST1 TEST2 TEST3 TEST4 TEST5))
  {
    my $test_keyword = make_autotest_name($ns, $channel);
    my $test_channel = $ns->create(DB::BehavioralChannel->blank(
      name => $channel,
      account_id => $cases[0]->{account},
      keyword_list => $test_keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]
      ));

    $ns->output("$channel/KEYWORD", $test_keyword);
    $ns->output("$channel/CHANNEL", $test_channel->channel_id());
  }
  $ns->output("DEFAULT/COLO", DB::Defaults::instance()->isp->{colo_id});
}
1;
