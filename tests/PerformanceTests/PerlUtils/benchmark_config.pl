#!/usr/bin/perl

use Carp;

$SIG{__DIE__} = sub { Carp::confess(@_) };
$SIG{__WARN__} = sub { Carp::cluck(@_) };

use warnings;
use strict;

=head1 NAME

make_config.pl - scipt for making performance test configuration file

=head1 SYNOPSIS

  make_config.pl OPTIONS <XMLFilePath>

=head1 OPTIONS

=over

=item C<--connection-string string>

=item C<--dbname, -d db>

=item C<--host, -h host>

=item C<--user, -u user>

=item C<--password, -p password>

=item C<--server, -s server>

=item C<--xsd, -x xsd-file path>

B<Required.>

=item C<--prefix prefix>

S<test entities prefix>

=item C<--campaigns_count>

S<total campaigns count, not used if campaign_config_file set>

=item C<--threads>

S<Count of test threads>

=item C<--benchmark_size>

S<Count of requests in each benchmark part>

=item C<--channels_count> 

S<total channels count, not used if channel_config_file set>

=item C<--campaign_channels_count> 

S<count of channels linked to the one campaign, must be less then channels_count>

=item C<--campaign_config_file> 

S<config file path, file contain information for creating campaigns>

=item C<--channel_config_file> 

S<config file path, file contain information for creating channels>

=item C<--free_tags_size> 

S<Count of "free" tags>

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
use constant DEFAULT_MAX_EXEC_DELAY  => 1000;

use constant URLS_FILE_NAME       => "urls-Storm";
use constant KWS_FILE_NAME        => "keywords-Storm";

use constant DEFAULT_CAMPAIGNS_COUNT          => 10;
use constant DEFAULT_CHANNELS_COUNT           => 10;
use constant DEFAULT_CAMPAIGN_CHANNELS_COUNT  => 12;
use constant DEFAULT_FREE_TAGS_SIZE           => 10;
use constant DEFAULT_BENCHMARK_SIZE           => 10000;

my %options = (prefix                  => '',
               campaigns_count         => DEFAULT_CAMPAIGNS_COUNT,
               channels_count          => DEFAULT_CHANNELS_COUNT,
               benchmark_size          => DEFAULT_BENCHMARK_SIZE,
               campaign_channels_count => DEFAULT_CAMPAIGN_CHANNELS_COUNT,
               threads                 => DEFAULT_THREAD_NUMBER,
               free_tags_size          => DEFAULT_FREE_TAGS_SIZE);

if (! GetOptions(\%options,
  qw(connection-string=s host|h=s dbname|d=s user|u=s password|p=s
     server|s=s xsd|x=s prefix=s campaigns_count=i channels_count=i
     campaign_channels_count=s campaign_config_file=s
     channel_config_file=s benchmark_size=i free_tags_size=i
     threads=i)))
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

my %campaign_flags;

if (defined $options{campaign_config_file})
{
  my $config_file  = new CampaignConfig::File($options{campaign_config_file});
  %campaign_flags  = $config_file->get();
}

my @channels_info;
if (defined $options{channel_config_file})
{
  my $config_file  = new ChannelConfig::File($options{channel_config_file});
  @channels_info = $config_file->get();
}

XML::Encoder::set_namespace("test");

# Database
my $namespace_name = "BT";

my $database = new PerformanceDB::Database($options{host},
                                           $options{dbname},
                                           $options{user}, 
                                           $options{password},
                                           $namespace_name,
                                           $options{prefix});

# Advertising channels
if (defined $options{channel_config_file})
{
  foreach my $channel (@channels_info)  
  {
    my ($time_from, $time_to, $minimum_visits, $count, $channel_type) = @$channel;
    if (!defined $channel_type or $channel_type ne 'D') {
      $database->create_channels($count, $time_from, $time_to, $minimum_visits, $channel_type, \@queries);
    }
  }
}
else
{
  $database->create_channels($options{channels_count}, 0, 0);
}

# Campaigns
if (defined $options{campaign_config_file})
{
  foreach my $campaign_flag (keys %campaign_flags)
  {
    my $count = $campaign_flags{$campaign_flag};
    if ($count)
    {
      $database->create_campaigns($count, $options{campaign_channels_count}, $campaign_flag);
    }
  }
}
else
{
  $database->create_campaigns($options{campaigns_count}, 
                              $options{campaign_channels_count}, 0);
}

# RON campaign
$database->create_ron_campaign(1);

# "Free" tags
if ($options{free_tags_size})
{
  $database->create_free_tags($options{free_tags_size});
}

