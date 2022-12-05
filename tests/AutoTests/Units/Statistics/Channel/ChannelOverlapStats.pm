
package ChannelOverlapStats::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_keywords
{
  my ($self, $name, $args) = @_;

  if (defined $args)
  {
    my @keywords = ();
    
    foreach my $k (split(/\W+/, $args))
    {
      my $keyword;
      if (exists $self->{keywords_}->{$k})
      {
        $keyword =  $self->{keywords_}->{$k};
      }
      else
      {
        $keyword = 
          make_autotest_name($self->{ns_}, $k);
        $self->{ns_}->output("KWD/" . $k , $keyword);       
        $self->{keywords_}->{$k} = $keyword;
      }
      push @keywords, $keyword;
    }
    return join("\n", @keywords );
  }
  else
  {
    my $keyword = 
      make_autotest_name($self->{ns_}, $name);
    $self->{ns_}->output("KWD/" . $name, $keyword);
    return $keyword;
   }
}

sub create_accounts
{
  my ($self, $args) = @_;
  
  foreach my $a (@$args)
  {
    die $self->{prefix_} . ".Account '$a->{name}' is redefined!" 
      if exists $self->{accounts_}->{$a->{name}};

    die $self->{prefix_} . ".Account '$a->{internal}' is undefined!" 
      if defined $a->{internal} and not exists $self->{accounts_}->{$a->{internal}};

    my $account = $self->{ns_}->create(Account => {
      name => $a->{name},
      internal_account_id => 
        exists $a->{internal}?
          defined $a->{internal}?
            $self->{accounts_}->{$a->{internal}}:
              $a->{internal}: DB::Defaults::instance()->internal_account, 
      role_id => defined $a->{role}?
        $a->{role}: DB::Defaults::instance()->advertiser_role });

    $self->{ns_}->output("Account/" . $a->{name},  $account);
    $self->{accounts_}->{$a->{name}} = $account;
  }
}

sub make_expression_
{
  my ($self, $expression) = @_;
  
  foreach my $w ( split(/\W+/, $expression) )
  {
    my $c = $self->{channels_}->{$w};
    die $self->{prefix_} . ".Channel '$w' is not defined!" 
        if not defined $c;
    $expression =~ s/$w/$c->{channel_id}/;
  }

  return $expression;
}

sub create_expressions
{
  my ($self, $args) = @_;
  
  foreach my $e (@$args)
  {

    die $self->{prefix_} . ".Channel '$e->{name}' is redefined!" 
       if exists $self->{channels_}->{$e->{name}};

    my $expression = 
      $self->make_expression_($e->{expression});

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
         flags => $e->{overlap}?
           DB::CMPChannelBase::OVERLAP: 0,                                             
         expression => $expression ));

    $self->{ns_}->output("Expr/" . $e->{name},  $channel->channel_id());
    $self->{channels_}->{$e->{name}} = $channel;
  }
}

sub create_channels
{
  my ($self, $args) = @_;
  
  foreach my $c (@$args)
  {
    die $self->{prefix_} . ".Channel '$c->{name}' is redefined!" 
      if exists $self->{channels_}->{$c->{name}};

    die $self->{prefix_} . ".Account '$c->{account}' is not defined!" 
      if defined $c->{account} and not exists $self->{accounts_}->{$c->{account}};

    my ($keyword, $url);
    
    my @behavs = ();
    
    sub create_params
    {
      my ($type, $args, $behavs) = @_;
      my %behav_params = (
        trigger_type => $type);
      
      $behav_params{time_to} = $args->{to} || 3*60*60;
      $behav_params{time_from} = $args->{from} || 0;
      $behav_params{minimum_visits} = $args->{visits} || 1;

      push @$behavs, 
         DB::BehavioralChannel::BehavioralParameter->blank(
           %behav_params);
    }


    if (defined $c->{keyword})
    {
      $keyword = 
          $self->create_keywords($c->{name}, $c->{keyword});

      create_params('P', $c, \@behavs);
    }

    if (defined $c->{url})
    {
      $url = "www." . 
        make_autotest_name(
         $self->{ns_}, $c->{url}) . ".com";

      $self->{ns_}->output("URL/" . $c->{url}, $url);

      create_params('U', $c, \@behavs);
    }

    my %params = (
      name => $c->{name},
      account_id => defined $c->{account}?
        $self->{accounts_}->{$c->{account}}: 
          $self->{ns_}->create(Account => {
            name => 'Advertiser-CH-' .  $c->{name},
            role_id => DB::Defaults::instance()->advertiser_role }),
      keyword_list => $keyword,
      url_list => $url,
      behavioral_parameters => \@behavs);

    if (defined $c->{channel_type})
    {
      $params{channel_type} = $c->{channel_type};
    }
    
    if (defined $c->{visibility})
    {
      $params{visibility} = $c->{visibility};
    }

    if ($c->{overlap})
    {
      $params{flags} = 
        DB::CMPChannelBase::AUTO_QA | DB::CMPChannelBase::OVERLAP;
    }

    {
      $params{country_code} = $c->{country} if defined $c->{country};
      my $class = 'DB::' . ($c->{type} || 'BehavioralChannel');
      no strict 'refs';
      my $channel =
        $self->{ns_}->create(
          $class->blank(%params));

      $self->{ns_}->output("Channel/" . $c->{name}, $channel);
      $self->{channels_}->{$c->{name}} = $channel;
    }
  }
}

