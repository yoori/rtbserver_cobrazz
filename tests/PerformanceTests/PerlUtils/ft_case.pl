#!/usr/bin/perl

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

ft_case.pl - scipt for making 'full text' test configuration file

=head1 SYNOPSIS

  ft_case.pl OPTIONS <XMLFilePath>

=head1 OPTIONS

=over

=item C<--connection-string string>

B<Optional>. Allow db connection params in one string param in form
"param1=value1 param2=value2 ... paramN=valueN"
Note that option --paramN redefines earlier defined value.

=item C<--dbname, -d db>

=item C<--host, -h host>

=item C<--user, -u user>

=item C<--password, -p password>

=item C<--server, -s server>

=item C<--xsd, -x xsd-file path>

=item C<--source, -f source file path>

B<Required.>

=item C<--prefix prefix>

S<test entities prefix>

=item C<--threads>

S<Count of test threads>

=back

=cut

use Cwd;
use Getopt::Long qw(:config gnu_getopt bundling);
use Pod::Usage;
use File::Spec;
use File::Basename; 
use File::Path;

use DB::Database;
use DB::EntitiesImpl;

use XMLEncoder;
use PerformanceDB;
use CampaignConfig;
use ChannelConfig;
use CreateNews;
use FTGenerator;
use FTLinguist;
use Utils;

# constants
use constant XML_CONFIG_HTML_PATH => "http://www.peopleonpage.com/xsd/tests/PerformanceTests";
use constant DEFAULT_THREAD_NUMBER => 30;
use constant DEFAULT_MAX_EXEC_DELAY => 1000;
use constant DEFAULT_FT_COUNT => 10; # How many ft parameters should created
use constant FT_SIZE => 2048;
use constant CLIENTS_COUNT => 100_000;
use constant FT_CHANNELS_PERCENTAGE => 200;

my %options = (
  prefix => '',
  threads => DEFAULT_THREAD_NUMBER );

if (! GetOptions(\%options, 
                 qw(connection-string=s host|h=s dbname|d=s user|u=s
                    password|p=s server|s=s 
                    xsd|x=s prefix=s source|f=s threads=i )))
{
  pod2usage(1);
}

if (defined $options{'connection-string'})
{
  foreach (split(/[\s;]+/, $options{'connection-string'}))
  {
    my ($name, $value) = m/^(host|dbname|user|password|port)=(.*)$/;
    $options{$name} = $value if defined $name and not defined $options{$name};
  }
}

my @requered    = ("host", "dbname", "user", "password", "server");
my @not_defined = grep {not defined $options{$_}} @requered;

if (@not_defined)
{
  print "Options '". join(", ", @not_defined) . "' is not defined!\n";
  pod2usage(1);
}

if (scalar(@ARGV) != 1)
{
  print "Destination config folder is not defined!\n";
  pod2usage(1);
}

my $config_path = $ARGV[0];

XML::Encoder::set_namespace("test");

# Database
my $namespace_name = "BT-FullText";

my $database = new PerformanceDB::Database($options{host},
                                           $options{dbname},
                                           $options{user}, 
                                           $options{password},
                                           $namespace_name,
                                           $options{prefix});

my @ft_phrases = ();

my $xml_config_path = join("/", $config_path, "FullText-optin.xml");
my $ft_path = undef;
$ft_path = join("/", $config_path, "FT" . $options{prefix});

if (! -d $ft_path)
{
File::Path::mkpath($ft_path) || die "Cann't create directory $ft_path";
}

if ($options{source})
{
  @ft_phrases = FTLinguist::create_ft($options{source}, $ft_path);
}
else
{
  @ft_phrases = FTGenerator::create_ft($ft_path, DEFAULT_FT_COUNT, FT_SIZE);
}

$database->create_ft_channels(\@ft_phrases, FT_CHANNELS_PERCENTAGE, $options{source}? 'cn': undef);

$database->commit;

my $server = "http://" . $options{server};


my $global =
  XML::Encoder::GROUP_PARAM({},
    "GlobalSettings", 
     [ XML::Encoder::PARAM("ThreadsNumber", $options{threads}),
       XML::Encoder::PARAM("URLsFile", ""),
       XML::Encoder::NEW_LINE,
       XML::Encoder::PARAM("RefererKWsFile", ""),
       XML::Encoder::NEW_LINE,
       XML::Encoder::PARAM("ftPath", $ft_path),
       XML::Encoder::NEW_LINE,
       XML::Encoder::PARAM("ServerURL", $server),
       XML::Encoder::NEW_LINE]);

# 1st step "Create users"

my @benchmarks;

my @params_create_user = [
  XML::Encoder::PARAM({name => "op"}, "parameter", 'in'),
  XML::Encoder::PARAM("body")];

push(
  @benchmarks, 
  XML::Encoder::GROUP_PARAM(
    {  frontend => "nslookup",
       initial => "true",
       size => CLIENTS_COUNT,
       description => "Create users" },
    "Benchmark", 
     [ XML::Encoder::GROUP_PARAM(
       { url => "/services/OO",
         method => "get"},
         "request", @params_create_user )]));

# 2nd step "Full text"

my @params_full_text = [
  XML::Encoder::PARAM({name => "app"}, "parameter", 'PS'),
  XML::Encoder::PARAM({name => "format"}, "parameter", 'unit-test'),
  XML::Encoder::PARAM({name => "v"}, "parameter", '1.3.0-3.ssv1'),
  XML::Encoder::PARAM({name=>"ft"}, "parameter", "#sequence:ft=0#"),
  XML::Encoder::PARAM({name => "prck"}, "parameter", 0),
  XML::Encoder::PARAM({name => "loc.name"}, "parameter", 'gn'),
  XML::Encoder::PARAM({name => "glbfcap"}, "parameter", 0),
  XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header-short"),
  XML::Encoder::PARAM("body")];

push(
  @benchmarks, 
  XML::Encoder::GROUP_PARAM(
    {  frontend => "nslookup",
       size => CLIENTS_COUNT * 10,
       description => "Full text" },
    "Benchmark", 
     [ XML::Encoder::GROUP_PARAM(
       { url => "/services/nslookup",
         method => "get"},
         "request", @params_full_text )]));

open(my $XML, ">$xml_config_path") || die "Cann't open file $xml_config_path for write.\n";

my $benchmarks =  
  XML::Encoder::GROUP_PARAM(
    {}, "Benchmarks", \@benchmarks);
  
print $XML XML::Encoder::GROUP_PARAM(
  { "xmlns:test"=> XML_CONFIG_HTML_PATH,
    "xmlns:xsi" => "http://www.w3.org/2001/XMLSchema-instance",
    "xsi:schemaLocation"=>  XML_CONFIG_HTML_PATH . "  " . $options{xsd}},
  "Test", [$global, $benchmarks]);

close($XML);
