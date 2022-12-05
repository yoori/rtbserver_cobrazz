
package CMPStatTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub output_revenue
{
  my ($self, $entity, $args) = @_;
  if (defined $args->{cpm})
  {
    $self->{ns_}->output(
      $entity . "/CPM/" . $args->{name},  $args->{cpm}); 
  }
  if (defined $args->{cpc})
  {
    $self->{ns_}->output(
      $entity . "/CPC/" . $args->{name},  $args->{cpc}); 
  }
}

sub get_site_list
{
  my ($self, $args) = @_;
  my @sites = ();
  foreach my $p ( split(/,/, $args->{publisher}) )
  {
    my $pub = $self->{publishers_}->{$p};
    die $self->{prefix_} . ".Publisher '$p' is not defined!" 
        if not defined $pub;
    
    push @sites, { site_id => $pub->{site_id} };
  }

  return \@sites;
}

sub create_sizes
{
  my ($self, $args) = @_;
  
 foreach my $s (@$args)
 {
    die $self->{prefix_} . ".Size '$s->{name}' is redefined!" 
      if exists $self->{sizes_}->{$s->{name}};

    my $size = $self->{ns_}->create(CreativeSize => 
      { name => $s->{name},
        max_text_creatives => 
          defined $s->{max_text_creatives}? 
            $s->{max_text_creatives}: 1 });
  
    $self->{sizes_}->{$s->{name}} = $size;
  }
}


sub create_accounts
{
  my ($self, $args) = @_;

  foreach my $a (@$args)
  {
    die $self->{prefix_} . ".Account '$a->{name}' is redefined!" 
      if exists $self->{accounts_}->{$a->{name}};

    my %account_args = (
      name => $a->{name},
      role_id => exists $a->{role}?
        $a->{role}: DB::Defaults::instance()->advertiser_role,
      account_type_id => exists $a->{type}?
        $a->{type}: DB::Defaults::instance()->advertiser_type,
      currency_id => defined $a->{currency}?
        $a->{currency}: DB::Defaults::instance()->currency(),
      timezone_id => defined $a->{timezone}?
        DB::TimeZone->blank(tzname => $a->{timezone}):
          DB::Defaults::instance()->timezone() );

    if (defined $a->{agency} and 
        (not defined $a->{role} or
         $a->{role} == DB::Defaults::instance()->advertiser_role))
    {
      die $self->{prefix_} . ".Agency '$a->{agency}' is not defined!" 
        if not defined $self->{accounts_}->{$a->{agency}};

      $account_args{agency_account_id} =
        $self->{accounts_}->{$a->{agency}}->{account_id},

      $account_args{text_adserving} = undef;
    }

    my $account = $self->{ns_}->create(
      Account => \%account_args);

    if (defined $a->{agency} and 
        defined $a->{role} and 
        $a->{role} == DB::Defaults::instance()->publisher_role)
    {
      die $self->{prefix_} . ".Agency '$a->{agency}' is not defined!" 
        if not defined $self->{accounts_}->{$a->{agency}};
      $self->{ns_}->create(WalledGarden => {
        pub_account_id => $account->{account_id},
        agency_account_id => 
           $self->{accounts_}->{$a->{agency}}->{account_id},
        pub_marketplace => 'WG',
        agency_marketplace => 'WG'});
    }
    $self->{ns_}->output("Account/" . $a->{name},  $account);
    $self->{accounts_}->{$a->{name}} = $account;
  }
}