sub create_geo_channels
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {
   die $self->{prefix_} . ".Channel '$c->{name}' is redefined!" 
      if exists $self->{channels_}->{$c->{name}};

   die $self->{prefix_} . ".Account '$c->{account}' is not defined!" 
      if defined $c->{account} and not exists $self->{accounts_}->{$c->{account}};

   die $self->{prefix_} . ".Parent channel '$c->{name}' is undefined!" 
      if defined $c->{parent} and not exists $self->{channels_}->{$c->{parent}};

   my $channel = $self->{ns_}->create(DB::GEOChannel->blank(
    name => $c->{name},
    country_code => 
      defined $c->{country}? $c->{country}: 'GN',
    geo_type => $c->{type},
    parent_channel_id =>  
      defined $c->{parent}?
        $self->{channels_}->{$c->{parent}}->{channel_id}: 
          DB::Defaults::instance()->geo_country->{channel_id},
    city_list => 
      $c->{type} eq 'CITY'?
        $self->{ns_}->namespace . "-"  . $c->{name}: undef,
    latitude => $c->{latitude},
    longitude => $c->{longitude}));

   $self->{ns_}->output("Channel/" . $c->{name}, $channel);
   $self->{ns_}->output("LOC/" . $c->{name}, $channel->{name});
   $self->{channels_}->{$c->{name}} = $channel;
 }
}

sub create_targeting
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {
    die $self->{prefix_} . ".Channel '$c->{name}' is redefined!" 
       if exists $self->{channels_}->{$c->{name}};

    my $expression = 
      $self->make_expression_($c->{expression});

    my $channel = 
      $self->{ns_}->create(DB::TargetingChannel->blank(
        name => $c->{name},
        expression => $expression));

    $self->{ns_}->output("Targeting/" . $c->{name}, $channel);
    $self->{channels_}->{$c->{name}} = $channel;
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
  $self->{keywords_} = ();

  $self->{channels_}->{'Device'} = 
    DB::Defaults::instance()->device_channel;

  $self->{ns_}->output("Device",
    DB::Defaults::instance()->device_channel);                       

  my @creators = (
    [ "accounts", \&create_accounts ],
    [ "channels", \&create_channels ],
    [ "geo_channels", \&create_geo_channels ],
    [ "expressions", \&create_expressions ], 
    [ "targeting", \&create_targeting ]);

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


package ChannelOverlapStats;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub flags_case
{

  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'Flags', 
      { accounts => [ 
        { name => 'Agency', role => DB::Defaults::instance()->agency_role},
        { name => 'Advertiser' }],
       channels => [ 
        { name => "B1", account => 'Advertiser', 
          visibility => 'PRI', keyword => 'Kwd1', overlap => 1 },
        { name => "B2", account => 'Advertiser', 
          visibility => 'PRI', keyword => 'Kwd1' },
        { name => "B3", account => 'Agency', 
          visibility => 'PRI', keyword => 'Kwd2', overlap => 1 } ] ,
       expressions => [
        { name => "E1", expression => "B1", 
          account => 'Advertiser', visibility => 'PRI' },
        { name => "E2", expression => "B2", 
          account => 'Advertiser', visibility => 'PRI', overlap => 1 } ]}); 
}

sub country_case
{

  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'Country', 
      { accounts => [ 
        { name => 'Internal', role => DB::Defaults::instance()->internal_role,
          internal => undef}],
       channels => [ 
        { name => "B1", account => 'Internal', country => 'GN', 
          visibility => 'PRI', keyword => 'Kwd', overlap => 1 },
        { name => "B2", account => 'Internal',  country => 'LU',
          visibility => 'PRI', keyword => 'Kwd', overlap => 1 },
        { name => "B3", account => 'Internal', country => 'GN',
          visibility => 'PRI', keyword => 'Kwd', overlap => 1 } ]}); 
}

