
package GEOCreativeSelection::City;
{
  sub new
  {
    my $self = shift;
    my ($name, $latitude, $longitude, $altname) = @_;
    
    unless (ref $self) {
      $self = bless {}, $self;
    }
    $self->{name} = $name;
    $self->{latitude} = $latitude;
    $self->{longitude} = $longitude;
    $self->{altname} = $altname;
    
    return $self;
  }
}

package GEOCreativeSelection::Channels;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub create
{
  my ($self, $channels) = @_;

  foreach my $state (sort keys(%$channels))
  {
    my $statechannel = 
      $self->{ns}->create(DB::GEOChannel->blank(
        name => $state,
        country_code => 'GB',
        parent_channel_id => 
          DB::Defaults::instance()->geo_gb_country->{channel_id},
        geo_type => 'STATE'));

    $self->{ns}->output(
      $state . "-CH", 
      $statechannel);
    $self->{ns}->output(
      $state . "-STATE", 
      $statechannel->{name});

    $self->{state}->{$state} = $statechannel;

  
    foreach my $city (@{$channels->{$state}})
    {

      (my $city_name = $city->{name}) =~ s/ /-/;

      my $citylist = $self->{ns}->namespace . "-" . $city->{name};

      if ( defined $city->{altname})
      {
        $citylist .= "\n" . $city->{altname};
        $self->{ns}->output(
          $city_name . "-ALTCITY", $city->{altname});
      }

      my $citychannel = 
        $self->{ns}->create(DB::GEOChannel->blank(
          name => $city->{name},
          country_code => 'GB',
          geo_type => 'CITY',
          parent_channel_id =>  $statechannel->{channel_id},
          city_list => $citylist,
          latitude =>  $city->{latitude},
          longitude => $city->{longitude}));

      $self->{city}->{$city->{name}} = $citychannel;

      $self->{ns}->output(
        $city_name . "-CH", 
        $citychannel);
      $self->{ns}->output(
        $city_name . "-CITY", $citychannel->{name});

    }
  }
}

sub state
{
  my ($self, $name) = @_;
   die "State '$name' is undefined" 
       if not defined $self->{state}->{$name};
  return $self->{state}->{$name};
}

sub city
{
  my ($self, $name) = @_;
   die "City '$name' is undefined" 
       if not defined $self->{city}->{$name};
  return $self->{city}->{$name};
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $account) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{ns} = $ns;
  $self->{account} = $account;
  $self->{state} = {};
  $self->{city} = {};
  
  return $self;
}

1;

package GEOCreativeSelection;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_channnel_text_ccg
{
  my ($self, $ns, $name, $size, $publisher, $cpm, $channels) = @_;
  my $keyword = make_autotest_name($ns, $name);

  my $campaign = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => $name,
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      country_code => 'GB',
      behavioralchannel_keyword_list => $keyword,
      behavioralchannel_country_code => 'GN',
      campaigncreativegroup_geo_channels =>  $channels,
      campaigncreativegroup_cpm => $cpm,
      site_links => [{site_id => $publisher->{site_id} }] });

  my $bp = $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank( 
      channel_id => $campaign->{channel_id},  
      trigger_type => "P",
      time_to => 3*60*60 ));

  $ns->output($name . "-KWD", $keyword);
  $ns->output($name . "-CC", $campaign->{cc_id});
  $ns->output($name . "-BP", $campaign->{channel_id} . "P");

  return $campaign;
}

sub create_ta_ccg
{
  my ($self, $ns, $name, $size, $publisher, $cpc, $channels) = @_;
  my $keyword = make_autotest_name($ns, $name);

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser-' . $name,
    country_code => 'GB',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => "Channel-" . $name,
      account_id => $advertiser,
    keyword_list => $keyword,
    channel_type => 'K',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  my $campaign = 
    $ns->create(TextAdvertisingCampaign => { 
      name => $name,
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      account_id => $advertiser,
      country_code => 'GB',
      ccgkeyword_original_keyword => $keyword,
      ccgkeyword_channel_id => $channel->{channel_id},
      campaigncreativegroup_geo_channels =>  $channels,
      ccgkeyword_ctr => 0.01,
      max_cpc_bid => $cpc,
      site_links => [{site_id => $publisher->{site_id} }] });

  $ns->output($name . "-KWD", $keyword);
  $ns->output($name . "-CC", $campaign->{cc_id});
  $ns->output($name . "-BP", $channel->page_key());

  return $campaign;
}

sub display_case {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 'Display',
    role_id => DB::Defaults::instance()->advertiser_role });

  # GEO channels
  my $geochannels = 
    GEOCreativeSelection::Channels->new(
      $ns, 
      $account);
  
  $geochannels->create({
    'Worcestershire' => 
      [ GEOCreativeSelection::City->new(
          'Ab Lench', 
          52.15, 
          -1.9667),
        GEOCreativeSelection::City->new(
          'Abberley', 
           52.3, 
           -2.3667),
        GEOCreativeSelection::City->new(
          'Elmbridge', 
          52.3167, 
          -2.15)],
    'Belfast' => 
      [ GEOCreativeSelection::City->new(
          'Cliftonville', 
          54.6167, 
          -5.9333) ]});
        
  # Behavioral channel
  my $keyword =  make_autotest_name($ns, "Display-KWD");
  
  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'DisplayChannel',
    account_id => $account,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  my $citypublisher = 
     $ns->create(Publisher => { 
       name => 'PublisherCity' });

  my $statepublisher = 
     $ns->create(Publisher => { 
       name => 'PublisherState' });

  # Display campaigns
  my $citycampaign = $ns->create(DisplayCampaign => {
    name => 'DisplayCity',
    account_id => $account,
    channel_id => $channel,
    country_code => 'GB',
    campaigncreativegroup_cpm => 3,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_geo_channels => 
      [ $geochannels->city('Ab Lench'), 
        $geochannels->city('Abberley'), 
        $geochannels->city('Cliftonville') ],
    site_links => 
      [{ site_id => $citypublisher->{site_id} }] });

  my $statecampaign = $ns->create(DisplayCampaign => {
    name => "DisplayState",
    account_id => $account,
    channel_id => $channel,
    country_code => 'GB',
    campaigncreativegroup_cpm => 3,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_geo_channels => 
      $geochannels->state('Worcestershire'),
    site_links => 
      [{ site_id => $statepublisher->{site_id}  }] });

  $ns->output("DisplayKWD", $keyword);
  $ns->output("DisplayChannelBP", $channel->page_key());
  $ns->output("DisplayStateTID", $statepublisher->{tag_id});
  $ns->output("DisplayCityTID", $citypublisher->{tag_id});
  $ns->output("DisplayStateCC", $statecampaign->{cc_id});
  $ns->output("DisplayCityCC", $citycampaign->{cc_id});
}

