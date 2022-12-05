package ChannelImpInventoryTest::Diffs;

use strict;
use warnings;

sub create_diff
{
  my ($diff) = @_;

  my $value = sub { defined $_[0]? $_[0]: 0; };
 
  return 
    [ $value->($diff->{imps}),
      $value->($diff->{clicks}),
      $value->($diff->{actions}),
      $value->($diff->{revenue}),
      $value->($diff->{impops_user_count}),
      $value->($diff->{imps_user_count}),
      $value->($diff->{imps_value}),
      $value->($diff->{imps_other}),
      $value->($diff->{imps_other_user_count}),
      $value->($diff->{imps_other_value}),
      $value->($diff->{impops_no_imp}),
      $value->($diff->{impops_no_imp_user_count}),
      $value->($diff->{impops_no_imp_value}) ];
}

sub output_diff
{
  my ($ns, $prefix, $diff) = @_;

  my $i = 0;
  foreach my $row (@{$diff})
  {
    my $j = 0;
    #my @row = $r;
    foreach my $elem (@{$row})
    {
      $ns->output($prefix."$i-$j", $elem);
      ++$j;
    }
    ++$i;
  }
}

1;

package ChannelImpInventoryTest::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant DEFAULT_CTR  => 0.01;

sub output_hash
{
  my ($self, $name, $hash) = @_;
  $self->{ns_}->output(
     $self->{prefix_} . "/" . $name, 
     scalar keys(%$hash));
  foreach my $k (sort keys(%$hash))
  {
    my $v =  join(",", 
      map(
        $self->entity($_), 
        split(/,/, $hash->{$k})));
    $self->{ns_}->output(
      $self->{prefix_} . "/" . $k, $v);
  }
  $self->{ns_}->output(
    $self->{prefix_} . "/" . $name . "End", 
     scalar keys(%$hash));
}

sub store
{
  my ($self, $prefix, $value) = @_;
  $self->{entities_}->{$prefix} = $value;
}

sub entity
{
  my ($self, $name) = @_;
  die "'$name' not defined in the case '$self->{prefix_}'" 
       if not defined $self->{entities_}->{$name};
  return $self->{entities_}->{$name};
}

sub calc_text_keyword_revenue
{
  my ($cpc_bids, $index, $ctr) = @_;
  die "Invalid creative index: $index" 
    if $index >= @$cpc_bids;
  my @cpcs = grep {defined $_} @$cpc_bids;
  if ($index || @cpcs == 1) 
  {
    return $cpcs[$index] * 1000 * 100 * $ctr;
  }
  return min($cpcs[$index], $cpcs[1]) * 1000 * 100 * $ctr;
}

sub create_publisher
{
  my ($self, $prefix, $params) = @_;

  if (defined $params->{publisher})
  {
    return $params->{publisher};
  }

  my $publisher = 
      $self->{ns_}->create(Publisher => {
        name => $self->{prefix_} . "-" . $prefix . "-Publisher",
        pubaccount_currency_id => 
          defined $params->{currency}?
            $params->{currency}: DB::Defaults::instance()->currency(),
        pricedtag_size_id => $params->{size},
        pricedtag_cpm => $params->{tag_cpm},
        pricedtag_adjustment => 1.0});

  $self->store($prefix . "/MIN_ECPM",  $params->{tag_cpm});
  $self->store($prefix . "/TAG", $publisher->{tag_id});
  $self->store($prefix . "/SITE", $publisher->{site_id});

  return $publisher;
}

sub create_targeting_channel
{
  my ($self, $prefix, $name, $ch) = @_;

  my $channel = 
    $self->{ns_}->create(DB::TargetingChannel->blank(
    $self->{prefix_} . "-" . $name,
    expression => $ch));

  $self->store($name, $channel->{channel_id});
}

