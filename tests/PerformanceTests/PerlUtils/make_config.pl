#!/usr/bin/perl

use warnings;
use strict;

=head1 NAME

make_config.pl - scipt for making performance test configuration file

=head1 SYNOPSIS

  make_config.pl OPTIONS <XMLFilePath>

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

=item C<--confluence_report_path confluence report path>

S<confluence report path>

=item C<--prefix prefix>

S<test entities prefix>

=item C<--campaigns_count>

S<total campaigns count, not used if campaign_config_file set>

=item C<--initial_requests_count>

S<Count of requests sending to server at 1st second>

=item C<--requests_increment>

S<Requests increment, request's count difference between current and next seconds>

S<total campaigns count, not used if campaign_config_file set>

=item C<--channels_count>

S<total channels count, not used if channel_config_file set>

=item C<--campaign_channels_count>

S<count of channels linked to the one campaign, must be less then channels_count>

=item C<--campaign_config_file>

S<config file path, file contain information for creating campaigns>

=item C<--channel_config_file>

S<config file path, file contain information for creating channels>

=item C<--empty_tids_percentage>

S<percentage of request with empty tid>

=item C<--referer_func>

S<referer selector function>

=item C<--referer_size>

S<referer size for selector function>

=item C<--click_rate>

S<click requests percentage>

=item C<--action_rate>

S<action request percentage>

=item C<--passback_rate>

S<click passback percentage>

=item C<--optout_rate>

S<user with no cookies (opted out user) percentage>

=item C<--ad_all_optouts>

S<Send requests with tid for all optouts client>

=item C<--have_ron_campaign>

S<Create one RON campaign>

=item C<--create_free_tags>

S<Create free tags, which haven't link to any campaign>

=item C<--ft_size>

S<Set max full text size for 'ft' test>

=item C<--ft_channels_percentage>

S<Set percentage of channel's phrase in one full text>

=item C<--http_method>

S<HTTP method GET or POST>

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

# constants
use constant DEFAULT_ENTITY_SIZE  => 2;
use constant XML_CONFIG_HTML_PATH   => "http://www.peopleonpage.com/xsd/tests/PerformanceTests";
use constant DEFAULT_EXCLISION_TIME => 1200;
use constant DEFAULT_THREAD_NUMBER  => 30;
use constant DEFAULT_SERVER_START_TIME  => 1;
use constant DEFAULT_STAT_INTERVAL_DUMP => 15;
use constant DEFAULT_MAX_EXEC_DELAY  => 1000;

use constant URLS_FILE_NAME       => "urls";
use constant KWS_FILE_NAME        => "keywords";
use constant URLFILTERS_FILE_NAME => "urlfilters";
use constant FT_FILES_PATH        => "FT";
use constant DEFAULT_HTTP_PORT    => 80;

use constant DEFAULT_CAMPAIGNS_COUNT          => 10;
use constant DEFAULT_CHANNELS_COUNT           => 10;
use constant DEFAULT_CAMPAIGN_CHANNELS_COUNT  => 2;
use constant DEFAULT_INITIAL_REQUESTS_COUNT   => 100;
use constant DEFAULT_REQUESTS_INCREMENT       => 10;

use constant DEFAULT_FT_COUNT                 => 10; # How many ft parameters should created

my %options = (prefix                  => '',
               campaigns_count         => DEFAULT_CAMPAIGNS_COUNT,
               channels_count          => DEFAULT_CHANNELS_COUNT,
               campaign_channels_count => DEFAULT_CAMPAIGN_CHANNELS_COUNT,
               initial_requests_count  => DEFAULT_INITIAL_REQUESTS_COUNT,
               requests_increment      => DEFAULT_REQUESTS_INCREMENT,
               empty_tids_percentage   => 0,
               click_rate              => 0,
               action_rate             => 0,
               passback_rate           => 0,
               optout_rate             => 10,
               ad_all_optouts          => 0,
               ft_channels_percentage  => 0,
               have_ron_campaign       => 0,
               create_free_tags        => 1,
               http_method             => "get");

