package ChannelPriceRangeLogging::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub get_keyword 
{
  my ($self, $name, $args) = @_;
  
  return 
    defined $args->{keyword}?
      make_autotest_name(
        $self->{ns_}, $args->{keyword}):
        defined $self->{keyword_}?
          $self->{keyword_}: 
            make_autotest_name(
              $self->{ns_}, $name);
}

sub get_advertiser
{
  my ($self, $name, $args) = @_;
  my $account = (defined $args->{country_code} or defined $args->{currency})?
    $self->{ns_}->create(Account => {
      name => 'Advertiser-' . $name,
      currency_id => 
        defined $args->{currency}?
          $args->{currency}: DB::Defaults::instance()->currency(),
      country_code => 
        defined $args->{country_code}?
          $args->{country_code}: DB::Defaults::instance()->country()->{country_code},
      role_id => DB::Defaults::instance()->advertiser_role }) : $self->{account_};

  return $account;
} 

sub create_publisher
{
  my ($self, $name, $args) = @_;

  my $publisher = 
      $self->{ns_}->create(Publisher => {
        name => "Publisher-". $name,
        pubaccount_currency_id => 
          defined $args->{currency}?
            $args->{currency}: DB::Defaults::instance()->currency(),
        pricedtag_size_id => 
          defined $args->{size}? 
            $args->{size}: DB::Defaults::instance()->size(),
        pricedtag_cpm => $args->{tag_cpm},
        pubaccount_country_code => defined $args->{country_code}?
          $args->{country_code}: DB::Defaults::instance()->country()->{country_code},
        pricedtag_adjustment => $args->{adjustment} ? $args->{adjustment} : 1.0});

  $self->{ns_}->output(
   'MINECPM/' . $name, $args->{tag_cpm});
  $self->{ns_}->output(
   'TAG/' . $name, $publisher->{tag_id});
  $self->{ns_}->output(
   'SITE/' . $name, $publisher->{site_id});

  return $publisher;
}

sub create_channel
{
  my ($self, $name, $args) = @_;

  my $keyword = $self->get_keyword($name, $args);
  my $channel = $self->{ns_}->create(DB::BehavioralChannel->blank(
    name => 
      'Channel-' . $name,
    account_id => $self->get_advertiser($name, $args),
    keyword_list => $keyword,
    channel_type => 
      defined $args->{type}? $args->{type}: 'B',
    country_code => defined $args->{country_code}
      ? $args->{country_code}
      : DB::Defaults::instance()->country()->{country_code},
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => 'P',
      minimum_visits => 
       defined $args->{min_visits}?
         $args->{min_visits}: 1,
      time_from =>
        defined $args->{time_from}?
          $args->{time_from}: 0,
      time_to => 
       defined $args->{time_to}?
         $args->{time_to}: 0) ]
    ));

  $self->{ns_}->output(
    'BP/' . $name,  $channel->page_key());
  $self->{ns_}->output(
    $name,  $channel->channel_id());
  $self->{ns_}->output(
    'Kwd/' . $name,  $keyword);
  $self->{storage}->{$name} = $channel;
}

sub create_expression
{
  my ($self, $name, $args) = @_;

  my $expression = $args->{expression};
  foreach my $w ( split(/\W/, $expression ) )
  {
    my $c = $self->{storage}->{$w};
    die "Channel '$w' is not defined " .
        "in the case '$self->{ns_}->namespace'" 
       if not defined $c;
    $expression =~ s/$w/$c->{channel_id}/;
  }
 
  my $channel = 
    $self->{ns_}->create(DB::ExpressionChannel->blank(
    name => 
      'Expression-' . $name,
    account_id => $self->get_advertiser($name, $args),
    country_code => defined $args->{country_code}
      ? $args->{country_code}
      : DB::Defaults::instance()->country()->{country_code},
    expression => $expression));

  $self->{ns_}->output(
     $name, $channel->channel_id());
  $self->{storage}->{$name} = $channel;
}

sub create {
  my ($self, $args, $name, $prefix, $fn) = @_;
  if (exists $args->{$name})
  {
    my $index = 0;
    foreach my $v (@{$args->{$name}})
    {
      $fn->($self, $prefix . ++$index, $v);
    }
  }
}

