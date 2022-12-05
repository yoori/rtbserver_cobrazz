#!/usr/bin/perl

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

openrtb_case.pl - scipt for making bidding frontend performance test configuration file

=head1 SYNOPSIS

  openrtb_case.pl OPTIONS <XMLFilePath>

=head1 OPTIONS

=over

=item C<--connection-string string>

=item C<--dbname, -d db>

=item C<--host, -h host>

=item C<--user, -u user>

=item C<--password, -p password>

=item C<--server, -s server>

=item C<--rtb_server, -r RTB server>

=item C<--xsd, -x xsd-file path>

B<Required.>

=item C<--prefix prefix>

S<test entities prefix>

=item C<--campaigns_count>

S<total campaigns count>

=item C<--threads>

S<Count of test threads>

=item C<--users_count>

S<Users count that will be created in initial benchmark>

=item C<--benchmark_size>

S<Count of requests in each benchmark part>

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
use constant XML_CONFIG_HTML_PATH   => "http://www.peopleonpage.com/xsd/tests/PerformanceTests";
use constant DEFAULT_THREAD_NUMBER  => 30;

use constant DEFAULT_CAMPAIGNS_COUNT          => 10;
use constant DEFAULT_USERS_COUNT              => 10000;
use constant DEFAULT_BENCHMARK_SIZE           => 200000;

my %options = (prefix                  => '',
               campaigns_count         => DEFAULT_CAMPAIGNS_COUNT,
               benchmark_size          => DEFAULT_BENCHMARK_SIZE,
               users_count             => DEFAULT_USERS_COUNT,
               threads                 => DEFAULT_THREAD_NUMBER);

if (! GetOptions(\%options,
  qw(connection-string=s host|h=s dbname|d=s user|u=s password|p=s
     server|s=s rtb_server|r=s xsd|x=s prefix=s campaigns_count=i benchmark_size=i
     users_count=i threads=i)))
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
my $namespace_name = "BT";

my $database = new PerformanceDB::Database($options{host},
                                           $options{dbname},
                                           $options{user}, 
                                           $options{password},
                                           $namespace_name,
                                           $options{prefix});

# Size
$database->allow_html_format();

# Country
$database->set_default_country('RU');

# Campaigns
for (1 .. $options{campaigns_count})
{
  $database->create_ron_campaign(0, $database->rtb_publisher()->{site_id});
}

$database->commit;

my $server = Utils::make_server_url($options{server});

my @useragents = ['Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)',
                  'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.7) Gecko/20050414 Firefox/1.0.3',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows 98)',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)',
                  'Mozilla/4.0 (compatible; MSIE 6.0; AOL 9.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows 98; Win 9x 4.90)',
                  'Mozilla/4.0 (compatible; MSIE 5.23; Mac_PowerPC)',
                  'Mozilla/4.0 (compatible; MSIE 5.5; Windows 98; Win 9x 4.90)',
                  'Mozilla/5.0 (Macintosh; U; PPC Mac OS X; en) AppleWebKit/312.1 (KHTML, like Gecko) Safari/312',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1) Opera 7.54  [en]',
                  'Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.7.5) Gecko/20041107 Firefox/1.0',
                  'Mozilla/5.0 (X11; U; SunOS sun4u; en-US; rv:1.4) Gecko/20041214',
                  'Mozilla/5.0 (Macintosh; U; PPC Mac OS X Mach-O; en-US; rv:1.7.8) Gecko/20050427 Camino/0.8.4',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; Crazy Browser 1.0.5; .NET CLR 1.1.4322)',
                  'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.0.1) Gecko/20020823 Netscape/7.0 (nscd2)',
                  'Mozilla/5.0 (Windows; U; Win98; en-US; rv:1.5a) Gecko/20030728 Mozilla Firebird/0.6.1',
                  'Mozilla/4.0 (compatible; MSIE 6.0; CS 2000 6.0; Windows NT 5.1)',
                  'Mozilla/4.0 (compatible; MSIE 5.5; Windows 95; T312461)',
                  'Mozilla/4.0 (compatible; MSIE 4.01; Windows CE; Sprint:PPC6600-1; PPC; 240x320)',
                  'Mozilla/5.0 (Windows; U; WinNT4.0; en-US; rv:1.7.5) Gecko/20041107 Firefox/1.0',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 4.0)',
                  'Mozilla/5.0 (Windows; U; Win98; en-US; rv:1.7.2) Gecko/20040804 Netscape/7.2 (ax)',
                  'Mozilla/5.0 (Macintosh; U; PPC Mac OS X Mach-O; en-US; rv:1.7.5) Gecko/20041107 Firefox/1.0',
                  'Mozilla/5.0 (Macintosh; U; PPC; en-US; rv:1.0.2) Gecko/20030208 Netscape/7.02',
                  'Mozilla/5.0 (Windows; U; Win95; en-US; rv:1.4) Gecko/20030624 Netscape/7.1 (ax) (CK-SillyDog)',
                  'Mozilla/4.0 (compatible; MSIE 6.0; Windows ME) Opera 7.23  [en]',
                  'Mozilla/4.0 (compatible; MSIE 5.0; Windows XP) Opera 6.04  [en]'];


