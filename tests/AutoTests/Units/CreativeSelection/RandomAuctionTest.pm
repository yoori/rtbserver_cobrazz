
package RandomAuctionTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant COUNTRY_CODE => 'LC';

sub create_sizes
{
  my ($self, $ns, $args, $format) = @_;
  my @sizes = ();
  my $idx = 0;
  foreach my $arg (@$args)
  {
    my $size = $ns->create(CreativeSize => {
      name => "Size$idx",
      size_type_id =>
        DB::Defaults::instance()->other_size_type,
      max_text_creatives => $arg });

    $ns->output("SIZE/$idx", $size->{protocol_name});
    $format = DB::Defaults::instance()->app_format_no_track
      unless defined $format;

    $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->display_template,
      app_format_id => $format,
      size_id => $size});

    $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->text_template,
      size_id => $size,
      app_format_id => $format,
      template_file => 'UnitTests/textad.xsl',
      template_type => 'X'});

    push(@sizes, $size);
    ++$idx;
  }

  return @sizes;
}

sub create_rtb_campaigns
{
  my ($self, $ns, $sites, $currency, $args) = @_;

  my $i  = 0;

  for my $a (@$args)
  {
    my %arg = 
    (
      name => "Campaign" . ++$i,
      country_code => COUNTRY_CODE,
      campaigncreativegroup_cpm => $a->{cpm},
      campaigncreativegroup_cpc => $a->{cpc},
      advertiser_currency_id => $currency,
      cc_id => undef,
      creative_id => undef,
      site_links => [ map { site_id => $_->{site_id} }, @$sites ]
    );

    $arg{campaigncreativegroup_rate_type} = $a->{rate_type} 
       if $a->{rate_type};

    my $channel;    
    my $url = make_autotest_name($ns, $i) . ".openrtb.com";
    my $keyword = make_autotest_name($ns, "kwd" . $i);

    if ($a->{ron})
    {
      $arg{campaigncreativegroup_flags} = 
        DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON;
      $arg{channel_id} = undef;
    }
    else
    {
      $channel = $ns->create(
        DB::BehavioralChannel->blank(
          name => 'Channel' . $i,
           url_list => $url,
           keyword_list => $keyword,
           behavioral_parameters => [
             DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U'),
             DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P'),
             DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S') ]));

      $ns->output('URL/' . $i, $url);
      $ns->output('KWD/' . $i, $keyword);
    }

    if ($a->{entity} eq 'TextAdvertisingCampaign')
    {
      $arg{ccgkeyword_channel_id} = $channel;
      $arg{ccgkeyword_max_cpc_bid} = $a->{cpc};
      $arg{ccgkeyword_original_keyword} = $keyword;
      $arg{ccgkeyword_trigger_type} = 'S';
    }
    else
    {
      $arg{channel_id} = $channel;
    }

    my $campaign = $ns->create($a->{entity} => \%arg);

    $ns->output('CCG/' . $i, $campaign->{ccg_id});

    my $j = 0;

    for my $size (@{$a->{sizes}})
    {
      my $creative = $ns->create(Creative => {
        name => $i . "-" . ++$j,
        account_id => $campaign->{account_id},
        template_id => 
          $a->{entity} eq 'DisplayCampaign'?
            DB::Defaults::instance()->display_template:
              DB::Defaults::instance()->text_template,
        tag_sizes => [$size] });
      
      my $cc = $ns->create(CampaignCreative => {
        ccg_id => $campaign->{ccg_id},
        creative_id => $creative });
      
      $ns->output('CC/' . $i . "/" . $j, $cc);
      $ns->output('CREATIVE/' . $i . "/" . $j, get_tanx_creative($creative));
      $ns->output('CREATIVEID/' . $i . "/" . $j, $creative);
    }
  }
}

sub create_sites
{
  my ($self, $ns, $args) = @_;

  my @sites = ();

  my $i  = 0;
  for my $arg (@$args)
  {
    my $site = $ns->create(Site => {
      name => "Site-" . ++$i,
      account_id => $arg->{account},
    });

    push(@sites, $site);

    my $j  = 0;
    for (@{$arg->{tags}})
    {
      my $tag = $ns->create(PricedTag => {      
        name => "Tag-" . $i . "-" . ++$j,
        site_id => $site,
        size_id => $_->{size},
        cpm => $_->{cpm},
        max_ecpm_share => $_->{max_ecpm},
        prop_probability_share => $_->{probability},
        random_share => $_->{random} });
      $ns->output('Tag/' . $i . "/" . $j, $tag);
    }
  }

  return @sites;
}

sub create_display_campaigns
{
  my ($self, $ns, $publisher, $args) = @_;

  my $i = 0;
  for my $arg (@$args)
  {
    my $keyword = make_autotest_name($ns, "KWDDISP" . ++$i);

    my $campaign = $ns->create(DisplayCampaign => {
      name => "Display" . $i,
      country_code => COUNTRY_CODE,
      channel_id => 
         DB::BehavioralChannel->blank(
           name => 'Display' . $i,
           keyword_list => $keyword,
           behavioral_parameters => [
              DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
      campaigncreativegroup_cpm => $arg->{cpm},
      campaigncreativegroup_cpc => $arg->{cpc},
      campaigncreativegroup_ctr => 0.01,
      creative_tag_sizes => $arg->{sizes},
      site_links => [{site_id => $publisher->{site_id}} ] });

    $ns->output('DISP/KWD/' . $i, $keyword);
    $ns->output('DISP/CC/' . $i , $campaign->{cc_id});
  }
}

sub create_channel_campaigns
{
  my ($self, $ns, $publisher, $args) = @_;

  my $i = 0;
  for my $arg (@$args)
  {
    my $keyword = make_autotest_name($ns, "KWDCH" . ++$i);

    my $campaign = $ns->create(ChannelTargetedTACampaign => {
      name => "ChannelText" . $i,
      template_id => DB::Defaults::instance()->text_template,
      country_code => COUNTRY_CODE,
      channel_id => 
         DB::BehavioralChannel->blank(
           name => 'ChannelText' . $i,
           keyword_list => $keyword,
           behavioral_parameters => [
              DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
      campaigncreativegroup_cpm => $arg->{cpm},
      campaigncreativegroup_cpc => $arg->{cpc},
      campaigncreativegroup_ctr => 0.01,
      creative_tag_sizes => $arg->{sizes},
      site_links => [{site_id => $publisher->{site_id}} ] });

    $ns->output('CH/KWD/' . $i, $keyword);
    $ns->output('CH/CC/' . $i , $campaign->{cc_id});
  }
}

sub random_text_1
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('RANDOMTEXT1');

  my @sizes = $self->create_sizes($ns, [1, 1, 1]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1], $sizes[2]],
    pricedtag_random_share => 100,
    pricedtag_adjustment => 1.0,
    pubaccount_country_code => COUNTRY_CODE });

  $self->create_display_campaigns(
    $ns, $publisher,
    [ { cpc => 3, sizes => [$sizes[0]] },
      { cpc => 2, sizes => [$sizes[1]] },
      { cpc => 1, sizes => [$sizes[2]] } ]);

 $self->create_channel_campaigns(
    $ns, $publisher,
    [ { cpc => 4, sizes => [$sizes[0], $sizes[1], $sizes[2]] } ]);

  $ns->output('TAG', $publisher->{tag_id});
}

sub random_text_2
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('RANDOMTEXT2');

  my @sizes = $self->create_sizes($ns, [5, 2, 0, 2]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1], $sizes[2], $sizes[3]],
    pricedtag_random_share => 100,
    pricedtag_adjustment => 1.0,
    pubaccount_country_code => COUNTRY_CODE });

 $self->create_channel_campaigns(
    $ns, $publisher,
    [ { cpc => 1, sizes => [$sizes[1], $sizes[2], $sizes[3]] },
      { cpc => 2, sizes => [$sizes[1]] },
      { cpc => 3, sizes => [$sizes[2]] },
      { cpc => 4, sizes => [$sizes[3]] } ]);

  $ns->output('TAG', $publisher->{tag_id});
}