sub create_channel_ta_campaign
{
  my ($self, $name, $args) = @_;

  my $publisher = defined $args->{tag_cpm}?
    $self->create_publisher($name, $args): $self->{publisher_};

  my $channel = $self->{storage}->{$args->{channel}};

  my $campaign =  
      $self->{ns_}->create(ChannelTargetedTACampaign => {
      name => $name,
      size_id => defined $args->{size} ? $args->{size} : DB::Defaults::instance()->text_size,
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $channel,
      campaigncreativegroup_cpm => defined $args->{cpm} ? $args->{cpm} : 0,
      campaigncreativegroup_cpc => defined $args->{cpc} ? $args->{cpc} : 0,
      campaigncreativegroup_rate_type => defined $args->{cpc} ? 'CPC' : 'CPM',
      campaigncreativegroup_ctr => defined $args->{ctr} ? $args->{ctr} : undef, 
      site_links => [
        {site_id => $publisher->{site_id} }] });

  $self->{ns_}->output(
    'CC/' . $name, $campaign->{cc_id});
  $self->{ns_}->output(
    'CPM/' . $name, $args->{cpm}) if defined $args->{cpm};
  $self->{ns_}->output(
    'CPC/' . $name, $args->{cpc}) if defined $args->{cpc};
}

sub create_text_ta_campaign
{
  my ($self, $name, $args) = @_;

  my $publisher = defined $args->{tag_cpm}?
    $self->create_publisher($name, $args): $self->{publisher_};

  my $channel = $self->{storage}->{$args->{channel}};
  
  my $campaign = 
    $self->{ns_}->create(TextAdvertisingCampaign => { 
      name => $name,
      size_id => defined $args->{size} ? $args->{size} : DB::Defaults::instance()->text_size,
      template_id =>  DB::Defaults::instance()->text_template,
      original_keyword => $channel->{keyword_list_},
      ccgkeyword_channel_id => $channel->{channel_id},
      ccgkeyword_ctr => defined $args->{ctr} ? $args->{ctr} : 0.1,
      max_cpc_bid => $args->{cpc},
      site_links => [{site_id => $publisher->{site_id} }] });

  $self->{ns_}->output(
    'CC/' . $name, $campaign->{cc_id});
  $self->{ns_}->output(
    'CPC/' . $name, $args->{cpc});
}

sub create_display_campaign
{
  my ($self, $name, $args) = @_;

  my $channel = $self->{storage}->{$args->{channel}};

  die "Undefined channel_id for campaign '$self->{ns_}->namespace-$name'" 
      if not defined $channel;

  my $publisher = defined $args->{tag_cpm}?
    $self->create_publisher($name, $args): $self->{publisher_};

  my $campaign = $self->{ns_}->create(DisplayCampaign => {
    name => $name,
    size_id => 
      defined $args->{size}? 
        $args->{size}: DB::Defaults::instance()->size(),
    account_id => 
      $self->get_advertiser($name, $args),
    template_id => DB::Defaults::instance()->display_template(),
    channel_id => $channel,
    country_code => defined $args->{country_code}?
      $args->{country_code}: DB::Defaults::instance()->country()->{country_code},
    campaigncreativegroup_cpm => defined $args->{cpm} ? $args->{cpm} : 0,
    campaigncreativegroup_cpc => defined $args->{cpc} ? $args->{cpc} : 0,
    campaigncreativegroup_rate_type => defined $args->{cpc} ? 'CPC' : 'CPM',
    campaigncreativegroup_ctr => defined $args->{ctr} ? $args->{ctr} : undef,
    site_links => [
      { site_id => $publisher->{site_id} } ] }); 

  $self->{ns_}->create(TemplateFile => {
    template_id => DB::Defaults::instance()->display_template(),
    size_id => defined $args->{size}?
      $args->{size}: DB::Defaults::instance()->size(),
    template_file => 'UnitTests/banner_img_clk.html' });

  $self->{ns_}->output(
    'CC/' . $name,
    $campaign->{cc_id});
  $self->{ns_}->output(
    'CPM/' . $name,
    $args->{cpm}) if defined $args->{cpm};
  $self->{ns_}->output(
    'CPC/' . $name,
    $args->{cpc}) if defined $args->{cpc};
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{ns_} = $ns->sub_namespace($prefix);
  $self->{storage} = ();
  $self->{publisher_} = undef;
  $self->{keyword_} = undef;
  $self->{account_} = 
    $self->{ns_}->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });

  # Common publisher
  if (exists $args->{publisher})
  {
    $self->{publisher_} =
      $self->create_publisher('COMMON', $args->{publisher});
  }

  if (exists $args->{keyword})
  {
    $self->{keyword_} = 
        make_autotest_name(
        $self->{ns_}, $args->{keyword});

    $self->{ns_}->output('Kwd',  $self->{keyword_});
  }
  
  # Create channels
   $self->create($args, "channels", 'B', \&create_channel);

  # Create expressions
  $self->create($args, "expressions", 'Expr', \&create_expression);

  # Create display campaigns
  $self->create($args, "campaigns", 'Display',  \&create_display_campaign);

  # Create channel TA campaigns
  $self->create($args, "ta_channel", 'Channel',  \&create_channel_ta_campaign);

  # Create text TA campaigns
  $self->create($args, "ta_text", 'Text',  \&create_text_ta_campaign);

  return $self;
}