sub create_expchannel
{
  my ($self, $prefix, $params, $ch, $suffix) = @_;
  my $account = $self->{ns_}->create(Account => {
    name => $self->{prefix_} . "-" . $prefix,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $name = $prefix . "-Channel";
  my $pname = $prefix . "/CH";

  if (defined $suffix)
  {
    $name = $name . "-" . $suffix;
    $pname = $pname . $suffix;
  }

  my $channel = $self->{ns_}->create(DB::ExpressionChannel->blank(
      name => $self->{prefix_} . "-" . $name,
      account_id => $account,
      expression => $ch));

  $self->store($pname, $channel->{channel_id});
  
  return $channel;
}

sub create_channel
{
  my ($self, $prefix, $type, $params, $suffix) = @_;

  if (defined $params->{channel})
  {
    return $params->{channel};
  }

  my $account = defined $params->{account}?
    $params->{account}:
    $self->{ns_}->create(Account => {
      name => $self->{prefix_} . "-" . $prefix,
      role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = defined $params->{keyword}?
   $params->{keyword}: 
      make_autotest_name(
        $self->{ns_},  
        $self->{prefix_} . "-" . $prefix);

  $self->store($prefix . "/KWD", $keyword);

  my $name = $prefix . "-Channel";
  my $pname = $prefix . "/CH";
  if (defined $suffix)
  {
    $name = $name . "-" . $suffix;
    $pname = $pname . $suffix;
  }

  my $channel = $self->{ns_}->create(DB::BehavioralChannel->blank(
    name => $self->{prefix_} . "-" . $name,
    account_id => $account,
    keyword_list => $keyword,
    url_list => "www.".$keyword.".com",
    channel_type => $type,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P"),
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 259200,
        trigger_type => "U")] ));

  $self->store($pname, $channel->{channel_id});
  return $channel;
}

sub create_display_campaign
{
  my ($self, $prefix, $params) = @_;

  my $publisher = $self->create_publisher(
    $prefix, $params);

  $self->{ns_}->create(TemplateFile => {
    template_id => DB::Defaults::instance()->display_template(),
    size_id => $params->{size},
    template_file => 'UnitTests/textad.xsl',
    template_type => 'X',
    app_format_id => DB::Defaults::instance()->app_format_no_track });

  my $account = $self->{ns_}->create(Account => {
    name => $self->{prefix_} . "-" . $prefix,
    role_id => DB::Defaults::instance()->advertiser_role,
    currency_id =>
      defined $params->{currency}?
        $params->{currency}: DB::Defaults::instance()->currency()
  });

  my %channel_args = %$params;
  $channel_args{account} = $account;

  my $campaign = $self->{ns_}->create(DisplayCampaign => {
    name => $self->{prefix_} . "-" . $prefix,
    size_id => $params->{size},
    account_id => $account,
    template_id => DB::Defaults::instance()->display_template(),
    channel_id => 
      $self->create_channel(
        $prefix, 'B', \%channel_args)->{channel_id},
    campaigncreativegroup_cpm => $params->{cpm},
    site_links => [
      {site_id => $publisher->{site_id}} ] });  
    $self->store($prefix . "/ACCOUNT", $account->{account_id});
    $self->store($prefix . "/CC", $campaign->{cc_id});
    $self->store($prefix . "/CPM", $params->{cpm});

}

sub create_display_track_campaign
{
  my ($self, $prefix, $params) = @_;

  my $publisher = $self->create_publisher(
    $prefix, $params);

  $self->{ns_}->create(TemplateFile => {
    template_id => DB::Defaults::instance()->display_template(),
    size_id => $params->{size},
    template_file => 'UnitTests/textad.xsl',
    template_type => 'X',
    flags => DB::TemplateFile::PIXEL_TRACKING,
    app_format_id => DB::Defaults::instance()->app_format_track });

  my $channel_id = $self->create_channel($prefix, 'B', $params, "H")->{channel_id};
  $channel_id = $self->create_expchannel($prefix, $params, $channel_id,"E")->{channel_id};

  my $campaign = $self->{ns_}->create(DisplayCampaign => {
    name => $self->{prefix_} . "-" . $prefix,
    size_id => $params->{size},
    creative_template_id => DB::Defaults::instance()->display_template(),
    channel_id => $channel_id,
    campaigncreativegroup_cpm => $params->{cpm},
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [
      {site_id => $publisher->{site_id}} ] });  
    $self->store($prefix . "/CC", $campaign->{cc_id});
    $self->store($prefix . "/CPM", $params->{cpm});
}

sub create_text_keyword_campaigns
{
  my ($self, $prefix, $params) = @_;

  my $ctr = defined $params->{ctr}? 
    $params->{ctr}: DEFAULT_CTR;

  my $publisher = $self->create_publisher(
    $prefix, $params);

  my %channel_args = %$params;
  if (defined $params->{keyword})
  {
    $channel_args{channel} = 
      $self->create_channel(
        $prefix, 'K', $params)
  }

  my $idx = 0;

  foreach my $cpc_bid (@{$params->{cpc_bids}}) 
  {
    my $revenue =
     defined $cpc_bid ?
     calc_text_keyword_revenue($params->{cpc_bids}, $idx, $ctr) : 0;

    my $channel = $self->create_channel(
      $prefix, 'K', \%channel_args, ++$idx);

    my @sites = defined $cpc_bid? 
       ({ site_id => $publisher->{site_id} }): ();

    my $campaign = 
      $self->{ns_}->create(TextAdvertisingCampaign => { 
        name => $self->{prefix_} . "-" . $prefix . "-" . $idx,
        size_id => $params->{size},
        template_id =>  DB::Defaults::instance()->text_template,
        original_keyword => $channel->{keyword_list_},
        ccgkeyword_channel_id => $channel->{channel_id},
        ccgkeyword_ctr => $ctr,
        max_cpc_bid => $cpc_bid,
        site_links => \@sites });

    $self->store(
      $prefix ."/CC".$idx, $campaign->{cc_id});
    $self->store(
      $prefix ."/REVENUE".$idx, $revenue);
  }
}

sub create_text_channel_campaigns
{
  my ($self, $prefix, $params) = @_;

  my $ctr = defined $params->{ctr}? 
    $params->{ctr}: DEFAULT_CTR;

  my $publisher = $self->create_publisher(
     $prefix, $params);

  my %channel_args = %$params;
  if (defined $params->{keyword})
  {
    $channel_args{channel} = 
      $self->create_channel(
        $prefix, 'B', $params)
  }

  my $idx = 0;
  foreach my $cpm (@{$params->{cpms}}) 
  {

    my $channel = $self->create_channel(
      $prefix, 'B', \%channel_args, ++$idx);

    my $campaign =  $self->{ns_}->create(ChannelTargetedTACampaign => {
      name => $self->{prefix_} . "-" . $prefix . "-" . $idx,
      size_id => $params->{size},
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $channel->{channel_id},
      campaigncreativegroup_cpm => $cpm,
      campaigncreativegroup_ctr => $ctr,
      site_links => [
        {site_id => $publisher->{site_id} }] });
    $self->store($prefix ."/CC".$idx, $campaign->{cc_id});
    $self->store($prefix . "/REVENUE" . $idx, $cpm);
   }
}

sub create_case_data 
{
  my ($self, $args) = @_;

  my $size = $self->{ns_}->create(CreativeSize => {
    name => $self->{prefix_},
    max_text_creatives => 
      $args->{max_text_creatives} });
  $self->store("SIZE", $size);
  # Create publisher
  my $publisher; 
  if (defined $args->{publisher})
  {
    my %pub_args = %{$args->{publisher}};
    $pub_args{size} = $size;
    $publisher = 
      $self->create_publisher("COMMON", \%pub_args);
  }

  my $keyword;
  if (defined $args->{keyword})
  {
    $keyword = make_autotest_name($self->{ns_}, $self->{prefix_});
    $self->store("KWD", $keyword);
  }

  # Create display campaign
  if (exists $args->{display})
  {
    my %display_args = %{$args->{display}};
    $display_args{size} = $size;
    $display_args{publisher} = $publisher;
    $display_args{keyword} = $keyword;
    $self->create_display_campaign("DISPLAY", \%display_args);
  }

  # Create display track campaign
  if (exists $args->{display_track})
  {
    my %display_args = %{$args->{display_track}};
    $display_args{size} = $size;
    $display_args{publisher} = $publisher;
    $display_args{keyword} = $keyword;
    $self->create_display_track_campaign("DISPLAY_TRACK",
        \%display_args);
   }

  # Create channel targeted text campaigns
  if (exists $args->{channels})
  {
    my %channels_args = %{$args->{channels}};
    $channels_args{size} = $size;
    $channels_args{publisher} = $publisher;
    $channels_args{keyword} = $keyword;
    $self->create_text_channel_campaigns(
      "CHANNEL", \%channels_args);
  }

  # Create keyword text campaigns
  if (exists $args->{keywords})
  {
    my @keywords = ref($args->{keywords}) eq 'ARRAY'?
        @{$args->{keywords}}: ($args->{keywords});
    my $index = 0;
    foreach my $k (@keywords)
    {
      my $suffix = @keywords == 1? "": ++$index;
      my %keywords_args = %$k;
      $keywords_args{size} = $size;
      $keywords_args{publisher} = $publisher;
      $keywords_args{keyword} = $keyword;
      $self->create_text_keyword_campaigns(
        "KEYWORD" . $suffix, \%keywords_args);
    }
  }
}

sub output
{
  my ($self, $args) = @_;

  $self->output_hash("Tags", $args->{cpp_tags});
  $self->output_hash("Keywords", $args->{cpp_keywords});
  $self->output_hash("Channels", $args->{cpp_channels});
  
  my $index=0;
  foreach my $channel (@{ $args->{cpp_checks}->{channels} })
  {
    my $name = "ReqChannels-" . $index++;
    $self->output_hash("$name", $channel);
  }

  $index=0;
  foreach my $cc (@{ $args->{cpp_checks}->{creatives} })
  {
    my $name = "ReqCreatives-" . $index++;
    $self->output_hash("$name", $cc);
  }

  ChannelImpInventoryTest::Diffs::output_diff(
    $self->{ns_}, $self->{prefix_}, $args->{cpp_diffs});
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{ns_} = $ns;
  $self->{prefix_} = $prefix;
  $self->{entities_} = ();

  $self->create_case_data($args);

  return $self;
}

1;

package ChannelImpInventoryTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant COUNT => 30; #not made it less !!!
use POSIX qw(floor);

sub ipms_scenario
{
  my ($self, $ns) = @_;

  my ($cpm, $cpc, $cpa) = (10, 20, 30);

  $ns->output("ImpsScenario/CPM", $cpm);
  $ns->output("ImpsScenario/CPC", $cpc);
  $ns->output("ImpsScenario/CPA", $cpa);

  my $acc = $ns->create(Account => {
    name => 'IMPS',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword1 = make_autotest_name($ns, "IMPS-1");
  my $keyword2 = make_autotest_name($ns, "IMPS-2");

  my $channel1 = $ns->create(
    DB::BehavioralChannel->blank(
      name => "IMPS-CH1",
      keyword_list => $keyword1,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $channel2 = $ns->create(
    DB::BehavioralChannel->blank(
      name => "IMPS-CH2",
      keyword_list => $keyword2,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $expression = $ns->create(DB::ExpressionChannel->blank(
    name => 'IMPS-EXPR',
    account_id => $acc,
    expression => 
      join('|', $channel1->channel_id, 
           $channel2->channel_id)));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 'IMPS1',
    account_id => $acc,
    campaigncreativegroup_cpm => 10,
    campaigncreativegroup_cpc => 20,
    campaigncreativegroup_cpa => 30,
    site_links => [{name => 'IMPS1'}],
    channel_id => $expression->channel_id()
    });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'IMPS2',
    account_id => $acc,
    channel_id => $channel2->channel_id,
    campaigncreativegroup_cpm => 10,
    campaigncreativegroup_cpc => 20,
    campaigncreativegroup_cpa => 30,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 'IMPS2'}] });

  my $tag1 = $ns->create(PricedTag => {
    name => 'IMPS1',
    site_id => $campaign1->{Site}[0]->{site_id},
    cpm => 1});

  my $tag2 = $ns->create(PricedTag => {
    name => 'IMPS2',
    site_id => $campaign2->{Site}[0]->{site_id},
    cpm => 1});

  $ns->output("ImpsScenario/Tags", 2, "list of tags");
  $ns->output("ImpsScenario/TAG1", $tag1);
  $ns->output("ImpsScenario/TAG2", $tag2);
  $ns->output("ImpsScenario/TagsEnd", 2, "list of tags");
  $ns->output("ImpsScenario/Keywords", 2, "list of keywords");
  $ns->output("ImpsScenario/KW1", $keyword1);
  $ns->output("ImpsScenario/KW2", $keyword2);
  $ns->output("ImpsScenario/KeywordsEnd", 2, "list of keywords");

  $ns->output("ImpsScenario/ReqChannels-0", 0, "list of channels of 1");
  $ns->output("ImpsScenario/CH", "0");
  $ns->output("ImpsScenario/ReqChannels-0End", 0, "list of channels of 1");
  $ns->output("ImpsScenario/ReqChannels-1", 0, "list of channels of 2");
  $ns->output("ImpsScenario/CH", "0");
  $ns->output("ImpsScenario/ReqChannels-1End", 0, "list of channels of 2");
  $ns->output("ImpsScenario/ReqCreatives-0", 1, "list of creatives of 1");
  $ns->output("ImpsScenario/CC1", $campaign1->{cc_id});
  $ns->output("ImpsScenario/ReqCreatives-0End", 1, "list of creatives of 1");
  $ns->output("ImpsScenario/ReqCreatives-1", 1, "list of creatives of 2");
  $ns->output("ImpsScenario/CC2", $campaign2->{cc_id});
  $ns->output("ImpsScenario/ReqCreatives-1End", 1, "list of creatives of 2");
  
  $ns->output("ImpsScenario/Channels", 3, "list of channels");
  $ns->output("ImpsScenario/CH1", $channel1->channel_id);
  $ns->output("ImpsScenario/CH2", $channel2->channel_id);
  $ns->output("ImpsScenario/CHE", $expression->channel_id());
  $ns->output("ImpsScenario/ChannelsEnd", 3, "list of channels");
  
  my $ch2_imps = floor(COUNT / 2);
  my $ch2_clicks = floor(COUNT / 2 / 3);
  my $ch2_actions = floor(COUNT / 2 / 3 / 5);
  my $ch2_revenue = $cpm * $ch2_imps / 1000 +
    $cpc * $ch2_clicks +
    $cpa * $ch2_actions;

  my @diff = (
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       revenue => $cpm * COUNT / 1000,
       imps_value => $cpm * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps_other => 2 * COUNT,
       imps_other_value => $cpm * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       clicks => $ch2_clicks,
       actions => $ch2_actions,
       revenue => $ch2_revenue,
       imps_value => $cpm * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps_other => 2 * COUNT,
       imps_other_value => $cpm * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       revenue => $cpm * COUNT / 1000,
       imps_value => $cpm * COUNT / 1000,
       imps_other => COUNT,
       imps_other_value => $cpm * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps_other => 4 * COUNT,
       imps_other_value => 2 * $cpm * COUNT / 1000 }) );

  ChannelImpInventoryTest::Diffs::output_diff($ns, "ImpsScenario", \@diff);

  $ns->output("ImpsScenario/COLO",
    DB::Defaults::instance()->ads_isp->{colo_id});
}

