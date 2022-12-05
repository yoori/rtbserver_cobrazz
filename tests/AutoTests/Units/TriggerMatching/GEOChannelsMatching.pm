
package GEOChannelsMatching;

use strict;
use warnings;
use encoding 'utf8';
use DB::Defaults;
use DB::Util;

use constant DEFAULT_COUNTRY => 'GB';

sub create_states {
  my ($self, $ns, $states) = @_;

  my $index = 1;
  foreach my $state (@$states)
  {
    my $channel = $ns->create(DB::GEOChannel->blank(
    name => $state->{name},
    parent_channel_id => $state->{country_channel},
    country_code => defined $state->{country_code}?
      $state->{country_code}: DEFAULT_COUNTRY,
    geo_type => 'STATE'));

    $ns->output("State" . $index ."CH", $channel);
    $ns->output("State" . $index++, $channel->{name});
    $self->{states}->{$state->{name}} = $channel;
  }
}

sub create_global_states {
  my ($self, $ns, $states) = @_;

  my $index = 1;
  foreach my $state (@$states)
  {
    my $channel = $ns->create(DB::GlobalStateChannel->blank(
    name => $state->{name},
    parent_channel_id => $state->{country_channel},
    country_code => defined $state->{country_code}?
      $state->{country_code}: DEFAULT_COUNTRY));

    $ns->output("GlobalState" . $index++ ."CH", $channel);
    $self->{states}->{$state->{name}} = $channel;
  }
}

sub create_cities {
  my ($self, $ns, $cities) = @_;

  my $index = 1;
  foreach my $city (@$cities)
  {
    my $state_id;
    
    if (defined $city->{state})
    {
      my $state = $city->{state};
      
      die "State '$state' is not defined" 
        if not defined $self->{states}->{$state};

      $state_id = 
        $self->{states}->{$state}->{channel_id};
    }

    my $channel = $ns->create(DB::GEOChannel->blank(
      name => $city->{name},
      country_code => defined $city->{country_code}?
        $city->{country_code}: DEFAULT_COUNTRY,
      geo_type => 'CITY',
      parent_channel_id => $state_id,
      city_list => 
        defined $city->{city_list}? 
          $city->{city_list}: $ns->namespace . "-" . $city->{name},
      latitude => $city->{latitude},
      longitude => $city->{longitude}));

    $ns->output("City" . $index ."CH", $channel);
    Encode::_utf8_on($channel->{name});
    $ns->output("City" . $index++, $channel->{name});
  }
}

sub create_global_cities {
  my ($self, $ns, $cities) = @_;

  my $index = 1;
  foreach my $city (@$cities)
  {
    my $state_id;
    
    if (defined $city->{state})
    {
      my $state = $city->{state};
      
      die "State '$state' is not defined" 
        if not defined $self->{states}->{$state};

      $state_id = 
        $self->{states}->{$state}->{channel_id};
    }
    elsif ($city->{country_code} eq 'HK')
    {
      $state_id = 
        DB::Defaults::instance()->geo_hk_country->{channel_id};
    }

    my $channel = $ns->create(DB::GlobalCityChannel->blank(
      name => $city->{name},
      country_code => defined $city->{country_code}?
        $city->{country_code}: DEFAULT_COUNTRY,
      parent_channel_id => $state_id,
      city_list => 
        defined $city->{city_list}? 
          $city->{city_list}: $ns->namespace . "-" . $city->{name},
      latitude => $city->{latitude},
      longitude => $city->{longitude}));

    $ns->output("GlobalCity" . $index++ ."CH", $channel);
  }
}

sub init {
  my ($self, $ns) = @_;

  $self->{states} = {};

  my $state1 = "Herefordshire";
  my $state2 = "London, City of";
  my $city   = "London";
  my $latitude = 51.5002;
  my $longitude = -0.1262;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  $ns->output(
    "GB/CntryCH", 
     DB::Defaults::instance()->geo_gb_country);
  $ns->output(
    "FR/CntryCH", 
     DB::Defaults::instance()->geo_fr_country);
  $ns->output(
    "US/CntryCH", 
     DB::Defaults::instance()->geo_us_country);
  $ns->output(
    "HK/CntryCH", 
     DB::Defaults::instance()->geo_hk_country);

  $self->create_states($ns,
    [ 
      { 
        name => "Herefordshire", 
        country_channel => 
          DB::Defaults::instance()->geo_gb_country->{channel_id}
      },
      { 
        name => "London, City of", 
        country_channel => 
          DB::Defaults::instance()->geo_gb_country->{channel_id}
      }
     ]);

  # Channels for 'debug.ip' cases
  $self->create_global_states($ns,
    [
      { 
        name => "Bretagne", 
        country_code => "FR", 
        country_channel => 
          DB::Defaults::instance()->geo_fr_country->{channel_id} 
      },
      { 
        name => "Pennsylvania", 
        country_code => "US", 
        country_channel => 
          DB::Defaults::instance()->geo_us_country->{channel_id}
      },
      { 
        name => "Indiana", 
        country_code => "US", 
        country_channel => 
          DB::Defaults::instance()->geo_us_country->{channel_id}
      } 
    ]);

  $self->create_cities($ns,
    [ 
      { 
        name => "London", state => "London, City of", 
        latitude => 51.5002, longitude => -0.1262 
      }
     ]);

  # Channels for 'debug.ip' cases
  $self->create_global_cities($ns,
    [
      { 
        name => qq[Foug\xc3\xa8res], state => "Bretagne", 
        country_code => "FR", latitude => 48.35, 
        longitude => -1.2, city_list => qq[Foug\xc3\xa8res]
      },
      { 
        name => "Central District", country_code => "HK", 
        latitude => 22.2833, longitude => 114.15,
        city_list => 
          "Central District\n" .
          "Mid Levels\n" .
          "Sheung Wan\n" .
          "Hong Kong\n" .
          "Tai Ping Shan"
      },
      { 
        name => qq[port-aux-fran\xc3\xa7ais], country_code => "TF", 
        latitude => -49.35, longitude => 70.2167,
        city_list => qq[port-aux-fran\xc3\xa7ais]
      },
      { 
        name => "Pittsburgh", country_code => "US",
        state => "Pennsylvania", 
        latitude => 40.4495, longitude => -79.988,
        city_list => 
          "Pittsburgh\n" .
          "Warrendale\n" .
          "Mckeesport\n" .
          "West Mifflin\n" .
          "Greenock\n" .
          "Curtisville\n" .
          "Ingomar\n" .
          "Wildwood\n" .
          "Mc Keesport"
      },
      { 
        name => "Indianapolis", country_code => "US",
        state => "Indiana", 
        latitude => 39.793, longitude => -86.2853,
        city_list => "Indianapolis"
      } 
    ]);

  my @locations = (
    qq[fr/bretagne/Foug\xc3\xa8res],
    "hk//central district",
    "us/pennsylvania/pittsburgh",
    "us/indiana/indianapolis");

  my $index = 0;
  foreach my $location (@locations)
  {
    Encode::_utf8_on($location);
    $ns->output("Location" . ++$index, $location);    
  }
}

1;
