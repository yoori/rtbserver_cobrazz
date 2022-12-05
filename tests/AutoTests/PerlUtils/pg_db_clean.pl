#! /usr/bin/perl
#

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

pg_db_clean.pl - clean DB entities matching name prefix.

=head1 SYNOPSIS

  pg_db_clean.pl OPTIONS

=head1 OPTIONS

=over

=item C<--host, -h host>

=item C<--db, -d db>

=item C<--user, -u user>

=item C<--password, -p password>

B<Required.>  Specify database connection.

=item C<--namespace namespace>

=item C<--prefix prefix>

=item C<--test test>

The script deletes records with names matching
I<namespace-prefix-test-%> SQL pattern.  By default namespace is
I<UT>, prefix is I<user_name>, and test is empty (when some
field is empty, its trailing I<'-'> is omitted too).  Thus, by default
you clean the data you created.  If you want to clean after particular
test, specify I<--test=name>.  If you want to clean all test data
after all users, specify I<--prefix=''> (empty string).  To clean the
namespace other than I<UT>, specify I<--namespace=name>.
Namespace name must not be empty to avoid accidental full clean.

=item C<--app-format name[,name]...>

List of AppFormat names to be deleted.  Default is
I<unit-test,unit-test-imp>.  Since AppFormats are shared among all
tests, the deletion happens only when I<--prefix=''>.  This option may
be given multiple times.

=back

=cut


use Getopt::Long qw(:config gnu_getopt pass_through);
use Pod::Usage;

(my $user_name = $ENV{USER}) =~ s/_/./;
my @test_app_format;
my %option = (
  namespace => 'UT',
  prefix => $user_name,
  test => '',
  'app-format' => \@test_app_format);

if (! GetOptions(\%option,
  qw(host|h=s db|d=s user|u=s password|p=s
     namespace=s prefix:s test=s), 'app-format=s@') ||
  (grep { not defined } @option{qw(host db user password)}) ||
  $option{namespace} eq '')
{
    pod2usage(1);
}

my $prefix =
  join('-', grep({ $_ ne '' } @option{qw(namespace prefix test)}), '%');

@test_app_format = ('unit-test', 'unit-test-imp')
  unless @test_app_format;

@test_app_format = split(',', join(',', @test_app_format));

my $test_app_formats = join(',', map {"'$_'"} @test_app_format);

print "Deleting records with name prefix '$prefix'.\n";

use DBI;

my $dbh = DBI->connect("DBI:Pg:host=$option{host};dbname=$option{db}",
  $option{user}, $option{password},
  { AutoCommit => 0, PrintError => 0, RaiseError => 1 });

$prefix = $dbh->quote($prefix);

# Make special condition for currency & currency_excange

my $stmt = $dbh->prepare_cached(qq[
  SELECT cur.currency_id
  FROM currency cur LEFT JOIN account acc
    ON ((acc.currency_id = cur.currency_id OR cur.currency_id = (
      SELECT currency_id
      FROM country
      WHERE country_code = acc.country_code)) AND
      acc.name NOT LIKE $prefix)
  WHERE cur.currency_id IN (
    SELECT currency_id
    FROM account
    WHERE name like $prefix)
  GROUP BY cur.currency_id
  HAVING COUNT(acc.account_id) = 0]);

$stmt->execute();

my $table = $stmt->fetchall_arrayref;
my $ids_string = join(", ", map {@$_[0]} @$table); 

my $feed_stmt = $dbh->prepare_cached(qq[
  SELECT feed_id
  FROM feed
  WHERE
    feed_id in (
      SELECT feed_id
      FROM wdtagfeed_optedin
      WHERE wdtag_id in (
        SELECT wdtag_id
        FROM wdtag
        WHERE name like $prefix)) OR
    feed_id in (
      SELECT feed_id
      FROM wdtagfeed_optedout
      WHERE wdtag_id in (
        SELECT wdtag_id
        FROM wdtag
        WHERE name like $prefix))]);

$feed_stmt->execute();

my $feed_table = $feed_stmt->fetchall_arrayref;
my $feed_ids_string = join(", ", map {@$_[0]} @$feed_table); 

