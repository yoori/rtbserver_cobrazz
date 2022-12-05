
# Store SQL clause for argument
package DB::Entity::SQL;

use warnings;
use strict;

sub new
{
  my ($self, $sql) = @_;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }
  
  $self->{__sql} = $sql;

  return $self;
}

sub sql
{
   $_[0]->{__sql};
}

1;

# Proxy object for entity.
# Store entity class & blank arguments.
package DB::Entity::Base::Blank;

use strict;
use warnings;

sub create
{
  my $class = shift;
  my $entity = shift;
  my $self = { 
    __entity => ref($entity) || $entity, @_ };
  return bless $self, $class; 
}

# return entity class name
sub entity_class_name
{
  $_[0]->{__entity}
}

# return arguments
sub args
{
  my $self = shift;
  my %args = %$self;
  $args{__entity} = undef;
  delete $args{__entity};
  return \%args;
}

our $AUTOLOAD;

sub AUTOLOAD {
  my $self = shift;
  my $method = $AUTOLOAD;
  $method =~ s/.*:://;
  $self->{__entity}->$method(@_);
}

1;

# DB::Entity::Base : entity associated with DB
#   _new : c-tor
#   _table : name of DB table associated with entity
#   _sequence : sequence field name
#   _fields : entity (table) fields
#   _unique : unique fields (using in __select)
#   _auto : auto fields (using in RETURNING clause)
#
# DB::Entity::Base create:
#   if field is ARRAY ref it will be interpreted as [$class_name, [ fields hash ]],
#   if field is class ref and isa DB::Entity: mean that entity already created, need only link it.
#   if field is class ref and isa DB::Entity::Base::Blank: will be created associated DB::Entity
#     and replace ref to it.
#   if field is simple scalar (not isa of EntityBlank or Entity) it will be saved to DB as is.
#   depending on base type child entities can be created
#
package DB::Entity::Base;

use warnings;
use strict;
use DB::Entity::Type;

# Entity struct
# trait is one of:
# - type => 'types'
#   where types is space separated list of types : number, string, array, or package name
# - sequence => 'sequence-name' or sequence => undef
#   way to generate sequence name by table name : '<table-name>Seq'
# - is_name => 1
#   is name & required to add namespace prefix before save it to DB,
#   allowed only for string fields
# - unique => 1
#   add field to base unique key, that will be used for select row if it already exists
# - default => any-value
#   default value, must pass type restrictions
# - private => 1
#   field is private, used nested entities and doesn't appear in SQL statements
sub struct 
{ 
  my $class = ref($_[0]) || $_[0];
  $class->STRUCT
}

# SQL expression wrapper
sub sql
{
  my ($class, $sql) = @_;
  return new DB::Entity::SQL($sql);
}

# Create entity blank
sub blank
{
  my ($class, %args) = @_;
  DB::Entity::Base::Blank->create($class, %args);
}

# Entity table name
sub _table
{
  my $name = ref($_[0]) || $_[0];
  $name =~ s/.+:://;
  return $name;
}

sub _object_name
{
  my $object = ref($_[0]) || $_[0];
  $object =~ s/^.+::(\w+)$/$1/;
  $object
}


# Get entity fields
sub _fields
{
  my $class = shift;
  my %struct = %{ $class->struct };
  grep {
    not (ref($struct{$_}) && 
       ((defined $struct{$_}->{unique} && 
          $struct{$_}->{unique} == 1) ||
        (defined $struct{$_}->{private} && 
          $struct{$_}->{private} == 1) || 
        exists $struct{$_}->{sequence} ))} (keys %struct)
}

# Get entity sequenced fields
sub _sequence
{
  my $class = shift;
  my %struct = %{ $class->struct };
  my @seq = grep {
    ref($struct{$_}) && exists $struct{$_}->{sequence}} (keys %struct);
  return @seq == 1? $seq[0]: undef;
}

