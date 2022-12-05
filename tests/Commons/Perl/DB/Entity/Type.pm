
# Base type class
package DB::Entity::Type::Base;

use warnings;
use strict;
use Scalar::Util qw(reftype);

sub __options
{
  qw(default unique nullable private is_auto)
}

sub new
{
  my $self = shift;

  unless (ref $self)
  {
    $self = bless({ @_ }, $self);
  }

  my %options = map { $_ => 1 } ($self->__options);

  my @unknown_options = grep { not exists $options{$_} } (keys %$self);

  die "$self unknown options: @unknown_options"
     if @unknown_options;

  $self
}

sub type
{
  my $self = shift;
  my $class = ref($self);
  (my $type = $class) =~ s/^.+::(\w+)$/$1/;
  lc($type)
}

sub check
{
  my ($self, $value, $code) = @_;

  if (($self->{nullable} || $self->{private}) && !defined($value))
  {
    return 1;
  }

  # Don't check CODE
  if (ref($value) eq 'CODE')
  {
    return 1;
  }

  return 
   defined($value) && (reftype(\$value) eq "SCALAR")? 
     $code? $code->($value): 1: undef;
}

sub get
{
  undef;
}

1;

# Integer type
package DB::Entity::Type::Int;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);


sub check
{
  my ($self, $value) = @_;

  $self->SUPER::check($value, sub { $_[0] =~ /^[+-]?\d+$/ } );
}

1;

# Float type
package DB::Entity::Type::Float;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);

sub check
{
  my ($self, $value) = @_;

  $self->SUPER::check(
    $value, 
    sub { $_[0] =~ /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/ } );
}

1;

# String type
package DB::Entity::Type::String;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);

1;

# Entity name typex
package DB::Entity::Type::Name;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);

sub __options
{
  qw(unique)
}

sub new
{
  my $class =  shift;

  my $self = $class->SUPER::new(@_);

  $self->{is_name} = 1;
 
  $self;
}

1;

# Sequence type
package DB::Entity::Type::Sequence;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Int);

sub __options
{
  ()
}

sub new
{
  my $class =  shift;
  my $sequence = shift;

  my $self = $class->SUPER::new(@_);

  $self->{sequence} = $sequence;
  $self->{is_auto} = 1;
 
  $self;
}

1;

# Postgres sequence
# Temporary solution, until our postgres DB have
#  auto-increment (serial or sequence).
package DB::Entity::Type::RawSequence;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Int);

sub __options
{
  ()
}

sub new
{
  my $class =  shift;
 
  my $self = $class->SUPER::new(@_);

  $self->{sequence} = undef;
  $self->{is_auto} = 1;
 
  $self;
}

sub get
{
  my ($self, $ns, $entity, $name) = @_;

  my $table = $entity->_table;
  
  $entity->dbh($ns)->selectrow_array(
    qq[SELECT coalesce(max($name)+1, 1) FROM $table])
}

1;

# Postgres sequence
# Solution for postgres sequence having 
# 'INSERT RETURNING' issue
package DB::Entity::Type::NextSequence;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Int);

sub __options
{
  ()
}

sub new
{
  my $class =  shift;
 
  my $self = $class->SUPER::new(@_);

  $self->{sequence} = undef;
  $self->{is_auto} = 0;
 
  $self;
}

sub get
{
  my ($self, $ns, $entity, $name) = @_;

  my $seq = $entity->_sequence_name;

  $entity->dbh($ns)->selectrow_array(
    qq[SELECT nextval('$seq')])
}

1;

# Link (reference) type
package DB::Entity::Type::Link;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Int);

sub new
{
  my $class =  shift;
  my $entity = shift;

  my $self = $class->SUPER::new(@_);
  
  $self->{__entity} = $entity;

  $self;
}

sub check
{
  my ($self, $value) = @_;

  if (UNIVERSAL::isa($value, $self->{__entity}))
  {
    return 1;
  }

  if (ref($value) eq 'DB::Entity::Base::Blank' &&
        $value->{__entity}->isa($self->{__entity}))
  {
    return 1;
  }

  $self->SUPER::check($value)
}

1;

# StringObj type
package DB::Entity::Type::StringObj;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::String);

sub check
{
  my ($self, $value) = @_;

  if (ref($value) eq 'DB::Entity::Base::Blank')
  {
    return 1;
  }

  $self->SUPER::check($value)
}

1;

# Array of links (references)
package DB::Entity::Type::LinkArray;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Link);

sub type
{
  my $self = shift;
  'array of ' . $self->{__entity};
}