my $global_settings =
    XML::Encoder::GROUP_PARAM({},
                              "GlobalSettings", [XML::Encoder::PARAM("ThreadsNumber", $options{threads}),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM("ServerURL", $server),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"user_agent"}, "Set", "item", @useragents),
                                         XML::Encoder::NEW_LINE]);

my $benchmark1 = 
    XML::Encoder::GROUP_PARAM(
      { frontend => "userbind",
        initial => "true",
        size => $options{users_count},
        description => "Bind users",
        type => "openrtb" },
      "Benchmark", 
      [ XML::Encoder::GROUP_PARAM(
        { url => "/userbind",
          method=>"get"},
        "request",
        [ XML::Encoder::PARAM("body") ])]);

# 2nd case "Advertising profiling"
my @request_params = [
  XML::Encoder::PARAM({ name => "debug.size" }, "parameter", $database->{_default_size}->{protocol_name}),
  XML::Encoder::PARAM({ name => "min_cpm_price" }, "parameter", 0),
  XML::Encoder::PARAM({ name => "user_agent"}, "parameter", "#random:user_agent#"),
  XML::Encoder::PARAM({ name => "aid" }, "parameter", $database->rtb_publisher()->{account_id}->{account_id}),
  XML::Encoder::PARAM({ name => "Content-Type"}, "header", "application/json"),
  XML::Encoder::PARAM("body") ];

my %bid_benchmark_params = (
  frontend => "openrtb",
  size => $options{benchmark_size},
  description => "Bidding request",
  type => "openrtb" );

if ($options{rtb_server})
{
  $bid_benchmark_params{server_url} = 
    Utils::make_server_url($options{rtb_server});
}

my $benchmark2 = 
    XML::Encoder::GROUP_PARAM(
      \%bid_benchmark_params,
      "Benchmark", 
      [ XML::Encoder::GROUP_PARAM(
         { url => "/bid",
           method=>"post" },
        "request", @request_params)]);

my $xml_config_path = join("/", $config_path, "OpenRTB-optin.xml");

open(my $XML, ">$xml_config_path") || die "Cann't open file $xml_config_path for write.\n";


my $benchmarks =  XML::Encoder::GROUP_PARAM({},
  "Benchmarks", 
  [ $benchmark1, $benchmark2 ]);

# Make XML document with header
print $XML XML::Encoder::GROUP_PARAM({"xmlns:test"=> XML_CONFIG_HTML_PATH,
                                      "xmlns:xsi" => "http://www.w3.org/2001/XMLSchema-instance",
                                      "xsi:schemaLocation"=>  XML_CONFIG_HTML_PATH . "  " . $options{xsd}},
                                     "Test", [$global_settings, $benchmarks]);

close($XML);


exit(0);








