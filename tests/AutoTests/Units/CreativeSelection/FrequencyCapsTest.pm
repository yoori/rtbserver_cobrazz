
package FrequencyCapsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant BP_TIME_TO => 0;
use constant FC_CONFIRM_TIMEOUT => 60;

use constant ONE_CHANNEL_FOR_ALL => 'One-Channel-For-All';
use constant FIRST_CAMPAIGN_IS_CHANNEL_TARGETED => 'First-Campaign-Is-Channel-Targeted';
use constant CMP_FLAGS => 'Cmp-Flags';

# scenario entities naming:
#  name prefix format is <test entity type>-<test freq cap type>[-Text|-TextCh][-Track]
#  <test entity type>: Cmp, Ccg, Cc, Ch, Site
#  <test freq cap type>: Window, LifeCount, Period, Comb
#  -Text : text ads, first ad is keyword based
#  -TextCh : text ads, first ad is channel based
#  -Track : track pixel enabled, use it in test
#

sub init_display_case
{
  my ($self, $ns, $advertiser, $publisher, $name, $freq_caps) = @_;

  my $keyword = make_autotest_name($ns, $name);

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => $name,
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => BP_TIME_TO) ]));

  my $campaign = $ns->create(DisplayCampaign => {
    name => $name,
    account_id => $advertiser,
    channel_id => $channel,
    campaign_freq_cap_id => $freq_caps->{campaign},
    campaigncreativegroup_freq_cap_id => $freq_caps->{campaign_creative_group},
    campaigncreative_freq_cap_id => $freq_caps->{campaign_creative},
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{
      name => $name,
      account_id => $publisher,
      freq_cap_id => $freq_caps->{site}}] });

  my $tag_id = $ns->create(PricedTag => {
    name => $name,
    site_id => $campaign->{Site}[0]->{site_id} });

  $ns->output("KWD/$name", $keyword);
  $ns->output("CC/$name", $campaign->{cc_id}, "ccid");
  $ns->output("Tag/$name", $tag_id, "tid");
}

sub init_display_two_creatives_case
{
  my ($self, $ns, $advertiser, $name, $freq_cap) = @_;

  my $keyword = make_autotest_name($ns, $name);

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => $name,
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => BP_TIME_TO) ]
      ));

  my $campaign = $ns->create(DisplayCampaign => {
    name => "$name-1",
    account_id => $advertiser,
    channel_id => $channel,
    campaigncreative_freq_cap_id => $freq_cap,
    site_links => [{name => $name}] });

  my $creative_id = $ns->create(Creative => {
    name => "$name-2",
    account_id => $advertiser});

  my $cc_id = $ns->create(CampaignCreative => {
    ccg_id => $campaign->{ccg_id},
    creative_id => $creative_id });

  my $tag_id = $ns->create(PricedTag => {
    name => $name,
    site_id => $campaign->{Site}[0]->{site_id} });

  $ns->output("KWD/$name", $keyword,);
  $ns->output("CC/$name-1", $campaign->{cc_id}, "ccid");
  $ns->output("CC/$name-2", $cc_id, "ccid");
  $ns->output("Tag/$name", $tag_id, "tid");
}

