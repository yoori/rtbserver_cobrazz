#! /usr/bin/perl
# -*- cperl -*-
#

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

delete_cascade.pl - clean given DB entities and what references them

=head1 SYNOPSIS

  delete_cascade.pl OPTIONS

=head2 OPTIONS

=over

=item C<--db, -d db>

=item C<--user, -u user>

=item C<--password, -p password>

B<Required.>  Specify database connection.  For instance

  --db=//oracle.ocslab.com/addbdev.ocslab.com \
  --user=adserver \
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

Print I<rowid> of B<dangling> rows, i.e. rows that have B<disabled>
foreign key, and reference parent row that is no longer there.  Such
rows will be preserved.

=item C<-vv>

Like C<-v>, but also print I<rowid> of unreferenced rows of B<leaf>
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
    FREQCAP => ["freq_cap_id NOT IN
                   (SELECT param_value
                    FROM adSConfig
                    WHERE param_name IN ('GLOBAL_FCAP_ID',
                                         'FREQ_CAP_GLOBAL',
                                         'FREQ_CAP_RON',
                                         'FREQ_CAP_TARGETED'))"],
    SITERATE => []
);


use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my $verbose = 0;
my %option = (
  verbose => \$verbose,
);
if (! GetOptions(\%option,
                 qw(db|d=s user|u=s password|p=s delete|D=s%
                    verbose|v+ nocommit|n delete_dangling))
    || @ARGV || (grep { not defined } @option{qw(db user password)})) {
    pod2usage(1);
}

my @delete;
while (my ($t, $c) = each %{$option{delete}}) {
    push @delete, { table => uc $t, condition => $c };
}


use DBI;

my $dbh = DBI->connect("DBI:Oracle:$option{db}",
                       $option{user}, $option{password},
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1,
                         FetchHashKeyName => 'NAME_lc' });
# Fix for https://jira.corp.foros.com/browse/ADSC-1983.  This script
# doesn't query the database for large values, so it should be safe
# here.  In general case, however, this might mask out real problems.
$dbh->{LongTruncOk} = 1;

my $reference = $dbh->prepare(q[
    SELECT uk.constraint_name uk_name,
           fk.table_name fk_table, fk.constraint_name fk_name,
           fk.status status, fk.deferrable deferrable
    FROM user_constraints uk
    JOIN user_constraints fk ON fk.r_constraint_name = uk.constraint_name
    WHERE uk.table_name = :1
]);

my $key_columns = $dbh->prepare(q[
    SELECT column_name
    FROM user_cons_columns
    WHERE constraint_name = :1
    ORDER BY position
]);

my $column_type = $dbh->prepare(q[
    SELECT data_type
    FROM user_tab_columns
    WHERE table_name = :1 AND column_name = :2
]);


my %key_columns;

sub get_key_columns {
    my ($key) = @_;

    my $columns = \$key_columns{$key};
    unless (defined $$columns) {
        $$columns = $dbh->selectcol_arrayref($key_columns, undef, $key);
    }

    return $$columns;
}

sub valid_reference {
    my ($table, $key) = @_;

    my $columns = get_key_columns($key);

    my $valid_reference_condition = sub {
        my ($column) = @_;

        my ($type) = $dbh->selectrow_array($column_type, undef,
                                           $table, $column);

        return ($type eq 'NUMBER' ? "$column >= 0" : "$column IS NOT NULL");
    };

    return join(' AND ', map { $valid_reference_condition->($_) } @$columns);
}

sub equal {
    my ($uk_name, $fk_name) = @_;

    my $uk_columns = get_key_columns($uk_name);
    my $fk_columns = get_key_columns($fk_name);

    die "(@$uk_columns) and (@$fk_columns) have different number of elements"
        if @$uk_columns != @$fk_columns;

    my @equals;
    for (my $i = 0; $i < @$uk_columns; ++$i) {
        push @equals, "parent.$uk_columns->[$i] = child.$fk_columns->[$i]";
    }

    return join(' AND ', @equals);
}


my $channel_expression;

sub pre_channel_expression {
    my ($d) = @_;

    unless ($channel_expression) {
        my $expression = $dbh->prepare(qq[
            SELECT channel_id, expression
            FROM $d->{table}
            WHERE expression IS NOT NULL
        ]);
        $expression->execute;
        while (my $row = $expression->fetchrow_hashref) {
            my @channels = $row->{expression} =~ /(\d+)/g;
            foreach my $ch (@channels) {
                ++$channel_expression->{$ch}->{$row->{channel_id}};
            }
        }
    }

    my $select = $dbh->prepare(qq[
        SELECT channel_id
        FROM $d->{table}
        WHERE $d->{condition}
    ]);
    $select->execute;
    while (my $channel = $select->fetchrow_array) {
        if (exists $channel_expression->{$channel}) {
            foreach my $ch (keys %{$channel_expression->{$channel}}) {
                do_delete ({ table => $d->{table},
                             condition => "channel_id = $ch" });
            }
        }
    }
}

sub pre_audit_log {
  my ($d) = @_;
  my $select = $dbh->prepare(qq[
        SELECT account_id
        FROM $d->{table}
        WHERE $d->{condition}
    ]);
    $select->execute;
    while (my $account = $select->fetchrow_array) {
      do_delete ({ table => "AUDITLOG",
                      condition => "object_account_id = $account" });
    }
}


my %pre_action = (
    CHANNEL => \&pre_channel_expression,
    CHANNELOLD => \&pre_channel_expression,
    ACCOUNT => \&pre_audit_log
);


