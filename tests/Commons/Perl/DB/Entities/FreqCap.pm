package DB::FreqCap;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  freq_cap_id => DB::Entity::Type::sequence(),
  period => DB::Entity::Type::int(nullable => 1),
  window_length => DB::Entity::Type::int(nullable => 1),
  window_count => DB::Entity::Type::int(nullable => 1),
  life_count => DB::Entity::Type::int(nullable => 1) 
};

sub _output
{
  my ($self) = @_;
  return $self->freq_cap_id();
}

1;
