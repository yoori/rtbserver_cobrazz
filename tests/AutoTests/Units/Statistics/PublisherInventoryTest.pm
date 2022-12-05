package PublisherInventoryTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_behavioral_channels
{
  my ($self, $args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{case_name}: Behavioral channel '$arg->{name}' is redefined!'"
      if exists $self->{channels}->{$arg->{name}};

    my $kwd_idx = 1;
    my (@keywords, @urls);
    if (defined $arg->{keyword_list})
    {
      foreach my $keyword (split(/\W+/, $arg->{keyword_list}))
      {
        $self->{keywords}->{$keyword} = make_autotest_name($self->{ns}, $keyword)
          unless exists $self->{keywords}->{$keyword};
        push @keywords, $self->{keywords}->{$keyword};
        $self->{ns}->output("$arg->{name}/KEYWORD#".$kwd_idx++, $self->{keywords}->{$keyword});
      }
    }
    $kwd_idx = 1;
    if (defined $arg->{url_list})
    {
      foreach my $url (split(/\W+/, $arg->{url_list}))
      {
        $self->{urls}->{$url} = "www" . make_autotest_name($self->{ns}, $url) . "com"
          unless exists $self->{urls}->{$url};
        push @urls, $self->{urls}->{$url};
        $self->{ns}->output("$arg->{name}/URL#".$kwd_idx++, $self->{urls}->{$url});
      }
    }
    $arg->{keyword_list} = join("\n", @keywords);
    $arg->{url_list} = join("\n", @urls);

    die "$self->{case_name}: must be defined keyword or url for '$arg->{name}' channel!"
      unless @keywords or @urls;

    foreach my $bp (@{ $arg->{behavioral_parameters} })
    {
      $bp = DB::BehavioralChannel::BehavioralParameter->blank(%$bp);
    }

    $arg->{account_id} = $self->{ns}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role }) unless defined $arg->{account_id};

    $self->{channels}->{$arg->{name}} = $self->{ns}->create(DB::BehavioralChannel->blank(%$arg));
    $self->{ns}->output("$arg->{name}/ID", $self->{channels}->{$arg->{name}}->channel_id());
  }
}

sub create_channel_targeted_campaigns_
{
  my ($self, $type, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns}->{$arg->{name}};

    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};
    $arg->{campaign_flags} = 0
      unless defined $arg->{campaign_flags};

    if (defined $arg->{channel})
    {
      die "$self->{case_name}: channel '$arg->{channel}' is undefined!"
        unless defined $self->{channels}->{$arg->{channel}};
      $arg->{channel_id} = $self->{channels}->{$arg->{channel}};
      delete $arg->{channel};
    }
    if (defined $arg->{campaign})
    {
      die "$self->{case_name}: campaign '$arg->{campaign}' is undefined!"
        unless defined $self->{campaigns}->{$arg->{campaign}};
      $arg->{campaign_id} = $self->{campaigns}->{$arg->{campaign}}->{campaign_id};
      delete $arg->{campaign};
    }
    if (defined $arg->{specific_sites})
    {
      $arg->{site_links} = [];
      foreach my $site (split(/\W+/, $arg->{specific_sites}))
      {
        die "$self->{case_name}: publisher '$site' is undefined!"
          unless defined $self->{publishers}->{$site};

        push @{$arg->{site_links}},
             { site_id => $self->{publishers}->{$site}->{site_id} };
      }
      delete $arg->{specific_sites};
    }
    $arg->{campaigncreativegroup_cpm} = 0
      unless defined $arg->{campaigncreativegroup_cpm} or
             defined $arg->{cpm};

    $arg->{campaigncreativegroup_ar} = 0.01
      if defined $arg->{campaigncreativegroup_cpa};

    $self->{campaigns}->{$arg->{name}} = $self->{ns}->create($type => $arg);

    my $cpm = $arg->{campaigncreativegroup_cpm} || $arg->{cpm} || undef;
    my $cpc = $arg->{campaigncreativegroup_cpc} || $arg->{cpc} || undef;
    my $cpa = $arg->{campaigncreativegroup_cpa} || $arg->{cpa} || undef;

    $self->{ns}->output("$arg->{name}/CCID", $self->{campaigns}->{$arg->{name}}->{cc_id});
    $self->{ns}->output("$arg->{name}/CPM", $cpm) if defined $cpm;
    $self->{ns}->output("$arg->{name}/CPC", $cpc) if defined $cpc;
    $self->{ns}->output("$arg->{name}/CPA", $cpa) if defined $cpa;
    $self->{ns}->output("$arg->{name}/COMMISSION", $arg->{campaign_commission})
      if defined $arg->{campaign_commission};
  }
}