sub text_case {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 'Text',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $size = $ns->create(CreativeSize => {
    name => "Text",
    max_text_creatives => 4 });

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'PublisherText',
       pubaccount_country_code => 'GB',
       pricedtag_adjustment => 1.0,
       size_id => $size });

  # GEO channels
  my $geochannels = 
    GEOCreativeSelection::Channels->new(
      $ns, 
      $account);
  
  $geochannels->create({
    'Dorset' => [],
    'Devon' => 
      [ GEOCreativeSelection::City->new(
          'Abbotskerswell', 
          50.5, 
          -3.6167) ],
    'Conwy' => 
      [ GEOCreativeSelection::City->new(
          'Aber', 
          53.2333, 
          -4.0167) ]});

  $self->create_channnel_text_ccg(
    $ns, "ChannelText-1", $size, $publisher, 200,
    [ $geochannels->state('Dorset'),
      $geochannels->city('Abbotskerswell'),
      $geochannels->city('Aber') ]);
  $self->create_channnel_text_ccg(
    $ns, "ChannelText-2", $size, $publisher, 100,
    [ $geochannels->state('Dorset'),
      $geochannels->state('Devon'),
      $geochannels->city('Aber') ]);
  $self->create_ta_ccg(
    $ns, "Text-1", $size, $publisher, 2,
    [ $geochannels->state('Dorset'),
      $geochannels->state('Devon'),
      $geochannels->state('Conwy') ]);
  $self->create_ta_ccg(
    $ns, "Text-2", $size, $publisher, 1,
    [ $geochannels->state('Dorset'),
      $geochannels->state('Devon'),
      $geochannels->city('Aber') ]);

  $ns->output("TextTID", $publisher->{tag_id});  
}

sub alternative_name_case {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 'AltNameAccount',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'AltNamePublisher' });

  # GEO channels
  my $geochannels = 
    GEOCreativeSelection::Channels->new(
      $ns, 
      $account);
  
  $geochannels->create({
    'Hertford' => 
      [ GEOCreativeSelection::City->new(
          'Hallingbury', 
          51.8333, 
          0.1833,
          'Great Hallingbury') ] });

  my $keyword =  make_autotest_name($ns, "AltName-KWD");

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'AltNameChannel',
    account_id => $account,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'AltNameDisplay',
    account_id => $account,
    channel_id => $channel,
    country_code => 'GB',
    campaigncreativegroup_cpm => 100,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_geo_channels => 
      [ $geochannels->city('Hallingbury') ],
    site_links => 
      [{ site_id => $publisher->{site_id} }] });  

  $ns->output("AltNameKWD", $keyword);
  $ns->output("AltNameBP", $channel->page_key());
  $ns->output("AltNameTID", $publisher->{tag_id});
  $ns->output("AltNameCC", $campaign->{cc_id});
}

sub geo_competition {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 'Competition',
    role_id => DB::Defaults::instance()->advertiser_role });

  # GEO channels
  my $geochannels = 
    GEOCreativeSelection::Channels->new(
      $ns, 
      $account);
  
  $geochannels->create({
    'Highland' => 
      [ GEOCreativeSelection::City->new(
          'Acharacle', 
          56.7333, 
          -5.7833)] });
        
  # Behavioral channel
  my $keyword =  make_autotest_name($ns, "Competition-KWD");
  
  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'CompetitionChannel',
    account_id => $account,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'CompetitionPublisher' });

  # Display campaigns
  my $statecampaign = $ns->create(DisplayCampaign => {
    name => 'CompetitionState',
    account_id => $account,
    channel_id => $channel,
    country_code => 'GB',
    campaigncreativegroup_cpm => 10,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_geo_channels => 
      [ $geochannels->state('Highland') ],
    site_links => 
      [{ site_id => $publisher->{site_id} }] });

  my $citycampaign = $ns->create(DisplayCampaign => {
    name => "CompetitionCity",
    account_id => $account,
    channel_id => $channel,
    country_code => 'GB',
    campaigncreativegroup_cpm => 20,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_geo_channels => 
      $geochannels->city('Acharacle'),
    site_links => 
      [{ site_id => $publisher->{site_id} }] });

  $ns->output("CompetitionKWD", $keyword);
  $ns->output("CompetitionBP", $channel->page_key());
  $ns->output("CompetitionTID", $publisher->{tag_id});
  $ns->output("CompetitionStateCC", $statecampaign->{cc_id});
  $ns->output("CompetitionCityCC", $citycampaign->{cc_id});
}

sub init {
  my ($self, $ns) = @_;
  
  $self->display_case($ns);
  $self->text_case($ns);
  $self->alternative_name_case($ns);
  $self->geo_competition($ns);
}

1;