sub init_text_case
{
  my ($self, $ns, $advertiser, $publisher,
    $name, $freq_caps, $params) = @_;

  my $first_campaign_is_channel_targeted =
    defined($params->{FIRST_CAMPAIGN_IS_CHANNEL_TARGETED}) &&
    $params->{FIRST_CAMPAIGN_IS_CHANNEL_TARGETED} > 0;

  my $one_channel =
    defined($params->{ONE_CHANNEL_FOR_ALL}) &&
    $params->{ONE_CHANNEL_FOR_ALL} > 0 ? 1 : undef;

  my $keyword = make_autotest_name($ns, "ta-$name");
  $ns->output("KWD/$name", $keyword);

  my $cmp_flags = DB::Campaign::INCLUDE_SPECIFIC_SITES;

  my $tag_cpm = 10.0;

  my $site_id = $ns->create(Site => {
    name => "ta-$name",
    account_id => $publisher,
    freq_cap_id => $freq_caps->{site} });

  if(defined($freq_caps->{channel}) && $first_campaign_is_channel_targeted)
  {
    die "FrequencyCapsTest: incorrect combination: first campaign is channel targeted & " .
      "channel freq caps defined";
  }

  my $keyword_channel_id = $ns->create(DB::BehavioralChannel->blank(
    name => $name,
    account_id => $advertiser,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        time_to => BP_TIME_TO) ],
    channel_type => 'K',
    freq_cap_id => $freq_caps->{channel}))->channel_id();

  # create four campaign/ccg/creative bundles
  #   first 3 shown when bundle freq cap allow
  #   second 3 when not allow
  # first ccg is channel based
  # other ccgs keyword based
  for (my $i = 1; $i <= 4; ++$i)
  {
    my $campaign;
    if($first_campaign_is_channel_targeted && $i == 1)
    {
      my $channel = $ns->create(DB::BehavioralChannel->blank(
        name => $name,
        account_id => $advertiser,
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            time_to => BP_TIME_TO) ],
        channel_type => 'K'));

      $campaign = $ns->create(ChannelTargetedTACampaign => {
        name => "$name-ta-$i",
        size_id => $self->{text_size_id},
        template_id => $self->{text_template_id},
        template_file_id => $self->{dummy_template_file_id},
        channel_id => $channel->channel_id(),
        campaign_flags => 0,
        campaign_freq_cap_id => (
          $i > 1 ? undef : $freq_caps->{campaign}),
        campaigncreativegroup_flags => $cmp_flags,
        campaigncreative_freq_cap_id => (
          $i > 1 ? undef : $freq_caps->{campaign_creative}),
        campaigncreativegroup_freq_cap_id => (
          $i > 1 ? undef : $freq_caps->{campaign_creative_group}),
        campaigncreativegroup_cpm => ((4 - $i) + $tag_cpm) * 1000,
        site_links => [{ site_id => $site_id }] });
    }
    else
    {
      $campaign = $ns->create(TextAdvertisingCampaign => {
        name => "$name-ta-$i",
        size_id => $self->{text_size_id},
        template_id => $self->{text_template_id},
        template_file_id => $self->{dummy_template_file_id},
        campaign_flags => 0,
        campaign_freq_cap_id => ($i > 1 ? undef : $freq_caps->{campaign}),
        campaigncreativegroup_flags => $cmp_flags,
        campaigncreative_freq_cap_id => (
          $i > 1 ? undef : $freq_caps->{campaign_creative}),
        campaigncreativegroup_freq_cap_id => (
          $i > 1 ? undef : $freq_caps->{campaign_creative_group}),
        original_keyword => $keyword,
        ccgkeyword_channel_id => (
          defined($one_channel) || $i == 1 ? $keyword_channel_id : undef),
        max_cpc_bid => (4 - $i) + $tag_cpm,
        site_links => [{ site_id => $site_id }] });
    }

    $ns->output("CC/$name-$i", $campaign->{cc_id});
  };

  my $tag_id = $ns->create(PricedTag => {
    name => "ta-$name",
    site_id => $site_id,
    size_id => $self->{text_size_id},
    cpm => $tag_cpm });

  $ns->output("Tag/$name", $tag_id);
}

sub init_text_cases_common
{
  my ($self, $ns) = @_;

  # init CreativeSize with max_text_creatives = 3
  $self->{text_size_id} = $ns->create(CreativeSize => {
    name => "TextSize-Max3",
    max_text_creatives => 3 });

  $self->{text_template_id} = DB::Defaults::instance()->text_template;

  # create common text templates (no track, track)
  $self->{dummy_template_file_id} = $ns->create(TemplateFile => {
    template_id => $self->{text_template_id},
    size_id => $self->{text_size_id},
    template_file => 'UnitTests/textad.xsl',
    flags => 0,
    app_format_id => DB::Defaults::instance()->app_format_no_track,
    template_type => 'X' });

  $ns->create(TemplateFile => {
    template_id => $self->{text_template_id},
    size_id => $self->{text_size_id},
    template_file => 'UnitTests/textad.xsl',
    flags => DB::TemplateFile::PIXEL_TRACKING,
    app_format_id => DB::Defaults::instance()->app_format_track,
    template_type => 'X' });
}