# Discover channels
if (defined $options{channel_config_file})
{
  foreach my $channel (@channels_info)  
  {
    my ($time_from, $time_to, $minimum_visits, $count, $channel_type) = @$channel;
    if (defined $channel_type and $channel_type eq 'D') {
      $database->create_channels($count, $time_from, $time_to, $minimum_visits, $channel_type, \@queries);
    }
  }
}

$database->commit;

my $server = Utils::make_server_url($options{server});

my @versions = ['1.0.160',
                '1.0.168',
                '1.0.180'];

my @browsers = ['1.1',
                '1.2'];

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


my @OS = ['5.1.2600 (Service Pack 2) PI=2'];

my @countries = ['gn'];

my @formats  = ['unit-test'];

my @bool = [0,1];

my $urls_file_path =  join("/", $config_path, URLS_FILE_NAME . $options{prefix}); 
my $keywords_file_path = join("/", $config_path, KWS_FILE_NAME . $options{prefix});

my $section1 =
    XML::Encoder::GROUP_PARAM({},
                              "GlobalSettings", [XML::Encoder::PARAM("ThreadsNumber", $options{threads}),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM("URLsFile", $urls_file_path),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM("RefererKWsFile", $keywords_file_path),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM("ServerURL", $server),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"version"}, "Set", "item", @versions),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"browser"}, "Set", "item", @browsers),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"user_agent"}, "Set", "item", @useragents),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"os"}, "Set", "item", @OS),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"tid"}, "Set", "item", 
                                                                  \@{$database->tids(PerformanceDB::Database::TAGS_USED)}),
                                         XML::Encoder::PARAM_LIST({name=>"tid_free"}, "Set", "item", 
                                                                  \@{$database->tids(PerformanceDB::Database::TAGS_FREE)}),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"country"}, "Set", "item", @countries),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"format"}, "Set", "item", @formats),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"queries"}, "Set", "item", \@queries),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"bool"}, "Set", "item", @bool),
                                         XML::Encoder::NEW_LINE]);

# optin config
{
  # 1st case "Create users"
  my @params1 = [XML::Encoder::PARAM({name=>"op"}, "parameter", "in"),
                 XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                 XML::Encoder::PARAM("body")];
  my $benchmark1 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 initial=>"true",
                                 size=>$options{benchmark_size},
                                 description=>"Create users"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/OO",
                                                           method=>"get"},
                                                          "request", @params1)]);

  # 2nd case "Advertising profiling"
  my @params2 =  [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                  XML::Encoder::PARAM({name=>"referer-kw"}, "parameter", "#sequence:referer-kw=100#"),
                  XML::Encoder::PARAM({name=>"format"}, "parameter", "#random:format#"),
                  XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                  XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                  XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                  XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                  XML::Encoder::PARAM({name=>"loc.name"}, "parameter", "#random:country#"),
                  XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                  XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header-short"),
                  XML::Encoder::PARAM({name=>"Referer"}, "header", "#referer#"),
                  XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                  XML::Encoder::PARAM("body")];

  my $benchmark2 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 size=>$options{benchmark_size},
                                 description=>"Advertising profiling"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                           method=>"get"},
                                                          "request", @params2)]);

  # 3d case "Discover profiling"
  my @params3=  [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                 XML::Encoder::PARAM({name=>"referer-kw"}, "parameter", "#sequence:discover_kwd=10#"),
                 XML::Encoder::PARAM({name=>"format"}, "parameter", "#random:format#"),
                 XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                 XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                 XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                 XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"loc.name"}, "parameter", "#random:country#"),
                 XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header"),
                 XML::Encoder::PARAM({name=>"Referer"}, "header", "#random:discover_url#"),
                 XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                 XML::Encoder::PARAM("body")];

  my $benchmark3 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 size=>$options{benchmark_size},
                                 description=>"Discover profiling"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                           method=>"get"},
                                                          "request", @params3)]);

  # 5th case "Advertising"
  my @params5=  [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                 XML::Encoder::PARAM({name=>"tid"}, "parameter", "#random:tid#"),
                 XML::Encoder::PARAM({name=>"format"}, "parameter", "#random:format#"),
                 XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                 XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                 XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                 XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"loc.name"}, "parameter", "#random:country#"),
                 XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header-short"),
                 XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                 XML::Encoder::PARAM("body")];

  my $benchmark5 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 size=>$options{benchmark_size},
                                 description=>"Advertising"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                           method=>"get"},
                                                          "request", @params5)]);

  # 6th case "Asvertising passback"
  my @params6=  [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                 XML::Encoder::PARAM({name=>"tid"}, "parameter", "#random:tid_free#"),
                 XML::Encoder::PARAM({name=>"format"}, "parameter", "#random:format#"),
                 XML::Encoder::PARAM({name=>"pt"}, "parameter", "redir"),
                 XML::Encoder::PARAM({name=>"pb"}, "parameter", PerformanceDB::Tags::PASSBACK_URL),
                 XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                 XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                 XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                 XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"loc.name"}, "parameter", "#random:country#"),
                 XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header"),
                 XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                 XML::Encoder::PARAM("body")];

  my $benchmark6 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 size=>$options{benchmark_size},
                                 description=>"Advertising passback"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                           method=>"get"},
                                                          "request", @params6)]);

  # 7th case "Passback"
  my $benchmark7 = 
      XML::Encoder::GROUP_PARAM({frontend=>"passback",
                                 size=>$options{benchmark_size},
                                 description=>"Passback"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/passback",
                                                           method=>"get"},
                                                          "request", [])]);


  my $xml_config_path = join("/", $config_path, "Benchmark-optin.xml");
  
  open(my $XML, ">$xml_config_path") || die "Cann't open file $xml_config_path for write.\n";
  

  my $benchmarks =  XML::Encoder::GROUP_PARAM({},
                                              "Benchmarks", 
                                              [$benchmark1, $benchmark2, 
                                               $benchmark3, $benchmark4,
                                               $benchmark5, $benchmark6,
                                               $benchmark7]);
  
  # Make XML document with header
  print $XML XML::Encoder::GROUP_PARAM({"xmlns:test"=> XML_CONFIG_HTML_PATH,
                                        "xmlns:xsi" => "http://www.w3.org/2001/XMLSchema-instance",
                                        "xsi:schemaLocation"=>  XML_CONFIG_HTML_PATH . "  " . $options{xsd}},
                                       "Test", [$section1, $benchmarks]);

  close($XML);
}

