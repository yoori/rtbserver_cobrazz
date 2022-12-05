
package TargetingChannelStats::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant DEFAULT_CPA => 1;

sub create_geo_channel
{
  my ($self, $name) = @_;

  my $geo_channel = DB::Defaults::instance()->geo_country;
  my $location = "gn";

  if (defined $name)
  {
    my ($state_name, $city_name) = split('/', $name);

    if (defined $state_name)
    {
      $geo_channel = $self->{ns_}->create(
        DB::GEOChannel->blank(
          name => $state_name,
          parent_channel_id => $geo_channel->{channel_id},
          geo_type => 'STATE'));

      $location .= "/$geo_channel->{name}";
    }

    if (defined $city_name)
    {
      $geo_channel = $self->{ns_}->create(
        DB::GEOChannel->blank(
          name => $city_name,
          city_list => $self->{ns_}->namespace . "-" . $city_name,
          latitude => sprintf('%.4f', rand(180) - 90),
          longitude => sprintf('%.4f', rand(360) - 180),
          parent_channel_id => $geo_channel->{channel_id},
          geo_type => 'CITY'));

      $location .= "/$geo_channel->{name}";
    }
  }

  $self->{ns_}->output('GEO', $geo_channel );
  $self->{ns_}->output('Location', lc($location) );
  $self->{channels_}->{'GEO'} = $geo_channel;  
}

sub create_device_channel
{
  my ($self, $args) = @_;

  my $platform = $self->{ns_}->create(
     DB::DeviceChannel::Platform->blank(
       name => $args->{platform},
       type => 'OS'));

  my $channel = $self->{ns_}->create(
     DB::DeviceChannel->blank(
       name => 'Device',
       expression => $platform->platform_id ));

  $self->{ns_}->output('Device', $channel);
  $self->{ns_}->output('UserAgent', $args->{user_agent}) ;
  $self->{channels_}->{'Device'} = $channel;
}

sub create_audience_channel
{
  my ($self, $args) = @_;

  my $channel = $self->{ns_}->create(
    DB::AudienceChannel->blank(
      name => 'Audience',
      account_id => $self->{account_},
      %$args));

  $self->{ns_}->output('Audience', $channel);
  $self->{channels_}->{'Audience'} = $channel;
}

sub create_excluded_channel
{
  my ($self, $suffix) = @_;

  my $channel = $self->create_behavioral_channel($suffix);
  push @{$self->{channels_}->{'Excluded'}}, $channel;
}

sub create_behavioral_channel
{
  my ($self, $suffix) = @_;

  $suffix = "" if not defined $suffix;

  my $keyword = 
    make_autotest_name($self->{ns_}, 'KWD' . $suffix);

  my $channel = $self->{ns_}->create(DB::BehavioralChannel->blank(
    name => 'Channel' . $suffix,
    account_id => $self->{account_},
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => 'P',
      time_to => 3*24*60*60) ] ));

  $self->{ns_}->output('KWD' . $suffix, $keyword);
  $self->{ns_}->output('Channel' . $suffix, $channel);
  $self->{channels_}->{$suffix} = $channel if $suffix;
  $self->{keywords_}->{$suffix} = $keyword;
  return $channel;
}

sub create_expression_channel
{
  my ($self, $expression, $idx) = @_;

  my $name = defined $idx? 'Expr' . $idx: 'Expression';

  if (not defined $expression)
  {
    $expression = $self->create_behavioral_channel()->{channel_id};
    $expression .= "|$self->{channel_}->{'Audience'}->{channel_id}"
      if exists $self->{channel_}->{'Audience'}
  }
  else
  {
    foreach my $w (grep {m/.+/} split(/\W+/, $expression))
    {
      my $c = $self->{channels_}->{$w};
      $c = $self->create_behavioral_channel($w)
        if not defined $c;
      $expression =~ s/$w/$c->{channel_id}/;
    }
  }

  my $channel = 
    $self->{ns_}->create(DB::ExpressionChannel->blank(
      name => $name,
      account_id => $self->{account_},
      expression => $expression));

  $self->{ns_}->output($name, $channel);
  $self->{channels_}->{$name} = $channel;

  return $channel;
}

