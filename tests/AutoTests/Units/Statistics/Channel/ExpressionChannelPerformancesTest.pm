package ExpressionChannelPerformancesTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

# Create channels
sub create_channels
{
  my ($self, $ns, $account) = @_;

  my $url1 = "http://" . make_autotest_name($ns, "URL1") . ".com";
  my $keyword1 = make_autotest_name($ns, "KWD1");
  my $keyword2 = make_autotest_name($ns, "KWD2");
  my $keyword3 = make_autotest_name($ns, "KWD3");
  my $keyword4 = make_autotest_name($ns, "KWD4");

  $ns->output("URL/1", $url1);  
  $ns->output("KW/1", $keyword1);  
  $ns->output("KW/2", $keyword2);  
  $ns->output("KW/3", $keyword3);  
  $ns->output("KW/4", $keyword4);  

  # Behavioral channels
  my $channel1 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      keyword_list => $keyword1,
      account_id => $account,
      behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 600) ]));

  $ns->output("ChannelId/01", $channel1->channel_id);

  my $channel2 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 2,
      keyword_list => $keyword2,
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          time_to => 600) ]));

  $ns->output("ChannelId/02", $channel2->channel_id);

  my $channel3 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 3,
      url_list => $url1,
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "U",
          time_to => 600) ]));

   $ns->output("ChannelId/03", $channel3->channel_id);

  my $channel4 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 4,
      keyword_list => $keyword3,
      account_id => $account,
      behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 600) ]));

  $ns->output("ChannelId/04", $channel4->channel_id);

  my $channel5 = $ns->create(
    DB::BehavioralChannel->blank(
      name => 5,
      keyword_list => $keyword4,
      account_id => $account,
      behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 600) ]));

  $ns->output("ChannelId/05", $channel5->channel_id);

  # Create expression channels

  my $e2 = $ns->create(
      DB::ExpressionChannel->blank(
      name => 'E02',
      expression => $channel1->channel_id . "|" . $channel2->channel_id,
      account_id => $account));

  $ns->output("ExpressionId/02", 
              $e2->channel_id);

  my $e3 = $ns->create(DB::ExpressionChannel->blank(
    name => 'E03',
    expression => ($channel1->channel_id . "&" . $channel3->channel_id .
      "|" . $channel4->channel_id),
    account_id => $account));

  $ns->output("ExpressionId/03", 
              $e3->channel_id);

  my $e1 = $ns->create(DB::ExpressionChannel->blank(
    name => 'E01',
    expression => ("(" . $e2->channel_id . "|" . $channel3->channel_id . 
      ")^" . $channel5->channel_id),
    account_id => $account));

  $ns->output("ExpressionId/01", $e1->channel_id);

  my @expression_channels = ();

  push(@expression_channels, $e1);
  push(@expression_channels, $e2);
  push(@expression_channels, $e3);
  
  return @expression_channels;
}

# Create display campaigns
sub create_display_campaigns
{
  my ($self, $ns, $account, $currency, $channels) = @_;

  my $campaign1 = $ns->create(DisplayCampaign =>
  { name => "Display1",
    account_id => $account,
    channel_id => $channels->[0],
    targeting_channel_id => undef,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => 50,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => "Display1"}] });

  my $tag1 = $ns->create(PricedTag => {
    name => "Display1",
    site_id => $campaign1->{Site}[0]->{site_id} });

  my $campaign2 = $ns->create(DisplayCampaign =>
  { name => "Display2",
    channel_id => $channels->[0],
    targeting_channel_id => undef,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => 20,
    campaigncreativegroup_ar => 0.01,
    advertiser_currency_id => $currency, # non default currency
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => "Display2"}] });

  my $tag2 = $ns->create(PricedTag => {
    name => "Display2",
    site_id => $campaign2->{Site}[0]->{site_id} });

  $ns->output("DisplayTagId/1", $tag1);
  $ns->output("DisplayCC/1", $campaign1->{cc_id});
  $ns->output("DisplayCCG/1", $campaign1->{ccg_id});
  $ns->output("DisplayCPA/1", 50);

  $ns->output("DisplayTagId/2", $tag2);
  $ns->output("DisplayCC/2", $campaign2->{cc_id});
  $ns->output("DisplayCCG/2", $campaign2->{ccg_id});
  $ns->output("DisplayCPA/2", 20);
}

# Create TA campaigns
sub create_ta_campaigns
{
  my ($self, $ns, $channels) = @_;

  my $size = $ns->create(CreativeSize => {
    name => "TA",
    max_text_creatives => 2 });

  my $site = $ns->create(Site => { name => "TA"});
  
  my $tag = $ns->create(PricedTag => {
    name => "TA",
    site_id => $site,
    size_id => $size,
    cpm => 0.1});

  $ns->output("TATagId/1", $tag);

  my $campaign1 = $ns->create(ChannelTargetedTACampaign => {
    name => "TA1",
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    campaigncreativegroup_channel_id => $channels->[1],
    campaigncreativegroup_targeting_channel_id => undef,
    campaigncreativegroup_cpm =>  100,
    site_links => [{site_id => $site}] });

  $ns->output("TACC/1", $campaign1->{cc_id});
  $ns->output("TACCG/1", $campaign1->{ccg_id});
  $ns->output("TACPM/1", 100);

  my $campaign2 = $ns->create(ChannelTargetedTACampaign => {
    name => "TA2",
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    campaigncreativegroup_channel_id => $channels->[2],
    campaigncreativegroup_targeting_channel_id => undef,
    campaigncreativegroup_cpm => 40,
    site_links => [{site_id => $site}] });

  $ns->output("TACC/2", $campaign2->{cc_id});
  $ns->output("TACCG/2", $campaign2->{ccg_id});
  $ns->output("TACPM/2", 40);
}

sub init
{
  my ($self, $ns) = @_;

  # Commmon data
  my $account = $ns->create(Account => { # REVIEW: isp & advertiser
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  $ns->output("Colocation", 
    DB::Defaults::instance()->ads_isp->{colo_id});

  my $exchange_rate = 20;

  $ns->output("ExchangeRate", $exchange_rate);

  my $currency = $ns->create(Currency => { rate => $exchange_rate });

  # Channels & campaigns
  my @channels = $self->create_channels($ns, $account);
  $self->create_display_campaigns($ns, $account, $currency, \@channels);
  $self->create_ta_campaigns($ns, \@channels);

}

1;