sub create_expressions
{
  my ($self, $args) = @_;

  foreach my $e (@$args)
  {
    my $expression = $e->{expression};

    foreach my $w ( split(/\W+/, $expression) )
    {
      my $c = $self->{channels_}->{$w};
      die $self->{prefix_} . ".Channel '$w' is not defined!" 
          if not defined $c;
      $expression =~ s/$w/$c->{channel_id}/;
    }

    
    die $self->{prefix_} . ".Account '$e->{account}' is not defined!" 
      if defined $e->{account} and not exists $self->{accounts_}->{$e->{account}};
 
    my $channel = 
      $self->{ns_}->create(DB::ExpressionChannel->blank(
         name => $e->{name},
         account_id => defined $e->{account}?
           $self->{accounts_}->{$e->{account}}:  
             $self->{ns_}->create(Account => {
               name => 'Advertiser-' .  $e->{name},
               role_id => DB::Defaults::instance()->advertiser_role }),
         visibility => 
           defined $e->{visibility}?
             $e->{visibility}: 'PUB',
         expression => $expression,
        cpm => $e->{cpm},
        cpc => $e->{cpc} ));

    $self->{ns_}->output("Expr/" . $e->{name},  $channel->channel_id());
    $self->output_revenue("Expr", $e);
    $self->{channels_}->{$e->{name}} = $channel;
  }
}

sub create_channels
{
  my ($self, $args) = @_;

  my $index = 0;

  foreach my $c (@$args)
  {
   die $self->{prefix_} . ".Channel '$c->{name}' is redefined!" 
      if exists $self->{channels_}->{$c->{name}};

   die $self->{prefix_} . ".Account '$c->{account}' is not defined!" 
      if defined $c->{account} and not exists $self->{accounts_}->{$c->{account}};

    my $keyword = 
      make_autotest_name(
        $self->{ns_}, $c->{name});
   
    my $channel =
      $self->{ns_}->create(
        DB::BehavioralChannel->blank(
        name => $c->{name},
        account_id => defined $c->{account}?
          $self->{accounts_}->{$c->{account}}: 
            $self->{ns_}->create(Account => {
              name => 'Advertiser-CH-' .  $c->{name},
              role_id => DB::Defaults::instance()->advertiser_role }),
        keyword_list => $keyword,
        visibility => 
          defined $c->{visibility}? $c->{visibility}: "PUB",
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ],
        cpm => $c->{cpm},
        cpc => $c->{cpc} ));

    $self->{ns_}->output("KWD/" . $c->{name}, $keyword);
    $self->{ns_}->output("Channel/" . $c->{name}, $channel);
    $self->output_revenue("Channel", $c);
    $self->{channels_}->{$c->{name}} = $channel;
  }
}

sub create_display_campaigns
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {

   die $self->{prefix_} . ".Campaign '$c->{name}' is redefined!" 
     if exists $self->{campaigns_}->{$c->{name}};
   
   die $self->{prefix_} . ".Channel '$c->{channel}' is not defined!" 
      if not defined $self->{channels_}->{$c->{channel}};

   die $self->{prefix_} . ".Account '$c->{account}' is not defined!" 
      if defined $c->{account} and not exists $self->{accounts_}->{$c->{account}};

   die $self->{prefix_} . ".Size '$c->{size}' is not defined!" 
      if defined $c->{size} and not exists $self->{sizes_}->{$c->{size}};


   my $campaign = 
     $self->{ns_}->create(DisplayCampaign => {
       name =>  $c->{name},
       account_id => defined $c->{account}?
         $self->{accounts_}->{$c->{account}}:
           $self->{ns_}->create(Account => {
             name => 'Advertiser-D-' .  $c->{name},
             role_id => DB::Defaults::instance()->advertiser_role }),
       size_id =>  defined $c->{size}?
           $self->{sizes_}->{$c->{size}}: DB::Defaults::instance()->size(),
       cpm => defined $c->{cpm}? $c->{cpm}: DB::Defaults::instance()->cpm,
       channel_id => $self->{channels_}->{$c->{channel}},
       targeting_channel_id => undef,
       marketplace => 
          exists $c->{marketplace}? $c->{marketplace}: undef,
       site_links => $self->get_site_list($c) });

   $self->{ns_}->output("CC/" . $c->{name}, $campaign->{cc_id});
   $self->{ns_}->output("CCG/" . $c->{name} , $campaign->{ccg_id});
   
   $self->output_revenue("Display", $c);
   $self->{campaigns_}->{$c->{name}} = $campaign;
  }
}

