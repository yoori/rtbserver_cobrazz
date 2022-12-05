#! /usr/bin/perl
# -*- cperl -*-
#

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

pg_delete_cascade.pl - clean given DB entities and what references them

=head1 SYNOPSIS

  pg_delete_cascade.pl OPTIONS

=head2 OPTIONS

=over

=item C<--host, -h host>

=item C<--db, -d db>

=item C<--user, -u user>

=item C<--password, -p password>

B<Required.>  Specify database connection.  For instance

  --host=stat-dev1.ocslab.com \
  --db=ads_dev \
  --user=test_ads \
  --password=adserver

=item C<--delete, -D table=condition>

Specify root rows to be deleted.  This option may be given multiple
times.  I<table> is the name of the table, and I<condition> is I<SQL
WHERE> condition.  I.e., I<SELECT * FROM table WHERE condition> should
be a valid SQL statement that matches rows to be deleted.  I<table>
may have an alias, like

  -D 'Account a1'='not exists
                     (select * from account a2
                      where a2.account_id = a1.internal_account_id)'

=item C<--verbose, -v>

Increase verbosity level.  This option may be given multiple times.

=over

=item C<-v>

Print I<ctid> of B<dangling> rows, i.e. rows that have B<disabled>
foreign key, and reference parent row that is no longer there.  Such
rows will be preserved.

=item C<-vv>

Like C<-v>, but also print I<ctid> of unreferenced rows of B<leaf>
tables.  Such rows will be preserved.  B<Leaf> tables are I<FreqCap>
and I<SiteRate> rows of which are normally
referenced from other tables

=item C<-vvv>

Like C<-vv> plus print every I<DELETE> statement being executed.

=item C<--nocommit, -n>

Do not commit the result.  Statements are executed, consistency of the
result is checked, but then the transaction is rolled back.  This may
be useful for error checking together with C<--verbose>.

=item C<--delete_dangling>

Do not preserve dangling rows, delete them.  B<USE WITH CARE!>

=back

=head1 Algorithm

The algorithm is to delete specified rows, and then to delete child
rows that were referencing those deleted rows (that became B<dangling>
now).  Note that we preserve rows that were B<dangling> from the start
(such rows are possible if foreign key is B<disabled>).  The process
is repeated recursively until there are no more rows to delete.  Then
for B<leaf> tables I<FreqCap>, I<SiteRate> and
I<Triggers>, also delete rows that aren't referenced from any table.
On this step we also preserve B<leaf> rows that weren't referenced
from the start.

=cut

my %leaf_table = (
    freqcap => ["freq_cap_id NOT IN
                   (SELECT cast(param_value as integer)
                    FROM adSConfig
                    WHERE param_name IN ('GLOBAL_FCAP_ID',
                                         'FREQ_CAP_GLOBAL',
                                         'FREQ_CAP_RON',
                                         'FREQ_CAP_TARGETED'))"],
    siterate => [],
    triggers => []
);


use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my $verbose = 0;
my %option = (
  verbose => \$verbose,
);
if (! GetOptions(\%option,
                 qw(host|h=s db|d=s user|u=s password|p=s delete|D=s%
                    verbose|v+ nocommit|n delete_dangling))
    || @ARGV || (grep { not defined } @option{qw(host db user password)}))
{
  pod2usage(1);
}

my @delete;
while (my ($t, $c) = each %{$option{delete}})
{
  push @delete, { table => lc $t, condition => $c };
}


use DBI;

my $dbh = DBI->connect("DBI:Pg:dbname=$option{db};host=$option{host}",
                       $option{user}, $option{password},
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1,
                         FetchHashKeyName => 'NAME_lc' });
# Fix for https://jira.corp.foros.com/browse/ADSC-1983.  This script
# doesn't query the database for large values, so it should be safe
# here.  In general case, however, this might mask out real problems.
$dbh->{LongTruncOk} = 1;

