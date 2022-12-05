#! /usr/bin/perl

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

db_dump.pl - dump DB entities matching name prefix.

=head1 SYNOPSIS

  db_dump.pl OPTIONS

=head1 OPTIONS

=over

=item C<--host, -d host>

=item C<--db, -d db>

=item C<--user, -u user>

=item C<--password, -p password>

B<Required.>

=item C<--namespace namespace>

=item C<--prefix prefix>

=item C<--test test>

=item C<--table table=cond>

=back

=cut


use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;
use DBI;

use FindBin;
use lib "$FindBin::Dir/../../Commons/Perl";

use DB::Database;
use DB::EntitiesImpl;

use constant SELECTION_DEEP => 2; # Prevent cyclic links

# Set script output encoding to UTF-8
binmode STDOUT, ':utf8';

(my $user_name = $ENV{USER}) =~ s/_/./;
my %table_cond;
my %option = (namespace => 'UT',
              prefix => $user_name,
              test => ' -- no such test -- ',
              table => \%table_cond);

if (! GetOptions(\%option,
                 qw(host|h=s db|d=s user|u=s password|p=s
                    namespace=s prefix=s test=s table=s%))
    || @ARGV || (grep { not defined }
                 @option{qw(host db user password namespace prefix test)})) {
    pod2usage(1);
}

my $prefix = join('-',
                  grep({ $_ ne '' } @option{qw(namespace prefix test)}), '%');


my $dbh = DBI->connect("DBI:Pg:host=$option{host};dbname=$option{db}",
  $option{user}, $option{password},
  { AutoCommit => 0, PrintError => 0, RaiseError => 1 });

$prefix = $dbh->quote($prefix);


my $reference = $dbh->prepare(q[
  SELECT confrelid::regclass as pk_table,
         ARRAY(SELECT attname
               FROM pg_attribute
               WHERE attrelid = confrelid AND
                     attnum IN ( SELECT unnest(confkey) ) ) AS pk_cols,
         conrelid::regclass AS fk_table,
         ARRAY(SELECT attname
               FROM pg_attribute
               WHERE attrelid = conrelid AND
                     attnum IN (SELECT unnest(conkey))) AS fk_cols,
         condeferrable AS deferrable
  FROM pg_constraint cnst
  WHERE cnst.confrelid = $1::regclass ]);


my $links = $dbh->prepare(q[
  SELECT confrelid::regclass as pk_table,
         ARRAY(SELECT attname
               FROM pg_attribute
               WHERE attrelid = confrelid AND
                     attnum IN ( SELECT unnest(confkey) ) ) AS pk_cols,
         conrelid::regclass AS fk_table,
         ARRAY(SELECT attname
               FROM pg_attribute
               WHERE attrelid = conrelid AND
                     attnum IN (SELECT unnest(conkey))) AS fk_cols,
         condeferrable AS deferrable
  FROM pg_constraint cnst
  WHERE cnst.conrelid = $1::regclass AND cnst.condeferrable]);

my %classes;

# Be careful. Adding table to the list can lead to infinite cycle.
my @leaf_tables = qw(CURRENCYEXCHANGERATE);

sub valid_condition {
    my ($table, $columns, $values) = @_;

    die "Invalid values for key @$columns" if @$columns != @$values;
    my $idx = 0;
    return join(' AND ', map { "$_ = '" . $values->[$idx++] . "'" } @$columns);
}

sub check_key_value
{
  my ($values, $keys) = @_;
  
  return (grep {defined $_} @$values) == @$keys;
}

my %tables;
my %references;
my %links;

sub get_references 
{
  my ($table) = @_;
  unless (defined $references{$table})
  {
    $reference->execute($table);
    @{$references{$table}} = ();
    while (my $row = $reference->fetchrow_hashref) 
    {
      push @{$references{$table}}, $row;
    }
  }
  return $references{$table};
}

sub get_links 
{
  my ($table) = @_;
  unless (defined $links{$table})
  {
    $links->execute($table);
    @{$links{$table}} = ();
    while (my $row = $links->fetchrow_hashref) 
    {
      push @{$links{$table}}, $row;
    }
  }
  return $links{$table};
}

sub table_columns
{
  my ($table) = @_;
  unless ($tables{$table}{columns}) 
  {
    my $class = $classes{$table};
    if (defined $class)
    {
      @{$tables{$table}{columns}} = 
        sort map {lc $_ } 
           (grep {defined $_ and $_ ne ""} 
              ($class->_sequence, 
               $class->_unique, 
               $class->_fields));
    }
    else
    {
      my @columns = ();
      my $select = $dbh->prepare(q[
       SELECT column_name FROM information_schema.columns WHERE
       table_name=lower($1) and table_schema = 'public']);
      $select->execute($table);
      while (my $row = $select->fetchrow_hashref) 
      {
        push @columns, $row->{column_name};
      }

      @{$tables{$table}{columns}} = sort @columns;
    }
  }
}