sub imps_other_scenario_channel
{
  my ($self, $ns) = @_;

  my $case = 
   ChannelImpInventoryTest::Case->new(
     $ns, "ImpsOtherChannel",
     { max_text_creatives => 3,
       keyword => "ImpsOtherChannel",
       display => { 
         tag_cpm => 1,
         cpm => 50 },
       channels => { 
         tag_cpm => 1,
         cpms => [20, 10] } });

  my $cpmD = $case->entity("DISPLAY/CPM") / 1000;
  my $min_ecpm = $case->entity("DISPLAY/MIN_ECPM") / 1000;
  my $revenue1 = $case->entity("CHANNEL/REVENUE1") / 1000;
  my $revenue2 = $case->entity("CHANNEL/REVENUE2") / 1000;

  my @diffs = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       revenue => COUNT * $cpmD,
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * $cpmD,
       imps_other => COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue1 + $revenue2) }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 5 * COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => 
        COUNT * ($revenue1 + $revenue2) +
        COUNT * $cpmD, 
      impops_no_imp => COUNT,
      impops_no_imp_user_count => COUNT,
      impops_no_imp_value => COUNT * $min_ecpm / 3  }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2 * COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => 
        COUNT * ($revenue1 + $revenue2) +
        COUNT * $cpmD }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => 2 * COUNT,
       revenue => COUNT * ($revenue1 + $revenue2),
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * ($revenue1 + $revenue2),
       imps_other => 3 * COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD,
       impops_no_imp => COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 3 }));

  $case->output(
   { cpp_tags => {
       "TAG1" => "DISPLAY/TAG",
       "TAG2" => "CHANNEL/TAG" },
     cpp_keywords => {
       "KW1" => "KWD",
       "KW2" => "KWD" },
     cpp_channels => {
       "CH1" => "DISPLAY/CH",
       "CH2" => "CHANNEL/CH" },
     cpp_checks => {
       channels => [ 
         { "CH" => "CHANNEL/CH" },
         { "CH" => "DISPLAY/CH" } ],
       creatives => [ 
         { "CC" => "DISPLAY/CC" },
         { "CC1" => "CHANNEL/CC1",
           "CC2" => "CHANNEL/CC2" } ] },
     cpp_diffs => \@diffs });

}

sub imps_other_scenario_keyword
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
     $ns, "ImpsOtherKeyword",
     { max_text_creatives => 4,
       publisher => { tag_cpm => 1 },
       display =>  { cpm => 1 },
       keywords => { cpc_bids => [20, 10, 5, 2] } });

  # +0.01 top eCPM correction (REQ-2849) 
  my $ctr = ChannelImpInventoryTest::Case::DEFAULT_CTR;
  my $revenue1 = ($case->entity("KEYWORD/REVENUE1") + 0.01 / $ctr) / 1000;
  my $revenue1_cpc = $case->entity("KEYWORD/REVENUE1") / 1000 + 0.01;

  my $revenue2 = $case->entity("KEYWORD/REVENUE2") / 1000;
  my $revenue3 = $case->entity("KEYWORD/REVENUE3") / 1000;
  my $revenue4 = $case->entity("KEYWORD/REVENUE4") / 1000;
  my $imp_revenue1 = $revenue1 / 100;
  my $imp_revenue2 = $revenue2 / 100;
  my $imp_revenue3 = $revenue3 / 100;
  my $imp_revenue4 = $revenue4 / 100;

  my @diffs = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
         COUNT * ($imp_revenue1 + $imp_revenue2 + 
                  $imp_revenue3 + $imp_revenue4)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       clicks => COUNT,
       revenue => COUNT * $revenue1_cpc,
       imps_user_count => COUNT,
       imps_value => COUNT * $imp_revenue1,
       impops_user_count => COUNT,
       imps_other => 3 * COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
          COUNT * ($imp_revenue2 + 
                   $imp_revenue3 + $imp_revenue4) }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
         COUNT * ($imp_revenue1 + $imp_revenue2 + 
                  $imp_revenue3 + $imp_revenue4)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       clicks => COUNT,
       revenue => COUNT * $revenue2,
       imps_user_count => COUNT,
       imps_value => COUNT * $imp_revenue2,
       impops_user_count => COUNT,
       imps_other => 3 * COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
          COUNT * ($imp_revenue1 + 
                   $imp_revenue3 + $imp_revenue4) }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
         COUNT * ($imp_revenue1 + $imp_revenue2 + 
                  $imp_revenue3 + $imp_revenue4)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       clicks => COUNT,
       revenue => COUNT * $revenue3,
       imps_user_count => COUNT,
       imps_value => COUNT * $imp_revenue3,
       impops_user_count => COUNT,
       imps_other => 3 * COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
          COUNT * ($imp_revenue1 + 
                   $imp_revenue2 + $imp_revenue4) }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
         COUNT * ($imp_revenue1 + $imp_revenue2 + 
                  $imp_revenue3 + $imp_revenue4)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       clicks => COUNT,
       revenue => COUNT * $revenue4,
       imps_user_count => COUNT,
       imps_value => COUNT * $imp_revenue4,
       impops_user_count => COUNT,
       imps_other => 3 * COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
         COUNT * ($imp_revenue1 + 
                  $imp_revenue2 + $imp_revenue3) }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => 
         COUNT * ($imp_revenue1 + $imp_revenue2 + 
                  $imp_revenue3 + $imp_revenue4)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 4 * COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value =>          
         COUNT * ($imp_revenue1 + $imp_revenue2 + 
                  $imp_revenue3 + $imp_revenue4)}) );
 
  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KW" => "DISPLAY/KWD,KEYWORD/KWD" },
     cpp_channels => {
       "CH1" => "KEYWORD/CH1",
       "CH2" => "KEYWORD/CH2",
       "CH3" => "KEYWORD/CH3",
       "CH4" => "KEYWORD/CH4",
       "CH5" => "DISPLAY/CH" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2",
           "CH3" => "KEYWORD/CH3",
           "CH4" => "KEYWORD/CH4",
           "CH5" => "DISPLAY/CH" } ],
       creatives => [ 
         { "CC1" => "KEYWORD/CC1",
           "CC2" => "KEYWORD/CC2",
           "CC3" => "KEYWORD/CC3",
           "CC4" => "KEYWORD/CC4" } ] },
     cpp_diffs => \@diffs });
}