sub create_targeting_channel
{
  my ($self, $expression, $suffix) = @_;

  $suffix = "" if not defined $suffix;

  my $stat_expression = $expression;
  foreach my $w (grep {m/.+/} split(/\W+/, $expression))
  {
    $self->create_expression_channel() if ($w eq 'Expression');

    my $c = $self->{channels_}->{$w};
    die "Channel '$w' is not defined " .
        "in the case '$self->{ns_}->namespace'" 
       if not defined $c;
    $expression =~ s/$w/$c->{channel_id}/;
    if (( $c->{channel_type} eq 'E' ||
          $c->{channel_type} eq 'B' ||
          $c->{channel_type} eq 'A' ) &&
        ( not grep {$_->{channel_id} eq $c->{channel_id}}
            @{$self->{channels_}->{'Excluded'}} ))
    {
      $stat_expression =~ s/$w/$c->{channel_id}/ 
          if $c->{channel_type} eq 'B' ||
             $c->{channel_type} eq 'A'; 
      $stat_expression =~ s/$w/$c->{expression}/ 
          if $c->{channel_type} eq 'E'; 
    }
    else
    {
      $stat_expression =~ s/[\|&^]?$w//;
    }
  }

  $stat_expression =~ s/^\s*[\|&^]?//;

  my $channels_count=(grep {m/.+/} split(/\W+/, $stat_expression));
 
  my $channel = 
    $self->{ns_}->create(DB::TargetingChannel->blank(
    name => 'Targeting' . $suffix,
    expression => $expression));

  $self->{ns_}->output('Targeting'  . $suffix, $channel);
  $self->{channels_}->{'Targeting'} = $channel if !$suffix;
  my $e = $stat_expression;
  $e =~ s/&/ & /g;
  $e =~ s/\|/ | /g;
  $e = "($e)" if $channels_count > 1;
  $self->{ns_}->output(
    'TargetingExpression'  . $suffix, $e);

  return $channel;
}

sub create_display_ccg
{
  my ($self, $args) = @_;

  my %campaign_args = (
    name => 'Campaign',
    account_id => $self->{account_},
    campaigncreativegroup_cpm => undef,
    campaigncreativegroup_cpa => DEFAULT_CPA,
    campaigncreativegroup_ar => 0.01,
    flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    targeting_channel_id => exists $self->{channels_}->{'Targeting'}
      ? $self->{channels_}->{'Targeting'}->{channel_id} : undef,
    excluded_channels => exists $self->{channels_}->{'Excluded'}
      ? $self->{channels_}->{'Excluded'} : undef,
    geo_channels => exists $self->{channels_}->{'GEO'}
      ? $self->{channels_}->{'GEO'}->{channel_id} : undef,
    channel_id => exists $self->{channels_}->{'Expression'}
      ? $self->{channels_}->{'Expression'}->{channel_id}
      : exists $self->{channels_}->{'Audience'}
        ? $self->{channels_}->{'Audience'}->{channel_id}
        : undef,
    site_links => [{ site_id => $self->{publisher_}->{site_id} }] );

  $self->{ns_}->output("KWD", $self->{keywords_}->{$args->{channel_id}})
    if defined $args->{channel_id} and
       defined $self->{keywords_}->{$args->{channel_id}};

  for my $channel_field (qw(targeting_channel_id
                            excluded_channels
                            geo_channels
                            channel_id))
  {
    if (defined $args->{$channel_field})
    {
      my @channels = map {
        $self->{channels_}->{$_}->{channel_id} or
        die "Channel '$_' isn't defined!" } split(/\s+/, $args->{$channel_field});
      $args->{$channel_field} = scalar(@channels) > 1
        ? \@channels
        : $channels[0];
    }
  }

  $campaign_args{flags} |= DB::Campaign::RON
    unless defined $args->{channel_id} or
           defined $campaign_args{channel_id};

  my $campaign = $self->{ns_}->create(DisplayCampaign => {
    %campaign_args, %$args});

  $self->{ns_}->output('CC', $campaign->{cc_id});
}