sub create_display_campaigns
{
  my ($self, $args) = @_;
  $self->create_channel_targeted_campaigns_("DisplayCampaign", $args);
}

sub create_channel_targeted_text_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    $arg->{template_id} = DB::Defaults::instance()->text_template
      unless defined $arg->{template_id};
    unless (defined $arg->{size_id})
    {
      $arg->{creative_size_type_id} =
        DB::Defaults::instance()->other_size_type;
      $arg->{size_id} = DB::Defaults::instance()->text_size;
    }
  }
  $self->create_channel_targeted_campaigns_("ChannelTargetedTACampaign", $args);
}

sub create_text_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns}->{$arg->{name}};

    $arg->{template_id} = DB::Defaults::instance()->text_template
      unless defined $arg->{template_id};
    
    unless (defined $arg->{size_id})
    {
      $arg->{creative_size_type_id} =
        DB::Defaults::instance()->other_size_type;
      $arg->{size_id} = DB::Defaults::instance()->text_size;
    }

    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};

    if (defined $arg->{campaign})
    {
      die "$self->{case_name}: campaign '$arg->{campaign}' is undefined!"
        unless defined $self->{campaigns}->{$arg->{campaign}};
      $arg->{campaign_id} = $self->{campaigns}->{$arg->{campaign}}->{campaign_id};
      delete $arg->{campaign};
    }

    if (defined $arg->{keyword})
    {
      die "$self->{case_name}: Keyword '$arg->{keyword}' is not defined!"
        unless exists $self->{keywords}->{$arg->{keyword}};
      $arg->{original_keyword} = $self->{keywords}->{$arg->{keyword}}
        unless defined $arg->{original_keyword};
      delete $arg->{keyword};
    }

    if (defined $arg->{channel})
    {
      die "$self->{case_name}: Channel '$arg->{channel}' is not defined!"
        unless exists $self->{channels}->{$arg->{channel}};
      $arg->{ccgkeyword_channel_id} = $self->{channels}->{$arg->{channel}}
        unless defined $arg->{ccgkeyword_channel_id};
      delete $arg->{channel};
    }

    $self->{campaigns}->{$arg->{name}} =
      $self->{ns}->create(TextAdvertisingCampaign => $arg);

    $self->{ns}->output("$arg->{name}/CCID", $self->{campaigns}->{$arg->{name}}->{cc_id});
    $self->{ns}->output("$arg->{name}/CPC", $arg->{max_cpc_bid})
      if defined $arg->{max_cpc_bid};
  }
}

sub create_publishers
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    if (defined $arg->{site})
    {
      die "$self->{case_name}: Site '$arg->{site}' is undefined!"
        unless exists $self->{publishers}->{$arg->{site}};
      $arg->{site_id} = $self->{publishers}->{$arg->{site}}->{site_id};
      delete $arg->{site};
    }

    $self->{publishers}->{$arg->{name}} = $self->{ns}->create(Publisher => $arg);

    $self->{ns}->output("$arg->{name}/TAG_ID", $self->{publishers}->{$arg->{name}}->{tag_id});
    $self->{ns}->output("$arg->{name}/SITE_ID", $self->{publishers}->{$arg->{name}}->{site_id});
    $self->{ns}->output("$arg->{name}/ACCOUNT_ID", $self->{publishers}->{$arg->{name}}->{account_id});
    $self->{ns}->output("$arg->{name}/MARGIN_RULE",
      $self->{publishers}->{$arg->{name}}->{PubAccount}->{margin_rule_id})
      if defined $self->{publishers}->{$arg->{name}}->{PubAccount}->{margin_rule_id};
  }
}