if (! GetOptions(\%options, qw(connection-string=s host|h=s dbname|d=s user|u=s
        password|p=s server|s=s@ xsd|x=s prefix=s confluence_report_path=s
        campaigns_count=i channels_count=i campaign_channels_count=s
        initial_requests_count=i requests_increment=i
        campaign_config_file=s channel_config_file=s
        empty_tids_percentage=i referer_func=s
        referer_size=i click_rate=f action_rate=f passback_rate=f
        optout_rate=i ad_all_optouts|1 have_ron_campaign|1 create_free_tags|1
        ft_size=i ft_channels_percentage=i http_method=s)))
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
  print "XMLFilePath not defined!\n";
  pod2usage(1);
}

my $xml_config_path = $ARGV[0];

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

my @ft_phrases = ();
my $ft_path = undef;
if (defined $options{ft_size})
{
  $ft_path = get_abs_path($xml_config_path, FT_FILES_PATH);
  if (! -d $ft_path)
  {
    File::Path::mkpath($ft_path) || die "Cann't create directory $ft_path";
  }
  @ft_phrases = FTGenerator::create_ft($ft_path, DEFAULT_FT_COUNT, $options{ft_size});
}


# Database
my $namespace_name = "PT";

my $test_name      = $options{prefix};

my $database = new PerformanceDB::Database($options{host},
                                           $options{dbname},
                                           $options{user},
                                           $options{password},
                                           $namespace_name,
                                           $test_name);


#   Database entities generation

#    Channels
if (defined $options{channel_config_file})
{
  foreach my $channel (@channels_info)
  {
    my ($time_from, $time_to, $minimum_visits, $count, $channel_type) = @$channel;
    $database->create_channels($count, $time_from, $time_to, $minimum_visits, $channel_type, \@queries);
  }
}
else
{
  $database->create_channels($options{channels_count}, 0, 0);
}

if (defined $options{ft_size} and
    $options{ft_channels_percentage} ne 0)
{
  $database->create_ft_channels(\@ft_phrases, $options{ft_channels_percentage});
}

#    Campaigns
my $total_count = 0;
if (defined $options{campaign_config_file})
{
  foreach my $campaign_flag (keys %campaign_flags)
  {
    my $count = $campaign_flags{$campaign_flag};
    $total_count += $count;
    if ($count)
    {
      $database->create_campaigns($count, $options{campaign_channels_count}, $campaign_flag);
    }
  }
}
else
{
  $total_count = $options{campaigns_count};
  $database->create_campaigns($options{campaigns_count},
                              $options{campaign_channels_count}, 0);
}

# RON campaign must have 10% of all tags
if ($options{have_ron_campaign}) {

  my $ron_tags_count = int(0.25 * $total_count);
  $database->create_ron_campaign($ron_tags_count);
  $total_count += $ron_tags_count;
}

# Free tags count = RON tags count + AD tags count
if ($options{create_free_tags})
{
  $database->create_free_tags($total_count);
}

$database->commit;

my @servers = map (make_server_url($_), @{$options{server}});

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

my @countries = ['GN'];

my @formats  = ['unit-test'];


my @ct_pswnd_cookies = ['ct.pswnd.A=1;'];

my @sizenotsup  = ['popunder'];

my @colo_ids = [1,2];

my @bool = [0,1];


my $urls_file_path = get_abs_path($xml_config_path, URLS_FILE_NAME . $options{prefix});
my $keywords_file_path = get_abs_path($xml_config_path, KWS_FILE_NAME . $options{prefix});

# Global parameters
my %global_attributes = ();
if (defined $options{confluence_report_path} ) {
  print $options{confluence_report_path};
  %global_attributes = (confluenceReportPath => $options{confluence_report_path});
}

my $description = XML::Encoder::PARAM("description", $options{prefix});