sub creative_size
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('CREATIVESIZE');

  my $internal = $ns->create(Account => {
    name => 'Internal',
      internal_account_id => undef,
      role_id => DB::Defaults::instance()->internal_role,
      account_type_id => DB::Defaults::instance()->internal_type,
      max_random_cpm => 2000 });

  my @sizes = $self->create_sizes($ns, [4, 1, 2]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1], $sizes[2]],
    pricedtag_random_share => 100,
    pricedtag_cpm => 2000,
    pricedtag_adjustment => 1.0,
    pubaccount_internal_account_id => $internal,
    pubaccount_country_code => COUNTRY_CODE });

 $self->create_channel_campaigns(
    $ns, $publisher,
    [ { cpc => 0.4, sizes => [$sizes[0]] },
      { cpc => 0.3, sizes => [$sizes[0]] },
      { cpc => 0.2, sizes => [$sizes[0]] },
      { cpc => 0.05, sizes => [$sizes[2]] },
      { cpc => 0.04, sizes => [$sizes[2]] },
      { cpc => 0.03, sizes => [$sizes[2]] },
      { cpc => 0.02, sizes => [$sizes[2]] },
      { cpc => 0.01, sizes => [$sizes[2]] } ]);

  $self->create_display_campaigns(
    $ns, $publisher,
    [ { cpc => 1, sizes => [$sizes[0]] } ]);


  $ns->output('TAG', $publisher->{tag_id});
}