sub new
{
  my $self = shift;
  my ($ns, $case_name) = @_;

  unless (ref $self)
  { $self = bless {}, $self; }

  $self->{ns} = $ns->sub_namespace($case_name);
  $self->{case_name} = $case_name;

  return $self;
}

1;

package PublisherInventoryTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub base_scenario
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "BaseCase");

  $test_case->create_behavioral_channels([
    { name => "Channel1",
      keyword_list => "Keyword1",
      url_list => "Keyword1",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => "P" },
        { trigger_type => "U" } ] },

    { name => "Channel2",
      keyword_list => "Keyword2",
      url_list => "Keyword2",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => "P" },
        { trigger_type => "U" } ] }
  ]);

  $test_case->create_display_campaigns([
    { name => 'NetCampaign1',
      account_id => $self->{AccountNet},
      size_id => $self->{size_id},
      channel => "Channel1",
      campaigncreativegroup_cpm =>
        ($self->{threshold_cpm} + 1) },

    { name => 'NetCampaign2',
      account_id => $self->{AccountNet},
      size_id => $self->{size_id},
      channel => "Channel2",
      campaigncreativegroup_cpm =>
        ($self->{threshold_cpm} - 1) } ]);

  my $passback_url = "https://confluence.ocslab.com";
  $test_case->{ns}->output("PASSBACK", $passback_url, "");

  $test_case->create_publishers([
    { name => 'Publisher1',
      account_id => $self->{moscow_account},
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_size_id => $self->{size_id} },

    { name => 'Publisher2',
      site => "Publisher1",
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_size_id => $self->{size_id} },

    { name => 'Publisher3',
      site => "Publisher1",
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_size_id => $self->{size_id} },

    { name => 'Publisher4',
      pubaccount_currency_id => $self->{pub_currency},
      pubaccount_timezone_id => DB::TimeZone->blank(tzname => 'Europe/Moscow'),
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_passback => $passback_url,
      pricedtag_size_id => $self->{size_id},
      pricedtag_cpm => 7 }
  ]);
}

sub ta_campaigns_scenario
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "TACampaignsCase");

  $test_case->create_behavioral_channels([
    { name => "BChannel",
      keyword_list => "TAKeyword",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => "P" } ] }
  ]);

  $test_case->create_text_campaigns([
    { name => 'TACampaign1',
      account_id => $self->{AccountNet},
      keyword => "TAKeyword",
      max_cpc_bid =>
        (($self->{threshold_cpm} + 2) / (DB::Defaults::default_ctr() * 1000)) },

    { name => 'TACampaign2',
      account_id => $self->{AccountNet},
      keyword => "TAKeyword",
      max_cpc_bid =>
        (($self->{threshold_cpm} + 3) / (DB::Defaults::default_ctr() * 1000)) }
  ]);

  $test_case->create_channel_targeted_text_campaigns([
    { name => 'TACampaign3',
      account_id => $self->{AccountNet},
      rate_type => 'CPC',
      campaigncreativegroup_cpc =>
        (($self->{threshold_cpm} + 1) / (DB::Defaults::default_ctr() * 1000)),
      campaigncreativegroup_ctr => DB::Defaults::default_ctr(),
      channel => "BChannel" } 
  ]);

  $test_case->create_publishers([
    { name => 'Publisher',
      pubaccount_currency_id => $self->{pub_currency},
      pubaccount_timezone_id => DB::TimeZone->blank(tzname => 'Europe/Moscow'),
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_size_id => $self->{size_id} }
  ]);
}

