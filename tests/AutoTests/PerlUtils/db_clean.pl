#! /usr/bin/perl
#

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

db_clean.pl - clean DB entities matching name prefix.

=head1 SYNOPSIS

  db_clean.pl OPTIONS

=head1 OPTIONS

=over

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
my %option = (namespace => 'UT',
              prefix => $user_name,
              test => '',
              'app-format' => \@test_app_format);
if (! GetOptions(\%option,
                 qw(db|d=s user|u=s password|p=s
                    namespace=s prefix:s test=s), 'app-format=s@')
    || (grep { not defined } @option{qw(db user password)})
    || $option{namespace} eq '') {
    pod2usage(1);
}

my $prefix = join('-',
                  grep({ $_ ne '' } @option{qw(namespace prefix test)}), '%');
@test_app_format = ('unit-test', 'unit-test-imp')
    unless @test_app_format;
@test_app_format = split(',', join(',', @test_app_format));


print "Deleting records with name prefix '$prefix'.\n";

use DBI;

my $dbh = DBI->connect("DBI:Oracle:$option{db}",
                       $option{user}, $option{password},
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1 });

$prefix = $dbh->quote($prefix);

# Make special condition for currency & currency_excange

my $stmt =  
    $dbh->prepare_cached("SELECT A.currency_id FROM Currency A "
                         ."LEFT JOIN Account B ON ((B.currency_id = A.currency_id "
                         ."OR A.currency_id = (Select currency_id From Country "
                         ."WHERE country_code = B.country_code)) AND "
                         ."B.Name NOT LIKE $prefix) WHERE A.currency_id IN (SELECT "
                         ."currency_id FROM Account WHERE name like $prefix) "
                         ."GROUP BY A.currency_id HAVING COUNT(B.account_id) = 0");

$stmt->execute();

my $table = $stmt->fetchall_arrayref;
my $ids_string = join(", ", map {@$_[0]} @$table); 

my $feed_stmt = $dbh->prepare_cached("SELECT feed_id FROM FEED "
                             ."WHERE feed_id in (SELECT feed_id FROM WDTAGFEED_OPTEDIN WHERE "
                             ."wdtag_id in (SELECT wdtag_id FROM WDTAG where name like $prefix)) "
                             ."OR feed_id in (SELECT feed_id FROM WDTAGFEED_OPTEDOUT WHERE "
                             ."wdtag_id in (SELECT wdtag_id FROM WDTAG where name like $prefix))");

$feed_stmt->execute();

my $feed_table = $feed_stmt->fetchall_arrayref;
my $feed_ids_string = join(", ", map {@$_[0]} @$feed_table); 

my $used_feed_stmt = $dbh->prepare_cached("SELECT feed_id FROM FEED "
                                          ."WHERE feed_id in (SELECT feed_id FROM WDTAGFEED_OPTEDIN WHERE "
                                          ."wdtag_id in (SELECT wdtag_id FROM WDTAG where name not like $prefix)) "
                                          ."OR feed_id in (SELECT feed_id FROM WDTAGFEED_OPTEDOUT WHERE "
                                          ."wdtag_id in (SELECT wdtag_id FROM WDTAG where name not like $prefix))");

$used_feed_stmt->execute();

my $used_feed_table = $used_feed_stmt->fetchall_arrayref;
my $used_feed_ids_string = join(", ", map {@$_[0]} @$used_feed_table); 

my $feed_id_cond = $feed_ids_string ne ''? "feed_id IN ($feed_ids_string)": '';
if ($used_feed_ids_string ne '') {
  if ($feed_id_cond ne '') {
    $feed_id_cond.=" AND ";
  }
  $feed_id_cond.="feed_id NOT IN ($used_feed_ids_string)";
}

$dbh->disconnect;

my $delete_shared_data = ($option{prefix} eq '' && $option{test} eq '');

my %special = (
  CHANNELOLD => "name LIKE $prefix"
               . "OR TO_CHAR(channel_id) = (SELECT expression FROM ChannelOld
                                            WHERE name LIKE $prefix)",
  APPFORMAT => ($delete_shared_data
                ? "name IN (@{[ join ', ',
                                     map { qq['$_'] } @test_app_format ]})"
                : undef),
  CURRENCY => ($ids_string ? "currency_id IN ($ids_string)" : undef),
  FEED => ($feed_ids_string ? "$feed_id_cond" : undef),
  WDREQUESTMAPPING => "description LIKE $prefix",
  DYNAMICRESOURCES => "INSTR(key, 'CategoryChannel.', 1, 1) != 0 AND "
                      . "SUBSTR(key, INSTR(key, 'CategoryChannel.', 1, 1) + LENGTH('CategoryChannel.')) NOT IN" 
                      . " (SELECT TO_CHAR(channel_id) FROM Channel)",
  BEHAVIORALPARAMETERS => "channel_id is null AND (behav_params_list_id NOT IN " .
               "(SELECT behav_params_list_id from Channel where behav_params_list_id is not null) " .
               "OR behav_params_list_id is NULL)",
  BEHAVIORALPARAMETERSLIST => "behav_params_list_id NOT IN " .
               "(SELECT behav_params_list_id from Channel where behav_params_list_id is not null) ",
  FRAUDCONDITION => "fraud_condition_id IS NOT NULL",
  OPTIONS => "UPPER(token) in ('ABSENTURL', 'ABSENTFILE')",
  AUDITLOG => "OBJECT_ACCOUNT_ID in (SELECT account_id FROM Account WHERE name LIKE $prefix)"
);


use FindBin;
use lib "$FindBin::Dir/../../Commons/Perl";


use DB::Database;
use DB::EntitiesImpl;

my %delete;
foreach my $class (keys %{DB::}) 
{
  $class =~ s/::$//;
  my $table = uc $class;
  $class = "DB::$class";
  
  if (exists $special{$table}) {
    $delete{$table} = $special{$table}
      if defined $special{$table};
  } 
  elsif ($class->isa('DB::Entity::Base'))
  {
    my ($unique) = ($class->_unique);
    my ($name) = ($class->_name);
    $table = uc($class->_table) if $class->_table;
    $name = 
      $unique && $unique eq 'name'? 'name': 
        $name? $name: undef;
    $delete{$table} = "$name LIKE $prefix" if ($name);
  }
}


my @command = ('xargs', '-0', '--no-run-if-empty',
               "$FindBin::Dir/delete_cascade.pl",
               "--db=$option{db}",
               "--user=$option{user}",
               "--password=$option{password}",
               @ARGV);
open(my $fh, '|-', @command)
  or die "open(|- @command): $!";

while (my ($table, $condition) = each %delete) {
     print $fh "--delete=$table=$condition\0";
}

use POSIX qw(WEXITSTATUS);
close($fh)
  or die "close(|- @command): $! exit status " . WEXITSTATUS($?);
