
package DB::TargetingChannel;

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
  name => DB::Entity::Type::name(),
  channel_id => DB::Entity::Type::sequence(),
  account_id => undef, 
  status => DB::Entity::Type::status(),
  qa_status => DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('TargetingChannel'),
  country_code => DB::Entity::Type::country(unique => 1),
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  expression => DB::Entity::Type::string(unique => 1),
  message_sent => 0,
  parent_channel_id => undef,
  channel_type => 'T',
  flags => 0,
  namespace => 'T',
  visibility => DB::Entity::Type::enum(['PRI', 'PUB', 'CMP'])
};

1;