sub create_creatives
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {
    die $self->{prefix_} . ".Campaign '$c->{campaign}' is not defined!" 
      if not defined $self->{campaigns_}->{$c->{campaign}};

    my $size = defined $c->{size}?
      $self->{sizes_}->{$c->{size}}: DB::Defaults::instance()->size();

    my $template = $self->{ns_}->create(Template =>
      { name => "Template-" . $c->{campaign} . "-" . $c->{name} } );

    $self->{ns_}->create(TemplateFile =>
      { template_id => $template,
        size_id => $size,
        template_file => 'UnitTests/banner_img_clk.html',
        flags => 0 });

    my $creative = $self->{ns_}->create(Creative => 
      {  name => $c->{campaign} . "-" . $c->{name},
         account_id => 
           $self->{campaigns_}->{$c->{campaign}}->{account_id},
         size_id => $size,
       template_id => $template });

    my $cc =  $self->{ns_}->create(CampaignCreative =>
     { ccg_id => 
         $self->{campaigns_}->{$c->{campaign}}->{ccg_id},
       creative_id => $creative });

    $self->{ns_}->output(
       "CC/" . $c->{campaign} . "/" . $c->{name}, $cc);
  }
}

sub create_publishers
{
  my ($self, $args) = @_;

  foreach my $p (@$args)
  {
   die $self->{prefix_} . ".Publisher '$p->{name}' is redefined!" 
     if exists $self->{publishers_}->{$p->{name}};
   
   die $self->{prefix_} . ".Account '$p->{account}' is not defined!" 
      if defined $p->{account} and not exists $self->{accounts_}->{$p->{account}};

   die $self->{prefix_} . ".Size '$p->{size}' is not defined!" 
      if defined $p->{size} and not exists $self->{sizes_}->{$p->{size}};

    my $publisher = 
     $self->{ns_}->create(Publisher => {
       name =>  $p->{name},
       site_account_id => defined $p->{account}?
         $self->{accounts_}->{$p->{account}}:
           $self->{ns_}->create(Account => {
             name => 'Publisher-' .  $p->{name},
             role_id => DB::Defaults::instance()->publisher_role }),
       pricedtag_cpm => defined $p->{cpm}?
         $p->{cpm}: 0,
       size_id =>  defined $p->{size}?
         $self->{sizes_}->{$p->{size}}: DB::Defaults::instance()->size(),
       pricedtag_marketplace => defined $p->{marketplace}?
         $p->{marketplace}: undef });

   $self->{publishers_}->{$p->{name}} = $publisher;
   $self->{ns_}->output("TAG/" . $p->{name}, $publisher->{tag_id});
  }
}

sub create_ta_campaigns
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {

   die $self->{prefix_} . ".Campaign '$c->{name}' is redefined!" 
     if exists $self->{campaigns_}->{$c->{name}};
   
   die $self->{prefix_} . ".Channel '$c->{channel}' is not defined!" 
      if not defined $self->{channels_}->{$c->{channel}};

   die $self->{prefix_} . ".Account '$c->{account}' is not defined!" 
      if defined $c->{account} and not exists $self->{accounts_}->{$c->{account}};

   die $self->{prefix_} . ".Size '$c->{size}' is not defined!" 
      if defined $c->{size} and not exists $self->{sizes_}->{$c->{size}};

    my $campaign = 
      $self->{ns_}->create(ChannelTargetedTACampaign => {
        name =>  $c->{name},
        account_id => defined $c->{account}?
          $self->{accounts_}->{$c->{account}}:
            $self->{ns_}->create(Account => {
              name => 'Advertiser-T-' .  $c->{name},
              role_id => DB::Defaults::instance()->advertiser_role }),
        size_id =>  defined $c->{size}?
            $self->{sizes_}->{$c->{size}}: DB::Defaults::instance()->size(),
        channel_id => $self->{channels_}->{$c->{channel}},
        targeting_channel_id => undef,
        template_id => DB::Defaults::instance()->text_template,
        campaigncreativegroup_cpm => $c->{cpm},
        campaigncreativegroup_ctr => 0.1,
        site_links => $self->get_site_list($c) });

    $self->{ns_}->output("CC/" . $c->{name}, $campaign->{cc_id});
    $self->{ns_}->output("CCG/" . $c->{name} , $campaign->{ccg_id});
   
    $self->output_revenue("TA", $c);
    $self->{campaigns_}->{$c->{name}} = $campaign;
 }
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;  }
  $self->{ns_} = $ns->sub_namespace($prefix);
  $self->{prefix_} = $prefix;
  $self->{accounts_} = ();
  $self->{channels_} = ();
  $self->{publishers_} = ();
  $self->{campaigns_} = ();
  $self->{sizes_} = ();

  my @creators = (
    [ "accounts",  \&create_accounts ],
    [ "sizes",  \&create_sizes ],
    [ "channels",  \&create_channels ],
    [ "expressions",  \&create_expressions ],
    [ "publishers",  \&create_publishers ],
    [ "displays",  \&create_display_campaigns ],
    [ "ta_campaigns",  \&create_ta_campaigns ],
    [ "creatives",  \&create_creatives ] );

  foreach my $creator (@creators)
  {
    my ($dict, $fn) = @$creator;
    if (defined $args->{$dict})
    {
      $fn->($self, $args->{$dict})
    }
  }

  return $self;
}

