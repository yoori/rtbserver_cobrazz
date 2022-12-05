
package OpenRTBExtensionsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use Data::Dumper;

sub openx_extensions_case_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('OpenX');

  my $openx = $ns->create(Publisher => {
    name => '728x90',
    account_id => DB::Defaults::instance()->openx_account,
    pricedtag_size_id => $self->{sizes}->{'728x90'} });

  for (my $i = 1; $i < 4; ++$i)
  {
    my $url = 'www.'.make_autotest_name($ns, "url$i").'.com';

    my $campaign = $ns->create(DisplayCampaign => {
      name => "$i",
      account_id => $self->{account},
      creative_tag_sizes => [$self->{sizes}->{'728x90'}],
      cpm => 1000,
      behavioralchannel_url_list => $url,
      campaigncreativegroup_country_code => 'RU',
      site_links => [ { site_id => $openx->{site_id} },
                      { site_id => $self->{default_openrtb}->{site_id} } ] });

    if ($i == 3)
    {
      $ns->create(CreativeCategory_Creative => {
        creative_category_id => $self->{categories}->{content},
        creative_id => $campaign->{creative_id} });
    }

    $ns->create(
      DB::BehavioralChannel::BehavioralParameter->blank(
        channel_id => $campaign->{channel_id},
        trigger_type => "U" ));

    $ns->output("URL#$i", $url);
    $ns->output("HTTP_URL#$i", "http://$url");
    $ns->output("HTTPS_URL#$i", "https://$url");
    $ns->output("CC#$i", $campaign->{cc_id});
    $ns->output("CREATIVE#$i", $campaign->{creative_id});
    $ns->output("CCG#$i", $campaign->{ccg_id});
  }

  $ns->output("ACCOUNT", DB::Defaults::instance()->openx_account);
}

sub vast_extensions_case_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('Vast');

  my $size_type = $ns->create(SizeType => {
    name              => 'vast',
    flags             => DB::SizeType::SIZE_LEVEL_SELECTION});

  my $creative_size = $ns->create(CreativeSize => {
    name              => 'Video (VAST)',
    width             => 700,
    height            => 670,
    size_type_id      => $size_type,
    protocol_name     => 'vast'});

  my $vast = $ns->create(Publisher => {
    name              => 'vast',
    account_id        => DB::Defaults::instance()->openx_account,
    pricedtag_size_id => $creative_size });

  for (my $i = 1; $i < 3; ++$i)
  {
    my $is_ron = ($i == 2 ? 1 : 0);

    my $url = undef;
    if (!$is_ron)
    {
      $url = 'www.'.make_autotest_name($ns, "url$i").'.com';
    }

    my $app_format = $ns->create(DB::AppFormat->blank(
      name => 'vast',
      mime_type => 'video/mp4'));

    my $template = $ns->create(Template => {
      name              => 'vast',
      size_id           => $creative_size,
      template_file     => 'UnitTests/banner_img_clk.html',
      flags             => 0,
      app_format_id     => $app_format }) ;

    my $creative = $ns->create(Creative => {
      name              => "video #$i (MP4_DURATION=".($i == 1 ? '15)' : '30)'),
      account_id        => $self->{account},
      size_id           => $creative_size,
      template_id       => $template });

    my $option = $ns->create(Options => {
      token             => 'MP4_DURATION',
      type              => 'Integer',
      template_id       => $template,
      option_group_id   => $template->{option_group_id}});

    my $cr_opt_value = $ns->create(CreativeOptionValue => {
      option_id         => $option,
      creative_id       => $creative,
      value             => ($i == 1 ? '15' : '30')});

    my $campaign = undef;
    if ($is_ron)
    {
      $campaign = $ns->create(DisplayCampaign => {
        name                               => "vast campaign #$i",
        channel_id                         => undef,
        campaigncreativegroup_flags        => DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON,
        account_id                         => $self->{account},
        creative_id                        => $creative,
        cpm                                => 2000,
        campaigncreativegroup_country_code => 'RU',
        site_links                         => [ { site_id => $vast->{site_id} },
                                                { site_id => $self->{default_openrtb}->{site_id} } ] });
    }
    else
    {
      $campaign = $ns->create(DisplayCampaign => {
        name                               => "vast campaign #$i",
        account_id                         => $self->{account},
        creative_id                        => $creative,
        cpm                                => 2100,
        behavioralchannel_url_list         => $url,
        campaigncreativegroup_country_code => 'RU',
        site_links                         => [ { site_id => $vast->{site_id} },
                                                { site_id => $self->{default_openrtb}->{site_id} } ] });

      $ns->create(
        DB::BehavioralChannel::BehavioralParameter->blank(
          channel_id => $campaign->{channel_id},
          trigger_type => "U" ));

    }

    if ($i == 1)
    {
      $ns->create(CreativeCategory_Creative => {
        creative_category_id => $self->{categories}->{visual},
        creative_id => $campaign->{creative_id} });
    }

    if (defined $url)
    {
      $ns->output("URL#$i",       $url               );
      $ns->output("HTTP_URL#$i",  "http://$url"      );
      $ns->output("HTTPS_URL#$i", "https://$url"     );
    }
    $ns->output("CC#$i",  $campaign->{cc_id} );
    $ns->output("CREATIVE#$i",  $campaign->{creative_id} );
    $ns->output("CCG#$i", $campaign->{ccg_id});

    $ns->output('ACCOUNT', DB::Defaults::instance()->openx_account);
  }
}