# Get entity sequence name
sub _sequence_name
{
  my $class = shift;
  my %struct = %{ $class->struct };
  my @seq = grep {
    ref($struct{$_}) && defined $struct{$_}->{sequence}} (keys %struct);
  @seq && $struct{$seq[0]}{sequence}?
      $struct{$seq[0]}{sequence}: $class->_table . "Seq";
}

# Get entity unique fields
sub _unique
{
  my $class = shift;
  my %struct = %{ $class->struct };
  grep {
    ref($struct{$_}) && 
      defined $struct{$_}->{unique} && 
         $struct{$_}->{unique} == 1} (keys %struct)
}

# Get entity auto fields
sub _auto
{
  my $class = shift;
  my %struct = %{ $class->struct };
  grep {
    ref($struct{$_}) && $struct{$_}->{is_auto}} (keys %struct);
}

sub _name
{
  my $class = shift;
  my %struct = %{ $class->struct };
  grep {
    ref($struct{$_}) && 
      $struct{$_}->{is_name}} (keys %struct)
}

# Get external (private) fields
sub _external_fields
{
  my $class = shift;
  my %struct = %{ $class->struct };
  grep {
    ref($struct{$_}) && $struct{$_}->{private}} (keys %struct)
}

# Get entity ID
sub _id
{
  defined $_[0]->_sequence? 
    $_[0]->_sequence: $_[0]->_fields;
}

# Get value used in ns->output
sub _output
{
  my $self = shift;
  return $self->{$self->_id};
}

# Get values for entity fields from DB
sub __get_values
{
  my $self = shift;
  my ($ns, @columns) = @_;

  foreach my $col_name (@columns)
  {
    my $col = \$self->{$col_name};

    if (UNIVERSAL::isa($$col, 'DB::Entity::Base::Blank'))
    {
      $$col = $ns->create($$col);
    }
    elsif (ref($$col) eq 'ARRAY')
    {
      $$col = $ns->create(@$$col);
    }
    elsif (ref($$col) eq 'SCALAR')
    {
      $$col = $self->{ $$$col };
    }
    elsif (ref($$col) eq 'CODE')
    {
      $$col = $$col->($self);
    }
  }

  my @placeholders = map({ 
    ref $_ eq 'DB::Entity::SQL'? $_->sql($ns): '?' }
        @$self{ @columns });

  my @values = $self->__get_db_values(@columns);

  return (\@placeholders, \@values);
}

# Check entity struct
sub __check_struct
{
  my $class = shift;
  
  my %struct = %{ $class->struct };
  my @invalid_fields = 
      grep {ref($struct{$_}) && 
        !UNIVERSAL::isa($struct{$_}, 'DB::Entity::Type::Base')} 
          (keys %struct);

  die "'$class' unknown types: [" . join(", ", @invalid_fields) . "]" 
    if @invalid_fields;
}

# Check arguments for entity fields
sub __check_args
{
  my ($self, $args) = @_;

  my $class = ref($self) || $self;

  my %struct = %{$self->struct};
  while (my ($k, $v) = each %$args)
  {
    if (not exists $struct{$k})
    {
      die "$class unknown field '$k'";
    }
    elsif (ref($struct{$k}))
    {
      my $vs = defined $v? $v: 'undef'; 
      my $class = ref($self) || $self;
      !$struct{$k}->check($v) && 
         die "'$class' incorrect '$k' value: '$vs', expected type: '" .
           $struct{$k}->type() . "'";
    }
  }
}

# Check entry arguments
sub __init_args
{
  my ($self, $ns, $args) = @_;

  my $class = ref($self) || $self;

  $self->preinit_($ns, $args);
  
  my %struct = %{$self->struct};
  while (my ($k, $v) = each %struct )
  {
    # Name field special case
    if (ref($v) && defined $v->{is_name}) 
    { 
      die "$class should defined value for field '$k'"
        if not defined $args->{$k};
      # Store original name
      $self->{'__' . $k} = $args->{$k}; 
      $args->{$k} = $ns->namespace . '-' . $args->{$k}; 
    }
    if (not exists $args->{$k})
    {
      # Constant values
      if (not ref($v)) 
      { 
        $args->{$k} = $v; 
      }
      else
      {
        # Default values
        if (exists ($v->{default})) 
        { 
          $args->{$k} = $v->{default};
        }
        else
        { 
          my $value = $v->get($ns, $self, $k);
          $args->{$k} = $value if $value;
        }
      }
    }
    elsif (not ref($v))
    {
      die "$class try to change constant field '$k'";
    }
  }
}