my $section1 =
    XML::Encoder::GROUP_PARAM(\%global_attributes,
                              "global", [XML::Encoder::PARAM("executionTime",   DEFAULT_EXCLISION_TIME),
                                         XML::Encoder::PARAM("threadsNumber",   DEFAULT_THREAD_NUMBER),
                                         XML::Encoder::PARAM("serverStartTime", DEFAULT_SERVER_START_TIME),
                                         XML::Encoder::PARAM("maxTaskExecutionDelay", DEFAULT_MAX_EXEC_DELAY),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM("urlsListFile", $urls_file_path),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM("RefererKWsListFile", $keywords_file_path),
                                         defined $ft_path?
                                         XML::Encoder::PARAM("ftPath", $ft_path):XML::Encoder::NEW_LINE,
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST("serverBase", "url", \@servers),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"version"}, "set", "item", @versions),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"browser"}, "set", "item", @browsers),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"user_agent"}, "set", "item", @useragents),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"os"}, "set", "item", @OS),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"tid"}, "set", "item",
                                                                  \@{$database->tids()}),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"country"}, "set", "item", @countries),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"format"}, "set", "item", @formats),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"ct_pswnd_cookie"}, "set", "item",
                                                                    @ct_pswnd_cookies),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"sizenotsup"}, "set", "item",
                                                                    @sizenotsup),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"colo_id"}, "set", "item",
                                                                  @colo_ids),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"queries"}, "set", "item", \@queries),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM_LIST({name=>"bool"}, "set", "item", @bool),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::GROUP_PARAM("requestConstraints",
                                                                   [XML::Encoder::GROUP_PARAM({name=>"default",
                                                                                               samplingSize=>3000},
                                                                                              "requestConstraint",
                                                                                              [XML::Encoder::PARAM("timeout",
                                                                                                                     2),
                                                                                               XML::Encoder::PARAM("intendedTime",
                                                                                                                   1),
                                                                                               XML::Encoder::PARAM("failedPercentage",
                                                                                                                   1),
                                                                                               XML::Encoder::PARAM("prolongedPercentage",
                                                                                                                   10)

                                                                                               ])]),
                                         XML::Encoder::NEW_LINE,
                                         XML::Encoder::PARAM({statisticsIntervalDump=>DEFAULT_STAT_INTERVAL_DUMP},
                                                             "requestAggregatedStatistics"),
                                         XML::Encoder::NEW_LINE]);

# Request parameters
my $use_referer_header = 0;
if (defined $options{referer_size})
{
  $use_referer_header = $options{referer_size} > 1;
}
my @ns_request_params = [XML::Encoder::PARAM({name=>"app"}, "parameter", "PS"),
                         XML::Encoder::PARAM({name=>"tid", empty_prc=>$options{empty_tids_percentage}}, "parameter", "#random:tid#"),
                         XML::Encoder::PARAM({name=>"referer-kw"}, "parameter", encode_ref("referer-kw")),
                         defined $ft_path?XML::Encoder::PARAM({name=>"ft"}, "parameter", encode_ref("ft")):"",
                         XML::Encoder::PARAM({name=>"format"}, "parameter", "#random:format#"),
                         XML::Encoder::PARAM({name=>"v"}, "parameter", "#random:version#"),
                         XML::Encoder::PARAM({name=>"b"}, "parameter", "#random:browser#"),
                         XML::Encoder::PARAM({name=>"o"}, "parameter", "#random:os#"),
                         XML::Encoder::PARAM({name=>"prck"}, "parameter", 0),
                         XML::Encoder::PARAM({name=>"country"}, "parameter", "#random:country#"),
                         XML::Encoder::PARAM({name=>"glbfcap"}, "parameter", 0),
                         XML::Encoder::PARAM({name=>"setuid"}, "parameter", 1),
                         XML::Encoder::PARAM({name=>"require-debug-info"}, "parameter", "header"),
                         $use_referer_header? XML::Encoder::PARAM({name=>"Referer"}, "header", "#referer#"):"",
                         XML::Encoder::PARAM({name=>"User-Agent"}, "header", "#random:user_agent#"),
                         XML::Encoder::PARAM("body")];


