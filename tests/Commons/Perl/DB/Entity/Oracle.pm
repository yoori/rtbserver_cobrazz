
# Base oracle entity
package DB::Entity::Oracle;

use warnings;
use strict;
use DB::Entity::Base;

our @ISA = qw(DB::Entity::Base);

# Get Oracle DB handler
sub dbh
{
  my ($self, $ns) = @_;
  $ns->dbh;
}

# Insert entity
sub __insert
{
  my $self = shift;
  my ($ns) = @_;

  my $table = $self->_table;
  my @columns = grep { defined($_) && exists $self->{$_} } (
    $self->_sequence(), $self->_unique(), $self->_fields());

  my ($placeholders, $values) = $self->__get_values($ns, @columns);

  my @auto_columns = grep { 
     ref $self->{$_} eq 'CODE' || ref $self->{$_} eq 'DB::Entity::SQL' } @columns;

  my $sql = qq|
    INSERT INTO $table (@{[ join ', ', @columns ]})
    VALUES (@{[ join ', ', @$placeholders ]})
  |;

  if (@auto_columns)
  {
    $sql .= qq|
      RETURNING @{[ join ', ', @auto_columns ]}
      INTO @{[ join ', ', ('?') x @auto_columns ]}
      |;
  }

  my $stmt = $self->dbh($ns)->prepare_cached($sql, undef, 1);
  my $i = 0;

  foreach my $in (grep { ! ref } @$values)
  {
    $stmt->bind_param(++$i, $in);
  }

  foreach my $out (@$self{ @auto_columns })
  {
    $out = undef;
    $stmt->bind_param_inout(++$i, \$out, 1024);
  }

  $stmt->execute;

  return $self;
}

sub __get_update_cause
{
  my $self = shift;
  my ($columns, $placeholders, $where) = @_;

  my $table = $self->_table;

  my @set;

  for (my $i = 0; $i < @$columns; ++$i)
  {
    push @set, "@$columns[$i] = $placeholders->[$i]";
  }

  qq|
    UPDATE $table
    SET @{[ join ', ', @set ]}
    $where|
}

sub __get_select_cause
{
  my $self = shift;
  my ($result_columns, $where) = @_;

  my $table = $self->_table;

  qq|
    SELECT @{[ join(', ', @$result_columns) or 1 ]}
      FROM $table 
      $where|
}

sub __set_sequence
{
  my $self = shift;

  if ($self->_sequence and not exists $self->{ $self->_sequence })
  {
    $self->{ $self->_sequence } = 
      $self->sql($self->_sequence_name . '.nextval');
  }
}

sub get_display_status
{
  my ($self, $ns, $status) = @_;

  # TODO. use _object_name instead
  my $object = $self->_table;

  my $stmt = $ns->dbh->prepare_cached(q[
    SELECT display_status_id 
    FROM DisplayStatus ds
    JOIN ObjectType ot
    ON (ds.object_type_id = ot.object_type_id) 
    WHERE replace(upper(ot.name), ' ') = upper(?) and ds.description = ?]);

  $stmt->execute($object, $status);
  (my $result) = $stmt->fetchrow_array();
  $stmt->finish;
  $result || die "Display status '$status' for '$object' not found";
}

sub __set_args
{
  my ($self, $ns, $args) = @_;

  if (defined $args->{display_status_id} &&
      $args->{display_status_id} !~ /^\d+$/)
  {
    $args->{display_status_id} = 
      $self->get_display_status($ns, $args->{display_status_id});
  }

  $self->SUPER::__set_args($ns, $args);
}

1;