# Display Creative Served in 2-slot banner.
sub imps_other_scenario_display
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
     $ns, "ImpsOtherDisplay",
     { max_text_creatives => 2,
       publisher => { tag_cpm => 1 },
       display =>  { cpm => 1500 },
       keywords => { cpc_bids => [20, 10] } });

  my $cpmD = $case->entity("DISPLAY/CPM") / 1000;

  my @diffs = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       revenue => COUNT * $cpmD,
       imps_user_count => COUNT,
       imps_value => COUNT * $cpmD,
       impops_user_count => COUNT }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}) );

  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KW" => "DISPLAY/KWD,KEYWORD/KWD" },
     cpp_channels => {
       "CH1" => "KEYWORD/CH1",
       "CH2" => "KEYWORD/CH2",
       "CH3" => "DISPLAY/CH" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2",
           "CH3" => "DISPLAY/CH" } ],
       creatives => [ 
         { "CC" => "DISPLAY/CC" } ] },
     cpp_diffs => \@diffs });
}

# Test 12. Channel Inventory with non-opted-in users,
# Test 12.2. User Without Cookies
sub nocookies
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
     $ns, "NoCookies",
     { max_text_creatives => 2,
       publisher => { tag_cpm => 1 },
       display => { cpm => 1500 }
     });

  my $cpmD = $case->entity("DISPLAY/CPM") / 1000;

  my @diffs = (
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       imps => COUNT,
       clicks => COUNT,
       revenue => COUNT * $cpmD,
       impops_user_count => 0,
       imps_user_count => 0,
       imps_value => COUNT * $cpmD,
       imps_other => 0, 
       imps_other_user_count => 0,
       imps_other_value => 0}),
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       impops_user_count => 0,
       imps_other => 2*COUNT, 
       imps_other_user_count => 0,
       imps_other_value => COUNT * $cpmD}) );

  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KW" => "DISPLAY/KWD" },
     cpp_channels => {
       "CH" => "DISPLAY/CH"
       },
     cpp_checks => {
       channels => [ 
         { "CH" => "DISPLAY/CH"} ],
       creatives => [ 
         { "CC" => "DISPLAY/CC"} ] },
     cpp_diffs => \@diffs });
}

# Test 12. Channel Inventory with non-opted-in users,
# Test 12.1. Opt-out User
sub oouser
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
     $ns, "OOUser",
     { max_text_creatives => 2,
       publisher => { tag_cpm => 1 },
       display => { cpm => 1500 }
     });

  my $cpmD = $case->entity("DISPLAY/CPM") / 1000;

  my @diffs = (
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       imps => COUNT,
       clicks => COUNT,
       revenue => COUNT * $cpmD,
       impops_user_count => 0,
       imps_user_count => 0,
       imps_value => COUNT * $cpmD,
       imps_other => 0, 
       imps_other_user_count => 0,
       imps_other_value => 0
     }),
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       impops_user_count => 0,
       imps_other => 2*COUNT, 
       imps_other_user_count => 0,
       imps_other_value => COUNT * $cpmD
     }) );

  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KW" => "DISPLAY/KWD" },
     cpp_channels => {
       "CH" => "DISPLAY/CH"
       },
     cpp_checks => {
       channels => [ 
         { "CH" => "DISPLAY/CH"} ],
       creatives => [ 
         { "CC" => "DISPLAY/CC"} ] },
     cpp_diffs => \@diffs });
  $ns->output("OOUser/COLO", DB::Defaults::instance()->ads_isp->{colo_id});
}

# Trak Display Creative Served in 2-slot banner.
sub not_verified_impressions
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
     $ns, "NotVerifiedImpressions",
     { max_text_creatives => 2,
       publisher => { tag_cpm => 1 },
       display_track => { cpm => 1500 }
     });

  my $cpmD = $case->entity("DISPLAY_TRACK/CPM") / 1000;

  my @diffs = (
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       imps => COUNT,
       imps_user_count => COUNT,
       impops_user_count => COUNT,
       imps_value => COUNT * $cpmD,
       imps_other => 0, 
       imps_other_user_count => 0,
       imps_other_value => 0}),
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       impops_user_count => COUNT,
       imps_other => 2*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}),
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       imps => COUNT,
       imps_user_count => COUNT,
       impops_user_count => COUNT,
       imps_value => COUNT * $cpmD,
       imps_other => 0, 
       imps_other_user_count => 0,
       imps_other_value => 0}),
   ChannelImpInventoryTest::Diffs::create_diff(
     {
       impops_user_count => COUNT,
       imps_other => 2*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $cpmD}) );

  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KW" => "DISPLAY_TRACK/KWD" },
     cpp_channels => {
       "CH1" => "DISPLAY_TRACK/CHH",
       "CH2" => "DISPLAY_TRACK/CHE"
       },
     cpp_checks => {
       channels => [ 
         { "CH" => "DISPLAY_TRACK/CHH"} ],
       creatives => [ 
         { "CC" => "DISPLAY_TRACK/CC"} ] },
     cpp_diffs => \@diffs });
}

sub no_imps_scenario {
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "NoImps",
      { max_text_creatives => 2,
        publisher => { tag_cpm => 10},
        display => { cpm => 1 } });

  my $min_ecpm = $case->entity("COMMON/MIN_ECPM") / 1000;

  $case->output(
   { cpp_tags => {
       "TAG1" => "COMMON/TAG" },
     cpp_keywords => {
       "KW1" => "DISPLAY/KWD" },
     cpp_channels => {
       "CH1" => "DISPLAY/CH" },
     cpp_checks => {
       channels => [ 
         { "CH" => "DISPLAY/CH" } ],
       creatives => [ {} ] },
     cpp_diffs => [
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       impops_no_imp => COUNT,
       impops_no_imp_user_count => COUNT, 
       impops_no_imp_value => COUNT * $min_ecpm }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT, 
       impops_no_imp_value => COUNT * $min_ecpm})] });
}

sub no_imps_opportunity_scenario {
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "NoImpsOpportunity",
      { max_text_creatives => 2,
        publisher => { tag_cpm => 10},
        display => { cpm => 1 } });

  $case->output(
   { cpp_tags => {
       "TAG1" => "COMMON/TAG" },
     cpp_keywords => {
       "KW1" => "DISPLAY/KWD" },
     cpp_channels => {
       "CH1" => "DISPLAY/CH" },
     cpp_checks => {
       channels => [ 
         { "CH" => "DISPLAY/CH" } ],
       creatives => [ {} ] },
     cpp_diffs => [
   ChannelImpInventoryTest::Diffs::create_diff(
     {}),
   ChannelImpInventoryTest::Diffs::create_diff(
     {})] });
}

# Channel Inventory for Text Advertising Test Plan
# https://confluence.ocslab.com/display/QA/Channel+Inventory+for+Text+Advertising+Test+Plan