sub check
{
  my ($self, $value) = @_;

  if (ref($value) eq 'ARRAY')
  {
    return 1 if (!$self->{__entity});

    for my $v (@$value)
    {
      return undef if !$self->SUPER::check($v);
    }
    return 1;
  }

  $self->SUPER::check($value)
}

1;

# Enum type
package DB::Entity::Type::Enum;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);

sub __options
{
  qw(unique private nullable)
}

sub new
{
  my $class =  shift;
  my $enum = shift;

  my $self = $class->SUPER::new(@_);

  $self->{__enum} = $enum;
  $self->{default} = $enum->[0] 
    if !($self->{nullable} || $self->{private});

  $self;
}

sub type
{
  my $self = shift;
  'enum(' . join(", ", @{ $self->{__enum} }) . ')';
}

sub check
{
  my ($self, $value) = @_;

  if (!$self->SUPER::check($value))
  {
    return undef;
  }
  
  if ( !($self->{nullable} || $self->{private}) )
  {
    my @check = grep { $_ eq $value } (@{ $self->{__enum} });
    return scalar(@check) == 1
  }

  1
}

1;

# Status field
package DB::Entity::Type::Status;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Enum);

sub new
{
  my $class =  shift;

  my $self = $class->SUPER::new(['A', 'D', 'I'], @_);

  $self;
}

1;

# QA status field
package DB::Entity::Type::QAStatus;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Enum);

sub new
{
  my $class =  shift;

  my $self = $class->SUPER::new(['A', 'D', 'H'], @_);

  $self;
}

1;

# Display status
package DB::Entity::Type::DisplayStatus;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Int);

sub new
{
  my $class =  shift;
  my $entity = shift;

  my $self = $class->SUPER::new(@_);
  
  $self->{default} = 
    sub { DB::Defaults::instance()->live_display_status($entity) } 
      if not defined $self->{default};

  $self;
}

1;

# Country code
package DB::Entity::Type::Country;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);

sub new
{
  my $class =  shift;

  my $self = $class->SUPER::new(@_);

  $self->{default} = 
    sub {  DB::Defaults::instance()->country()->{country_code} } 
      if !($self->{nullable} || $self->{private} || defined $self->{default});
 
  $self;
}

sub check
{
  my ($self, $value) = @_;

  $self->SUPER::check(
    $value, 
    sub { length($_[0]) == 2 && $_[0]eq uc($_[0]) })
}

1;

# SQL based
package DB::Entity::Type::SQL;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::Base);

sub new
{
  my $class = shift;
  my $entity = shift;
  my $default = shift;
  
  my $self = $class->SUPER::new(@_);

  $self->{default} = $entity->sql($default)
      if defined $default;

  $self->{is_auto} = 1 if not exists $self->{is_auto};
 
  $self;
}

sub check
{
  my ($self, $value) = @_;

  # Don't check SQL
  if (UNIVERSAL::isa($value, 'DB::Entity::SQL'))
  {
    return 1;
  }

  $self->SUPER::check($value)
}

1;

# Date type
package DB::Entity::Type::Date;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::SQL);

1;

# Timestamp type
package DB::Entity::Type::Timestamp;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Type::SQL);

1;


# Type factory
package DB::Entity::Type;

use warnings;
use strict;

sub int
{
  new DB::Entity::Type::Int(@_)
}

sub float
{
  new DB::Entity::Type::Float(@_)
}

sub string
{
  new DB::Entity::Type::String(@_)
}

sub string_obj
{
  new DB::Entity::Type::StringObj(@_)
}

sub ora_date
{
  new DB::Entity::Type::Date('DB::Entity::Oracle',  @_)
}

sub ora_timestamp
{
  new DB::Entity::Type::Timestamp('DB::Entity::Oracle', @_)
}

sub pq_date
{
  new DB::Entity::Type::Date('DB::Entity::PQ', @_)
}

sub pq_timestamp
{
  new DB::Entity::Type::Timestamp('DB::Entity::PQ', @_)
}

sub link
{
  new DB::Entity::Type::Link(@_)
}

sub link_array
{
  new DB::Entity::Type::LinkArray(@_)
}

sub name
{
  new DB::Entity::Type::Name(@_)
}

sub sequence
{
  new DB::Entity::Type::Sequence(@_)
}

sub raw_sequence
{
  new DB::Entity::Type::RawSequence(@_)
}

sub next_sequence
{
  new DB::Entity::Type::NextSequence(@_)
}

sub display_status
{
  new DB::Entity::Type::DisplayStatus(@_)
}

sub enum
{
  new DB::Entity::Type::Enum(@_)
}

sub status
{
  new DB::Entity::Type::Status(@_)
}

sub qa_status
{
  new DB::Entity::Type::QAStatus(@_)
}

sub country
{
  new DB::Entity::Type::Country(@_)
}

1;