# optout config
{

  # 1st case "Advertising"
  my @params2=  [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                 XML::Encoder::PARAM({name=>"tid"}, "parameter", "#random:tid#"),
                 XML::Encoder::PARAM({name=>"format"}, "parameter", "#random:format#"),
                 XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                 XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                 XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                 XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"loc.name"}, "parameter", "#random:country#"),
                 XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header"),
                 XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                 XML::Encoder::PARAM("body")];

  my $benchmark2 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 size=>$options{benchmark_size},
                                 description=>"Advertising"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                           method=>"get"},
                                                          "request", @params2)]);

  # 2nd case "Advertising passback"
  my @params3=  [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                 XML::Encoder::PARAM({name=>"tid"}, "parameter", "#random:tid#"),
                 XML::Encoder::PARAM({name=>"format"}, "parameter", "fake"),
                 XML::Encoder::PARAM({name=>"pt"}, "parameter", "redir"),
                 XML::Encoder::PARAM({name=>"pb"}, "parameter", PerformanceDB::Tags::PASSBACK_URL),
                 XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                 XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                 XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                 XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"loc.name"}, "parameter", "#random:country#"),
                 XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                 XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header"),
                 XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                 XML::Encoder::PARAM("body")];

  my $benchmark3 = 
      XML::Encoder::GROUP_PARAM({frontend=>"nslookup",
                                 size=>$options{benchmark_size},
                                 description=>"Advertising passback"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                           method=>"get"},
                                                          "request", @params3)]);

  # 4th case "Passback"
  my $benchmark4 = 
      XML::Encoder::GROUP_PARAM({frontend=>"passback",
                                 size=>$options{benchmark_size},
                                 description=>"Passback"},
                                "Benchmark", 
                                [XML::Encoder::GROUP_PARAM({url=>"/services/passback",
                                                           method=>"get"},
                                                          "request", [])]);


  my $xml_config_path = join("/", $config_path, "Benchmark-optout.xml");
  
  open(my $XML, ">$xml_config_path") || die "Cann't open file $xml_config_path for write.\n";
  

  my $benchmarks =  XML::Encoder::GROUP_PARAM({},
                                "Benchmarks", 
                                [$benchmark1, $benchmark2, 
                                 $benchmark3, $benchmark4]);

  
  # Make XML document with header
  print $XML XML::Encoder::GROUP_PARAM({"xmlns:test"=> XML_CONFIG_HTML_PATH,
                                        "xmlns:xsi" => "http://www.w3.org/2001/XMLSchema-instance",
                                        "xsi:schemaLocation"=>  XML_CONFIG_HTML_PATH . "  " . $options{xsd}},
                                       "Test", [$section1, $benchmarks]);
  
  close($XML);
}


Utils::store_list($urls_file_path, \@{$database->urls(PerformanceDB::Database::REFS_ADVERTISING)});
Utils::store_list($keywords_file_path, \@{$database->keywords(PerformanceDB::Database::REFS_ADVERTISING)});

exit(0);