1;

package CMPStatTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub base_case
{

  my ($self, $ns) = @_;

  my $currency1 = $ns->create(Currency => { rate => 4 });
  my $currency2 = $ns->create(Currency => { rate => 2 });

  $ns->output("RATE1", $currency1->{rate});
  $ns->output("RATE2", $currency2->{rate});

  CMPStatTest::TestCase->new(
    $ns, 'Base', 
      { 
        accounts => [ 
          { name => 'ADV', currency => $currency2, 
            timezone => "Asia/Tokyo" },
          { name => 'WG-AGENCY', currency => $currency2, 
            timezone => "Asia/Tokyo", role => DB::Defaults::instance()->agency_role,
            type => DB::Defaults::instance()->agency_type },
          { name => 'CMP', role => DB::Defaults::instance()->cmp_role, 
            type => DB::Defaults::instance()->cmp_type, currency => $currency1,
            timezone => "Europe/Moscow" },
          { name => 'WG-PUB', role => DB::Defaults::instance()->publisher_role, 
            type => DB::Defaults::instance()->publisher_type, agency => 'WG-AGENCY' },
          { name => 'WG-ADV', type => undef, 
            currency => $currency1, timezone => "Europe/Moscow", 
            agency => 'WG-AGENCY' } ],
        sizes => [ { name => 'SIZE' } ],
        publishers => [
          { name => 'PUBLISHER1' },
          { name => 'PUBLISHER2', size => 'SIZE' },
          { name => 'WG', account => 'WG-PUB', marketplace => 'WG' } ],
        channels => [ 
          { name => 'CPM', cpm => 12, account => 'CMP', visibility => 'CMP' },
          { name => 'CPC', cpc => 20, account => 'CMP', visibility => 'CMP' },
          { name => 'WG', cpm => 12, account => 'CMP', visibility => 'CMP' } ],
        displays => [
          { name => 'CPM', account => 'ADV', channel => 'CPM',
            publisher => 'PUBLISHER1,PUBLISHER2' },
          { name => 'CPC', account => 'ADV', channel => 'CPC',
            publisher => 'PUBLISHER1' },
          { name => 'WG', account => 'WG-ADV', 
            channel => 'WG', marketplace => 'WG',
            publisher => 'WG' } ],
        creatives => [ 
          { name => "CC2", campaign => "CPM", 
            size => "SIZE" }]  });
}

