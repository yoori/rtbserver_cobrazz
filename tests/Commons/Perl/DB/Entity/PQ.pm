
package DB::Entity::PQ;

use warnings;
use strict;
use DB::Entity::Base;
use DBI qw(:sql_types);
use DBD::Pg qw(:pg_types);

our @ISA = qw(DB::Entity::Base);

sub __get_col_sql
{
  my ($self, $col) = @_;
  my %struct = %{ $self->struct };
  if (not defined $self->{$col})
  {
    return "(\"$col\" is null)";
  }
  elsif (ref $self->{$col} eq 'DB::Entity::SQL')
  {
    return "(\"$col\"=" . $self->{$col}->sql . ")"; 
  }
  elsif ($struct{$col}->{is_name})
  {
    return "(lower(\"$col\")=lower(?))";
  }
  return "(\"$col\"=?)"
}

sub __get_where
{
  my ($self, $ns) = @_;

  my @unique = $self->_unique();

  my $id = $self->_id();


  if (! @unique && ! ref($self->{ $self->_id() }))
  {
    @unique = ($self->_id);
  }

  return unless @unique;

  my $where = qq|
    WHERE @{[ join ' AND ',
      map { $self->__get_col_sql($_) } @unique ]}
   |;

  # (not defined $_ or $_ eq ''? '': $_)
  my @values = map {  $_ eq ''? '': $_ } (
    $self->__get_db_values(
      grep {defined $self->{$_} and ref $self->{$_} ne 'DB::Entity::SQL'} 
        (@unique)));

  return ($where, \@values);
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

  my @auto = ($self->_auto());

  my @auto_columns = grep { defined($_) && exists $self->{$_} } 
    ($self->_auto());

  my $sql = qq|
    INSERT INTO $table (@{[ join ', ', map qq{"$_"}, @columns ]})
    VALUES (@{[ join ', ', @$placeholders ]})
  |;

  if (@auto_columns)
  {
    $sql .= qq|RETURNING @{[ join ', ', map qq{"$_"}, @auto_columns ]}|;
  }

  # print "$sql\n";

  my $stmt = $self->dbh($ns)->prepare_cached($sql, undef, 1);
  my $i = 0;

  foreach my $in (grep { ! ref } @$values)
  {
    $stmt->bind_param(++$i, $in);
  }

  $stmt->execute();

  if (@auto_columns)
  {
    my $res = $stmt->fetchrow_arrayref;
    for (my $i = 0; $i < @auto_columns; ++$i)
    {
      my $col = $auto_columns[$i];
      $self->{$col} = $res->[$i];
    }
  }

  $self->__select($ns);

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
    push @set, "\"@$columns[$i]\" = $placeholders->[$i]";
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
    SELECT @{[ join(', ', map qq{"$_"}, @$result_columns) or 1 ]}
      FROM $table 
      $where|
}

sub __set_sequence
{
  my $self = shift;

  if ($self->_sequence and not exists $self->{ $self->_sequence })
  {
    $self->{ $self->_sequence } = 
      $self->sql("nextval('" . $self->_sequence_name . "')");
  }
}

sub _sequence_name
{
  my $class = shift;
  my %struct = %{ $class->struct };
  my @seq = grep {
    ref($struct{$_}) eq 'DB::Entity::Type::Sequence' or
      ref($struct{$_}) eq 'DB::Entity::Type::NextSequence'} (keys %struct);

  @seq? 
    $struct{$seq[0]}{sequence}?
       $struct{$seq[0]}{sequence}: 
         lc $class->_table . "_" . $seq[0] . "_seq":
          lc $class->_table ."_seq" ;
}

sub get_display_status
{
  my ($self, $ns, $status) = @_;

  # TODO. use _object_name instead
  my $object = $self->_table;

  my $stmt = $ns->pq_dbh->prepare_cached(q[
    SELECT display_status_id 
    FROM DisplayStatus ds
    JOIN ObjectType ot
    ON (ds.object_type_id = ot.object_type_id) 
    WHERE replace(upper(ot.name), ' ', '') = upper(?) and ds.description = ?]);

  $stmt->execute($object, $status);
  (my $result) = $stmt->fetchrow_array();
  $stmt->finish;
  $result || die "Display status '$status' for '$object' not found";
}

# Get Postges DB handler
sub dbh
{
  my ($self, $ns) = @_;
  $ns->pq_dbh;
}


1;
