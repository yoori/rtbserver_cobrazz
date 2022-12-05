package DB::CategoryChannel;

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
  account_id => DB::Entity::Type::link('DB::Account'),
  parent_channel_id => DB::Entity::Type::int(nullable => 1),
  status => DB::Entity::Type::status(),
  qa_status => DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('CategoryChannel'),
  country_code => DB::Entity::Type::country(nullable => 1, default => undef),
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  newsgate_category_name => DB::Entity::Type::string(nullable => 1),
  channel_type => 'C',
  flags => 0,
  namespace => 'C',
  visibility => 'PRI',
  message_sent => 0
};

1;
