#!/usr/bin/perl

use warnings;
use strict;

=head1 NAME

profiling_storm.pl - scipt for making 'profiling storm' test configuration file

=head1 SYNOPSIS

  profiling_storm.pl OPTIONS <XMLFilePath>

=head1 OPTIONS

=over

=item C<--connection-string string>

B<Optional>. Allow db connection params in one string option in form
"param1=value1 param2=value2 ... paramN=valueN".
Note that option --paramN redefines earlier defined value.

=item C<--dbname, -d db>

=item C<--host, -h host>

=item C<--user, -u user>

=item C<--password, -p password>

=item C<--server, -s server>

=item C<--xsd, -x xsd-file path>

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
use Utils;

# constants
use constant XML_CONFIG_HTML_PATH => "http://www.peopleonpage.com/xsd/tests/PerformanceTests";
use constant DEFAULT_THREAD_NUMBER => 30;
use constant DEFAULT_MAX_EXEC_DELAY => 1000;
use constant URLS_FILE_NAME => "ProfilingStorm-urls";
use constant KWS_FILE_NAME => "ProfilingStorm-keywords";
use constant CLIENTS_COUNT => 10_000;
use constant PROFILING_SIZE => 1_000_000;

my %options = (
  prefix => '',
  threads => DEFAULT_THREAD_NUMBER );

if (! GetOptions(\%options, qw(connection-string host|h=s dbname|d=s
        user|u=s password|p=s server|s=s xsd|x=s prefix=s threads=i)))
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

my @channels_info = (
 [0, 2592000, 1, 3334, 'B'],
 [86400, 2592000, 1, 3333, 'B'],
 [0, 43200, 1, 3333, 'B']);

XML::Encoder::set_namespace("test");

# Database
my $namespace_name = "BT-Storm";

my $database = new PerformanceDB::Database($options{host},
                                           $options{dbname},
                                           $options{user}, 
                                           $options{password},
                                           $namespace_name,
                                           $options{prefix});

my @queries;

foreach my $channel (@channels_info)  
{
  my ($time_from, $time_to, $minimum_visits, $count, $channel_type) = @$channel;
  $database->create_channels($count, $time_from, $time_to, $minimum_visits, $channel_type, \@queries);
}

$database->commit;

my $server = "http://" . $options{server};
my $urls_file_path =  join("/", $config_path, URLS_FILE_NAME . $options{prefix}); 
my $keywords_file_path = join("/", $config_path, KWS_FILE_NAME . $options{prefix});

my $global =
  XML::Encoder::GROUP_PARAM({},
    "GlobalSettings", 
     [ XML::Encoder::PARAM("ThreadsNumber", $options{threads}),
       XML::Encoder::NEW_LINE,
       XML::Encoder::PARAM("URLsFile", $urls_file_path),
       XML::Encoder::NEW_LINE,
       XML::Encoder::PARAM("RefererKWsFile", $keywords_file_path),
       XML::Encoder::NEW_LINE,
       XML::Encoder::PARAM("ServerURL", $server),
       XML::Encoder::NEW_LINE]);

 # 1st case "Create users"

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

my @params_profiling =  [
  XML::Encoder::PARAM({name => "app"}, "parameter", "PS"),
  XML::Encoder::PARAM({name => "referer-kw"}, "parameter", "#sequence:referer-kw=9#"),
  XML::Encoder::PARAM({name => "format"}, "parameter", 'unit-test'),
  XML::Encoder::PARAM({name => "v"}, "parameter", '1.3.0-3.ssv1'),
  XML::Encoder::PARAM({name => "prck"}, "parameter", 0),
  XML::Encoder::PARAM({name => "loc.name"}, "parameter", 'gn'),
  XML::Encoder::PARAM({name => "glbfcap"}, "parameter", 0),
  XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header-short"),
  XML::Encoder::PARAM({name => "Referer"}, "header", "#referer#"),
  XML::Encoder::PARAM("body")];


for (my $i = 0; $i < 10; ++$i)
{
  push(
    @benchmarks, 
    XML::Encoder::GROUP_PARAM(
      {  frontend  =>  "nslookup",
         size => PROFILING_SIZE / 10,
         description => "Profiling storm#" . ($i+1)},
      "Benchmark", 
      [ XML::Encoder::GROUP_PARAM(
         { url => "/services/nslookup",
           method => "get"},
         "request", @params_profiling)]));
}

my $xml_config_path = join("/", $config_path, "ProfilingStorm-optin.xml");

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

Utils::store_list($urls_file_path, \@{$database->urls(PerformanceDB::Database::REFS_ADVERTISING)});
Utils::store_list($keywords_file_path, \@{$database->keywords(PerformanceDB::Database::REFS_ADVERTISING)});