sub proportional_1
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('PROPORTIONAL1');

  my @sizes = $self->create_sizes($ns, [1, 1, 1]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1], $sizes[2]],
    pricedtag_prop_probability_share => 100,
    pricedtag_adjustment => 1.0,
    pubaccount_country_code => COUNTRY_CODE });

  $self->create_display_campaigns(
    $ns, $publisher,
    [ { cpm => 20, sizes => [$sizes[0]] },
      { cpm => 20, sizes => [$sizes[1]] },
      { cpm => 20, sizes => [$sizes[2]] } ]);

 $self->create_channel_campaigns(
    $ns, $publisher,
    [ { cpm => 20, sizes => [$sizes[0], $sizes[1], $sizes[2]] } ]);

  $ns->output('TAG', $publisher->{tag_id});
}

sub proportional_2
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('PROPORTIONAL2');

  my @sizes = $self->create_sizes($ns, [0, 4, 0, 2]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1], $sizes[2], $sizes[3]],
    pricedtag_prop_probability_share => 100,
    pricedtag_adjustment => 1.0,
    pubaccount_country_code => COUNTRY_CODE });

 $self->create_channel_campaigns(
    $ns, $publisher,
    [ { cpm => 20, sizes => [$sizes[1], $sizes[2], $sizes[3]] },
      { cpm => 20, sizes => [$sizes[1]] },
      { cpm => 20, sizes => [$sizes[2]] },
      { cpm => 20, sizes => [$sizes[3]] } ]);

  $ns->output('TAG', $publisher->{tag_id});
}

sub open_rtb
{
  my ($self, $namespace, $sizes) = @_;
  my $ns = $namespace->sub_namespace('RTB');

  my $currency1 = $ns->create(
      DB::Currency->blank(rate => 30));

  my $currency2 = $ns->create(
    DB::Currency->blank(rate => 3.35));

  my $publisher1 = $ns->create(PubAccount => {
    name => "Publisher1",
    currency_id => $currency1,
    country_code => COUNTRY_CODE,
    internal_account_id => 
         DB::Internal->blank(
            name => 'Internal1',
            country_code => COUNTRY_CODE,
            currency_id => $currency1,
            max_random_cpm => 530 )});

  my $publisher2 = $ns->create(PubAccount => {
    name => "Publisher2",
    currency_id => $currency1,
    country_code => COUNTRY_CODE,
    internal_account_id => 
         DB::Internal->blank(
            name => 'Internal2',
            country_code => COUNTRY_CODE,
            currency_id => $currency2,
            max_random_cpm => 59.18)});
  
  my @sites = 
    $self->create_sites(
       $ns, 
       [{ account => $publisher1, 
          tags => [{size => $sizes->[0], cpm => 0, 
                    max_ecpm => 1, probability => 0, 
                    random => 99 }] },
        { account => $publisher1, 
          tags => [{ size => $sizes->[1], cpm => 0, 
                     max_ecpm => 0, probability => 0, 
                     random => 100 }] },
        { account => $publisher1, 
          tags => [{ size => $sizes->[2], cpm => 0, 
                     max_ecpm => 1, probability => 1, 
                     random => 98 }] },
        { account => $publisher1, 
          tags => [{ size => $sizes->[3], cpm => 0, 
                     max_ecpm => 1, probability => 1, 
                     random => 98 }] },
        { account => $publisher1, 
          tags => [{ size => $sizes->[4], cpm => 0,
                     max_ecpm => 0, probability => 0, 
                     random => 100 }] },
        { account => $publisher1, 
          tags => [{ size => $sizes->[5], cpm => 0, 
                     max_ecpm => 1, probability => 0, 
                     random => 99 }] },
        { account => $publisher1, 
          tags => [{ size => $sizes->[6], cpm => 0.01, 
                     max_ecpm => 1, probability => 0, 
                     random => 99 }] },
        { account => $publisher2, 
          tags => [{ size => $sizes->[0], cpm => 0, 
                     max_ecpm => 1, probability => 0, 
                     random => 99 }] },
        { account => $publisher2, 
          tags => [{ size => $sizes->[1], cpm => 0, 
                     max_ecpm => 0, probability => 0, 
                     random => 100 }] }]);

  
  $self->create_rtb_campaigns(
    $ns, \@sites, $currency1,
    [ { entity => 'DisplayCampaign', cpm => 2000, 
        sizes => [$sizes->[2], $sizes->[0], $sizes->[3]], ron => 1 },
      { entity => 'DisplayCampaign', cpm => 2100, 
        sizes => [$sizes->[2], $sizes->[0], $sizes->[3]]  },
      { entity => 'DisplayCampaign', cpc => 2100, 
        sizes => [ $sizes->[1], $sizes->[2], $sizes->[0]] },
      { entity => 'DisplayCampaign', cpc => 100, sizes => [ $sizes->[1]] },
      { entity => 'DisplayCampaign', cpc => 0, sizes => [ $sizes->[5], $sizes->[6]]  },
      { entity => 'ChannelTargetedTACampaign', cpc => 1000, 
        sizes => [ $sizes->[4] ], ron => 1 },
      { entity => 'TextAdvertisingCampaign', cpc => 1200, 
        sizes => [ $sizes->[4] ] } ]);

  $ns->output('ACCOUNT/1', $publisher1);
  $ns->output('ACCOUNT/2', $publisher2);
}