my @leaf_delete;

while (my ($table, $condition) = each %leaf_table) {
    my @condition = @$condition;
    $reference->execute($table);
    while (my $row = $reference->fetchrow_hashref) {
        push @condition, qq[
            NOT EXISTS (SELECT * FROM $row->{fk_table} child
                        WHERE @{[ equal($row->{uk_name},
                                        $row->{fk_name}) ]})
        ];
    }

    my $delete = { table => "$table parent",
                   condition => join("\nAND\n", @condition) };

    my $stmt = $dbh->prepare(qq[
        SELECT cast(rowid AS varchar2(1024)) FROM $delete->{table} WHERE $delete->{condition}
    ]);
    $stmt->execute;
    my %rowid;
    while (my ($rowid) = $stmt->fetchrow_array) {
        print "Preserving leaf $table.rowid = '$rowid'\n"
          if $verbose >= 2;

        ++$rowid{$rowid};
    }
    if (keys %rowid) {
        $delete->{preserve} = \%rowid;
    }

    push @leaf_delete, $delete;
}


sub do_delete {
    my ($d) = @_;

    my $count = 0;
    if (exists $d->{preserve}) {
        my $select = $dbh->prepare(qq[
            SELECT cast(rowid AS varchar2(1024)) FROM $d->{table} WHERE $d->{condition}
        ]);
        my $delete = $dbh->prepare(qq[
            DELETE FROM $d->{table} WHERE cast(rowid AS varchar2(1024)) = ?
        ]);
        $select->execute;
        while (my ($rowid) = $select->fetchrow_array) {
            unless ($d->{preserve}->{$rowid}) {
                print "DELETE FROM $d->{table} WHERE rowid = '$rowid'\n"
                  if $verbose >= 3;

                $delete->execute($rowid);
                ++$count;
            }
        }
    } else {
        print "DELETE FROM $d->{table} WHERE $d->{condition}\n"
          if $verbose >= 3;

        my $stmt = $dbh->prepare(qq[
            DELETE FROM $d->{table} WHERE $d->{condition}
        ]);
        $stmt->execute;
        $count = $stmt->rows;
    }

    return $count;
}


my %tables;
my @tables = (keys %leaf_table, map { $_->{table} } @delete);

while (my $table = shift @tables) {
    next if $tables{$table}++;
    $reference->execute($table);
    while (my $row = $reference->fetchrow_hashref) {
        push @tables, $row->{fk_table};
    }
}


sub dbh_do {
    my ($sql) = @_;

    print "$sql\n"
      if $verbose >= 3;

    $dbh->do($sql);
}


for my $table (sort keys %tables)
{
  dbh_do(qq[
      LOCK TABLE $table IN EXCLUSIVE MODE
  ]);
}

dbh_do(q[
    SET CONSTRAINTS ALL DEFERRED
]);

my %condition_cache;

while (my $d = shift @delete) {
    (my $table = $d->{table}) =~ s/ (?:parent|child)//;

    if (exists $pre_action{$table}) {
        $pre_action{$table}->($d);
    }

    unless (exists $condition_cache{$table}) {
        $reference->execute($table);
        while (my $row = $reference->fetchrow_hashref) {
          if ($row->{deferrable} eq 'NOT DEFERRABLE')
          {
             my $parent_columns = get_key_columns($row->{fk_name});
             my ($parent_column) = @$parent_columns;
             my $child_columns = get_key_columns($row->{uk_name});
             my ($child_column) = @$child_columns;

             if ($child_column && $parent_column)
             {
               my $c = qq[$parent_column IN (SELECT $child_column FROM $d->{table} 
                                      WHERE $d->{condition})];

               my $delete = { table => "$row->{fk_table}",
                              condition => $c };
               do_delete($delete);
             }
          }
          else
          {
            my $c = qq[
                @{[ valid_reference($row->{fk_table}, $row->{fk_name}) ]}
                AND NOT EXISTS (SELECT * FROM $table parent
                                WHERE @{[ equal($row->{uk_name},
                                                $row->{fk_name}) ]})
            ];

            my $delete = { table => "$row->{fk_table} child",
                           condition => $c };

            if (! $option{delete_dangling}  && $row->{status} ne 'ENABLED') {
                my $stmt = $dbh->prepare(qq[
                    SELECT cast(rowid AS varchar2(1024))
                    FROM $row->{fk_table} child
                    WHERE $c
                ]);
                $stmt->execute;
                my %rowid;
                while (my ($rowid) = $stmt->fetchrow_array) {
                    print "Preserving dangling"
                      . " $row->{fk_table}.rowid = '$rowid'\n"
                        if $verbose >= 1;

                    ++$rowid{$rowid};
                }
                if (keys %rowid) {
                    $delete->{preserve} = \%rowid;
                }
            }
            push @{$condition_cache{$table}}, $delete;
          }
        }
    }

    my $count = do_delete($d);
    if ($count && exists $condition_cache{$table}) {
        push @delete, @{$condition_cache{$table}};
    }
}

foreach my $d (@leaf_delete) {
    do_delete($d);
}


if ($option{nocommit}) {
    dbh_do(q[
        SET CONSTRAINTS ALL IMMEDIATE
    ]);

    print(q[
        ROLLBACK
    ])
      if $verbose >= 3;

    $dbh->rollback;
} else {
    print(q[
        COMMIT
    ])
      if $verbose >= 3;

    $dbh->commit;

}