sub create_text
{
  my ($self, $args) = @_;

  my $idx = 0;
  
  foreach my $t (@$args)
  {

    ++$idx;

    my $expression = 
      defined $t->{expression}?
        $self->create_expression_channel($t->{expression}, $idx):
          $self->create_behavioral_channel($t->{channel});

    my $targeting = 
      $self->create_targeting_channel($t->{targeting_expression}, $idx);
    
    my $campaign =  $self->{ns_}->create(
      ChannelTargetedTACampaign => {
      name => "TextCampaign-" . $idx,
      size_id => $self->{size_},
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $expression,
      targeting_channel_id => $targeting->{channel_id},
      campaigncreativegroup_cpm => $t->{cpm},
      campaigncreativegroup_cpa => $t->{cpa},
      campaigncreativegroup_ctr => 0.01,
      campaigncreativegroup_ar => 0.01,
      site_links => [
        {site_id => $self->{publisher_}->{site_id} }] });

    $self->{ns_}->output('CC/' . $idx, $campaign->{cc_id});
    $self->{ns_}->output('CPA/' . $idx, $t->{cpa}) 
       if defined $t->{cpa};
    $self->{ns_}->output('CPM/' . $idx, $t->{cpm})
       if defined $t->{cpm};
  }
} 

sub create_no_imp_publisher
{
  my $self = shift;
  my $publisher =
    $self->{ns_}->create(Publisher =>
      { name => 'Publisher-no-imp',
        size_id =>  $self->{size_} });
  
  $self->{ns_}->output("NoImp/Tag", $publisher->{tag_id});
} 