# 1 Text Creative Served in 4-slot banner
sub simple_1keywords {
  my ($self, $ns) = @_;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "1Keyword",
      { max_text_creatives => 4,
        keywords => { tag_cpm => 10,
                      cpc_bids => [30, undef] } });

  # Changed for REQ-2790
  my $min_ecpm = $case->entity("KEYWORD/MIN_ECPM") / 1000;
  my $revenue = $case->entity("KEYWORD/REVENUE1") / 1000 / 100;

  my @diff = (
    ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue}),
    ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue,
       impops_no_imp => 3 * COUNT, 
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => 3 * COUNT * $min_ecpm / 4}),
    ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue}),
    ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue,
       impops_no_imp => 3 * COUNT, 
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => 3 * COUNT * $min_ecpm / 4 } ));


  $case->output(
   { cpp_tags => {
       "TAG" => "KEYWORD/TAG" },
     cpp_keywords => {
       "KWD" => "KEYWORD/KWD" },
     cpp_channels => {
       "CH1" => "KEYWORD/CH1",
       "CH2" => "KEYWORD/CH2" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2" } ],
       creatives => [ 
         { "CC" => "KEYWORD/CC1" } ] },
     cpp_diffs => \@diff });
}

# 2 Text Creatives Served in 4-slot banner

# Tag cpm + margin taken as impression opportunity value
sub simple_2keywords_opportunity {
  my ($self, $ns) = @_;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "2KeywordsOpportunity",
      { max_text_creatives => 4,
        keywords => { tag_cpm => 10,
                      cpc_bids => [100, 5, undef, undef] } });

  my $min_ecpm = $case->entity("KEYWORD/MIN_ECPM") / 1000;
  # revenue1 == revenue2. Winner pay looser price !!
  my $ctr = ChannelImpInventoryTest::Case::DEFAULT_CTR;
  my $revenue2 = $case->entity("KEYWORD/REVENUE2") / 1000 / 100;
  # +0.01 top eCPM correction (REQ-2849) 
  my $revenue1 = $revenue2  + 0.01 / 1000; 

  my @diff = (
    ChannelImpInventoryTest::Diffs::create_diff({
        impops_user_count => COUNT,
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2) }),

    ChannelImpInventoryTest::Diffs::create_diff({
        imps => COUNT, 
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value => COUNT * $revenue1, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * $revenue2,
        impops_no_imp => 2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2}),
    #2
    ChannelImpInventoryTest::Diffs::create_diff({
        impops_user_count => COUNT,
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2) }),

    ChannelImpInventoryTest::Diffs::create_diff({
        imps => COUNT,
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value =>  COUNT * $revenue2, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * $revenue1,
        impops_no_imp => 2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2}),
    #3
    ChannelImpInventoryTest::Diffs::create_diff({
        impops_user_count => COUNT,
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2)}),

    ChannelImpInventoryTest::Diffs::create_diff({
        impops_user_count => COUNT,
        imps_other => 2 * COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp =>  2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2}),
     #4
    ChannelImpInventoryTest::Diffs::create_diff({
        impops_user_count => COUNT,
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2) }),

    ChannelImpInventoryTest::Diffs::create_diff({
        impops_user_count => COUNT,
        imps_other => 2 * COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp => 2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2})
  );

  $case->output(
   { cpp_tags => {
       "TAG" => "KEYWORD/TAG" },
     cpp_keywords => {
       "KWD" => "KEYWORD/KWD" },
     cpp_channels => {
         "CH1" => "KEYWORD/CH1",
         "CH2" => "KEYWORD/CH2",
         "CH3" => "KEYWORD/CH3",
         "CH4" => "KEYWORD/CH4" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2",
           "CH3" => "KEYWORD/CH3",
           "CH4" => "KEYWORD/CH4" } ],
       creatives => [ 
         { "CC1" => "KEYWORD/CC1",
           "CC2" => "KEYWORD/CC2"} ] },
     cpp_diffs => \@diff });
}

# Display CCG eCPM taken as impression opportunity value
sub simple_2keywords_display {
  my ($self, $ns) = @_;
  my $ctr = 0.1;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "2KeywordsDisplay",
      { max_text_creatives => 4,
        publisher => {tag_cpm => 10},
        display => { cpm => 20 },
        keywords => { ctr => $ctr,
                      cpc_bids => [1, 0.75] } });

  my $min_ecpm = $case->entity("DISPLAY/CPM") / 1000;
  my $revenue2 = $case->entity("KEYWORD/REVENUE1") / 1000 / 100;
  # +0.01 top eCPM correction (REQ-2849) 
  my $revenue1 = $revenue2  + 0.01 / 1000; 

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue2 + $revenue1)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue1,
       impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue2,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue2 + $revenue1)}),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue2,
       impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue1,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2}),
  );

  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KWD" => "KEYWORD/KWD,DISPLAY/KWD," },
     cpp_channels => {
       "CH1" => "KEYWORD/CH1",
       "CH2" => "KEYWORD/CH2" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2",
           "CH3" => "DISPLAY/CH" } ],
       creatives => [ 
         { "CC1" => "KEYWORD/CC1",
           "CC2" => "KEYWORD/CC2" } ] },
     cpp_diffs => \@diff });
}

# Top ad ecpm calculated as the difference to cover minimum cpm
sub simple_2keywords_difference {
  my ($self, $ns) = @_;
  my $ctr = 0.1;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "2KeywordsDifference",
      { max_text_creatives => 4,
        keywords => { tag_cpm => 145,
                      ctr => $ctr, 
                      cpc_bids => [1, 0.7, undef, undef] } });

  my $min_ecpm = $case->entity("KEYWORD/MIN_ECPM") / 1000;
  my $revenue2 = $case->entity("KEYWORD/REVENUE2") / 1000 / 100;
  # Winner pay difference between tag price and looser prices.
  my $revenue1 = $min_ecpm - $revenue2;

  my @diff = (
    ChannelImpInventoryTest::Diffs::create_diff({
        imps => 0, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => 0,
        imps_value => 0, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp => 0,
        impops_no_imp_user_count => 0, 
        impops_no_imp_value => 0}),

    ChannelImpInventoryTest::Diffs::create_diff({
        imps => COUNT, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value => COUNT * $revenue1, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * $revenue2,
        impops_no_imp => 2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2}),
    #2
    ChannelImpInventoryTest::Diffs::create_diff({
        imps => 0, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => 0,
        imps_value => 0, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp => 0,
        impops_no_imp_user_count => 0, 
        impops_no_imp_value => 0}),

    ChannelImpInventoryTest::Diffs::create_diff({
        imps => COUNT, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value =>  COUNT * $revenue2, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * $revenue1,
        impops_no_imp => 2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2}),
    #3
    ChannelImpInventoryTest::Diffs::create_diff({
        imps => 0, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => 0,
        imps_value => 0, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp => 0,
        impops_no_imp_user_count => 0, 
        impops_no_imp_value => 0}),

    ChannelImpInventoryTest::Diffs::create_diff({
        imps => 0, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => 0,
        imps_value => 0, 
        imps_other => 2 * COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp =>  2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2}),
     #4
    ChannelImpInventoryTest::Diffs::create_diff({
        imps => 0, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => 0,
        imps_value => 0, 
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp => 0,
        impops_no_imp_user_count => 0, 
        impops_no_imp_value => 0}),

    ChannelImpInventoryTest::Diffs::create_diff({
        imps => 0, clicks => 0, actions => 0, revenue => 0,
        impops_user_count => COUNT,
        imps_user_count => 0,
        imps_value => 0, 
        imps_other => 2 * COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => COUNT * ($revenue1 + $revenue2),
        impops_no_imp => 2 * COUNT,
        impops_no_imp_user_count => COUNT, 
        impops_no_imp_value => COUNT * $min_ecpm / 2})
  );


  $case->output(
   { cpp_tags => {
       "TAG" => "KEYWORD/TAG" },
     cpp_keywords => {
       "KWD" => "KEYWORD/KWD" },
     cpp_channels => {
         "CH1" => "KEYWORD/CH1",
         "CH2" => "KEYWORD/CH2",
         "CH3" => "KEYWORD/CH3",
         "CH4" => "KEYWORD/CH4" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2",
           "CH3" => "KEYWORD/CH3",
           "CH4" => "KEYWORD/CH4" } ],
       creatives => [ 
         { "CC1" => "KEYWORD/CC1",
           "CC2" => "KEYWORD/CC2"} ] },
     cpp_diffs => \@diff });
}

