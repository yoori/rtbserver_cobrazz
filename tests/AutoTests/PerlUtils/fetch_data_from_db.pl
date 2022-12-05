#!/usr/bin/perl

use Benchmark;
use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

use File::Find::Rule ();

=head1 NAME

fetch_data_from_db.pl - creates autotest DB data and fetchs ids or values of necessary entities to config file.

=head1 SYNOPSIS

  fetch_data_from_db.pl OPTIONS

=head1 OPTIONS

=over

=item C<--connection-file file>

B<Optional>. Specifies file that contains DB connection parameters.
You shouldn't use this option with '--connection-string', '--dbname',
'--user' or '--password' options, because they redefine params,
indicated in file.
 File example:

  %db_params = (
    host => 'hostname',
    port => 'port',
    dbname => 'database_name',
    user => 'user',
    password => 'pswd'
  );

=item C<--connection-string string>

B<Optional>. Specifies string that contains DB connection parameters.
You shouldn't use this option with '--dbname', '--user' or '--passsword' options,
because they redefine params, indicated in string.

=item C<--host, -h db_host>

B<Required> if not defined in '--connection-file' or '--connection-string'
options. Specifies host where DB located.

=item C<--dbname, -d db_name>

B<Required> if not defined in '--connection-file' or '--connection-string'
options. Specifies DB name, where autotest data will be created.

=item C<--user, -u username>

B<Required> if not defined in '--connection-file' or '--connection-string'
options. Specifies username for connecting to DB.

=item C<--password, -p password>

B<Required> if not defined '--connection-file' or '--connection-string'
options. Specifies password for indicated user for connecting to DB.

=item C<--port port>
B<Optional>. Specifies port number for connection to database;

=item C<--local_params_path dir>

B<Optional.> Specifies directory, where config file will be created.
If not defined, creates config file in '../Config/' directory by default.

=item C<--local_params_scheme xsd_scheme>

B<Optional.> Specifies path to XSD scheme for output config file.
If not defined, uses '../../../xsd/tests/AutoTests/LocalParams.xsd' by default.

=item C<-t list_of_tests_comma_separated>

B<Optional.> Specifies list of tests that required to fetch (all by default).

=back

=head1 See also:

https://confluence.ocslab.com/display/ADS/User%27s+Guide

=cut

use FindBin;
use lib "$FindBin::Dir/../../Commons/Perl";


use DB::Database;
use DB::EntitiesImpl;
use DB::Defaults;
use DB::Util;

my %options = (
  local_data_path => "$FindBin::Dir/../Config/Data/",
  local_params_path => "$FindBin::Dir/../Config/LocalParams.xml",
  local_params_scheme => "$FindBin::Dir/../../../xsd/tests/AutoTests/LocalParams.xsd"
);

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

if (GetOptions(\%options, qw(
      connection-file=s
      host|h=s dbname|d=s user|u=s password|pswd|p=s port=n
      connection-string=s
      local_params_path=s
      local_params_scheme=s
      tests|t=s
      trace=s
      help|h)) && $options{help})
{
  pod2usage( { -exitval => 0,
               -verbose => 2 } );
}

my $opt_trace = $options{'trace'};

our %db_params;

if (defined $options{'connection-file'})
{
  use File::Basename;

  my $db_file = basename($options{'connection-file'});
  my $db_dir  = dirname($options{'connection-file'});
  if ( $db_dir ne "" )
  {
    unshift(@INC, "$db_dir");
  }
  eval('require "' . $db_file . '"') ||
    die "File $options{'connection-file'} not found\n\n";
}
if (defined $options{'connection-string'})
{
  foreach (split(/[\s;]+/, $options{'connection-string'}))
  {
    my ($name, $value) = m/^(host|dbname|port|user|password)=(.*)$/;
    $db_params{$name} = $value if defined $name;
  }
}

map {$db_params{$_} = $options{$_} if defined $options{$_}}
  ('host', 'dbname', 'user', 'password', 'port');

if (grep { not defined $db_params{$_} }
    ('host', 'dbname', 'user', 'password'))
{
  warn "Params 'host', 'dbname', 'user' and 'password' must be defined " .
      "(via file, string or options)!\n";
  pod2usage(1);
}

# Execute standalone modules.  The implementation is quick and dirty.
# It should have been dynamic require, or at least do.  However code
# may reference global vars of this module, so we string-eval it
# instead.
sub execute_modules
{
  my ($db, $modules_path, $modules_mask) = @_;

  my $fetch_rule = File::Find::Rule->new;
  my @modules = $fetch_rule->file->name(@$modules_mask)->in($modules_path);

  local $/ = undef;

  # Detect test modules, which should be executed
  my @executed = ();
  my @loaded = ();
  foreach my $module (@modules)
  {
    if($module =~ m|(?:.*/)?([^/]+)\.pm$|)
    {
      my ($test_name) = $module =~ m|(?:.*/)?([^/]+)\.pm$|;

      my %symbols = ();
      eval
      {
        no strict;
        %symbols = %{$test_name . "::"}; 
      };

      if(scalar(keys %symbols) == 0)
      {
        eval
        {
          require $module;
        };
          
        if($@)
        {
          die "require of '$module': " . $@;
        }

        no strict;
        %symbols = %{$test_name . "::"}; 
      }

      if (exists $symbols{"init"})
      {
        push(@executed, $test_name);
      }
      else
      {
        use Symbol 'delete_package';
        delete_package $test_name;
        push(@loaded, $module);
      }
    }
  }
  # Load common modules
  foreach my $module (@loaded)
  {
    eval
    {
      require $module;
    };
    
    if($@)
    {
      die "require of '$module': " . $@;
    }
  }
  # Execute test modules
  foreach my $test_name (@executed)
  {
    my $ns = $db->namespace($test_name);
    print "data_loader_$test_name... ";
    my $module_class = bless({}, $test_name);
    $module_class->init($ns);
    print "done\n";
  }
}

my $dumper = new DB::Dump::XML($options{'local_params_path'}, $options{'local_params_scheme'});
(my $user_name = $ENV{USER}) =~ s/_/./;
my $database = new DB::Database($db_params{host}, $db_params{dbname},
  $db_params{user}, $db_params{password}, "UT-$user_name", $dumper,
  \%options, $opt_trace);

my @modules_mask = ( '*.pm' );

if(exists $options{"tests"})
{
  @modules_mask = ();
  foreach my $test_name(split(/,/, $options{"tests"}))
  {
    push(@modules_mask, "$test_name.pm");
  }
}

DB::Defaults::instance()->initialize($database);

execute_modules(
  $database,
  "$FindBin::Dir/../Units/",
  \@modules_mask);

calc_ctr_pq($database);

$database->commit;

DB::Defaults::instance()->close;

exit(0);
