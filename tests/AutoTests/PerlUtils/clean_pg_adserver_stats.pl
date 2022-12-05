#!/usr/bin/perl

=head1 NAME

clean_pg_adserver_stats.pl - deletes all adserver statistic that stored in PostgreSQL DB.

=head1 SYNOPSIS

  clean_adserver_stats.pl OPTIONS

=head1 OPTIONS

=over

=item C<--host hostname>

B<Optional>. Speccifies DB host, where PostgreSQL is running. 'localohost' used by default.

=item C<--db db_scheme>

B<Required>. Specifies DB scheme, where stored AdServer statistic.

=item C<--user username>

B<Required>. Specifies username for connecting with DB.

=item C<--pswd password>

B<Required>. Specifies password for indicated user for connecting with DB.

=item C<--tables, -t stats_tables>

B<Optional>. Determines list of statistical tables, that will be cleaned.
If not defined cleans all statistical tables.

=item C<--delete, -d>

B<Optional>. Script can work in two modes - verification and deletion.
In verification mode script just show adserver statistic content - how many records
will be deleted from statistical tables.
In deletion mode script deletes statistic from DB.
It is recommended run script in verification mode first to be sure
that you realy want to delete all statistic.
By default it works in verification mode.
This option turns on deletion mode.

=back

=cut

use warnings;
use strict;

use Pod::Usage;
use Getopt::Long qw(:config gnu_getopt);
use POSIX;

my %options = (host => 'localhost');

my $getopt_result = GetOptions(\%options,
  qw(delete|d tables|t=s host=s db=s user=s pswd=s help|h));

if ($getopt_result && $options{help})
{
  pod2usage( { -exitval => 0,
               -verbose => 2 } );
}

if (!$getopt_result
    || (grep { not defined } @options{qw(host db user pswd)}))
{
  pod2usage(1);
}

use DBI;

sub get_stat_tables ($)
{
  my $dbh = shift;
  my $tables_ref = $dbh->selectall_arrayref(qq[
    SELECT nspname, relname FROM pg_class
    JOIN pg_namespace ON pg_namespace.oid = relnamespace 
    WHERE relkind = 'r' AND nspname IN ('stat', 'ctr') AND 
    pg_class.oid NOT IN (select inhrelid from pg_inherits)], undef);
  return map {$_->[0] . "." . $_->[1]} @$tables_ref;
}

sub stats_count ($$)
{
  my ($dbh, $name) = @_;
  $name = lc($name);
  my $query = $dbh->prepare(qq[
    SELECT reltuples FROM pg_class WHERE oid = '$name'::regclass]);
  $query->execute();
  my @query_result = $query->fetchrow_array()
    or die $dbh->errstr;
  return $query_result[0];
}

my ($mode_string, $mode);

if ($options{delete}) {
  $mode_string = ' Deleted rows ';
  $mode = 'DELETION';
}
else {
  $mode_string = 'Rows to delete';
  $mode = 'VERIFICATION';
}


my $dbh = DBI->connect("DBI:Pg:dbname=$options{db};host=$options{host}",
                       $options{user}, $options{pswd},
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1 });

my @TO_CLEAR = defined $options{tables}
  ? grep { $_ } split(/\W+/, $options{tables})
  : get_stat_tables($dbh);

my $max_length = 0;
map {$max_length = length($_) if $max_length < length($_)} @TO_CLEAR;

my @need_to_clear = ();

if (@TO_CLEAR)
{
  print "You have run script in $mode mode (for more details see 'clean_adserver_stats.pl --help')!\nReport:\n";
  print "+" . "-" x ($max_length + 2) . "-" x 17 . "+\n";
  print "| Stats Table". " " x ($max_length - 10)."| $mode_string |\n";
  print "+" . "-" x ($max_length + 2) . "+" . "-" x 16 . "+\n";

  foreach my $stats (@TO_CLEAR)
  {
    $stats = uc($stats);
    my $deleted_rows_count = stats_count($dbh, $stats);
    print "| $stats";
    print " " x ($max_length - length($stats)) ;
    print " |    ".int($deleted_rows_count);
    print " " x (12 - length(qq[$deleted_rows_count]));
    print "|\n";
    
    push @need_to_clear, $stats if ($deleted_rows_count);
  }
  print "+" . "-" x ($max_length + 2) . "-" x 17 . "+\n\n";

}
else
{
  print "Stats tables list is empty. You should indicate correct stats tables or ignore '--tables' option!\n";
  pod2usage(1);
}

if ($options{delete} && @need_to_clear)
{
  $dbh->do("truncate " . join(",", @need_to_clear) ) or die $dbh->errstr;
  $dbh->commit or die $dbh->errstr;
}

$dbh->disconnect or die $dbh->errstr;

exit(0);