# No Ad Served in 4-slot banner
sub no_ad_keyword {
  my ($self, $ns) = @_;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "NoAdKeyword",
      { max_text_creatives => 4,
        keywords => { tag_cpm => 10,
                      cpc_bids => [undef, undef] } });

  my $min_ecpm = $case->entity("KEYWORD/MIN_ECPM") / 1000;

  my @diff = (
  ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       impops_no_imp => COUNT, 
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm }),
  ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       impops_no_imp => 4*COUNT, 
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm }),
  ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       impops_no_imp => COUNT, 
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm }),
  ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       impops_no_imp => 4*COUNT, 
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm }) );

  $case->output(
   { cpp_tags => {
       "TAG" => "KEYWORD/TAG" },
     cpp_keywords => {
       "KWD" => "KEYWORD/KWD" },
     cpp_channels => {
       "CH1" => "KEYWORD/CH1",
       "CH2" => "KEYWORD/CH2" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1",
           "CH2" => "KEYWORD/CH2" } ],
       creatives => [{}] },
     cpp_diffs => \@diff });
}

# 2 Text Creatives Served on the same keyword in 4-slot banner
sub case_1channel_2keywords {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => '1Channel2KeywordsCommon',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = 
    make_autotest_name($ns, "1ChannelTextDisplay-Common");

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "KEYWORD-Common",
    account_id => $account,
    keyword_list => $keyword,
    channel_type => 'K',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P")] ));
  
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "1Channel2Keywords",
      { max_text_creatives => 4,
        publisher => { tag_cpm => 1 },
        keywords =>  [{cpc_bids => [undef, undef]},
                      {channel => $channel, cpc_bids => [4, 3]}
                      ]});

  $case->store("COMMON/KWD", $keyword);
  $case->store("COMMON/CH", $channel->{channel_id});
  
  my $min_ecpm = $case->entity("COMMON/MIN_ECPM") / 1000;

  # +0.01 top eCPM correction (REQ-2849)
  my $revenue1 = $case->entity("KEYWORD2/REVENUE1") / 1000 / 100  + 0.01 / 1000;
  my $revenue1_cpc = $case->entity("KEYWORD2/REVENUE1") / 1000 + 0.01;
  my $revenue2 = $case->entity("KEYWORD2/REVENUE2") / 1000 / 100;

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue1 + $revenue2)} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => 2 * COUNT,
       clicks => 2 * COUNT,
       revenue => COUNT * ($revenue1_cpc + $revenue2 * 100),
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * ($revenue1 + $revenue2),
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue1 + $revenue2)} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2 * COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue1 + $revenue2),
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue1 + $revenue2)} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2 * COUNT,
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($revenue1 + $revenue2),
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ) );


  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KWD" => "COMMON/KWD,KEYWORD1/KWD"},
     cpp_channels => {
       "CH2" => "KEYWORD1/CH1",
       "CH3" => "KEYWORD1/CH2",
       "CH1" => "COMMON/CH"},
     cpp_checks => {
       channels => [ 
         { "CH2" => "KEYWORD1/CH1",
           "CH3" => "KEYWORD1/CH2",
           "CH1" => "COMMON/CH"} ],
       creatives => [ 
         { "CC1" => "KEYWORD2/CC1",
           "CC2" => "KEYWORD2/CC2"} ] },
     cpp_diffs => \@diff });
}

# Channel Targeted Text CCGs
sub channel_targeted {
  my ($self, $ns) = @_;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "ChannelTargeted",
      { max_text_creatives => 4,
        publisher => { tag_cpm => 0},
        channels => { cpms => [6, 5] } });

  my $min_ecpm = $case->entity("COMMON/MIN_ECPM") / 1000;
  my $revenue1 = $case->entity("CHANNEL/REVENUE1") / 1000;
  my $revenue2 = $case->entity("CHANNEL/REVENUE2") / 1000;

  $case->create_channel('NOAD', 'B', 
                        { keyword => 'NOAD'});

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue1 + COUNT * $revenue2} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       revenue => COUNT * $revenue1,
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue1,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue2,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue1 + COUNT * $revenue2} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       revenue => COUNT * $revenue2,
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue2,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue1,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue1 + COUNT * $revenue2} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 2*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue1 + COUNT * $revenue2,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ) );


  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KWD" => "CHANNEL/KWD,NOAD/KWD" },
     cpp_channels => {
       "CH1" => "CHANNEL/CH1",
       "CH2" => "CHANNEL/CH2",
       "CH3" => "NOAD/CH" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "CHANNEL/CH1" },
         { "CH2" => "CHANNEL/CH2" },
         { "CH2" => "NOAD/CH" } ],
       creatives => [ 
         { "CC1" => "CHANNEL/CC1" },
         { "CC2" => "CHANNEL/CC1" }]},
     cpp_diffs => \@diff });
}

# Keyword & Channel Targeted Text CCGs
sub keyword_vs_channel_targeted {
  my ($self, $ns) = @_;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "KeywordvsChannelTargeted",
      { max_text_creatives => 4,
        publisher => { tag_cpm => 0},
        keywords => { 
          ctr => 0.1, 
          cpc_bids => [1]},
        channels => { cpms => [5]} });

  my $min_ecpm = $case->entity("COMMON/MIN_ECPM") / 1000;
  my $revenue2 = $case->entity("CHANNEL/REVENUE1") / 1000;
  # Winner pay looser price (see case data)
  # +0.01 top eCPM correction (REQ-2849)
  my $imp_revenue1 = $revenue2 + 0.01 / 1000; 

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $imp_revenue1 + COUNT * $revenue2} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * $imp_revenue1,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue2,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $imp_revenue1  + COUNT * $revenue2} ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT,
       revenue => COUNT * $revenue2,
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue2,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $imp_revenue1,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2 } ) );


  $case->output(
   { cpp_tags => {
       "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KWD" => "CHANNEL/KWD,KEYWORD/KWD" },
     cpp_channels => {
       "CH1" => "KEYWORD/CH1",
       "CH2" => "CHANNEL/CH1" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "KEYWORD/CH1" },
         { "CH2" => "CHANNEL/CH1" } ],
       creatives => [ 
         { "CC" => "KEYWORD/CC1" },
         { "CC" => "CHANNEL/CC1" } ] },
     cpp_diffs => \@diff });
}

sub case_1channel_text_display {
  my ($self, $ns) = @_;
  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "1ChannelTextDisplay",
      { max_text_creatives => 4,
        keyword => "Keyword",
        publisher => { tag_cpm => 0 },
        channels => {
          cpms => [1.1, 0.9] } });

  my $account = $ns->create(Account => {
    name => '1ChannelTextDisplay',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = 
    make_autotest_name($ns, "1ChannelTextDisplay-Display");

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => "1ChannelTextDisplay-Display",
      keyword_list => $keyword,
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $expression = $ns->create(DB::ExpressionChannel->blank(
    name => "1ChannelTextDisplay-Expr",
    account_id => $account,
    expression => 
      join('&', $channel->channel_id, 
           $case->entity("CHANNEL/CH"))));

  $case->create_display_campaign('DISPLAY',
    { publisher => { site_id => $case->entity("COMMON/SITE")},
      size => $case->entity("SIZE"),
      channel => $expression,
      cpm => 2.5 });

  $case->store("DISPLAY/KWD", $keyword);
  $case->store("DISPLAY/CH", $channel->{channel_id});

  my $min_ecpm = $case->entity("COMMON/MIN_ECPM") / 1000;
  my $ch_revenue1 = $case->entity("CHANNEL/REVENUE1") / 1000;
  my $ch_revenue2 = $case->entity("CHANNEL/REVENUE2") / 1000;
  my $disp_revenue = $case->entity("DISPLAY/CPM") / 1000;

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       revenue => COUNT * $disp_revenue, 
       imps_user_count => COUNT,
       imps_value => COUNT * $disp_revenue,
       impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * ($ch_revenue1 + $ch_revenue2) } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => 2*COUNT, 
       revenue => COUNT * ($ch_revenue1 + $ch_revenue2),
       impops_user_count => COUNT,
       imps_user_count => COUNT,
       imps_value => COUNT * ($ch_revenue1 + $ch_revenue2),,
       imps_other => 4*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $disp_revenue,
       impops_no_imp => 2 * COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm / 2  } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { imps => COUNT, 
       revenue => COUNT * $disp_revenue, 
       imps_user_count => COUNT,
       imps_value => COUNT * $disp_revenue,
       impops_user_count => COUNT } ),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 4*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $disp_revenue } ));


  $case->output(
   { cpp_tags => {
       "TAG1" => "COMMON/TAG",
       "TAG2" => "COMMON/TAG" },
     cpp_keywords => {
       "KWD1" => "KWD",
       "KWD2" => "KWD,DISPLAY/KWD" },
     cpp_channels => {
       "CH1" => "CHANNEL/CH",
       "CH2" => "DISPLAY/CH" },
     cpp_checks => {
       channels => [ 
         { "CH" => "CHANNEL/CH" },
         { "CH1" => "CHANNEL/CH",
           "CH2" => "DISPLAY/CH" } ],
       creatives => [ 
         { "CC1" => "CHANNEL/CC1",
           "CC2" => "CHANNEL/CC2" },
         { "CC"  => "DISPLAY/CC" } ] 
    },
    cpp_diffs => \@diff });
}