sub adv_expression
{
  my ($self, $ns) = @_;

  CMPStatTest::TestCase->new(
    $ns, 'AdvExpr', 
      { 
        accounts => [ 
          { name => 'CMP', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type},
          { name => 'ADV' }],
        publishers => [ { name => 'PUBLISHER' } ],
        channels => [ 
          { name => 'CPM', cpm => 12, account => 'CMP', visibility => 'CMP' },
          { name => 'CPC', cpc => 20, account => 'CMP', visibility => 'CMP' },
          { name => 'PRI', account => 'ADV', visibility => 'PRI' }],
        expressions => [
          { name => "EXPR", expression => "CPM^CPC|PRI", 
            account => 'ADV', visibility => 'PRI' }],
        displays => [
          { name => 'CCG', account => 'ADV',
            channel => 'EXPR',
            publisher => 'PUBLISHER' }] });
}

sub cmp_expression
{
  my ($self, $ns) = @_;

  CMPStatTest::TestCase->new(
    $ns, 'CmpExpr',
    { 
      accounts => [ 
        { name => 'CMP', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type},
        { name => 'ADV' } ],
      publishers => [ { name => 'PUBLISHER' } ],
      channels => [ 
        { name => 'CPM', cpm => 12, account => 'CMP', visibility => 'CMP' },
        { name => 'PRI', cpm => undef, account => 'ADV', visibility => 'PRI' } ],
      expressions => [
        { name => "EXPR", expression => "CPM|PRI", 
          account => 'CMP', cpm => 15, visibility => 'CMP' }],
      displays => [
        { name => 'CCG', account => 'ADV', channel => 'EXPR',
          publisher => 'PUBLISHER'} ] });
}

sub pub_expression
{
  my ($self, $ns) = @_;

  CMPStatTest::TestCase->new(
    $ns, 'PubExpr',
    { 
      accounts => [ 
        { name => 'CMP', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type},
        { name => 'ADV' } ],
      publishers => [ { name => 'PUBLISHER' } ],
      channels => [ 
        { name => 'CPM', cpm => 12, account => 'CMP', visibility => 'CMP' } ],
      expressions => [
        { name => "EXPR", expression => "CPM", 
          account => 'CMP', visibility => 'PUB' }],
      displays => [
        { name => 'CCG', account => 'ADV', channel => 'EXPR',
          publisher => 'PUBLISHER'} ] });
}

sub currency_case
{

  my ($self, $ns, $currency) = @_;

  CMPStatTest::TestCase->new(
    $ns, 'Currency',
    { 
      accounts => [ 
        { name => 'CMP1', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type },
        { name => 'CMP2', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type, 
          currency => $currency },
        { name => 'ADV1' },
        { name => 'ADV2', currency => $currency } ],
      publishers => [ 
        { name => 'PUBLISHER1', cpm => 5 },
        { name => 'PUBLISHER2', cpm => 20 } ],
      channels => [
        { name => 'CPM1', cpm => 10, account => 'CMP1', 
          visibility => 'CMP' },  
        { name => 'CPM2', cpm => 15, account => 'CMP2', 
          visibility => 'CMP' },  
        { name => 'CPM3', cpm => 20, account => 'CMP2', 
          visibility => 'CMP' } ] ,
      expressions => [
        { name => "EXPR1", expression => "CPM1", 
          cpm => 5, account => 'CMP1', 
          visibility => 'CMP' },
        { name => "EXPR2", expression => "EXPR1", 
          cpm => 2,  account => 'CMP1', 
          visibility => 'CMP' }],
      displays => [
        { name => 'CCG1', account => 'ADV1', cpm => 10,
          channel => 'CPM2', publisher => 'PUBLISHER1' },
        { name => 'CCG2', account => 'ADV2', cpm => 10,
          channel => 'EXPR2', publisher => 'PUBLISHER1,PUBLISHER2' },
        { name => 'CCG3', account => 'ADV2', cpm => 20,
          channel => 'CPM3', publisher => 'PUBLISHER1' }] });
}

