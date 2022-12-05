
package DB::AudienceChannel;

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
  account_id => DB::Entity::Type::link('DB::Account', unique => 1),
  status => DB::Entity::Type::status(),
  qa_status => DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('AudienceChannel'),
  country_code => DB::Entity::Type::country(unique => 1),
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  message_sent => 0,
  channel_type => 'A',
  flags => 0,
  namespace => DB::Entity::Type::enum(['A'], unique => 1),
  visibility => DB::Entity::Type::enum(['PRI', 'PUB', 'CMP'])
};

1;