my $reference = $dbh->prepare(q[
  SELECT confrelid AS pk_table_id,
         ARRAY(SELECT attname
               FROM pg_attribute
               WHERE attrelid = confrelid AND
                     attnum IN ( SELECT unnest(confkey) ) ) AS pk_cols,
         conrelid AS fk_table_id,
         conrelid::regclass AS fk_table_name,
         ARRAY(SELECT attname
               FROM pg_attribute
               WHERE attrelid = conrelid AND
                     attnum IN (SELECT unnest(conkey))) AS fk_cols,
         condeferrable AS deferrable
  FROM pg_constraint cnst
  WHERE cnst.confrelid = $1::regclass ]);

my $column_type = $dbh->prepare(q[
  SELECT typcategory
  FROM pg_type
  WHERE oid = (
    SELECT atttypid
    FROM pg_attribute
    WHERE attrelid = $1 AND attname = $2) ]);

sub valid_reference
{
  my ($table_id, $columns) = @_;
  my $valid_reference_condition = sub
  {
    my ($column) = @_;

    my ($type) = $dbh->selectrow_array($column_type, undef,
                                       $table_id, $column);

    return ($type eq 'N' ? "$column >= 0" : "$column IS NOT NULL");
  };

  return join(' AND ', map { $valid_reference_condition->($_) } @$columns);
}

sub equal
{
  my ($pk_columns, $fk_columns) = @_;

  die "(@$pk_columns) and (@$fk_columns) have different number of elements"
    if @$pk_columns != @$fk_columns;

  my @equals;
  for (my $i = 0; $i < @$pk_columns; ++$i)
  {
    push @equals, "parent.$pk_columns->[$i] = child.$fk_columns->[$i]";
  }

  return join(' AND ', @equals);
}

sub pre_audit_log
{
  my ($d) = @_;
  my $select = $dbh->prepare(qq[
    SELECT account_id
    FROM $d->{table}
    WHERE $d->{condition} ]);
    $select->execute;
    while (my $account = $select->fetchrow_array)
    {
      do_delete ({table => "auditlog",
                  condition => "object_account_id = $account"});
    }
}


my %pre_action = (
  account => \&pre_audit_log );


my @leaf_delete;

while (my ($table, $condition) = each %leaf_table)
{
  my @condition = @$condition;
  $reference->execute($table);
  while (my $row = $reference->fetchrow_hashref)
  {
    push @condition, qq[
      NOT EXISTS (SELECT * FROM $row->{fk_table_name} child
                  WHERE @{[ equal($row->{pk_cols}, $row->{fk_cols}) ]}) ];
  }

  my $delete = { table => "$table parent",
                 condition => join("\nAND\n", @condition) };

  my $stmt = $dbh->prepare(qq[
    SELECT cast(ctid AS varchar) FROM $delete->{table} WHERE $delete->{condition} ]);
  $stmt->execute;
  my %ctid;
  while (my ($ctid) = $stmt->fetchrow_array)
  {
    print "Preserving leaf $table.ctid = '$ctid'\n"
      if $verbose >= 2;
    ++$ctid{$ctid};
  }

  if (keys %ctid)
  {
    $delete->{preserve} = \%ctid;
  }

  push @leaf_delete, $delete;
}


sub do_delete
{
  my ($d) = @_;

  my $count = 0;
  if (exists $d->{preserve})
  {
    my $select = $dbh->prepare(qq[
      SELECT cast(ctid AS varchar) FROM $d->{table} WHERE $d->{condition} ]);
    my $delete = $dbh->prepare(qq[
      DELETE FROM $d->{table} WHERE cast(ctid AS varchar) = ? ]);
    $select->execute;
    while (my ($ctid) = $select->fetchrow_array)
    {
      unless ($d->{preserve}->{$ctid})
      {
        print "DELETE FROM $d->{table} WHERE ctid = '$ctid'\n"
          if $verbose >= 3;
        $delete->execute($ctid);
        ++$count;
      }
    }
  }
  else
  {
    print "DELETE FROM $d->{table} WHERE $d->{condition}\n"
      if $verbose >= 3;

    my $stmt = $dbh->prepare(qq[
      DELETE FROM $d->{table} WHERE $d->{condition} ]);
    $stmt->execute;
    $count = $stmt->rows;
  }

  return $count;
}