# Channel Inventory Test Plan
# https://confluence.ocslab.com/display/QA/Channel+Inventory+Test+Plan

# Channel Inventory on tags which can't serve text ads
sub tag_not_serve_text_ads
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "NotServeTextAds",
      { max_text_creatives => 4,
        publisher => { tag_cpm => 0 },
        display => { cpm => 5 } });

  # Reject 'text ads' category

  my $category = $ns->create(CreativeCategory =>
    { name => "RejectTextAds",
      cct_id => 0 });

  $ns->create(SiteCreativeCategoryExclusion =>
    { site_id =>  $case->entity('COMMON/SITE'),
      approval => 'R',
      creative_category_id => $category });

  my $revenue = $case->entity("DISPLAY/CPM") / 1000;

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { revenue => COUNT * $revenue,
       impops_user_count => COUNT,
       imps => COUNT, 
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue     
     }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 4*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue
     }));

  $case->output(
   { cpp_tags => {
      "TAG" => "COMMON/TAG" },
     cpp_keywords => {
       "KWD" => "DISPLAY/KWD" },
     cpp_channels => {
       "CH" => "DISPLAY/CH" },
     cpp_checks => {
       channels => [ 
         { "CH" => "DISPLAY/CH" } ],
       creatives => [ 
         { "CC"  => "DISPLAY/CC" } ] 
   },
   cpp_diffs => \@diff });  

}

# Campaigns, tags with non-system currency
sub non_default_currency
{
  my ($self, $ns) = @_;

  my $pub_rate = 5;
  my $adv_rate = 10;

  my $pub_currency = 
    $ns->create(
      Currency => { rate => $pub_rate });

  my $adv_currency = 
    $ns->create(
      Currency => { rate => $adv_rate });

  my $case = 
    ChannelImpInventoryTest::Case->new(
      $ns, "NonDefaultCurrency",
      { max_text_creatives => 4,
        publisher => 
          { tag_cpm => 0,
            currency => $pub_currency->{currency_id} },
        display => 
          { cpm => 300,
            currency => $adv_currency->{currency_id} 
          } 
      });

  $case->create_channel(
    'UNLINKED', 'B',
     { account => $case->entity("DISPLAY/ACCOUNT"),
       keyword => $case->entity("DISPLAY/KWD")
     } );


  my $size = $ns->create(CreativeSize => {
    name => 'NonDefaultCurrency-BIGCPM-Size',
    max_text_creatives => 2 });

  my $tag = $ns->create(PricedTag => {
    name => 'NonDefaultCurrency-BIGCPM-Tag',
    site_id => $case->entity("COMMON/SITE"),
    size_id => $size,
    cpm => 100 });  

  $case->store("BIGCPM/TAG", $tag->{tag_id});

  my $min_ecpm =  (100 / $pub_rate) / 1000;
  my $revenue = $case->entity("DISPLAY/CPM") / 1000 / $adv_rate;

  my @diff = (
   ChannelImpInventoryTest::Diffs::create_diff(
     { revenue => COUNT * $revenue,
       impops_user_count => COUNT,
       imps => COUNT, 
       imps_user_count => COUNT,
       imps_value => COUNT * $revenue,
       impops_no_imp => COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm 
     }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 4*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue,
       impops_no_imp => 2*COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm 
     }),
   ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue,
       impops_no_imp => COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm 
     }),
      ChannelImpInventoryTest::Diffs::create_diff(
     { impops_user_count => COUNT,
       imps_other => 4*COUNT, 
       imps_other_user_count => COUNT,
       imps_other_value => COUNT * $revenue,
       impops_no_imp => 2*COUNT,
       impops_no_imp_user_count => COUNT,
       impops_no_imp_value => COUNT * $min_ecpm 
     }));

  $case->output(
   { cpp_tags => {
      "TAG1" => "COMMON/TAG",
      "TAG2" => "BIGCPM/TAG" },
     cpp_keywords => {
       "KWD1" => "DISPLAY/KWD",
       "KWD2" => "DISPLAY/KWD" },
     cpp_channels => {
       "CH1" => "DISPLAY/CH",
       "CH2" => "UNLINKED/CH" },
     cpp_checks => {
       channels => [ 
         { "CH1" => "DISPLAY/CH",
           "CH2" => "UNLINKED/CH" },
         { "CH1" => "DISPLAY/CH",
           "CH2" => "UNLINKED/CH" }],
       creatives => [ 
         { "CC"  => "DISPLAY/CC" }, {} ]
   },
   cpp_diffs => \@diff });  
}

sub tag_with_adjustment
{
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 'account',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $d_keyword = make_autotest_name($ns, "dkeyword");
  my $d_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "DChannel",
    account_id => $account,
    keyword_list => $d_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P")] ));

  my $ct_keyword = make_autotest_name($ns, "ctkeyword");
  my $ct_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "CTChannel",
    account_id => $account,
    keyword_list => $ct_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P")] ));

  my $k_keyword = make_autotest_name($ns, "kkeyword");
  my $k_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "KChannel",
    channel_type => 'K',
    account_id => $account,
    keyword_list => $k_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P")] ));

  my $ctr = 0.1;
  my $adjustment = 2;
  my $cpc = 0.13;

  $ns->output("TAG_CPM",
    $adjustment * $cpc * 100 * $ctr * 1000);

  my $d_campaign = $ns->create(DisplayCampaign =>
    { name => 'DCampaign',
      channel_id => $d_channel,
      account_id => $account,
      campaigncreativegroup_flags => 0,
      campaign_flags => 0,
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpm => 0,
      campaigncreativegroup_cpc => $cpc,
      campaigncreativegroup_ctr => $ctr } );

  my $ct_campaign = $ns->create(ChannelTargetedTACampaign =>
    { name => 'CTCampaign',
      size_id => DB::Defaults::instance()->text_size,
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $ct_channel,
      account_id => $account,
      campaigncreativegroup_flags => 0,
      campaign_flags => 0,
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpm => 0,
      campaigncreativegroup_cpc => $cpc,
      campaigncreativegroup_ctr => $ctr } );

  my $k_campaign = $ns->create(TextAdvertisingCampaign =>
    { name => 'KCampaign',
      size_id => DB::Defaults::instance()->text_size,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_flags => 0,
      campaign_flags => 0,
      original_keyword => $k_keyword,
      ccgkeyword_channel_id => $k_channel,
      max_cpc_bid => $cpc,
      ccgkeyword_ctr => $ctr } );

  my $tag_cpm = 20;

  my $publisher = $ns->create(Publisher =>
    { name => 'Publisher',
      pricedtag_cpm => $tag_cpm,
      pricedtag_adjustment => $adjustment } );

  $ns->create(DB::Tag_TagSize->blank(
      tag_id => $publisher->{tag_id},
      size_id => DB::Defaults::instance()->text_size ));

  $ns->output("TagAdjustment/Tags", 3, "list of tags");
  $ns->output("TagAdjustment/TAG1", $publisher->{tag_id});
  $ns->output("TagAdjustment/TAG2", $publisher->{tag_id});
  $ns->output("TagAdjustment/TAG3", $publisher->{tag_id});
  $ns->output("TagAdjustment/TagsEnd", 3, "list of tags");
  $ns->output("TagAdjustment/Keywords", 3, "list of keywords");
  $ns->output("TagAdjustment/KW1", $d_keyword);
  $ns->output("TagAdjustment/KW2", $ct_keyword);
  $ns->output("TagAdjustment/KW2", $k_keyword);
  $ns->output("TagAdjustment/KeywordsEnd", 3, "list of keywords");

  $ns->output("TagAdjustment/ReqChannels-0", 1, "list of channels of 1");
  $ns->output("TagAdjustment/CH", "0");
  $ns->output("TagAdjustment/ReqChannels-0End", 0, "list of channels of 1");
  $ns->output("TagAdjustment/ReqChannels-1", 0, "list of channels of 2");
  $ns->output("TagAdjustment/CH", "0");
  $ns->output("TagAdjustment/ReqChannels-1End", 0, "list of channels of 2");
  $ns->output("TagAdjustment/ReqChannels-2", 0, "list of channels of 3");
  $ns->output("TagAdjustment/CH", "0");
  $ns->output("TagAdjustment/ReqChannels-2End", 0, "list of channels of 3");
  $ns->output("TagAdjustment/ReqCreatives-0", 1, "list of creatives of 1");
  $ns->output("TagAdjustment/CC1", $d_campaign->{cc_id});
  $ns->output("TagAdjustment/ReqCreatives-0End", 1, "list of creatives of 1");
  $ns->output("TagAdjustment/ReqCreatives-1", 1, "list of creatives of 2");
  $ns->output("TagAdjustment/CC2", $ct_campaign->{cc_id});
  $ns->output("TagAdjustment/ReqCreatives-1End", 1, "list of creatives of 2");
  $ns->output("TagAdjustment/ReqCreatives-2", 1, "list of creatives of 3");
  $ns->output("TagAdjustment/CC3", $k_campaign->{cc_id});
  $ns->output("TagAdjustment/ReqCreatives-2End", 1, "list of creatives of 3");

  $ns->output("TagAdjustment/Channels", 3, "list of channels");
  $ns->output("TagAdjustment/CH1", $d_channel->channel_id);
  $ns->output("TagAdjustment/CH2", $ct_channel->channel_id);
  $ns->output("TagAdjustment/CH3", $k_channel->channel_id());
  $ns->output("TagAdjustment/ChannelsEnd", 3, "list of channels");

  my @diff = (
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps => COUNT,
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value => $adjustment * $cpc * $ctr * 1000 * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_other => 2 * COUNT, # 2 is a number of slots in tag
        imps_other_user_count => COUNT,
        imps_other_value => $adjustment * $cpc * $ctr * 1000 * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => $adjustment * $cpc * $ctr * 1000 * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps => COUNT,
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value => $adjustment * $cpc * $ctr * 1000 * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_other => COUNT,
        imps_other_user_count => COUNT,
        imps_other_value => $adjustment * $cpc * $ctr * 1000 * COUNT / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps => COUNT,
        impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_value => $adjustment * $cpc * $ctr * 1000 * COUNT / 1000 } ) );

  ChannelImpInventoryTest::Diffs::output_diff($ns, "TagAdjustment", \@diff);
}