1;


package ChannelPriceRangeLogging;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub one_group
{
  my ($self, $ns) = @_;

  ChannelPriceRangeLogging::Case->new(
    $ns, 'ONE_GROUP', 
    { publisher => { tag_cpm => 5 },
      channels => [ { name => "B1" },
                    { name => "B2" } ],
      expressions => [ { expression => "B1" }, 
                       { expression => "B2" }, 
                       { expression => "Expr1" }, 
                       { expression => "Expr2" } ],
      campaigns => 
       [ { cpm => 10, channel => "Expr3" },
         { cpm => 10, channel => "Expr4" } ]
   });
}

sub tag_cpm
{
  my ($self, $ns) = @_;

 ChannelPriceRangeLogging::Case->new(
    $ns, 'TAG_CPM', 
    { publisher => { tag_cpm => 5 },
      channels => [ { name => "B1" }, 
                    { name => "B2" } ],
      expressions => [ { expression => "B1" }, 
                       { expression => "Expr1" }, 
                       { expression => "B2" } ],
      campaigns => 
       [ { cpm => 10, channel => "Expr2" },
         { cpm => 20, channel => "Expr2", tag_cpm => 5 } ]
   });
}

sub groups
{
  my ($self, $ns) = @_;

  ChannelPriceRangeLogging::Case->new(
    $ns, 'GROUPS', 
    { publisher => { tag_cpm => 5 },
      channels => [ { name => "B1" },
                    { name => "B2" },
                    { name => "B3" },
                    { name => "B4",  time_to => 60, 
                      country_code => DB::Defaults::instance()->test_country_1()->{country_code} } ],
      expressions => [ { expression => "B1" }, 
                       { expression => "B2" }, 
                       { expression => "Expr1" }, 
                       { expression => "Expr2" }, 
                       { expression => "B3" },
                       { expression => "B4", 
                         country_code => DB::Defaults::instance()->test_country_1()->{country_code} }],
      campaigns => 
       [ { cpm => 10, channel => "Expr3" },
         { cpm => 10, channel => "Expr4" },
         { cpm => 20, channel => "Expr3", tag_cpm => 5 },
         { cpm => 40, channel => "Expr3", tag_cpm => 5 },
         { cpm => 15, channel => "Expr3", tag_cpm => 5 },
         { cpm => 5, tag_cpm => 0, channel => "Expr6",
            size => DB::Defaults::instance()->size(),
            country_code => DB::Defaults::instance()->test_country_1()->{country_code} },
          { cpm => 10, tag_cpm => 1, channel => "Expr6", 
            size => DB::Defaults::instance()->size(),
            country_code => DB::Defaults::instance()->test_country_1()->{country_code} } ]
   });
}

sub day_switch
{
  my ($self, $ns) = @_;

  ChannelPriceRangeLogging::Case->new(
    $ns, 'DAY_SWITCH', 
    { publisher => { tag_cpm => 5 },
      channels => [ { name => "B1" } ],
      expressions => [ { expression => "B1" }, 
                       { expression => "Expr1" }],
      campaigns => 
     [ { cpm => 20, channel => "Expr2", tag_cpm => 5 },
       { cpm => 15, channel => "Expr2", tag_cpm => 5 } ]
   });
}

sub key_variation
{
  my ($self, $ns) = @_;

  my $size = $ns->create(CreativeSize => {
    name => 'KeyVariation' });

  $ns->output("KeyVariation/CreativeSize", $size);  
  
  my $case = ChannelPriceRangeLogging::Case->new(
    $ns, 'KeyVariation', 
    { publisher => { 
        tag_cpm => 0,
        size => DB::Defaults::instance()->size(),
        country_code => DB::Defaults::instance()->test_country_1()->{country_code} },
      channels => [ { 
        time_to => 60, 
        country_code => DB::Defaults::instance()->test_country_1()->{country_code} }],
      expressions => [ {
        expression => "B1", 
        country_code => DB::Defaults::instance()->test_country_1()->{country_code} }],
      campaigns => 
        [ { cpm => 5, channel => "Expr1", 
            size => DB::Defaults::instance()->size(), 
            country_code => DB::Defaults::instance()->test_country_1()->{country_code} },
          { cpm => 10, size => $size, tag_cpm => 1, channel => "Expr1", 
            country_code => DB::Defaults::instance()->test_country_1()->{country_code} }]});

  $case->create_publisher('Country', 
    { tag_cpm => 1,
      size => DB::Defaults::instance()->size(),
      country_code => DB::Defaults::instance()->test_country_1()->{country_code} });
}

