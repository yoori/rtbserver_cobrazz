
package DB::GEOChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  name => DB::Entity::Type::name(unique => 1),
  channel_id => DB::Entity::Type::sequence(),
  account_id => undef,
  status => DB::Entity::Type::status(),,
  qa_status => DB::Entity::Type::qa_status(),,
  display_status_id => DB::Entity::Type::display_status('GEOChannel'),
  country_code => DB::Entity::Type::country(),
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  parent_channel_id => DB::Entity::Type::int(nullable => 1),
  city_list => DB::Entity::Type::string(nullable => 1),
  geo_type => DB::Entity::Type::enum(['CITY', 'STATE', 'ADDRESS']),
  latitude => DB::Entity::Type::float(nullable => 1),
  longitude => DB::Entity::Type::float(nullable => 1),
  channel_type => 'G',
  flags => 0,
  namespace => 'G',
  visibility => 'PUB',
  message_sent => 0
};

1;

package DB::CountryChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::GEOChannel);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  %{ DB::GEOChannel->STRUCT },

  name => DB::Entity::Type::string(),
  country_code => DB::Entity::Type::country(unique => 1),
  parent_channel_id => undef,
  city_list => undef,
  geo_type => DB::Entity::Type::enum(['CNTRY'], unique => 1),
  latitude => undef,
  longitude => undef,
};

1;

package DB::GlobalCityChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::GEOChannel);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  %{ DB::GEOChannel->STRUCT },

  name => DB::Entity::Type::string(unique => 1),
  geo_type => DB::Entity::Type::enum(['CITY'], unique => 1),
};

1;

package DB::GlobalStateChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::GEOChannel);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  %{ DB::GEOChannel->STRUCT },

  name => DB::Entity::Type::string(unique => 1),
  geo_type => DB::Entity::Type::enum(['STATE'], unique => 1),
};

1;

package DB::AddressChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::GEOChannel);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  %{ DB::GEOChannel->STRUCT },

  name => DB::Entity::Type::string(),
  country_code => DB::Entity::Type::country(unique => 1),
  latitude => DB::Entity::Type::float(unique => 1),
  longitude => DB::Entity::Type::float(unique => 1),
  address => DB::Entity::Type::string(unique => 1),
  radius => DB::Entity::Type::int(unique => 1),
  radius_units => DB::Entity::Type::enum(['m', 'km', 'yd', 'mi'], unique => 1),
  geo_type => 'ADDRESS',
};

1;
