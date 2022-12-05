package RTBWinPriceNotificationTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub openx_case_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('OpenX');

  my $openx_rtb = $ns->create(Publisher => {
      name => "OpenXRTB",
      account_id => DB::Defaults::instance()->openx_account,
      tag_id => undef });

  $ns->output("Account", $openx_rtb->{account_id});

  $self->create_sizes($ns, $openx_rtb,
    [ { name => '120x240' },
      { name => '121x241' },
      { name => '122x242',
        template_file_flags => 0 },
      { name => '469x61' },
      { name => '470x62' },
      { name => '471x63' },
      { name => '472x64',
        max_text_creatives => 3 }]);

  # Allow to show RON text campaigns
  foreach my $ctt_campaign (@{$self->{channel_text_campaigns}})
  {
    $ns->create(CCGSite => {
      ccg_id => $ctt_campaign->{ccg_id},
      site_id => $openx_rtb->{site_id} });
  }
}

sub tanx_case_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('TanX');

  my $tanx_rtb = $ns->create(Publisher => {
      name => "TanXRTB",
      account_id => DB::Defaults::instance()->tanx_account,
      tag_id => undef });

  $ns->output("Account", $tanx_rtb->{account_id});

  $self->create_sizes($ns, $tanx_rtb,
    [ { name => '120x240' },
      { name => '728x90' } ] );
}

sub allyes_case_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('Allyes');

  my $allyes_rtb = $ns->create(Publisher => {
    name => 'AllyesRTB',
    account_id => DB::Defaults::instance()->allyes_account,
    tag_id => undef});

  $ns->output("Account", $allyes_rtb->{account_id});

  $self->create_sizes($ns, $allyes_rtb,
    [ { name => '120x240' },
      { name => '469x61' },
      { name => '470x62' },
      { name => '472x64',
        max_text_creatives => 3 },
      # with disabled track pixel
      { name => '160x600',
        template_file_flags => 0  } ] );

  # Allow to show RON text campaigns
  foreach my $ctt_campaign (@{$self->{channel_text_campaigns}})
  {
    $ns->create(CCGSite => {
      ccg_id => $ctt_campaign->{ccg_id},
      site_id => $allyes_rtb->{site_id} });
  }
}