sub pub_adv_commission_scenario
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "GROSSCampaignsCase");

  $test_case->create_behavioral_channels([
    { name => 'Channel',
      account_id => $self->{AccountNet},
      keyword_list => "Keyword",
      behavioral_parameters => [{ trigger_type => 'P' }] }
  ]);

  $test_case->create_display_campaigns([
    { name => "GrossCampaign",
      advertiser_agency_account_id => 
        DB::Defaults::instance()->agency_gross(),
      advertiser_currency_id => $self->{adv_currency},
      campaign_commission => 0.3,
      campaigncreativegroup_cpm =>
        ($self->{threshold_cpm} + 5),
      channel => "Channel" }
  ]);

  $test_case->create_publishers([
    { name => "GrossPublisher",
      pubaccount_account_type_id =>
        $test_case->{ns}->create(AccountType => {
          name => "pubGross",
          account_role_id => DB::Defaults::instance()->publisher_role }),
      pubaccount_commission => 0.2,
      pubaccount_currency_id => $self->{pub_currency},
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_cpm => 11 },

    { name => 'NetPublisher',
      account_id => $self->{moscow_account},
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION }
  ]);
}

sub virtual_scenario
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "VirtualCampaignCase");

  $test_case->create_publishers([
    { name => "VirtPublisher1",
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_cpm => ($self->{threshold_cpm} + 7) / 10 },

    { name => 'VirtPublisher2',
      site => "VirtPublisher1",
      cpm => ($self->{threshold_cpm} + 7) / 10 }
  ]);

  $test_case->create_behavioral_channels([
    { name => 'Channel',
      account_id => $self->{AccountNet},
      keyword_list => "VKeyword",
      behavioral_parameters => [{ trigger_type => 'P' }] }
  ]);

  $test_case->create_display_campaigns([
    { name => "VirtualCampaign",
      campaign_freq_cap_id => 
        DB::FreqCap->blank(life_count => 2),
      campaigncreativegroup_cpm => $self->{threshold_cpm} + 7,
      specific_sites => "VirtPublisher1",
      channel => "Channel" }
  ]);
}


sub no_impression_scenario
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "NoImpCase");

  $test_case->create_publishers([
    { name => 'NoImpPublisher',
      pubaccount_currency_id => $self->{pub_currency},
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION }
  ]);
}

sub billing_stats_logging
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "BillingStatsLoggingCase");

  $test_case->create_publishers([
    { name => 'RatePublisher',
      pubaccount_currency_id => $self->{pub_currency},
      flags => DB::Tags::INVENORY_ESTIMATION },

    { name => 'AdvPublisher',
      site => 'RatePublisher' }
  ]);

  my $norate_tag = $test_case->{ns}->create(Tags => {
    name => 'NoRatePublisher',
    flags => DB::Tags::INVENORY_ESTIMATION,
    site_id => $test_case->{publishers}->{"RatePublisher"}->{site_id}
  });

  $test_case->{ns}->output("NoRatePublisher/TAG_ID", $norate_tag);
}

sub non_billing_stats_logging
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "NonBillingStatsLoggingCase");

  $test_case->create_behavioral_channels([
    { name => "KChannel1",
      channel_type => 'K',
      keyword_list => "Keyword1",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 },
        { trigger_type => 'S', time_to => 60 } ] },

    { name => "KChannel2",
      channel_type => 'K',
      keyword_list => "Keyword2",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 },
        { trigger_type => 'S', time_to => 60 } ] },

    { name => "MarkerChan",
      channel_type => 'K',
      keyword_list => "Keyword3",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 },
        { trigger_type => 'S', time_to => 60 } ] }
  ]);

  $test_case->create_text_campaigns([
    { name => 'TCampaign1',
      account_id => $self->{AccountNet},
      keyword => "Keyword1",
      max_cpc_bid => 0.1,
      channel => "KChannel1" },

    { name => 'TCampaign2',
      account_id => $self->{AccountNet},
      keyword => "Keyword2",
      max_cpc_bid => 0.1,
      channel => "KChannel2" }
  ]);

  $test_case->create_publishers([
    { name => 'InvPublisher',
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION },

    { name => 'AdvPublisher' },

    { name => 'MarkerPub' },
  ]);
}