sub text_adv_case
{
  my ($self, $ns) = @_;

  CMPStatTest::TestCase->new(
    $ns, 'TA',
    { 
      accounts => [ 
        { name => 'CMP', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type},
        { name => 'ADV1' },
        { name => 'ADV2' } ],
     sizes => [ { name => 'TEXTSIZE', max_text_creatives => 2 } ],
     publishers => [ { name => 'PUBLISHER', size => 'TEXTSIZE' } ],
     channels => [ 
        { name => 'TEXT', cpc => 30, account => 'CMP', 
          visibility => 'CMP' } ],
     ta_campaigns => [
        { name => 'CCG1', account => 'ADV1', cpm => 20,
          channel => 'TEXT', publisher => 'PUBLISHER',
          size => 'TEXTSIZE' },
        { name => 'CCG2', account => 'ADV2', cpm => 15,
          channel => 'TEXT', publisher => 'PUBLISHER',
          size => 'TEXTSIZE' } ]});
}

sub expression_case
{
  my ($self, $ns) = @_;

  CMPStatTest::TestCase->new(
    $ns, 'Expression',
    { 
      accounts => [ 
        { name => 'CMP', role => DB::Defaults::instance()->cmp_role,  type => DB::Defaults::instance()->cmp_type},
        { name => 'ADV' } ],
      publishers => [ 
        { name => 'PUBLISHER1' },
        { name => 'PUBLISHER2' },
        { name => 'PUBLISHER3' } ],
      channels => [ 
        { name => 'CH1', cpm => 1, account => 'CMP', visibility => 'CMP' },
        { name => 'CH2', cpm => 2, account => 'CMP', visibility => 'CMP' }, 
        { name => 'CH3', cpm => 3, account => 'CMP', visibility => 'CMP' }, 
        { name => 'CH4', cpm => 4, account => 'CMP', visibility => 'CMP' },
        { name => 'CH5', cpm => 5, account => 'CMP', visibility => 'CMP' },
        { name => 'CH6', cpm => 6, account => 'CMP', visibility => 'CMP' },
        { name => 'CH7', cpm => 7, account => 'CMP', visibility => 'CMP' },
        { name => 'CH8', cpm => 8, account => 'CMP', visibility => 'CMP' },
        { name => 'CH9', cpm => 9, account => 'CMP', visibility => 'CMP' },
        { name => 'CH10', cpm => 10, account => 'CMP', visibility => 'CMP' },
        { name => 'CH11', cpm => 11, account => 'CMP', visibility => 'CMP' },
        { name => 'UNLINK', cpm => 12, account => 'CMP', visibility => 'CMP' }],
     expressions => [
        { name => "EXPR1", expression => "CH2", cpm => 20,
          account => 'CMP', visibility => 'CMP' },
        { name => "EXPR2", expression => "CH1|EXPR1|CH3^CH4",
          account => 'ADV', visibility => 'PRI' },
        { name => "EXPR3", expression => "CH7&CH8", 
          account => 'ADV', visibility => 'PRI' },
        { name => "EXPR4", expression => "CH5^(CH6|EXPR3)", 
          account => 'ADV', visibility => 'PRI' },
        { name => "EXPR5", expression => "CH11", 
          account => 'ADV', visibility => 'PRI' },
        { name => "EXPR6", expression => "EXPR5|CH9^CH10", 
          account => 'ADV', visibility => 'PRI' } ],
    displays => [
        { name => 'CCG1', account => 'ADV',
          channel => 'EXPR2', publisher => 'PUBLISHER1' },
        { name => 'CCG2', account => 'ADV',
          channel => 'EXPR4', publisher => 'PUBLISHER2' },
        { name => 'CCG3', account => 'ADV',
          channel => 'EXPR6', publisher => 'PUBLISHER3' }] });
}


sub init
{
  my ($self, $ns) = @_;

  # Non default colo must have non default timezone
  # DB::Defaults::instance()->remote_isp - Europe/Moscow colo
  $ns->output("COLO", DB::Defaults::instance()->remote_isp->{colo_id});

  my $currency = $ns->create(Currency => { rate => 1.6 });
  $ns->output("RATE", $currency->{rate});  

  $self->base_case($ns);
  $self->adv_expression($ns);
  $self->cmp_expression($ns);
  $self->pub_expression($ns);
  $self->currency_case($ns, $currency);
  $self->expression_case($ns);
  $self->text_adv_case($ns);
}

1;