sub liverail_case_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('LiveRail');

  #Create Currency
  my $rubles = $ns->create(Currency => { currency_code => 'RUB', rate => 30.0 } );
  my $pounds = $ns->create(Currency => { currency_code => 'GBP', rate => 0.51 } );

  $ns->output('CURRENCY#1', 30.0);
  $ns->output('CURRENCY#2', 0.51);

  #Create Advertiser
  my $advertiser1_comm = 0.0;
  my $advertiser1 = $ns->create(Account => {
    name           => 'ADSC OpenRTB-Russia Advertiser11',
    role_id        => DB::Defaults::instance()->advertiser_role,
    text_adserving => "M",
    currency_id    => $rubles,
    commission     => $advertiser1_comm});

  my $advertiser2_comm = 0.3;
  my $advertiser2 = $ns->create(Account => {
    name           => 'ADSC OpenRTB - Gross commission',
    role_id        => DB::Defaults::instance()->advertiser_role,
    text_adserving => "M",
    currency_id    => $rubles,
    commission     => $advertiser2_comm,
    });

  $ns->output('ADVERTISER#1',             $advertiser1->{account_id});
  $ns->output('COMMISSIONS/ADVERTISER#1', $advertiser1_comm);
  $ns->output('ADVERTISER#2',             $advertiser2->{account_id});
  $ns->output('COMMISSIONS/ADVERTISER#2', $advertiser2_comm);

  #Create Publishers
  my $liverail_pub1_comm = 0.3;
  my $liverail_pub1 = $ns->create(Publisher => {
    name        => 'ADSC OpenRTB pub commission #1 (RUB)',
    commission  => $liverail_pub1_comm,
    currency_id => $rubles,
    tag_id      => undef } );

  $ns->output('PUBLISHER#1',             $liverail_pub1->{account_id});
  $ns->output('COMMISSIONS/PUBLISHER#1', $liverail_pub1_comm);

  my $liverail_pub2_comm = 0.0;
  my $liverail_pub2 = $ns->create(Publisher => {
    name        => 'ADSC OpenRTB-Russia',
    commission  => $liverail_pub2_comm,
    currency_id => $rubles,
    tag_id      => undef } );

  $ns->output('PUBLISHER#2',             $liverail_pub2->{account_id});
  $ns->output('COMMISSIONS/PUBLISHER#2', $liverail_pub2_comm);

  my $liverail_pub3_comm = 0.0;
  my $liverail_pub3 = $ns->create(Publisher => {
    name        => 'ADSC OpenRTB #4 (GBP)',
    commission  => $liverail_pub3_comm,
    currency_id => $pounds,
    tag_id      => undef } );

  $ns->output('PUBLISHER#3',             $liverail_pub3->{account_id});
  $ns->output('COMMISSIONS/PUBLISHER#3', $liverail_pub3_comm);

  #Create sizes.
  $self->create_sizes($ns, $liverail_pub1,
    [ { name => '120x240' },
      { name => '472x64', max_text_creatives => 3 } ], 0, 1 );

  $self->create_sizes($ns, $liverail_pub2,
    [ { name => '120x240' },
      { name => '469x61'  },
      { name => '471x63'  },
      { name => '472x64', max_text_creatives => 2 },
      { name => '474x66'  } ], 0, 2 );

  $self->create_sizes($ns, $liverail_pub3,
    [ { name => '120x240' },
      { name => '469x61'  },
      { name => '470x62'  },
      { name => '472x64', max_text_creatives => 2 } ], 0, 3 );

  #Create Campaigns.
  my $url = "www." . make_autotest_name($ns, 'url'). "1.com";
  $ns->output("URL#1", $url);
  my $campaign1_cpm = 2400.54;
  my $campaign1 = $ns->create(DisplayCampaign => {
    name                               => 'ADSC - Display - OpenRTB Russia',
    account_id                         => $advertiser1,
    campaigncreativegroup_cpm          => $campaign1_cpm,
    channel_id                         => DB::BehavioralChannel->blank(
      name                  => 'ADSC - OpenRTB - Portion Pub Amount - CPM #1 (GB)',
      keyword_list          => 'ADSCOpenRTBPortionCPM1',
      url_list              => $url,
      account_id            => $advertiser1,
      behavioral_parameters => [ DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U') ] ),
    site_links                         => [ { site_id => $liverail_pub1->{site_id} },
                                            { site_id => $liverail_pub2->{site_id} },
                                            { site_id => $liverail_pub3->{site_id} } ],
    #Creatives will be created later.
    creative_id                        => undef,
    cc_id                              => undef } );

  $url = "www." . make_autotest_name($ns, 'url'). "2.com";
  $ns->output("URL#2", $url);
  my $campaign2_cpm = 3000.0;
  my $campaign2 = $ns->create(DisplayCampaign => {
    name                               => 'Display #1 (comm = 30%)',
    campaign_commission                => 0.3,
    account_id                         => $advertiser2,
    campaigncreativegroup_cpm          => $campaign2_cpm,
    channel_id                         => DB::BehavioralChannel->blank(
      name                  => 'ADSC - OpenRTB - Portion Pub Amount - Comm - CPM #1 (GB)',
      keyword_list          => 'ADSCOpenRTBPortionCommCPM1',
      url_list              => $url,
      account_id            => $advertiser2,
      behavioral_parameters => [ DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U') ] ),
    site_links                         => [ { site_id => $liverail_pub1->{site_id} },
                                            { site_id => $liverail_pub2->{site_id} },
                                            { site_id => $liverail_pub3->{site_id} } ],
    #Creatives will be created later.
    creative_id                        => undef,
    cc_id                              => undef } );

  my $keyword1 = make_autotest_name($ns, 'Keyword1');
  my $campaign3_cpc = 2200.12;
  my $behavioral_channel3 = $ns->create(DB::BehavioralChannel->blank(
    name                  => 'KeywordChannel1',
    channel_type          => 'K',
    keyword_list          => $keyword1,
    account_id            => $advertiser1,
    behavioral_parameters => [DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P', time_from => 0, time_to => 3600) ] ) );
  my $campaign3 =  $ns->create(TextAdvertisingCampaign => {
    name                               => 'Text (K) LRP #1 (GB)',
    account_id                         => $advertiser1,
    size_id                            => $self->{sizes}->{'472x64'},
    template_id                        => DB::Defaults::instance()->text_template,
    campaigncreativegroup_cpc          => $campaign3_cpc,
    campaigncreativegroup_flags        => 0,
    original_keyword                   => $keyword1,
    max_cpc_bid                        => 1100.06,
    ccgkeyword_ctr                     => 0.002,
    ccgkeyword_channel_id              => $behavioral_channel3->channel_id() } );

  $ns->output('KEYWORD#1',  $keyword1);
  $ns->output('CREATIVE#8', get_tanx_creative($campaign3->{Creative}));
  $ns->output('CHANNEL#8',  $behavioral_channel3->channel_id());
  $ns->output('CCG#8',      $campaign3->{ccg_id});
  $ns->output('CC#8',       $campaign3->{cc_id});

  my $keyword2 = make_autotest_name($ns, 'Keyword2');
  my $campaign4_cpc = 2100.11;
  my $behavioral_channel4 = $ns->create(DB::BehavioralChannel->blank(
    name                  => 'KeywordChannel2',
    channel_type          => 'K',
    keyword_list          => $keyword2,
    account_id            => $advertiser1,
    behavioral_parameters => [DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P', time_from => 0, time_to => 3600) ] ) );
  my $campaign4 =  $ns->create(TextAdvertisingCampaign => {
    name                               => 'Text (K) LRP #2 (GB)',
    account_id                         => $advertiser1,
    size_id                            => $self->{sizes}->{'472x64'},
    template_id                        => DB::Defaults::instance()->text_template,
    campaigncreativegroup_cpc          => $campaign4_cpc,
    campaigncreativegroup_flags        => 0,
    original_keyword                   => $keyword2,
    max_cpc_bid                        => 2100.11,
    ccgkeyword_ctr                     => 0.001,
    ccgkeyword_channel_id              => $behavioral_channel4->channel_id() } );

  $ns->output('KEYWORD#2',  $keyword2);
  $ns->output('CREATIVE#9', get_tanx_creative($campaign4->{Creative}));
  $ns->output('CHANNEL#9',  $behavioral_channel4->channel_id());
  $ns->output('CCG#9',      $campaign4->{ccg_id});
  $ns->output('CC#9',       $campaign4->{cc_id});

  $ns->output('CPM/CCG#1', $campaign1_cpm);
  $ns->output('CPM/CCG#2', $campaign2_cpm);
  $ns->output('CPC/CCG#3', $campaign3_cpc);
  $ns->output('CPC/CCG#4', $campaign4_cpc);

  $ns->output('KEYWORDS',  "$keyword1,$keyword2");

  my $tag = $ns->create(PricedTag => {
    name      => "RTB" . $self->{sizes}->{'470x62'}->{protocol_name} . $liverail_pub2->{site_id},
    site_id   => $liverail_pub2->{site_id},
    size_id   => $self->{sizes}->{'470x62'},
    cpm       => 0.2,
    rate_type => 'RS'});
  $ns->output('TAGS/PUBLISHER#2/470x62', $tag);

  my $index = 1;
  my @sizes = ('120x240', '469x61', '470x62', '471x63', '474x66', '120x240', '474x66');
  foreach my $size (@sizes)
  {
    my $creative = $ns->create(Creative => {
      name       => $self->{sizes}->{$size}->{protocol_name}."$index",
      account_id => ($index <=5 ? $advertiser1 : $advertiser2),
      size_id    => $self->{sizes}->{$size} });

    my $cc = $ns->create(CampaignCreative => {
      ccg_id      => ($index <=5 ? $campaign1->{ccg_id} : $campaign2->{ccg_id}),
      creative_id => $creative });

    $ns->output("CREATIVE#$index", get_tanx_creative($creative));
    $ns->output("CCG#$index",      $cc->{ccg_id});
    $ns->output("CC#$index",       $cc);
    $index = $index + 1
  }
}

sub create_sizes
{
  my ($self, $ns, $rtb, $sizes, $create_creatives, $index) = @_;
  $create_creatives ||= 1;

  foreach my $size_args (@$sizes)
  {
    # Create size
    my $name = $size_args->{name};
    die "Undefined name for creative size!" unless defined $name;
    # Pixel tracking enabled by default
    my $tf_flags = defined $size_args->{template_file_flags}
      ? $size_args->{template_file_flags}
      : DB::TemplateFile::PIXEL_TRACKING;
    delete $size_args->{template_file_flags};
    my ($width, $height) = split('x', $name);
    die "'$name' must have WIDTHxHEIGTH format!"
      unless defined $width and defined $height;
    $size_args->{width} = $width;
    $size_args->{height} = $height;
    $size_args->{max_text_creatives} = 0
      unless defined $size_args->{max_text_creatives};

    my $size = $ns->create(CreativeSize => $size_args);
    $self->{sizes}->{$name} = $size;

    $ns->output("$name", $size->{protocol_name});

    if (defined($index))
    {
      my $tag = $ns->create(PricedTag => {
        name => "RTB" . $index . $size->{protocol_name} . $rtb->{site_id},
        site_id => $rtb->{site_id},
        size_id => $size});
      $ns->output("TAGS/PUBLISHER#$index/$name", $tag);
    }
    else
    {
      # Create corresponding tags for RTB's
      foreach my $site ({ name => "RTB",
                          id => $rtb->{site_id} },
                        { name => "DEFAULT_RTB",
                          id => $self->{default_rtb}->{site_id} },
                        { name => "OPEN_RTB",
                          id => $self->{open_rtb}->{site_id} })
      {
        my $tag = $ns->create(PricedTag => {
          name => $site->{name} . $size->{protocol_name} . $site->{id},
          site_id => $site->{id},
          size_id => $size});

        $ns->output("$site->{name}/Tags/$name", $tag);
      }
    }

    unless ($size_args->{max_text_creatives})
    {
      for my $format (DB::Defaults::instance()->html_format,
                      DB::Defaults::instance()->js_format)
      {
        $ns->create(TemplateFile => {
          template_id => DB::Defaults::instance()->display_template,
          size_id => $size,
          template_file => 'UnitTests/raw_tokens.html',
          flags => $tf_flags,
          app_format_id => $format });
      }

      if ($create_creatives)
      {
        my $creative = $ns->create(Creative => {
          name => $size->{protocol_name},
          account_id => $self->{display_campaign}->{account_id},
          size_id => $size });

        my $cc = $ns->create(CampaignCreative => {
          ccg_id => $self->{display_campaign}->{ccg_id},
          creative_id => $creative });

        $ns->output("Creatives/$name", get_tanx_creative($creative));
        $ns->output("CCIDs/$name", $cc);
        $ns->output("CREATIVEIDs/$name", $creative);
      }
    }
    else
    {
      for my $format (DB::Defaults::instance()->html_format,
                      DB::Defaults::instance()->js_format)
      {
        $ns->create(TemplateFile => {
          template_id => DB::Defaults::instance()->text_template,
          size_id => $size,
          template_file => 'UnitTests/raw_tokens.html',
          flags => $tf_flags,
          app_format_id => $format });
      }
    }
  }
}

sub init {
  my ($self, $ns) = @_;

  # RTB without encryption key in server config
  $self->{default_rtb} = $ns->create(Publisher => {
    name => "defaultRTB",
    tag_id => undef });

  $ns->output("DefaultRTB/Account", $self->{default_rtb}->{account_id});

  $self->{open_rtb} = $ns->create(Publisher => {
    name => "OpenRTB",
    account_id => DB::Defaults::instance()->openrtb_account,
    tag_id => undef });

  $ns->output("OpenRTB/Account", $self->{open_rtb}->{account_id});

  my $currency_rate = 65;
  $ns->output("AdvertiserRate", $currency_rate);

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role,
    text_adserving => "M",
    currency_id => DB::Currency->blank(rate => $currency_rate) });

  my $cpm = 2100;
  $ns->output("Global/CPMs/CPM", $cpm);
  my $url = "www." . make_autotest_name($ns, 'url'). ".com";
  $ns->output("URL", $url);
  $self->{display_campaign} = $ns->create(DisplayCampaign => {
    name => 'DisplayCampaign',
    account_id => $advertiser,
    cpm => $cpm,
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      url_list => $url,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U') ]),
    campaign_flags => 0,
    campaigncreativegroup_flags => 0,
    # Creatives will be created later in cases
    creative_id => undef,
    cc_id => undef
  });

#  my $text_size = $ns->create(CreativeSize => {
#    name => "Text",
#    width => undef,
#    height => undef });

#  $ns->create(TemplateFile => {
#    template_id => DB::Defaults::instance()->text_template,
#    size_id => $text_size,
#    template_file => 'UnitTests/raw_tokens.html',
#    flags => DB::TemplateFile::PIXEL_TRACKING,
#    app_format_id => DB::Defaults::instance()->html_format });

  my $keyword = make_autotest_name($ns, 'keyword');
  $ns->output("Keyword", $keyword);
  $ns->output("Search", "http://search.live.de/results.aspx?q=$keyword");
  $self->{text_campaign} = $ns->create(TextAdvertisingCampaign => {
    name => 'TextCampaign',
    account_id => $advertiser,
    template_id => DB::Defaults::instance()->text_template,
    size_id => DB::Defaults::instance()->text_size,
    size_type_id => DB::Defaults::instance()->other_size_type,
    original_keyword => $keyword,
    campaign_flags => 0,
    campaigncreativegroup_flags => 0,
    ccgkeyword_channel_id => $ns->create(DB::BehavioralChannel->blank(
      name => 'KeywordChannel',
      account_id => $advertiser,
      channel_type => 'K',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S') ]
      ))->channel_id(),
    max_cpc_bid => 220 });

  $ns->output("Global/Creatives/Text",
    get_tanx_creative($self->{text_campaign}->{Creative}));
  $ns->output("Global/CREATIVEIDs/Text", $self->{text_campaign}->{creative_id});
  $ns->output("Global/CCIDs/Text", $self->{text_campaign}->{cc_id});

  $self->{channel_text_campaigns} = [];
  foreach my $args ({name => 1, cpm => 2070},
                    {name => 2, cpm => 2050})
  {
    my $campaign = $ns->create(ChannelTargetedTACampaign => {
      name => "ChannelTextCampaign$args->{name}",
      account_id => $advertiser,
      template_id => DB::Defaults::instance()->text_template,
      size_id => DB::Defaults::instance()->text_size,
      size_type_id => DB::Defaults::instance()->other_size_type,
      rate_type => 'CPM',
      campaigncreativegroup_cpm => $args->{cpm},
      campaigncreativegroup_flags =>
        DB::Campaign::RON | DB::Campaign::INCLUDE_SPECIFIC_SITES,
      channel_id => undef,
      site_links => [{ site_id => $self->{default_rtb}->{site_id} },
                     { site_id => $self->{open_rtb}->{site_id} }] });

    push @{$self->{channel_text_campaigns}}, $campaign;

    $ns->output("Global/Creatives/ChannelText#$args->{name}",
      get_tanx_creative($campaign->{Creative}));
    $ns->output("Global/CREATIVEIDs/ChannelText#$args->{name}", $campaign->{creative_id});
    $ns->output("Global/CCIDs/ChannelText#$args->{name}", $campaign->{cc_id});
    $ns->output("Global/CPMs/TextCPM#$args->{name}", $args->{cpm});
  }

  $ns->output("RevenueShare",
    DB::Defaults::instance()->openrtb_isp->{Colocation}->{revenue_share});

  $ns->output("Colocation",
    DB::Defaults::instance()->openrtb_isp->{colo_id});

  $ns->output("GUINEA/IP", DB::Defaults::instance()->ip_address);

  $self->tanx_case_($ns);
  $self->openx_case_($ns);
  $self->allyes_case_($ns);
  #$self->liverail_case_($ns);
}

1;
