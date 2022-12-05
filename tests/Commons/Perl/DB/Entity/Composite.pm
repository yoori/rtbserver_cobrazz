
# Entity composition
package DB::Entity::Composite;

# ATTENTION! Do not change table creation order.
# It can bring to dependencies crashing.
# List of tables in a composite
sub tables
{
  my $class = shift;
  die "$class unable tables definition";
}

# List of linked entities in a composite
sub links
{
  {}
}

# Save table in the composite
sub store_table
{
  my ($self, $entity, $primary_key, $args) = @_;
  $self->{$entity->_table} = $entity;
  if ($primary_key)
  {
    $self->{$primary_key} = $entity->{$primary_key};
    $args->{$primary_key} = $entity->{$primary_key};
  }
}

# Save link in the composite
sub store_link
{
  my ($self,  $entity, $primary_key, $args) = @_;
  push (@{$self->{$entity->_table}}, $entity);
  $args->{$primary_key} = $entity->{$primary_key} 
    if ($primary_key);
}

# Create composite entity
sub __create_entity
{
  my ($self, $ns, $blank, $common_args, $own_args, $counters, $setter) = @_;
  my %args = (%$common_args, %$own_args);
 (my $class = $blank->{__entity}) =~ s/^.+::(\w+)$/$1/;
  my $prefix = lc $class;
  my $table = lc $blank->{__entity}->_table;
  my @fields = ($blank->{__entity}->_fields, 
    $blank->{__entity}->_unique, $blank->{__entity}->_external_fields);
  my $primary_key = $blank->{__entity}->_sequence;
  if ($primary_key && exists $args{$primary_key})
  {
    if (UNIVERSAL::isa($args{$primary_key}, 'DB::Entity::Base::Blank'))
    {
      $common_args->{$primary_key} = 
        $ns->create($args{$primary_key})->{$primary_key};
    }
    if (defined $counters->{$primary_key})
    {
      ++$counters->{$primary_key};
      $self->{$primary_key} = $common_args->{$primary_key};
    }
    return;
  }
  foreach my $f (@fields)
  {
    my @arg_names = ($prefix . '_' . $f, $table . '_' . $f, $f);
    foreach my $arg_name (@arg_names)
    {
      if (exists $args{$arg_name})
      {
        $blank->{$f} = $args{$arg_name};
        ++$counters->{$arg_name} 
          if defined $counters->{$arg_name};  
        last;
      }
    }
  }
  my $entity = $ns->create($blank);
  $setter->($self, $entity, $primary_key, $common_args);
}

# Create composite table entities
sub __create_tables
{
  my ($self, $ns, $args, $args_count) = @_;
  my $class = ref($self) || $self;
 
  for my $blank ($self->tables)
  {
     $self->__create_entity(
       $ns, $blank, $args, {}, 
       $args_count, \&store_table);
  }
}

# Create composite link entities
sub __create_links
{
  my ($self, $ns, $args, $args_count) = @_;
  my %links = %{$self->links};
  while (my ($k, $v) = each %links)
  {
    if ($args->{$k})
    {
      my $links_args = $args->{$k};
      die "Field \"$k\" must be ARRAY reference"
          unless ref($links_args) eq 'ARRAY';
      for my $link_args (@$links_args)
      {
        my %args_copy = (%$args);
        for my $blank (@$v)
        {
          $self->__create_entity(
             $ns, $blank, \%args_copy, $link_args, {}, \&store_link);
        }
      }
      delete $args_count->{$k};
    }
  }
}

# Composite constructor
sub _new
{
  my ($class_name, $ns, $args) = @_;

  my $self = $class_name;

  unless (ref $class_name)
  {
    $self = bless({}, $class_name);
  }

  my %args_count = map { $_ => 0 } (keys %$args);

  my %args_copy = %$args;
  $self->__create_tables($ns, \%args_copy, \%args_count);
  $self->__create_links($ns, \%args_copy, \%args_count);

  my @unknown = grep { !$args_count{$_} } (keys %args_count);
  die "$class_name unknown fields: [" . join(', ', @unknown) . "]" if @unknown;

  return $self;
}

1;