my $used_feed_stmt = $dbh->prepare_cached(qq[
  SELECT feed_id
  FROM feed
  WHERE
    feed_id in (
      SELECT feed_id
      FROM wdtagfeed_optedin
      WHERE wdtag_id in (
        SELECT wdtag_id
        FROM wdtag
        WHERE name not like $prefix)) OR
    feed_id in (
      SELECT feed_id
      FROM wdtagfeed_optedout
      WHERE wdtag_id in (
        SELECT wdtag_id
        FROM wdtag
        WHERE name not like $prefix))]);

$used_feed_stmt->execute();

my $used_feed_table = $used_feed_stmt->fetchall_arrayref;
my $used_feed_ids_string = join(", ", map {@$_[0]} @$used_feed_table); 

my $feed_id_cond = $feed_ids_string ne ''? "feed_id IN ($feed_ids_string)": '';
if ($used_feed_ids_string ne '')
{
  if ($feed_id_cond ne '')
  {
    $feed_id_cond.=" AND ";
  }
  $feed_id_cond.="feed_id NOT IN ($used_feed_ids_string)";
}

$dbh->disconnect;

my $delete_shared_data = ($option{prefix} eq '' && $option{test} eq '');

my %special = (
  appformat => $delete_shared_data
    ? qq[name IN ($test_app_formats)]
    : undef,

  currency => $ids_string
    ? qq[currency_id IN ($ids_string)]
    : undef,

  feed => $feed_ids_string
    ? qq[$feed_id_cond]
    : undef,

  wdrequestmapping => qq[
    description LIKE $prefix ],

  dynamicresources => qq[
    INSTR(key, 'CategoryChannel.', 1, 1) != 0 AND
    SUBSTR(key, INSTR(key, 'CategoryChannel.', 1, 1) +
      LENGTH('CategoryChannel.')) NOT IN
    (SELECT TO_CHAR(channel_id) FROM Channel) ],

  behavioralparameters => qq[
    channel_id is null AND (
    behav_params_list_id NOT IN (
      SELECT behav_params_list_id
      FROM channel
      WHERE behav_params_list_id IS NOT NULL) OR
    behav_params_list_id IS NULL) ],

  behavioralparameterslist => qq[
    behav_params_list_id NOT IN (
      SELECT behav_params_list_id
      FROM channel
      WHERE behav_params_list_id IS NOT NULL) ],

  fraudcondition => qq[
    fraud_condition_id IS NOT NULL ],

  options => qq[
    UPPER(token) in ('ABSENTURL', 'ABSENTFILE') ],

  auditlog => qq[
    object_account_id in (
      SELECT account_id
      FROM Account
      WHERE name LIKE $prefix) ],

  'triggers child' => qq[
    NOT EXISTS (
      SELECT *
      FROM channeltrigger parent
      WHERE parent.trigger_id = child.trigger_id) ]
);


use FindBin;
use lib "$FindBin::Dir/../../Commons/Perl";


use DB::Database;
use DB::EntitiesImpl;

my %delete;
foreach my $class (keys %{DB::}) 
{
  $class =~ s/::$//;
  my $table = lc $class;
  $class = "DB::$class";
  
  if (exists $special{$table})
  {
    $delete{$table} = $special{$table}
      if defined $special{$table};
  } 
  elsif ($class->isa('DB::Entity::Base'))
  {
    my ($unique) = ($class->_unique);
    my ($name) = ($class->_name);
    $table = lc($class->_table) if $class->_table;
    $name = $unique && $unique eq 'name'
      ? 'name'
      : $name
        ? $name
        : undef;

    $delete{$table} = qq[$name LIKE $prefix] if ($name);
  }
}


my @command = ('xargs', '-0', '--no-run-if-empty',
  "$FindBin::Dir/pg_delete_cascade.pl",
  "--host=$option{host}",
  "--db=$option{db}",
  "--user=$option{user}",
  "--password=$option{password}",
  @ARGV);

open(my $fh, '|-', @command) or die "open(|- @command): $!";

while (my ($table, $condition) = each %delete)
{
  print $fh "--delete=$table=$condition\0";
}

use POSIX qw(WEXITSTATUS);
close($fh) or die "close(|- @command): $! exit status " . WEXITSTATUS($?);
