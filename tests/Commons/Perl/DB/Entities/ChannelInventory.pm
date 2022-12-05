
package DB::ChannelInventory;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  sdate => DB::Entity::Type::pq_date("current_date", unique => 1, is_auto => 0),
  channel_id => DB::Entity::Type::link('DB::BehavioralChannel', unique => 1),
  colo_id => DB::Entity::Type::link('DB::Colocation', unique => 1),
  sum_ecpm => DB::Entity::Type::float(unique => 1, default => 0.0),
  active_user_count => DB::Entity::Type::int(default => 0),
  total_user_count => DB::Entity::Type::int(default => 0),
  hits => DB::Entity::Type::int(default => 0),
  hits_urls => DB::Entity::Type::int(default => 0),
  hits_kws => DB::Entity::Type::int(default => 0),
  hits_search_kws => DB::Entity::Type::int(default => 0),
  hits_url_kws => DB::Entity::Type::int(default => 0)
};

1;

