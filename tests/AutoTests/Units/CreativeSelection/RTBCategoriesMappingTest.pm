package RTBCategoriesMappingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use POSIX qw(strftime mktime);
use encoding 'utf8';

use constant VISUAL => 0;
use constant CONTENT => 1;

sub init
{
  my ($self, $ns) = @_;

  my $rus_char = "RTB_ADSC_cnt_spec_3ы";
  Encode::_utf8_on($rus_char);

  my %categories = (
    'vis-1-1' => { type => VISUAL,
                   iab => "RTB-ADSC-vis-1-open" },
    'vis-1-2' => { type => VISUAL,
                   tanx => 14012 },
    'vis-1-3' => { type => VISUAL,
                   openx => 111 },
    'vis-1' => { type => VISUAL,
                 iab => 1010,
                 tanx => 14001,
                 openx => 110,
                 allyes => 1,
                 baidu => 2000 },
    'vis-2' => { type => VISUAL,
                 iab => 1020,
                 tanx => 14002,
                 openx => 120,
                 allyes => 2,
                 baidu => 2001 },
    'vis-3' => { type => VISUAL,
                 iab => 1034,
                 tanx => 14034,
                 openx => 134,
                 allyes => 34,
                 baidu => 1234 },
    'vis-4' => { type => VISUAL,
                 iab => 1034,
                 tanx => 14034,
                 openx => 134,
                 allyes => 34,
                 baidu => 1234 },
    'vis-5' => { type => VISUAL },
    'cnt-1-1' => { type => CONTENT,
                   iab => "RTB-ADSC-cnt-1-open" },
    'cnt-1-2' => { type => CONTENT,
                   tanx => 14112 },
    'cnt-1-3' => { type => CONTENT,
                   openx => 211 },
    'cnt-1' => { type => CONTENT,
                 iab => "RTB-ADSC cnt-1",
                 tanx => 14101,
                 openx => 210,
                 allyes => 'allyes-cnt-1',
                 baidu => 2041 },
    'cnt-2' => { type => CONTENT,
                 iab => "RTB-ADSC-cnt-2",
                 tanx => 14102,
                 openx => 220,
                 allyes => 'allyes-cnt-2' },
    'cnt-3' => { type => CONTENT,
                 iab => "RTB-ADSC-cnt-3-4",
                 tanx => 14134,
                 openx => 234,
                 allyes => 'allyes-cnt-34',
                 baidu => 2034 },
    'cnt-4' => { type => CONTENT,
                 iab => "RTB-ADSC-cnt-3-4",
                 tanx => 14134,
                 openx => 234,
                 allyes => 'allyes-cnt-34',
                 baidu => 2034 },
    'cnt-5' => { type => CONTENT },
    'cnt-s-1' => { type => CONTENT,
                   iab => "RTB_ADSC_cnt_spec_1'" },
    'cnt-s-2' => { type => CONTENT,
                   iab => "RTB_ADSC_cnt_spec_2\\" },
    'cnt-s-3' => { type => CONTENT,
                   iab => $rus_char },
    'cnt-s-4' => { type => CONTENT,
                   iab => "RTB_ADSC_cnt_spec_4\nabc" },
    'text' => { type => VISUAL,
                iab => 12,
                tanx => 5 }
  );

  # Create categories
  while (my ($name, $fields) = each %categories)
  {
    $self->{categories}->{$name} = $ns->create(CreativeCategory =>
      { name => $name,
        cct_id => $fields->{type},
        iab_key => $fields->{iab},
        tanx_key => $fields->{tanx},
        openx_key => $fields->{openx},
        allyes_key => $fields->{allyes},
        baidu_key => $fields->{baidu} });

    $ns->output("CATEGORIES/$name/iab", $fields->{iab})
      if defined $fields->{iab};
    $ns->output("CATEGORIES/$name/openx", $fields->{openx})
      if defined $fields->{openx};
    $ns->output("CATEGORIES/$name/tanx", $fields->{tanx})
      if defined $fields->{tanx};
    $ns->output("CATEGORIES/$name/allyes", $fields->{allyes})
      if defined $fields->{allyes};
    $ns->output("CATEGORIES/$name/baidu", $fields->{baidu})
      if defined $fields->{baidu};
  }

  $ns->output("CATEGORIES/vis-6/iab", 1000);
  $ns->output("CATEGORIES/vis-7/iab", 2000);
  $ns->output("CATEGORIES/vis-7/tanx", 2222);
  $ns->output("CATEGORIES/cnt-6/iab", "RTB-unexisting-cat-1");
  $ns->output("CATEGORIES/cnt-6/tanx", 1111);
  $ns->output("CATEGORIES/cnt-7/iab", "RTB-unexisting-cat-2");
  $ns->output("CATEGORIES/cnt-7/tanx", 22);
  $ns->output("CATEGORIES/unexist", 2051);

  my $currency1 = $ns->create(Currency => { rate => 11 });
  my $iab = $ns->create(Publisher => {
    name => 'iab',
    account_type_id => DB::Defaults::instance()->tanx_account->{account_type_id},
    currency_id => $currency1,
    tag_id => undef });

  my $openx = $ns->create(Publisher => {
    name => 'openx', 
    account_type_id => DB::Defaults::instance()->tanx_account->{account_type_id}, 
    currency_id => $currency1,
    tag_id => undef });

  my $currency2 = $ns->create(Currency => { rate => 12 });
  my $tanx = $ns->create(Publisher => {
    name => 'tanx',
    account_type_id => DB::Defaults::instance()->tanx_account->{account_type_id},
    currency_id => $currency2,
    tag_id => undef  });

  my $baidu = $ns->create(Publisher => {
    name => 'baidu',
    account_type_id => DB::Defaults::instance()->tanx_account->{account_type_id},
    currency_id => $currency2,
    tag_id => undef });

  $ns->output("iab/ACCOUNT_ID", $iab->{account_id});
  $ns->output("OpenX/ACCOUNT_ID", $openx->{account_id});
  $ns->output("TanX/ACCOUNT_ID", $tanx->{account_id});
  $ns->output("Baidu/ACCOUNT_ID", $baidu->{account_id});

  my @sizes = ( "120x240", "120x600", "160x600",
                "240x400", "250x250", "300x250",
                "728x90", "468x61", "350x180", 
                "336x280" );

  # Create sizes
  foreach (@sizes)
  {
    my ($width, $height) = split('x', $_);

    $self->{sizes}->{$_} = $ns->create(CreativeSize => {
      name => $_,
      width => $width,
      height => $height });

    $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->display_template(),
      size_id => $self->{sizes}->{$_},
      app_format_id => DB::Defaults::instance()->html_format });

    $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->text_template(),
      size_id => $self->{sizes}->{$_},
      app_format_id => 
        DB::Defaults::instance()->html_format,
      template_file => 'UnitTests/textad.xsl',
      template_type => 'X' });

    # Tag for iab
    $ns->create(PricedTag => {
      name => "iab-$_",
      site_id => $iab->{site_id},
      size_id => $self->{sizes}->{$_} });

    # Tag for openx
    $ns->create(PricedTag => {
      name => "openx-$_",
      site_id => $openx->{site_id},
      size_id => $self->{sizes}->{$_} });

    # Tag for tanx
    $ns->create(PricedTag => {
      name => "tanx-$_",
      site_id => $tanx->{site_id},
      size_id => $self->{sizes}->{$_} });

    # Tag for baidu
    $ns->create(PricedTag => {
      name => "baidu-$_",
      site_id => $baidu->{site_id},
      size_id => $self->{sizes}->{$_} });

    $ns->output("Sizes/$_", $self->{sizes}->{$_}->{protocol_name});
  }

  my $keyword = make_autotest_name($ns, "RTBCategories");

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'S',
          time_to => 3*60*60) ]));

  my %campaigns = (
    'RTB' => {
      campaign_args => {
        advertiser_currency_id => $currency1,
        campaigncreativegroup_cpm => 2200,
        channel_id => $channel,
        campaigncreativegroup_country_code => 'RU',
        campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
        site_links => [ { site_id => $iab->{site_id} },
                        { site_id => $openx->{site_id} } ]},
      creatives => [
        { size => '120x240',
          categories => [ 'vis-1-1', 'cnt-1-2' ] },
        { size => '120x600',
          categories => [ 'vis-1-2', 'cnt-1-1' ] },
        { size => '160x600',
          categories => [ 'vis-1', 'vis-2', 'cnt-1' ] },
        { size => '240x400',
          categories => [ 'vis-1', 'cnt-1', 'cnt-2' ] },
        { size => '250x250',
          categories => [ 'vis-3', 'vis-4', 'cnt-3', 'cnt-4' ] },
        { size => '300x250',
          categories => [ 'vis-5', 'cnt-5' ] },
        { size => '728x90',
          categories => [ 'cnt-s-1', 'cnt-s-2', 'cnt-s-3', 'cnt-s-4' ] },
        { size => '350x180',
          categories => [ 'vis-1-1', 'vis-1', 'cnt-1-1', 'cnt-1' ] } ] },
    'RTB-RON' => {
      campaign_args => {
        advertiser_currency_id => $currency1,
        campaigncreativegroup_cpm => 2000,
        channel_id => undef, # RON
        campaigncreativegroup_country_code => 'RU',
        campaigncreativegroup_flags =>
          DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON,
        site_links => [ { site_id => $iab->{site_id} },
                        { site_id => $openx->{site_id} } ] },
      creatives => [
        { size => '120x240' },
        { size => '120x600' },
        { size => '160x600' },
        { size => '240x400' },
        { size => '250x250' },
        { size => '728x90' },
        { size => '468x61' }] },
    'TANX' => {
      campaign_args => {
        advertiser_currency_id => $currency2,
        campaigncreativegroup_cpm => 11,
        channel_id => $channel,
        campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
        campaigncreativegroup_country_code => 'GB',
        site_links => [ { site_id => $tanx->{site_id} } ] },
      creatives => [
        { size => '120x240',
          categories => [ 'vis-1-2', 'cnt-1-1' ] },
        { size => '120x600',
          categories => [ 'vis-1-1', 'cnt-1-2' ] },
        { size => '160x600',
          categories => [ 'vis-1', 'vis-2', 'cnt-1' ] },
        { size => '240x400',
          categories => [ 'vis-1', 'cnt-1', 'cnt-2' ] },
        { size => '250x250',
          categories => [ 'vis-3', 'vis-4', 'cnt-3', 'cnt-4' ] },
        { size => '300x250',
          categories => [ 'vis-5', 'cnt-5' ] }, ] },
    'TANX-RON' => {
      campaign_args => {
        advertiser_currency_id => $currency2,
        campaigncreativegroup_cpm => 10,
        channel_id => undef, # RON
        campaigncreativegroup_country_code => 'GB',
        campaigncreativegroup_flags =>
          DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON,
        site_links => [ { site_id => $tanx->{site_id} } ] },
      creatives => [
        { size => '120x240' },
        { size => '120x600' } ]},
    'BAIDU' => {
      campaign_args => {
        advertiser_currency_id => $currency2,
        campaigncreativegroup_cpm => 500,
        channel_id => $channel, # RON
        campaigncreativegroup_country_code => 'GB',
        campaigncreativegroup_flags =>
          DB::Campaign::INCLUDE_SPECIFIC_SITES,
        site_links => [ { site_id => $baidu->{site_id} } ] },
      creatives => [
        { size => '120x240' },
        { size => '240x400',
          categories => [ 'vis-1', 'cnt-3', 'cnt-4' ]  },
        { size => '300x250',
          categories => [ 'vis-5', 'cnt-5'] },
        { size => '250x250',
          categories => [ 'vis-2', 'vis-3', 'cnt-1'] },
        { size => '336x280',
          categories => [ 'vis-3', 'cnt-1', 'cnt-3'] } ]}
  );


  # Creating campaigns
  while (my ($name, $args) = each %campaigns)
  {
    $campaigns{$name}->{campaign_args}->{name} = $name;
    $campaigns{$name}->{campaign_args}->{creative_id} = undef;
    $campaigns{$name}->{campaign_args}->{cc_id} = undef;
    my $campaign = $ns->create(DisplayCampaign =>
      $campaigns{$name}->{campaign_args});

    $ns->output("$name/CCG_ID", $campaign->{ccg_id});

    foreach my $creative_args (@{$campaigns{$name}->{creatives}})
    {
      die "Size '$creative_args->{size}' wasn't previously created: " .
          "need to create it first before using for creative"
        unless exists $self->{sizes}->{$creative_args->{size}};
      my $creative = $ns->create(Creative => {
        name => "$name-" . $self->{sizes}->{$creative_args->{size}}->{name},
        account_id => $campaign->{account_id},
        size_id => $self->{sizes}->{$creative_args->{size}},
        creative_category_id =>
          [ map {$self->{categories}->{$_}} @{$creative_args->{categories}} ] });
      my $cc = $ns->create(CampaignCreative => {
        ccg_id => $campaign->{ccg_id},
        creative_id => $creative });

      $ns->output(
        "CREATIVE/$name/$creative_args->{size}/TANX", 
                  get_tanx_creative($creative));

      $ns->output(
        "CREATIVE/$name/$creative_args->{size}/BAIDU", 
                  get_baidu_creative($creative));

      $ns->output("CCIDS/$name/$creative_args->{size}", $cc);
      $ns->output("CREATIVEIDS/$name/$creative_args->{size}", $creative);
    }
  }

  # Creating text campaign
  my $text = $ns->create(ChannelTargetedTACampaign => {
    name => "TEXT",
    creative_tag_sizes => [$self->{sizes}->{'468x61'}],
    template_id =>  DB::Defaults::instance()->text_template,
    app_format_id => 
      DB::Defaults::instance()->html_format,
    channel_id => undef,
    advertiser_currency_id => $currency1,
    campaigncreativegroup_flags =>
      DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON,
    campaigncreativegroup_country_code => 'RU',
    cpm => 2070,
    creative_category_id => $self->{categories}->{text},
    site_links => [ { site_id => $iab->{site_id} },
                    { site_id => $openx->{site_id} } ] });

  $ns->output("TEXT/CCG_ID", $text->{ccg_id});
  $ns->output("CCIDS/TEXT/468x61", $text->{cc_id});
  $ns->output("CREATIVEIDS/TEXT/468x61", $text->{creative_id});
  $ns->output("SEARCH", "http://search.live.de/results.aspx?q=" . $keyword);
}

1;
