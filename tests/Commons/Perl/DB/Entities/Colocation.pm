
# Colocation rate
package DB::ColocationRate;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  colo_rate_id => DB::Entity::Type::sequence(),
  colo_id => DB::Entity::Type::link('DB::Colocation'),
  revenue_share => DB::Entity::Type::float(default => 0.5),
  effective_date => DB::Entity::Type::pq_date("now() - interval '60 days'")
};

1;

# Colocation
package DB::Colocation;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  colo_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  account_id => DB::Entity::Type::link('DB::Account'),
  colo_rate_id => DB::Entity::Type::link('DB::ColocationRate'),
  status => DB::Entity::Type::status(),
  optout_serving => 
    DB::Entity::Type::enum(
      ['NON_OPTOUT', 'OPTIN_ONLY', 'ALL', 'NONE']),
    
  # Private
  # ColocationRate
  revenue_share => DB::Entity::Type::float(private => 1)
};

sub postcreate_ 
{
  my ($self, $ns) = @_;
  
  unless (defined $self->{colo_rate_id}) {
    my %args;
    $args{colo_id} = $self->{colo_id};
    $args{revenue_share} = $self->{revenue_share}
      if exists $self->{revenue_share};
    $self->{colo_rate_id} = 
      $ns->create(
        DB::ColocationRate->blank(%args));
    $self->__update(
      $ns, 
      { colo_rate_id => $self->{colo_rate_id}->{colo_rate_id} });
  }
}

1;

# ISP composite
package DB::Isp;

use warnings;
use strict;
use DB::Entity::Composite;

our @ISA = qw(DB::Entity::Composite);

sub tables
{
 (
   DB::Account->blank(
     role_id => DB::Defaults::instance()->isp_role),
   DB::Colocation->blank()
 )
}

1;