sub __set_sequence
{
  my $class = shift;
  die "$class can't set sequence";
}

# Set arguments
sub __set_args
{
  my $self = shift;
  my ($ns, $args) = @_;

  if (defined $args->{display_status_id} &&
      $args->{display_status_id} !~ /^\d+$/)
  {
    $args->{display_status_id} = 
      $self->get_display_status($ns, $args->{display_status_id});
  }

  $self->__init_args($ns, $args);
  $self->__check_args($args);

  foreach my $field (
      $self->_fields, 
      $self->_unique, 
      $self->_sequence,
      $self->_external_fields)
  {
    if (defined($field) && exists($args->{$field}))
    {
      $self->{$field} = $args->{$field};
    }
  }
  $self->__set_sequence();
  $self->precreate_($ns, $args);
}

# Get DB handler
sub dbh
{
  my $class = ref($_[0]) || $_[0];
  die "$class DB is not defined";
}


# Insert entity
sub __insert
{
  my $class = ref($_[0]) || $_[0];
  die "$class insert method is unsupported";
}


sub __get_update_cause
{
 my $class = ref($_[0]) || $_[0];
 die "$class UPDATE clause is unsupported";
}

sub __get_select_cause
{
 my $class = ref($_[0]) || $_[0];
 die "$class SELECT clause is unsupported";
}

# Update entity
sub __update
{
  my $self = shift;
  my ($ns, $update) = @_;

  my ($where, $old) = $self->__get_where($ns);
  return unless $where;

  while (my ($k, $v) = each %$update)
  {
    $self->{$k} = $v;
  }

  my @columns = keys %$update;
  my ($placeholders, $new) = $self->__get_values($ns, @columns);

  my $stmt = $self->dbh($ns)->prepare_cached(
    $self->__get_update_cause(\@columns, $placeholders, $where), undef, 1);

  $stmt->execute(@$new, @$old);
}

# Select entity
sub __select
{
  my $self = shift;
  my ($ns) = @_;

  my ($where, $values) = $self->__get_where($ns);
  return unless $where;

  my $table = $self->_table;
  my @result_columns = ($self->_fields(), $self->_unique());

  if(defined($self->_sequence()))
  {
    unshift(@result_columns, $self->_sequence());
  }

  my $stmt = $self->dbh($ns)->prepare_cached(
    $self->__get_select_cause(\@result_columns, $where), undef, 1);


  $stmt->execute(@$values);
  my $res = $stmt->fetchrow_arrayref;
  if ($res)
  {
    for (my $i = 0; $i < @result_columns; ++$i)
    {
      my $r = \$self->{ $result_columns[$i] };

      if (ref($$r) eq 'DB::Entity::Base::Blank')
      {
        my $class = "$$r->{__entity}";
        die "$class undefined id"
          if (not $class->_id);

        $$r->{ $class->_id } = $res->[$i];
        $$r = $ns->create($$r);
      }
      else
      {
        $$r = $res->[$i] unless UNIVERSAL::isa($$r, 'DB::Entity::Base');
      }
    }
    return $self;
  }

  return undef;
}

# Actions before entry arguments initialization
sub preinit_ 
{ 
  return
}

# Actions after set arguments values
sub precreate_ 
{ 
  return
}

# Actions after create entity
sub postcreate_ 
{ 
  return
}

# Create entity
sub __create
{
  my ($self, $ns, $args) = @_;

  $self->__set_args($ns, $args);
  $self->{args__} = $args;
  $self->__select($ns) || $self->__insert($ns);
}

