#! /usr/bin/perl
# -*- cperl -*-
#
use warnings;
use strict;

=head1 NAME

row_count.pl - list row count for non-empty tables.

=head1 SYNOPSIS

  row_count.pl OPTIONS

=head2 OPTIONS

=over

=item C<--db, -d db>

=item C<--user, -u user>

=item C<--password, -p password>

B<Required.>  Specify database connection.

=back

=cut

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %option;
if (! GetOptions(\%option,
                 qw(db|d=s user|u=s password|p=s))
    || @ARGV || (grep { not defined } @option{qw(db user password)})) {
    pod2usage(1);
}

use DBI;

my $dbh = DBI->connect("DBI:Oracle:$option{db}",
                       $option{user}, $option{password},
                       { AutoCommit => 1, PrintError => 0, RaiseError => 1,
                         FetchHashKeyName => 'NAME_lc' });

my $table = $dbh->prepare(q[
    SELECT table_name
    FROM user_tables
]);

$table->execute;

my @list;
while (my ($t) = $table->fetchrow_array) {
    my ($count) = $dbh->selectrow_array(qq[
        SELECT COUNT(*)
        FROM $t
    ]);

#    push @list, "SELECT * FROM $t;\n"
    push @list, "$t: $count\n"
      if $count > 0;
}

print foreach sort @list;