sub allyes_extensions_test_
{
my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace('Allyes');

  my $allyes = $ns->create(Publisher => {
    name => 'Allyes',
    account_id => DB::Defaults::instance()->allyes_account,
    tag_id => undef } );

  my $url = 'www.' . make_autotest_name($ns, "url"). '.com';
  $ns->output("URL", $url);
  my $channel_id = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $self->{account},
      url_list => $url,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U') ]));

  my %display_campaigns = (
    default => {
      creative_tag_sizes => [$self->{sizes}->{'728x90'}] },
    no_imp_track => {
      creative_tag_sizes => [$self->{sizes}->{'160x600'}] },
    cat_fill => {
      creative_tag_sizes => [$self->{sizes}->{'240x400'}],
      creative_category_id => [ $self->{categories}->{visual}, $self->{categories}->{content} ] }
  );

  while (my ($name, $args) = each %display_campaigns)
  {
    my $campaign = $ns->create(DisplayCampaign => {
      name => $name,
      account_id => $self->{account},
      campaigncreativegroup_cpm => 1000,
      channel_id => $channel_id,
      campaigncreativegroup_country_code => 'RU',
      site_links => [ { site_id => $allyes->{site_id} },
                      { site_id => $self->{default_openrtb}->{site_id} } ],
      %$args });

    $ns->output("CCIDS/$name", $campaign->{cc_id});
    $ns->output("CREATIVEIDS/$name", $campaign->{creative_id});
    $ns->output("CREATIVES/$name", get_tanx_creative($campaign->{Creative}));

    foreach (@{$args->{creative_tag_sizes}})
    {
      $ns->create(PricedTag => {
        name => $name,
        site_id => $allyes->{site_id},
        size_id => $_ });
    }
  }

  my %text_campaigns = (
    text1 => {
      campaigncreativegroup_cpm => 2070,
      campaigncreativegroup_flags =>
        DB::Campaign::RON | DB::Campaign::INCLUDE_SPECIFIC_SITES,
      channel_id => undef },
    text2 => {
      campaigncreativegroup_cpm => 2050,
      campaigncreativegroup_flags =>
        DB::Campaign::RON | DB::Campaign::INCLUDE_SPECIFIC_SITES, 
      channel_id => undef } );

  while (my ($name, $args) = each %text_campaigns)
  {
    my $campaign = $ns->create(ChannelTargetedTACampaign => {
      name => $name,
      account_id => $self->{account},
      template_id => DB::Defaults::instance()->text_template,
      creative_tag_sizes => [DB::Defaults::instance()->text_size],
      size_type_id => DB::Defaults::instance()->other_size_type,
      rate_type => 'CPM',
      campaigncreativegroup_country_code => 'RU',
      site_links => [{ site_id => $allyes->{site_id} },
                     { site_id => $self->{default_openrtb}->{site_id} }],
      %$args });

    $ns->output("CCIDS/$name", $campaign->{cc_id});
    $ns->output("CREATIVEIDS/$name", $campaign->{creative_id});
    $ns->output("CREATIVES/$name", get_tanx_creative($campaign->{Creative}));
  }

  $ns->create(PricedTag => {
    name => '468x60',
    site_id => $allyes->{site_id},
    size_id => $self->{sizes}->{'468x60'} });

  $ns->output("ACCOUNT", DB::Defaults::instance()->allyes_account);
}

sub init {
  my ($self, $ns) = @_;

  $self->{account} = $ns->create(Account => {
    name => "default",
    text_adserving => "M",
    role_id => DB::Defaults::instance()->advertiser_role });

  $self->{default_openrtb} = $ns->create(Publisher => {
    name => 'DefaultOpenRTB',
    account_id => DB::Defaults::instance()->openrtb_account,
    tag_id => undef });

  my %sizes = ('728x90' => {},
               '468x60'=> { max_text_creatives => 3 },
               '160x600' => { disabled_imp_track => 1 },
               '240x400' => {} );


  while  (my ($name, $args) = each %sizes)
  {
    my $size_args;
    $size_args->{name} = $name;
    ($size_args->{width}, $size_args->{height}) = split('x', $name);
    $size_args->{max_text_creatives} = defined $args->{max_text_creatives}
      ? $args->{max_text_creatives} : 0;

    $self->{sizes}->{$name} = $ns->create(CreativeSize => $size_args);
    $ns->output("Sizes/$name", $self->{sizes}->{$name}->{protocol_name});

    foreach my $format (DB::Defaults::instance()->html_format,
                        DB::Defaults::instance()->js_format)
    {
      $ns->create(TemplateFile => {
        template_id => $size_args->{max_text_creatives}
          ? DB::Defaults::instance()->text_template()
          : DB::Defaults::instance()->display_template(),
        size_id => $self->{sizes}->{$name},
        template_file => 'UnitTests/raw_tokens.html',
        flags => defined $args->{disabled_imp_track}
          ? 0
          : DB::TemplateFile::PIXEL_TRACKING,
        app_format_id => $format });
    }

    $ns->create(PricedTag => {
      name => $name,
      site_id => $self->{default_openrtb}->{site_id},
      size_id => $self->{sizes}->{$name} });
  }

  my %categories = (
    visual => {
      cct_id => 0, # visual
      allyes_key => 1,
      iab_key => 1010 },
    content => {
      cct_id => 1, # content
      openx_key => 12345,
      allyes_key => 'allyes-cnt-cat' }
);

  while (my ($name, $args) = each %categories)
  {
    $self->{categories}->{$name} =
      $ns->create(CreativeCategory => { name => $name, %$args });
    foreach my $key (keys %$args)
    {
      $ns->output("CATEGORIES/$name/$key", $args->{$key}) if $key =~ m/^.*_key$/;
    }
  }

  $ns->output("OpenRTB/ACCOUNT", DB::Defaults::instance()->openrtb_account);

  $self->openx_extensions_case_($ns);
  $self->vast_extensions_case_($ns);
  $self->allyes_extensions_test_($ns);
}

1;
