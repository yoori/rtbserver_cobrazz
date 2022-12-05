package ChannelInventoryTest::Case;

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
        $self->{ns_}, 
        $args->{keyword}):
        defined $self->{keyword_}?
          $self->{keyword_}: 
            make_autotest_name(
              $self->{ns_}, $name);
}

sub add_ext_channels
{
  my ($self, $args) = @_;

  while (my ($name, $channel) = each %$args) 
  {
    $self->{ns_}->output('Ext-' . $name, $channel );
    $self->{channels_}->{$name} = $channel;  
  }
}


sub create_targeting_channel
{
  my ($self, $name, $expression) = @_;

  foreach my $w (grep {m/.+/} split(/\W+/, $expression))
  {
    my $c = $self->{channels_}->{$w};
    die "Channel '$w' is not defined " .
        "in the case '$self->{ns_}->namespace'" 
       if not defined $c;
    $expression =~ s/$w/$c->{channel_id}/;
  }
 
  my $channel = 
    $self->{ns_}->create(DB::TargetingChannel->blank(
    name => 'Targeting-' . $name,
    expression => $expression));

  $self->{ns_}->output(
    'Targeting-' . $name, 
     $channel->channel_id());
  $self->{channels_}->{$name} = $channel;
}

sub create_behavioral_channel
{
  my ($self, $name, $args) = @_;

  my $keyword = $self->get_keyword($name, $args);

  my $channel = $self->{ns_}->create(DB::BehavioralChannel->blank(
    name => 
      'Channel-' . $name,
    account_id => $self->{account_},
    keyword_list => $keyword,
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
         $args->{time_to}: 3*24*60*60) ]
    ));

  $self->{ns_}->output(
    'BP-'. $name, 
    $channel->page_key());
  $self->{ns_}->output(
    'Channel-' . $name, 
    $channel->channel_id());
  $self->{ns_}->output(
    'Kwd-' . $name, 
    $keyword);
  $self->{channels_}->{$name} = $channel;
}

sub create_expression
{
  my ($self, $name, $expression) = @_;

  foreach my $w (grep {m/.+/} split(/\W+/, $expression))
  {
    my $c = $self->{channels_}->{$w};
    die "Channel '$w' is not defined " .
        "in the case '$self->{ns_}->namespace'" 
       if not defined $c;
    $expression =~ s/$w/$c->{channel_id}/;
  }
 
  my $channel = 
    $self->{ns_}->create(DB::ExpressionChannel->blank(
    name => 
      'Expression-' . $name,
    account_id => $self->{account_},
    expression => $expression));

  $self->{ns_}->output(
    'Expression-' . $name, 
     $channel->channel_id());
  $self->{channels_}->{$name} = $channel;
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $keyword) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{ns_} = $ns->sub_namespace($prefix);
  $self->{channels_} = ();
  $self->{account_} = 
    $self->{ns_}->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });
  if (defined $keyword)
  {
    $self->{keyword_} = 
        make_autotest_name($self->{ns_}, $keyword);
    $self->{ns_}->output(
      'Kwd', $self->{keyword_});
  }
  return $self;
}

1;

package ChannelInventoryTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub base_scenario
{
  my ($self, $ns) = @_;
  my $case = 
    ChannelInventoryTest::Case->new(
       $ns, 'Base', 'Base');
  $case->add_ext_channels(
    { 'GEO' => DB::Defaults::instance()->geo_channel,
      'Device' => DB::Defaults::instance()->device_channel });
  $case->create_behavioral_channel('Context', { time_to => 3*60*60 });
  $case->create_behavioral_channel('History', { min_visits => 20 });
  $case->create_expression('1', 'Context');
  $case->create_expression('2', 'Context&History');
  $case->create_targeting_channel('A1', 'Context|History');
  $case->create_targeting_channel('A2', 'Context&GEO&Device');
  $case->create_targeting_channel('A3', 'Context&History&GEO&Device');
}