my %tables;
my @tables = (keys %leaf_table, map { $_->{table} } @delete);

while (my $table = shift @tables)
{
  next if $tables{$table}++;
  $reference->execute($table);
  while (my $row = $reference->fetchrow_hashref)
  {
    push @tables, $row->{fk_table_name};
  }
}


sub dbh_do
{
  my ($sql) = @_;

  print "$sql\n"
    if $verbose >= 3;

  $dbh->do($sql);
}


for my $table (sort keys %tables)
{
  dbh_do(qq[LOCK TABLE $table IN EXCLUSIVE MODE]);
}

dbh_do(qq[SET CONSTRAINTS ALL DEFERRED]);

my %condition_cache;

while (my $d = shift @delete)
{
  (my $table = $d->{table}) =~ s/ (?:parent|child)//;

  if (exists $pre_action{$table})
  {
    $pre_action{$table}->($d);
  }

  unless (exists $condition_cache{$table})
  {
    $reference->execute($table);
    while (my $row = $reference->fetchrow_hashref)
    {
      my $pk_columns = $row->{pk_cols};
      my $fk_columns = $row->{fk_cols};
      die "(@$pk_columns) and (@$fk_columns) have different number of elements"
        if @$pk_columns != @$fk_columns;

      unless ($row->{deferrable})
      {
        my @cond;
        for (my $i = 0; $i < @$pk_columns; ++$i)
        {
          my $c = qq[
            $fk_columns->[$i] IN (
              SELECT $pk_columns->[$i]
              FROM $d->{table}
              WHERE $d->{condition}) ];
          push @cond, $c;
        }
        my $delete = {
          table => "$row->{fk_table_name}",
          condition => join(' AND ', @cond) };
        do_delete($delete);
      }
      else
      {
        my @cond;
        for (my $i = 0; $i < @$pk_columns; ++$i)
        {
          my $c = qq[
            NOT EXISTS (
              SELECT *
              FROM $table parent
              WHERE @{[ equal($row->{pk_cols}, $row->{fk_cols}) ]}) ];
          push @cond, $c;
        }
        my $cond = join(' AND ',
          valid_reference($row->{fk_table_id}, $fk_columns),
          @cond);

        my $delete = { table => "$row->{fk_table_name} child",
                       condition => $cond };

        unless ($option{delete_dangling})
        {
          my $stmt = $dbh->prepare(qq[
            SELECT cast(ctid AS varchar)
            FROM $row->{fk_table_name} child
            WHERE $cond ]);
          $stmt->execute;
          my %ctid;
          while (my ($ctid) = $stmt->fetchrow_array)
          {
            print "Preserving dangling $row->{fk_table_name}.ctid = '$ctid'\n"
              if $verbose >= 1;
            ++$ctid{$ctid};
          }
          if (keys %ctid)
          {
            $delete->{preserve} = \%ctid;
          }
        }

        push @{$condition_cache{$table}}, $delete;
      }
    }
  }

  my $count = do_delete($d);
  if ($count && exists $condition_cache{$table})
  {
    push @delete, @{$condition_cache{$table}};
  }
}

foreach my $d (@leaf_delete)
{
  do_delete($d);
}

if ($option{nocommit})
{
  dbh_do(qq[SET CONSTRAINTS ALL IMMEDIATE\n]);

  print(qq[ROLLBACK\n])
    if $verbose >= 3;

  $dbh->rollback;
}
else
{
  print(qq[COMMIT\n])
    if $verbose >= 3;

  $dbh->commit;
}