sub get_key_values
{
  my ($table, $columns, $row) = @_;
  my @table_columns = @{$tables{$table}{columns}};
  if (defined $columns && @$columns >= 1)
  {
    my %cols;
    @cols{ map {lc $_} @table_columns} = (0 .. $#table_columns);
    my @key_values =  map {$row->[$_]} (grep {defined $_} map { $cols{$_} } @$columns); 
    if (check_key_value(\@key_values, $columns))
    {
      return @key_values;
    }
  }
  return ();
}

sub add_link
{
  my ($table, $columns, $row, $link) = @_;

  my $key = join('_', @$columns);
  my $values  = join('',get_key_values($table, $columns, $row));

  foreach my $col (@$columns)
  {
    if (not exists $tables{$table}{links}{$col}{$key}{$values} &&
        $values ne '')
    {
      $tables{$table}{links}{$col}{$key}{$values} = $link;
    }
  }
}

sub get_link
{
  my ($table, $col, $row) = @_;
  my ($key) = (keys %{ $tables{$table}{links}{$col}});
  if ($key)
  {
    my @keys = ($key);
    my $values  = join('',get_key_values($table, \@keys, $row));
    return $tables{$table}{links}{$col}{$key}{$values} if $values ne '';
  }
  return undef;
}

sub is_stat
{
  my ($table) = @_;
  return ($table =~ /^[\w\W]+\.[\w\W]+$/)
}


my %condition_cache;

sub do_select 
{
  my ($d) = @_;

  my $table = $d->{table};

  return () if (is_stat($table));

  if (exists $condition_cache{$table}{$d->{condition} })
  {
    return @{ $condition_cache{$table}{$d->{condition}} };
  }
  else
  {
    @{ $condition_cache{$table}{$d->{condition}} } = ();
  }
  
  table_columns($d->{table});

  my $columns = 
    join(", ", grep {defined $_ and $_ ne ""} (@{$tables{$table}{columns}}));

  my $select = $dbh->prepare(qq[
    SELECT cast(ctid AS varchar), $columns FROM $table WHERE $d->{condition}]);
  $select->execute;

  while (my @row = $select->fetchrow_array) 
  {
    my $rowid = shift @row;

    push @{ $condition_cache{$table}{$d->{condition}} }, $rowid;

    unless ($tables{$table}{rows}{$rowid}) 
    {
      @{$tables{$table}{rows}{$rowid}} = @row;
    }

    if (defined $d->{parent_id} && defined $d->{key})
    {
      add_link($table, $d->{key}, \@row, $d->{parent_id});
    }

    # Select childs
    
    foreach my $ref (@{get_references($table)})
    {
      my $is_leaf = 
          grep( /^$ref->{fk_table}$/, @leaf_tables);

      if (not defined $d->{not_select_child} or $is_leaf == 1)
      {
        my @key_values = 
          get_key_values($table, $ref->{pk_cols}, \@row);
        

        if (@key_values != 0)
        {
         
          my $c = 
              qq[ @{[ valid_condition($ref->{fk_table}, 
                                      $ref->{fk_cols}, \@key_values) ]} ];
          
          do_select( 
                     { table => $ref->{fk_table}, condition => $c,
                       parent_id => $rowid, key => $ref->{fk_cols} } );
        }
      }
    }
    
    # Select parents
    foreach my $link (@{get_links($table)})
    {

      my @key_values = 
        get_key_values($table, $link->{fk_cols}, \@row);

      if (@key_values  != 0)
      {
        my $c = 
          qq[ @{[ valid_condition($link->{pk_table}, 
                                  $link->{pk_cols}, \@key_values) ]} ];

        my ($parent_id) = 
          do_select( 
            { table => $link->{pk_table}, condition => $c,
              not_select_child => 1 } );

        if (defined $parent_id)
        {
          add_link($table, $link->{fk_cols}, \@row, $parent_id);
        }
      }
    }
  }
  
  return @{ $condition_cache{$table}{$d->{condition}} };
}

foreach my $class (keys %{DB::}) 
{
  $class =~ s/::$//;
  $classes{lc $class} = "DB::$class";
}


while (my ($table, $class) = each %classes) 
{
  if ($class->isa('DB::Entity::Base')) 
  {
    my ($unique) = ($class->_unique);
    $table = lc($class->_table) if $class->_table;
    my $name = 
      $unique && $unique eq 'name'? 'name': undef;
    if ($name) 
    {
      do_select( {table => $table, condition => "$name LIKE $prefix"} );
    }
  }
}

while (my ($t, $e) = each %table_cond) {
    do_select( {table => lc $t, condition => $e} );
}


print <<'EOF;';
<?xml version="1.0" encoding="utf-8" ?>
<?xml-stylesheet type="text/xsl" href="TablesStructHTML.xslt"?>
<struct>
EOF;

foreach my $t (sort keys %tables)
{
  my $v = $tables{$t};

  next unless $v->{rows};

  print <<"  EOF;";
  <table id="$t">
  EOF;

  while (my ($id, $r) = each %{ $v->{rows} }) 
  {

    print <<"    EOF;";
    <row id="$id">
    EOF;

    {
      print <<"      EOF;";
      <anchor>$id</anchor>
      EOF;
    }
    
    my @row = @$r;

    for (my $i =0; $i < @{$v->{columns}}; ++$i)
    {
      my $column = $v->{columns}->[$i];
      my $value = $row[$i];
      $value = "(null)" unless defined $value;
      
      $value =~ s/&/&amp;/g;
      $value =~ s/</&lt;/g;
      $value =~ s/>/&gt;/g;
      $value =~ s/'/&quot;/g;

      my $link = get_link($t, $column, $r);

      if ( defined $link )
      {
      print <<"      EOF;";
      <link href="$link" id="$column">$value</link>
      EOF;
      }
      else
      {
      print <<"      EOF;";
      <field id="$column">$value</field>
      EOF;
      }
    }

    print <<'    EOF;';
    </row>
    EOF;
  }

  print <<'  EOF;';
  </table>
  EOF;

}

print <<'EOF;';
</struct>
EOF;