sub different_colo
{
  my ($self, $ns) = @_;
  my $slots = 2;
  my $case = ChannelImpInventoryTest::Case->new($ns, "DifferentColo",
    { max_text_creatives => $slots,
      keyword => "Keyword",
      publisher => { tag_cpm => 0 },
      display => { cpm => 10 } });

  my $account_id = $case->entity("DISPLAY/ACCOUNT");
  my $channel_id = $case->create_channel('DISPLAY2', 'B', {account => $account_id})->{channel_id};
  my $expression = $case->create_expchannel('DISPLAY2', {account => $account_id}, $channel_id, "E");

  $case->create_display_campaign('DISPLAY2',
    { publisher => { site_id => $case->entity("COMMON/SITE")},
      size => $case->entity("SIZE"),
      channel => $expression,
      cpm => 5 });

  $case->create_publisher("NOIMP",
    { tag_cpm => 1,
      size => $case->entity("SIZE") });

  $ns->output("DifferentColo/COLO",
    DB::Defaults::instance()->ads_isp->{colo_id});

  my $cpm1 = $case->entity("DISPLAY/CPM");
  my $cpm2 = $case->entity("DISPLAY2/CPM");
  my $min_ecpm = $case->entity("NOIMP/MIN_ECPM");

  my @diff = (
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps => COUNT,
        revenue => $cpm2 * COUNT / 1000,
        imps_value => $cpm2 * COUNT / 1000,
        imps_other => COUNT,
        imps_other_value => $cpm1 * COUNT / 1000,
        impops_no_imp => COUNT,
        impops_no_imp_value => COUNT * $min_ecpm / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps_other => 2 * $slots * COUNT, # 2 requests in case
        imps_other_value => ($cpm1 + $cpm2) * COUNT / 1000,
        impops_no_imp => $slots * COUNT,
        impops_no_imp_value => COUNT * $min_ecpm / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps => COUNT,
        revenue => $cpm2 * COUNT / 1000,
        imps_value => $cpm2 * COUNT / 1000,
        imps_other => COUNT,
        imps_other_value => $cpm1 * COUNT / 1000,
        impops_no_imp => COUNT,
        impops_no_imp_value => COUNT * $min_ecpm / 1000 }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { imps_other => 2 * $slots * COUNT, # 2 requests in case
        imps_other_value => ($cpm1 + $cpm2) * COUNT / 1000,
        impops_no_imp => $slots * COUNT,
        impops_no_imp_value => COUNT * $min_ecpm / 1000 }));

  $case->output(
    { cpp_tags => {
        "TAG1" => "NOIMP/TAG",
        "TAG2" => "COMMON/TAG",
        "TAG3" => "COMMON/TAG" },
      cpp_keywords => {
        "KWD1" => "DISPLAY2/KWD",
        "KWD2" => "DISPLAY2/KWD",
        "KWD3" => "DISPLAY/KWD,DISPLAY2/KWD" },
      cpp_channels => {
        "CH1" => "DISPLAY2/CH",
        "CH2" => "DISPLAY2/CHE" },
      cpp_checks => {
        channels => [
          { "CH1" => "DISPLAY2/CH" },
          { "CH2" => "DISPLAY2/CH" },
          { "CH31" => "DISPLAY/CH",
            "CH32" => "DISPLAY2/CH" } ],
        creatives => [
          {},
          { "CC2" => "DISPLAY2/CC" },
          { "CC3" => "DISPLAY/CC" } ]
   },
   cpp_diffs => \@diff });

  $ns->output("DifferentColoUsers/Channels", 2, "list of channels");
  $ns->output("DifferentColoUsers/CH1", $case->entity("DISPLAY2/CH"));
  $ns->output("DifferentColoUsers/CH2", $case->entity("DISPLAY2/CHE"));
  $ns->output("DifferentColoUsers/ChannelsEnd", 2, "list of channels");

  @diff = (
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_other_user_count => COUNT,
        impops_no_imp_user_count => COUNT }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_other_user_count => COUNT,
        impops_no_imp_user_count => COUNT }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_user_count => COUNT,
        imps_other_user_count => COUNT,
        impops_no_imp_user_count => COUNT }),
    ChannelImpInventoryTest::Diffs::create_diff(
      { impops_user_count => COUNT,
        imps_other_user_count => COUNT,
        impops_no_imp_user_count => COUNT }));

  ChannelImpInventoryTest::Diffs::output_diff($ns, "DifferentColoUsers", \@diff);
}

sub init
{
  my ($self, $ns) = @_;
  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("COUNT", COUNT);
  $self->ipms_scenario($ns);
  $self->imps_other_scenario_channel($ns);
  $self->imps_other_scenario_keyword($ns);
  $self->imps_other_scenario_display($ns);
  $self->nocookies($ns);
  $self->oouser($ns);
  $self->not_verified_impressions($ns);
  $self->no_imps_scenario($ns);
  $self->no_imps_opportunity_scenario($ns);
  $self->simple_1keywords($ns);
  $self->simple_2keywords_opportunity($ns);
  $self->simple_2keywords_display($ns);
  $self->simple_2keywords_difference($ns);
  $self->no_ad_keyword($ns);
  $self->case_1channel_2keywords($ns);
  $self->channel_targeted($ns); 
  $self->keyword_vs_channel_targeted($ns);
  $self->case_1channel_text_display($ns); 
  $self->tag_not_serve_text_ads($ns);
  $self->non_default_currency($ns);
  $self->tag_with_adjustment($ns);
  $self->different_colo($ns);
}

1;