sub init
{
  my ($self, $ns) = @_;

  my $test_name = "FrequencyCapsTest";

  my $advertiser = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role() });

  $ns->output("FreqCapConfirmTimeout", FC_CONFIRM_TIMEOUT);

  my $window_time = 120; # 120 seconds, must be great then FC_CONFIRM_TIMEOUT
  my $window_limit = 2;
  my $life_limit = 2;
  my $period = 30;

  $ns->output("WindowTime", $window_time);
  $ns->output("WindowLimit", $window_limit);
  $ns->output("LifeLimit", $life_limit);
  $ns->output("Period", $period);

  my $comb_window_time = 60;
  my $comb_window_limit = 2;
  my $comb_life_limit = 4;
  my $comb_period = 10;

  $ns->output("CombWindowTime", $comb_window_time);
  $ns->output("CombPeriod", $comb_period);

  my $publisher = $ns->create(PubAccount => {
    name => "pub-fc",
    account_type_id => DB::Defaults::instance()->advertiser_type() });

  # ADSC-478
  my $window_freq_cap = DB::FreqCap->blank(
    window_length => $window_time, window_count => $window_limit);

  my $life_count_freq_cap = DB::FreqCap->blank(
    life_count => $life_limit);

  my $life_count_freq_cap_x3 = DB::FreqCap->blank(
    life_count => $life_limit * 3);

  my $period_freq_cap = DB::FreqCap->blank(period => $period);

  $self->init_text_cases_common($ns);

  # campaign level freq caps
  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cmp-Window",
    { campaign => $window_freq_cap });

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cmp-Window-Track",
    { campaign => $window_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Cmp-Window-TextCh",
    { campaign => $window_freq_cap },
    { FIRST_CAMPAIGN_IS_CHANNEL_TARGETED => 1 });

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cmp-LifeCount",
    { campaign => $life_count_freq_cap });

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cmp-LifeCount-Track",
    { campaign => $life_count_freq_cap });

  # combined frequency caps case (values must be synchronized with test etalons)
  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cmp-Comb",
    { campaign => DB::FreqCap->blank(
        life_count => $comb_life_limit,
        period => $comb_period,
        window_length => $comb_window_time,
        window_count => $comb_window_limit) });

  # CCG level freq caps
  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Ccg-Window",
    { campaign_creative_group => $window_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Ccg-Window-Text",
    { campaign_creative_group => $window_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Ccg-Window-TextCh",
    { campaign_creative_group => $window_freq_cap },
    { FIRST_CAMPAIGN_IS_CHANNEL_TARGETED => 1 });

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Ccg-LifeCount",
    { campaign_creative_group => $life_count_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Ccg-LifeCount-Text",
    { campaign_creative_group => $life_count_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Ccg-LifeCount-TextCh",
    { campaign_creative_group => $life_count_freq_cap },
    { FIRST_CAMPAIGN_IS_CHANNEL_TARGETED => 1});

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Ccg-Period",
    { campaign_creative_group => $period_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Ccg-Period-TextCh",
    { campaign_creative_group => $period_freq_cap },
    { FIRST_CAMPAIGN_IS_CHANNEL_TARGETED => 1 });

  # Creative level freq caps
  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cc-Window",
    { campaign_creative => $window_freq_cap });

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cc-LifeCount",
    { campaign_creative => $life_count_freq_cap });

  $self->init_display_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Cc-Period",
    { campaign_creative => $period_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Cc-Period-Text",
    { campaign_creative => $period_freq_cap });

  $self->init_display_two_creatives_case(
    $ns, $advertiser, "Cc2-Window",
    $window_freq_cap);

  # channel freq caps
  $self->init_text_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Ch-Window-Text",
    { channel => $window_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, DB::Defaults::instance()->publisher_account, "Ch-LifeCount-Text-OneChannel",
    { channel => $life_count_freq_cap_x3 },
    { ONE_CHANNEL_FOR_ALL => 1 });

  # Site level frequency caps
  # ADSC-4576
  $self->init_display_case(
    $ns, $advertiser, $publisher, "Site-Window-Track",
    { site => $window_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Site-Window-Text-Track",
    { site => $window_freq_cap });

  $self->init_display_case(
    $ns, $advertiser, $publisher, "Site-LifeCount-Track",
    { site => $life_count_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Site-LifeCount-Text",
    { site => $life_count_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Site-LifeCount-Text-Track",
    { site => $life_count_freq_cap });

  $self->init_display_case(
    $ns, $advertiser, $publisher, "Site-Period",
    { site => $period_freq_cap });

  $self->init_text_case(
    $ns, $advertiser, $publisher, "Site-Period-Text",
    { site => $period_freq_cap });
}

1;