sub channel_type_case
{
  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'ChannelType', 
      { accounts => [ 
        { name => 'Internal', role => DB::Defaults::instance()->internal_role,
          internal => undef}],
       channels => [ 
        { name => "Keyword", account => 'Internal', 
          visibility => 'PRI', channel_type => 'K', keyword => 'Kwd' },
        { name => "Behavioral1", account => 'Internal', 
          keyword => 'Kwd', overlap => 1 },
        { name => "Behavioral2", account => 'Internal', 
          keyword => 'Kwd', overlap => 1 } ],
       geo_channels => [
        { name => 'Guinea', type => 'STATE' },
        { name => 'Mamou', type => 'CITY', parent => 'Guinea',
          latitude => -12.09159, longitude => 10.37396 }],
       expressions => [
        { name => "EXPR", account => 'Internal', 
          expression => "Behavioral2", overlap => 1 }],
       targeting => [
        { name => "A1", expression => 'EXPR&Guinea&Device', visibility => 'PRI'},
        { name => "A2", expression => 'Behavioral2&Guinea&Device', visibility => 'PRI'} ] });
}

sub non_optin_case
{
  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'NonOptin', 
    { accounts => [ 
        { name => 'Internal', role => DB::Defaults::instance()->internal_role, internal => undef}],
      channels => [ 
        { name => "B1", account => 'Internal', 
          visibility => 'PRI', url => 'Url', overlap => 1 },
        { name => "B2", account => 'Internal',
          visibility => 'PRI', url => 'Url', overlap => 1 } ]}); 
}

sub config_case
{
  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'Config', 
    { accounts => [ 
        { name => 'Internal', role => DB::Defaults::instance()->internal_role, internal => undef}],
      channels => [ 
        { name => "B1", account => 'Internal', 
          visibility => 'PRI', keyword => 'Kwd', overlap => 1 },
        { name => "B2", account => 'Internal',
          visibility => 'PRI', keyword => 'Kwd', overlap => 1 } ]}); 
}

sub expression_case
{
  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'Expression',
    { 
    channels => [ 
      { name => "Channel1", keyword => 'Kwd1', overlap => 1 },
      { name => "Channel2", keyword => 'Kwd2', overlap => 1 } ],
    expressions => [
        { name => "E1", expression => "Channel1", overlap => 1 },
        { name => "E2", expression => "E1", overlap => 1 },
        { name => "E3", expression => "Channel1|Channel2", overlap => 1 },
        { name => "E4", expression => "Channel1&Channel2", overlap => 1 },
        { name => "E5", expression => "Channel1^Channel2", overlap => 1 } ] });
}

sub history_case
{
  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'History',
    { channels => [ 
      { name => "Session1", keyword => 'Kwd1', to => 60, overlap => 1 },
      { name => "HT", keyword => 'Kwd2', to => 3*24*60*60, overlap => 1 },
      { name => "Session2", keyword => 'Kwd3', visits => 2, 
        to => 23*60*60 + 59*60 + 59, overlap => 1},
      { name => "History", keyword => 'Kwd4',
        from => 24*60*60, to => 3*24*60*60, overlap => 1 } ] });
}

sub daily_proc_case
{
  my ($self, $ns) = @_;

  ChannelOverlapStats::TestCase->new(
    $ns, 'DailyProc',
    { channels => [ 
      { name => "History1", keyword => 'Kwd',
        from => 24*60*60, to => 4*24*60*60, overlap => 1 },
      { name => "History2", keyword => 'Kwd',  
        from => 24*60*60, to => 4*24*60*60, overlap => 1 } ] });
}

sub status_case
{
  my ($self, $ns) = @_;

  my $case =   ChannelOverlapStats::TestCase->new(
    $ns, 'Status',
    { 
     accounts => [ 
        { name => 'CMP', role => DB::Defaults::instance()->cmp_role},
        { name => 'Advertiser' }],
    channels => [ 
      { name => "Channel1", keyword => 'Kwd', 
        account => 'CMP',  overlap => 1, country => 'GB' },
      { name => "Channel2", keyword => 'Kwd',
        account => 'Advertiser', overlap => 1, country => 'GB' },
      { name => "Channel3", keyword => 'Kwd',
        account => 'Advertiser', overlap => 1, country => 'GB' } ]});

  my @total_users = (20, 20, 0);
  my $i = 0;

  foreach my $ch (qw(Channel1 Channel2 Channel3))
  {

    $ns->create(ChannelInventory => {
      sdate => DB::Entity::PQ->sql("current_date - 1"),
      channel_id => $case->{channels_}->{$ch},
      colo_id => DB::Defaults::instance()->isp->{colo_id} });

    $ns->create(ChannelInventory => {
      channel_id => $case->{channels_}->{$ch},
      colo_id => DB::Defaults::instance()->isp->{colo_id},
      total_user_count => $total_users[$i++] });
  }
}


sub init {
  my ($self, $ns) = @_;

  $ns->output("ADSCOLO", DB::Defaults::instance()->ads_isp->{colo_id});

  $self->flags_case($ns);
  $self->country_case($ns);
  $self->channel_type_case($ns);
  $self->non_optin_case($ns);
  $self->config_case($ns);
  $self->expression_case($ns);
  $self->history_case($ns);
  $self->daily_proc_case($ns);
  $self->status_case($ns);
}

1;