# TODO: move this map to Entity.pm (splitting across entities).
my %id_field = (
    'Account::role_id::AccountRole' => 'account_role_id',
    'Account::internal_account_id::Account' => 'account_id',
    'Account::agency_account_id::Account' => 'account_id',
    'Campaign::sold_to_user_id::Users' => 'user_id',
    'Campaign::bill_to_user_id::Users' => 'user_id',
);

# Get entity fields values from DB
sub __get_db_values
{
  my $self = shift;
  my (@fields) = @_;

  my @values;
  foreach my $field (@fields)
  {
    my $value = $self->{$field};

    use Scalar::Util qw(blessed);
    if (ref($value) eq 'CODE')
    {
      $value = $value->($self);
    }
    if (UNIVERSAL::isa($value, 'DB::Entity::Base'))
    {
      my $key = join('::', $self->_table, $field, $value->_table);
      $field = $id_field{$key} if exists $id_field{$key};

      die ref($value) . "::$field does not exists"
        unless exists $value->{$field};

      $value = $value->{$field};
    }

    push @values, $value;
  }

  return @values;
}

# Get WHERE clause
sub __get_where
{
  my ($self, $ns) = @_;

  my @unique = $self->_unique();

  if (! @unique && ! ref($self->{ $self->_id() }))
  {
    @unique = ($self->_id);
  }

  return unless @unique;

  my $where = qq|
    WHERE @{[ join ' AND ',
      map { "($_ = ? OR ($_ IS NULL AND ? IS NULL))" }
        @unique ]}
   |;

  my @values = map { ($_, $_) } $self->__get_db_values(@unique);

  return ($where, \@values);
}

# Constructor
sub _new 
{
  my ($class_name, $ns, $args) = @_;

  my $self = $class_name;

  $self->__check_struct;

  unless (ref $class_name)
  {
    $self = bless({}, $class_name);
  }

  $self->__create($ns, $args);
  $self->postcreate_($ns);

  return $self;
}

sub DESTROY
{
  # Suppress AUTOLOAD.
}

our $AUTOLOAD;
# Need to get access to entity fields values
sub AUTOLOAD
{
  my $self = shift;

  use Scalar::Util qw(blessed);

  my $method = $AUTOLOAD;
  $method =~ s/.*:://;

  if (exists $self->{$method})
  {
    if (blessed($self->{$method}) && $method =~ /_id$/)
    {
      my $obj = $self->{$method};
      return $obj->{$obj->_id};
    }
    else
    {
      return $self->{$method};
    }
  }
  else
  {
    my $method_id = "${method}_id";
    if (exists $self->{$method_id}
         && blessed($self->{$method_id}))
    {
      return $self->{$method_id};
    }
    else
    {
      # die ref($self) . "->{$method} does not exists";
    }
  }
}

1;

# Dictionary mixin
# Used for read-only entities (only select, no update or insert)
package DB::Entity::DictionaryMixin;

sub __create
{
  my ($self, $ns, $args) = @_;

  foreach my $field ($self->_fields(), $self->_unique())
  {
    if (exists $args->{$field})
    {
      $self->{$field} = $args->{$field};
    }
  }

  $self->__select($ns);
}

sub _new
{
  my $class = shift;
  my $self = DB::Entity::Base::_new($class, @_);
  bless $self, $class;
}

1;

# Update mixin
# Used to update entity by key when changed
package DB::Entity::UpdateMixin;

sub compare
{
  my ($self, $name) = @_;
  UNIVERSAL::isa($self->struct->{$name}, 'DB::Entity::Type::Int') || 
    UNIVERSAL::isa($self->struct->{$name}, 'DB::Entity::Type::Float')?
      $self->{$name} != $self->{args__}->{$name}:
        $self->{$name} ne $self->{args__}->{$name};
}

sub postcreate_
{
  my ($self, $ns) = @_;

  my @changed = 
    grep { $self->compare($_) } 
      (keys %{$self->{args__}});

  if (@changed)
  {
    my %args_copy = %{$self->{args__}};
    my %args;
    @args{@changed} = @args_copy{@changed};
    $self->__update($ns, \%args);
  }
}

1;