sub publisher_with_adjustment_coef
{
  my ($self, $ns) = @_;
  my $test_case = new PublisherInventoryTest::TestCase($ns, "PublisherAdjustment");

  $test_case->create_behavioral_channels([
    { name => "DChannel",
      keyword_list => "DKeyword",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "CTChannel",
      keyword_list => "CTKeyword",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "KChannel",
      channel_type => 'K',
      keyword_list => "KKeyword",
      account_id => $self->{AccountNet},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] }
  ]);

  my $ctr = 0.1;
  my $adjustment = 2;

  my $cpc = $self->{threshold_cpm} / 1000 / $ctr + 0.03;

  $test_case->{ns}->output("TAG_CPM",
    $adjustment * $cpc * 100 * $ctr * 1000);

  $test_case->create_display_campaigns([
    { name => 'DCampaign',
      channel => "DChannel",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => $cpc,
      campaigncreativegroup_ctr => $ctr } ] );

  $test_case->create_channel_targeted_text_campaigns([
    { name => 'CTCampaign',
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => $cpc,
      campaigncreativegroup_ctr => $ctr,
      channel => "CTChannel" } ] );

  $test_case->create_text_campaigns([
    { name => 'KCampaign',
      keyword => "KKeyword",
      max_cpc_bid => $cpc,
      channel => "KChannel",
      ccgkeyword_ctr => $ctr } ] );

  $test_case->create_publishers([
    { name => 'InvPublisher',
      pricedtag_flags => DB::Tags::INVENORY_ESTIMATION,
      pricedtag_adjustment => $adjustment } ] );
}

sub init
{
  my ($self, $ns) = @_;

  $self->{moscow_account} = $ns->create(PubAccount => {
    name => "MoscowAccount",
    timezone_id => DB::TimeZone->blank(tzname => 'Europe/Moscow') });

  # Rates
  $self->{pub_currency} = $ns->create(Currency => { rate => 4 });
  $self->{adv_currency} = $ns->create(Currency => { rate => 5.1 });

  $ns->output("PUBRATE", $self->{pub_currency}->{rate});
  $ns->output("ADVRATE", $self->{adv_currency}->{rate});
  $ns->output("CTR", DB::Defaults::default_ctr());

  $self->{AccountNet} = $ns->create(Account => {
    name => "Net",
    role_id => DB::Defaults::instance()->advertiser_role,
    text_adserving => "M",
    currency_id => $self->{adv_currency} } );
  $self->{threshold_cpm} = 20;

  #Size
  $self->{size_id} = $ns->create(CreativeSize => {
    name => 1,
    max_text_creatives => 3 });

  $ns->create(TemplateFile =>
    { template_id => DB::Defaults::instance()->display_template(),
      size_id => $self->{size_id},
      flags => 0 });

  $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->text_template,
      size_id => $self->{size_id},
      template_file => 'UnitTests/textad.xsl',
      flags => 0,
      template_type => 'X'});

  # Colocations
  $ns->output("ADS_COLO", DB::Defaults::instance()->ads_isp->{colo_id});
    
  $self->base_scenario($ns);
  $self->ta_campaigns_scenario($ns);
  $self->pub_adv_commission_scenario($ns);
  $self->virtual_scenario($ns);
  $self->no_impression_scenario($ns);
  $self->billing_stats_logging($ns);
  $self->non_billing_stats_logging($ns);
  $self->publisher_with_adjustment_coef($ns);
}

1;