sub active_users_scenario
{
  my ($self, $ns) = @_;
  my $case = 
    ChannelInventoryTest::Case->new(
       $ns, 'ActiveUsers', 'ActiveUsers');
  $case->create_behavioral_channel('B1');
  $case->create_behavioral_channel('B2');
  $case->create_expression('E', 'B1|B2');
}

sub daily_proc_scenario
{
  my ($self, $ns) = @_;

  my $case = 
    ChannelInventoryTest::Case->new(
       $ns, 'DailyProc');

  my $address_geo_channel =
    $ns->create(DB::AddressChannel->blank(
      name              => 'Russia, Moscow, Kremlin, 1',
      country_code      => 'RU',
      parent_channel_id => DB::Defaults::instance()->geo_ru_country->{channel_id},
      latitude          => 55.7558,
      longitude         => 37.6177,
      address           => 'Russia, Moscow, Kremlin, 1',
      radius            => 1000,
      radius_units      => 'm'));

  $case->add_ext_channels(
    { 'GEO' => DB::Defaults::instance()->geo_channel,
      'Device' => DB::Defaults::instance()->device_channel });

  $case->create_behavioral_channel(
    'C', { time_to => 23*60*60 + 59*60 + 59,
            keyword => 'C' });
  $case->create_behavioral_channel(
    'HT', { time_to => 24*60*60,
            keyword => 'HT' });
  $case->create_behavioral_channel(
    'S', { min_visits => 1, 
           time_to => 10*60,
           keyword => 'S' });
  $case->create_behavioral_channel(
    'H1', { min_visits => 1, 
            time_from => 24*60*60,
            time_to => 3*24*60*60,
            keyword => 'H1' });
  $case->create_behavioral_channel(
    'H2', { min_visits => 1, 
            time_from => 24*60*60,
            time_to => 3*24*60*60,
            keyword => 'H2' });
  $case->create_behavioral_channel(
    'H3', { min_visits => 1, 
            time_from => 24*60*60,
            time_to => 3*24*60*60,
            keyword => 'H3' });
  $case->create_expression('E', 'S|H1|H2|H3');
  $case->create_targeting_channel('A1', 'E&Device');
  $case->create_targeting_channel('A2', 'E&GEO&Device');
}

sub late_request_scenario
{
  my ($self, $ns) = @_;
  my $case = 
    ChannelInventoryTest::Case->new(
       $ns, 'LateRequest', 'LateRequest');
  $case->create_behavioral_channel('B');
}

sub delayed_logs
{
  my ($self, $ns) = @_;
  my $case = 
    ChannelInventoryTest::Case->new(
       $ns, 'DelayedLogs', 'DelayedLogs');
  $case->create_behavioral_channel('C', { time_to => 60 });
  $case->create_behavioral_channel('HT', { time_to => 3*24*60*60 });
  $case->create_behavioral_channel('S', 
    { time_to => 3*60*60,
      keyword => 'S' });
}

sub merging
{
  my ($self, $ns) = @_;
  my $case = 
    ChannelInventoryTest::Case->new(
       $ns, 'Merging');
  $case->create_behavioral_channel('C', 
    { time_to => 60,
      keyword => 'C' });
  $case->create_behavioral_channel('HT', 
    { time_to => 5*24*60*60,
      keyword => 'HT' });
  $case->create_behavioral_channel('S', 
    { time_to => 3*60*60,
      keyword => 'S' });
}

sub init
{
  my ($self, $ns) = @_;

  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("NonDefaultColo", DB::Defaults::instance()->ads_isp->{colo_id});

  $ns->output("Location", 
     DB::Defaults::instance()->geo_location);

  $self->base_scenario($ns);
  $self->active_users_scenario($ns);
  $self->daily_proc_scenario($ns);
  $self->late_request_scenario($ns);
  $self->delayed_logs($ns);
  $self->merging($ns);
}

1;