my @requests;



  push (@requests, XML::Encoder::GROUP_PARAM({period=>1,
                                              statisticsIntervalDump=>DEFAULT_STAT_INTERVAL_DUMP},
                                             "NSLookup",
                                             [XML::Encoder::PARAM({ref=>"default"},
                                                                  "requestConstraint"),
                                              XML::Encoder::GROUP_PARAM({url=>"/services/nslookup",
                                                                         method=>$options{http_method}},
                                                                        "request", @ns_request_params)]));

  push (@requests, XML::Encoder::GROUP_PARAM({statisticsIntervalDump=>DEFAULT_STAT_INTERVAL_DUMP},
                                             "Click",
                                             [XML::Encoder::PARAM({ref=>"default"},
                                                                  "requestConstraint")]));

  push (@requests, XML::Encoder::GROUP_PARAM({statisticsIntervalDump=>DEFAULT_STAT_INTERVAL_DUMP},
                                             "ActionTracking",
                                             [XML::Encoder::PARAM({ref=>"default"},
                                                                  "requestConstraint")]));
  push (@requests, XML::Encoder::GROUP_PARAM({statisticsIntervalDump=>DEFAULT_STAT_INTERVAL_DUMP},
                                             "Passback",
                                             [XML::Encoder::PARAM({ref=>"default"},
                                                                  "requestConstraint")]));

 my $section2 =
        XML::Encoder::GROUP_PARAM({count=>$options{initial_requests_count},
                                   lifetime=>2000,
                                   optout_rate=>$options{optout_rate},
                                   ad_all_optouts=>XML::Encoder::convert_to_bool($options{ad_all_optouts}),
                                   click_rate=>$options{click_rate},
                                   action_rate=>$options{action_rate},
                                   passback_rate=>$options{passback_rate},
                                   incr_count_step=>$options{requests_increment},
                                   step_interval=>1}, "adClient", \@requests);



open(my $XML, ">$xml_config_path") || die "Cann't open file $xml_config_path for write.\n";


#Make XML document with header
print $XML XML::Encoder::GROUP_PARAM({"xmlns:test"=> XML_CONFIG_HTML_PATH,
                                      "xmlns:xsi" => "http://www.w3.org/2001/XMLSchema-instance",
                                      "xsi:schemaLocation"=>  XML_CONFIG_HTML_PATH . "  " . $options{xsd}},
                                     "testParams", [$description, $section1, $section2]);

close($XML);

store_list($urls_file_path, \@{$database->urls()});
store_list($keywords_file_path, \@{$database->keywords()});

exit(0);

# utils
sub encode_ref
{
  my $param_name = shift;
  my $buf = "";
  if ( defined $options{referer_func} )
  {
    $buf= "#" . $options{referer_func} . ":" . $param_name;
    if (defined $options{referer_size})
    {
      my $ref_size = $options{referer_size} > 1?
          $options{referer_size} - 1: $options{referer_size};
      $buf.= "=" . $ref_size;
    }
    $buf .= "#";
  }
  return $buf;
}

sub make_server_url {
  my $server = shift;
  if (!($server =~ /([\w\W]*?):([\w\W]*)/o))
  {
    $server .= ":" . DEFAULT_HTTP_PORT;
  }
  return "http://$server";
}

sub get_abs_path {
  my $config_path = shift;
  my $filename    = shift;
  if (!File::Spec->file_name_is_absolute($config_path))
  {
    my $cwd         = getcwd;
    $config_path    = File::Spec->rel2abs($config_path, $cwd);
  }
  my ($name, $dir, $ext) = fileparse($config_path);
  return join("", ($dir, $filename));
}

sub store_list {
  my ($path, $list) = @_;
  open(my $FILE, ">$path") || die "Cann't open file $path for write.\n";
  for my $elem (@$list)
  {
    print $FILE "$elem\n";
  }
  close($FILE);
}