sub no_impression
{
  my ($self, $ns) = @_;

  my $case = ChannelPriceRangeLogging::Case->new(
    $ns, 'NoImpression', 
    { 
      channels => [ { name => "B1" }, { name => "B2" } ]
    });

  $case->create_publisher('PUBCPM', { tag_cpm => 1 } );
  $case->create_publisher('FIXEDMARGIN', { tag_cpm => 0 } );
}

sub currency
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

  my $case = ChannelPriceRangeLogging::Case->new(
    $ns, 'Currency', 
    { 
      publisher => { tag_cpm => 0, currency => $pub_currency },
      channels => [  { name => "B1" }, 
                     { name => "B2" } ],
      expressions => [ {expression => "B1"},
                       {expression => "Expr1|B2"}],
      campaigns =>  [ { cpm => 300, channel => "Expr2", 
                        currency => $adv_currency } ]
    });

  $case->create_publisher('2', 
    { tag_cpm => 100, currency => $pub_currency } );

  $ns->output("Currency/Pub/Rate", $pub_rate);    
  $ns->output("Currency/Adv/Rate", $adv_rate);
}

sub ta_text
{
  my ($self, $ns, $size) = @_;

  my $case = ChannelPriceRangeLogging::Case->new(
    $ns, 'TAText', 
    { 
      publisher => { tag_cpm => 0, size => $size },
      channels => [ { type => "K", time_to => 60*60 }, 
                    { type => "K", time_to => 60*60 } ],
      ta_text =>  [ { cpc => 2, size => $size, channel => "B1" },
                    { cpc => 1, size => $size, channel => "B2" }]
    });

  $ns->output("TAText/ECPM", 300);
}

sub ta_channel
{
  my ($self, $ns, $size) = @_;
 
  my $case = ChannelPriceRangeLogging::Case->new(
    $ns, 'TAChannel', 
    { 
      publisher => { tag_cpm => 0, size => $size },
      channels => [ { time_to => 60*60 }, 
                    { time_to => 60*60 } ],
      ta_channel =>  [ { cpm => 11, size => $size, channel => "B1" },
                       { cpm => 9, size => $size, channel => "B2" }]
    });

}

sub ta_mixed
{
  my ($self, $ns, $size) = @_;

  my $case = ChannelPriceRangeLogging::Case->new(
    $ns, 'TAMixed', 
    { 
      publisher => { tag_cpm => 0, size => $size },
      channels => [ { type => "K", time_to => 60*60 }, 
                    { time_to => 60*60 } ],
      ta_text =>  [ { cpc => 20, size => $size, channel => "B1" } ],
      ta_channel =>  [ { cpm => 10, size => $size, channel => "B2" }]
    });

  $ns->output("TAMixed/ECPM", 2010);
}

sub tag_adjustment
{
  my ($self, $ns) = @_;

  my $tag_cpm = 20;
  my $adjustment = 2;
  my $ctr = 0.1;
  my $cpc = 0.13;

  my $case = ChannelPriceRangeLogging::Case->new($ns, 'TagAdjustment',
    {
      publisher => { tag_cpm => $tag_cpm, adjustment => $adjustment },
      channels => [ {}, {}, { type => 'K' } ],
      campaigns => [ { channel => 'B1', cpc => $cpc, ctr => $ctr } ],
      ta_channel => [ { channel => 'B2', cpc => $cpc, ctr => $ctr } ],
      ta_text =>  [ { channel => 'B3', cpc => $cpc, ctr => $ctr } ]
    });

  $ns->output("TagAdjustment/ECPM", $adjustment * $cpc * $ctr * 1000);
}

sub init
{
  my ($self, $ns) = @_;

  $ns->output("CreativeSize/Common", DB::Defaults::instance()->size());
  $ns->output("FixedMargin", 0);
  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});

  $ns->output("COLO/Colo1", DB::Defaults::instance()->non_optout_isp->{colo_id});
  $ns->output("COLO/Colo2", DB::Defaults::instance()->ads_isp->{colo_id});

  my $size = $ns->create(CreativeSize => {
    name => 'CreativeSize-TA',
    max_text_creatives => 2 });

  $ns->output("CreativeSize-TA", $size);
 
  $self->one_group($ns);
  $self->tag_cpm($ns);
  $self->groups($ns);
  $self->day_switch($ns);
  $self->key_variation($ns);
  $self->no_impression($ns);
  $self->currency($ns);
  $self->ta_text($ns, $size);
  $self->ta_channel($ns, $size);
  $self->ta_mixed($ns, $size);
  $self->tag_adjustment($ns);
}

1;