sub tanx
{
  my ($self, $namespace, $sizes) = @_;
  my $ns = $namespace->sub_namespace('TANX');  

  my $currency1 = $ns->create(
    DB::Currency->blank(rate => 6));

  my $currency2 = $ns->create(
    DB::Currency->blank(rate => 3.35));

  my $publisher1 = $ns->create(PubAccount => {
    name => "Publisher1",
    currency_id => $currency1,
    country_code => COUNTRY_CODE,
    internal_account_id => 
         DB::Internal->blank(
            name => 'Internal1',
            country_code => COUNTRY_CODE,
            currency_id => $currency1,
            max_random_cpm => 106 )});

  my $publisher2 = $ns->create(PubAccount => {
    name => "Publisher2",
    currency_id => $currency1,
    country_code => COUNTRY_CODE,
    internal_account_id => 
         DB::Internal->blank(
            name => 'Internal2',
            country_code => COUNTRY_CODE,
            currency_id => $currency2,
            max_random_cpm => 59.18 )});

  
  my @sites = 
    $self->create_sites(
       $ns, 
       [{ account => $publisher1, 
          tags => [{size => $sizes->[0], cpm => 0, 
                    max_ecpm => 1, probability => 0, 
                    random => 99 }] },
        { account => $publisher1, 
          tags => [{size => $sizes->[1], cpm => 0, 
                    max_ecpm => 0, probability => 0, 
                    random => 100 },
                   {size => $sizes->[3], cpm => 0.01, 
                    max_ecpm => 1, probability => 0, 
                    random => 99 },
                   {size => $sizes->[5], cpm => 0, 
                    max_ecpm => 1, probability => 0, 
                    random => 99 },
                   {size => $sizes->[4], cpm => 0, 
                    max_ecpm => 0, probability => 0, 
                    random => 100 }] },
        { account => $publisher2, 
          tags => [{size => $sizes->[0], cpm => 0, 
                    max_ecpm => 1, probability => 0, 
                    random => 99 },
                   {size => $sizes->[1], cpm => 0,
                    max_ecpm => 0, probability => 0, 
                    random => 100 }] } ]);

  
  $self->create_rtb_campaigns(
    $ns, \@sites, $currency1,
    [ { entity => 'DisplayCampaign', cpm => 200, 
        sizes => [$sizes->[1], $sizes->[0]], ron => 1 },
      { entity => 'DisplayCampaign', cpc=> 110,
        sizes => [$sizes->[1], $sizes->[0]] },
      { entity => 'DisplayCampaign', 
        cpm => 0, cpc => 10, rate_type => 'CPM',
        sizes => [$sizes->[3], $sizes->[5]] },
      { entity => 'DisplayCampaign', cpm => 0.01,
        sizes => [$sizes->[3], $sizes->[5]] },
      { entity => 'ChannelTargetedTACampaign', cpc => 100,
        sizes => [$sizes->[4]], ron => 1 },
      { entity => 'TextAdvertisingCampaign', cpc => 110, 
        sizes => [ $sizes->[4] ] } ]);

  $ns->output('ACCOUNT/1', $publisher1);
  $ns->output('ACCOUNT/2', $publisher2);
}

sub init {
  my ($self, $ns) = @_;

  # Western Sahara
  $ns->create(Country => {
    country_code => COUNTRY_CODE,
    language => 'en',
    low_channel_threshold => 0,
    high_channel_threshold => 0,
    cpc_random_imps => 10000000,
    cpa_random_imps => 20000000 });

  $self->random_text_1($ns);
  $self->random_text_2($ns);
  $self->creative_size($ns);
  $self->proportional_1($ns);
  $self->proportional_2($ns);

  {
    my @sizes = $self->create_sizes($ns, [ 0, 0, 0, 0, 2, 0, 0],
      DB::Defaults::instance()->html_format);

    $self->open_rtb($ns, \@sizes);

    $self->tanx($ns, \@sizes);
  }

  $ns->output('COUNTRYCODE', COUNTRY_CODE);
  $ns->output('OpenRTBColo', DB::Defaults::instance()->openrtb_isp->{colo_id});
}

1;