sub create_other
{
  my $self = shift;
  my ($cpm) = @_;
  my $advertiser = 
    $self->{ns_}->create(Account => {
      name => 'Advertiser-Other',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = make_autotest_name($self->{ns_}, 'KWDOther');

  my $publisher =
    $self->{ns_}->create(Publisher =>
      { name => 'Publisher-other',
        size_id =>  $self->{size_} });

  my $campaign = $self->{ns_}->create(
    DisplayCampaign => { 
      name => 'Campaign-Other',
      account_id => $advertiser,
      campaigncreativegroup_cpm => $cpm,
      creative_template_id =>  DB::Defaults::instance()->text_template,
      creative_size_id =>  $self->{size_},
      channel_id => 
        DB::BehavioralChannel->blank(
          name => 'Channel-Other',
          account_id => $advertiser,
          keyword_list => $keyword,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
      site_links => [
        { site_id => $publisher->{site_id} }] });

  $self->{ns_}->output('Other/CC', $campaign->{cc_id});
  $self->{ns_}->output("Other/Tag", $publisher->{tag_id});
  $self->{ns_}->output("Other/KWD", $keyword);
  $self->{ns_}->output("Other/CPM", $cpm);
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) 
  {
    $self = bless {}, $self;  
  }
  
  $self->{ns_} = $ns->sub_namespace($prefix);
  $self->{channels_} = ();
  $self->{account_} = 
    $self->{ns_}->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $creative_size = 
     defined $args->{text}?
        @{$args->{text}}: 1;
  $self->{size_} = 
    defined $args->{text}?
      $self->{ns_}->create(
        CreativeSize => {
          name => 'Size',
          max_text_creatives => $creative_size}): 
      DB::Defaults::instance()->size();

  $self->{publisher_} =
    $self->{ns_}->create(Publisher =>
      { name => 'Publisher',
        size_id =>  $self->{size_} });

  $self->{ns_}->output(
    "Tag", $self->{publisher_}->{tag_id});

  my @creators = (
    [ "behavioral", \&create_behavioral_channel ],
    [ "geo", \&create_geo_channel ],
    [ "device", \&create_device_channel ],
    [ "audience", \&create_audience_channel ],
    [ "excluded", \&create_excluded_channel ],
    [ "targeting", \&create_targeting_channel ],
    [ "text", \&create_text ],
    [ "display", \&create_display_ccg ] );

  foreach my $creator (@creators)
  {
    my ($dict, $fn) = @$creator;
    if (exists $args->{$dict})
    {
      $fn->($self, $args->{$dict})
    }
  }

  unless (defined $args->{text} or defined $args->{display})
  {
    $self->create_display_ccg();
  }

  return $self;
}

1;


package TargetingChannelStats;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub all
{
  my ($self, $ns) = @_;

  my $case =
    new TargetingChannelStats::Case(
      $ns, 'ALL',
      { geo => 'Boke', 
        device => {
          platform => "Android",
          user_agent => 'Mozilla/5.0 (Linux; U; Android 2.2; '.
            'en-sa; HTC_DesireHD_A9191 Build/FRF91) AppleWebKit/533.1 ' . 
            '(KHTML, like Gecko) Version/4.0 Mobile Safari/533.1'},
        text => 
          [ { cpa => 0.01, channel => "B1",
              targeting_expression=> "B1&GEO&Device" },
            { cpm => 2, expression => "B2|B3", 
              targeting_expression=> "Expr2&GEO&Device" },
            { cpm => 1, expression => "((B2&B4)|B5)", 
              targeting_expression=> "Expr3&GEO&Device" } ] });

  $case->create_no_imp_publisher();
  $case->create_other(20);
}

sub channel_geo
{
  my ($self, $ns) = @_;

  new TargetingChannelStats::Case(
    $ns, 'CHANNEL-GEO',
    { geo => undef, # used default country channel (Guinea)
      targeting => 'Expression&GEO' } );
}

sub geo_only
{
  my ($self, $ns) = @_;

  new TargetingChannelStats::Case(
    $ns, 'GEO-ONLY',
    { geo => 'Macenta', 
      targeting => 'GEO' } );
}

sub geo_device
{
  my ($self, $ns) = @_;

  new TargetingChannelStats::Case(
    $ns, 'GEO-DEVICE',
    { geo => 'Siguiri/Doko', # state + town
      device => {
        platform => "FreeBSD",
        user_agent => 'Mozilla/5.0 (X11; U; FreeBSD i686; en-US; rv:1.8.1.9) ' .
          'Gecko/20071025 Firefox/2.0.0.9'},
      targeting => 'GEO&Device' } );
}

sub geo_audience
{
  my ($self, $ns) = @_;

  new TargetingChannelStats::Case(
    $ns, 'GEO-AUDIENCE',
    { geo => undef, # default country - Guinea
      audience => {},
      targeting => 'GEO&Audience' } );
}

sub channel_geo_excluded
{
  my ($self, $ns) = @_;

  new TargetingChannelStats::Case(
    $ns, 'GEO-EXCLUDED',
    { geo => undef, # default country - Guinea
      behavioral => 'BehavioralChannel',
      excluded => 'ExcludedChannel',
      targeting => 'GEO&BehavioralChannel^ExcludedChannel',
      display => { channel_id => 'BehavioralChannel' } });
}

sub init {
  my ($self, $ns) = @_;

  # Common data
  my $publisher_no_imp =
    $ns->create(Publisher =>
      { name => 'Publisher-no-imp' });

  my $advertiser_other = 
    $ns->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword_other = make_autotest_name($ns, 'KWD');

  my $publisher_other =
    $ns->create(Publisher =>
      { name => 'Publisher-other' });

  my $campaign_other = $ns->create(
    DisplayCampaign => { 
      name => 'Campaign-Other',
      account_id => $advertiser_other,
      campaigncreativegroup_cpm => 20,
      channel_id => 
        DB::BehavioralChannel->blank(
          name => 'Channel',
          account_id => $advertiser_other,
          keyword_list => $keyword_other,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
      site_links => [
        { site_id => $publisher_other->{site_id} }] });

  $ns->output("NoImp/Tag", $publisher_no_imp->{tag_id});
  $ns->output('Other/CC', $campaign_other->{cc_id});
  $ns->output("Other/Tag", $publisher_other->{tag_id});
  $ns->output("Other/KWD", $keyword_other);
  $ns->output("CPA", TargetingChannelStats::Case::DEFAULT_CPA);
  $ns->output("Other/CPM", 20);

  # Cases
  $self->all($ns);
  $self->channel_geo($ns);
  $self->geo_only($ns);
  $self->geo_device($ns);
  $self->geo_audience($ns);
  $self->channel_geo_excluded($ns);
}

1;
