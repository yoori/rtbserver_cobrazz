package UserPropertiesTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub add_user_agents
{
  my ($self, $ns, $agents) = @_;

  my $idx = 0;

  for my $args (@$agents)
  {
    my ($uagent, $os, $browser, $base64) = @$args;
    ++$idx;
    if ($base64)
    {
      $ns->output_base64("UserAgent/" . $idx, $uagent);
    }
    else
    {
      $ns->output("UserAgent/" . $idx, $uagent);
    }
    $ns->output("OsVersion/" . $idx, $os) if $os;
    $ns->output("BrowserVersion/" . $idx, $browser) if $browser;
  }
}

sub generate_some_value
{
  my $buffer = "";

  for (my $i = 0; $i < $_[0]; $i++)
  {
    $buffer .= chr(ord('0') + $i % 10);
  }

  return $buffer;
}

sub create_colos
{
  my ($self, $ns) = @_;

  my %colocations = (
     'OS' => 'ALL',
     'COUNTRY' => 'ALL',
     'VERSION' => 'ALL',
     'CLIENT_PROP' => 'ALL',
     'STATUS' => 'ALL',
     'INACTIVE' => 'ALL',
     'PROBE' => 'ALL',
     'UPVALUE' => 'ALL',
     'NONE' => 'NONE'  );

  while (my ($name, $oo_serving) = each %colocations)
  {
    my $colo = $ns->create(Isp => {
      name => $name,
      colocation_optout_serving => $oo_serving,
      account_internal_account_id =>
        DB::Defaults::instance()->no_margin_internal_account->{account_id} });

    $ns->output(
      'COLO/' . $name, $colo->{colo_id});
  }
}

sub init
{
  my ($self, $ns) = @_;


  my $publisher = 
   $ns->create(Publisher => {
     name => "Publisher" });

   my $tag_deleted = $ns->create(PricedTag => {
    name => "Tag-Deleted",
    site_id => $publisher->{site_id},
    status => 'D' });

  my $keyword1 = make_autotest_name($ns, 'KWD1');
  my $url1 = "http://" . lc(make_autotest_name($ns, "url1")) . ".com";
  my $keyword2 = make_autotest_name($ns, 'KWD2');
  my $url2 = "http://" . lc(make_autotest_name($ns, "url2")) . ".com";

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => "Display-1",
    campaigncreativegroup_country_code => 
      DB::Defaults::instance()->test_country_1->{country_code},
    campaigncreativegroup_cpa => 1,
    behavioralchannel_keyword_list => $keyword1,
    site_links => 
      [{ site_id => $publisher->{site_id} }] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      time_to => 3*60*60,
      channel_id => $campaign1->{channel_id} ));

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => "Display-2",
    campaigncreativegroup_country_code => 
      DB::Defaults::instance()->test_country_2->{country_code},
    campaigncreativegroup_cpa => 1,
    campaigncreativegroup_ar => 0.01,
    behavioralchannel_keyword_list => $keyword1,
    behavioralchannel_url_list => $url1,
    site_links => 
      [{ site_id => $publisher->{site_id} }] });
  
  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      time_to => 3*60*60,
      channel_id => $campaign2->{channel_id} ));

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "U",
      time_to => 3*60*60,
      channel_id => $campaign2->{channel_id} ));

  my $campaign3 = $ns->create(DisplayCampaign => {
    name => "Display-3",
    campaigncreativegroup_country_code => 
      DB::Defaults::instance()->test_country_2->{country_code},
    campaigncreativegroup_cpm => 10,
    behavioralchannel_keyword_list => $keyword2,
    behavioralchannel_url_list => $url2,
    site_links => 
      [{ site_id => $publisher->{site_id} }] });
  
  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      time_to => 3*60*60,
      channel_id => $campaign3->{channel_id} ));

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "U",
      time_to => 3*60*60,
      channel_id => $campaign3->{channel_id} ));


  my $stmt = $ns->pq_dbh->prepare("SELECT Max(tag_id) From Tags");
  $stmt->execute();
  my @result = $stmt->fetchrow_array;
  $stmt->finish();

  $ns->output("TID", $publisher->{tag_id});
  $ns->output("TIDDELETED", $tag_deleted);
  $ns->output("TIDABSENT", $result[0] + 10000);
  $ns->output("KWD1", $keyword1);
  $ns->output("KWD2", $keyword2);
  $ns->output("URL1", $url1);
  $ns->output("URL2", $url2);
  $ns->output("CC/01", $campaign1->{cc_id});
  $ns->output("CC/02", $campaign2->{cc_id});
  $ns->output("CC/03", $campaign3->{cc_id});

  my $valid_browser_suffix = generate_some_value(1024 - length('Firefox/'));
  my $valid_os_suffix = generate_some_value(1024 - length('Windows NT '));
  my $up_some_value = substr(generate_some_value(1026), 1, 1025);
  my $unicode_1 = Encode::decode('utf8', 'ADSCUserPropertyOS ' . qq[\xE2\x82\xAC] . ' ');
  my $exp_1 = 'adscuserpropertyos %E2%82%AC ';
  my $ext_1 = generate_some_value(1024 - length($exp_1));
  my $broken_unicode_1 = 'ADSCUserPropertyOS 1 %D0%D1 ';
  my $broken_ext_1 = generate_some_value(100 - length($broken_unicode_1));
  my $broken_exp_1 = 'adscuserpropertyos 1 %25d0%25d1 ';
  my $broken_unicode_2 = "ADSCUserPropertyOS 2 \xD0\xD1 ";
  my $broken_exp_2 = 'adscuserpropertyos 2 %D0%D1 ';
  my $broken_ext_2 = generate_some_value(100 - length($broken_unicode_2));


  #List of OS's:
  my @user_agents = (
    #1
    [ "Mozilla/5.0 (X11; U; Linux amd64; rv:5.0) Gecko/20100101 Firefox/5.0 (Debian)",
      "linux",
      "Firefox 5.0" ],
    #2
    [ "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; " .
      ".NET CLR 1.1.4322; .NET CLR 2.0.50727; InfoPath.1)",
      "windows nt 5.1",
      "IE 6.0" ],
    #3
    [ "Mozilla/5.0 (Windows; U; Windows NT 5.1; SV1; " .
      ".NET CLR 1.1.4322; .NET CLR 2.0.50727; InfoPath.1) Gecko/20071008",
      "windows nt 5.1",
      "Mozilla 5.0" ],
    #4
    [ "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; ru; rv:1.8.1.8)",
      "windows nt 5.1",
      "IE 6.0" ],
    #5
    [ "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) " .
      "AppleWebKit/534.55.3 (KHTML, like Gecko) " .
      "Version/5.1.3 Safari/534.53.10",
      "mac os x 10.7.3",
      "Safari 5.1.3" ],
    #6
    [ "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.6 " . 
      "(KHTML, like Gecko) Chrome/20.0.1092.0 Safari/536.6",
      "windows nt 6.1",
      "Chrome 20.0.1092.0" ],
    #7
    [ "Mozilla/5.0 (Windows NT $up_some_value; rv:12.0) " .
      "Gecko/20100101 Firefox/14.0",
      substr("windows nt $up_some_value", 0, 1024),
      "Firefox 14.0" ],
    #8
    [ "Mozilla/5.0 (Windows NT $valid_os_suffix; rv:12.0) " .
      "Gecko/20100101 Firefox/14.0",
      "windows nt $valid_os_suffix",
      "Firefox 14.0" ],
    #9
    [ "Mozilla/5.0 (Windows NT 6.0; rv:12.0) Gecko/20100101 " .
      "Firefox/$up_some_value",
      "windows nt 6.0",
      substr("Firefox $up_some_value", 0, 1024) ],
    #10
    [ "Mozilla/5.0 (Windows NT 6.0; rv:12.0) Gecko/20100101 " .
      "Firefox/$valid_browser_suffix",
      "windows nt 6.0",
      "Firefox $valid_browser_suffix" ],
    #11
    [ $unicode_1 . $ext_1,
      $exp_1 . $ext_1,
      undef ],
    #12
    [ $broken_unicode_1 . $broken_ext_1,
      $broken_exp_1 . $broken_ext_1,
      undef, 1 ],
    #13
    [ $broken_unicode_2 . $broken_ext_2,
      $broken_exp_2 . $broken_ext_2, 
      undef, 1 ],
 );

  $self->add_user_agents($ns, \@user_agents);
  $ns->output("OsVersion/Null", "");
  $ns->output("BrowserVersion/Null", "");

  #List of countries:
  $ns->output("CountryCode/01", lc(DB::Defaults::instance()->test_country_1->{country_code}));
  $ns->output("CountryCode/02", lc(DB::Defaults::instance()->test_country_2->{country_code}));
  $ns->output("CountryCode/03", "");
  $ns->output("CountryCode/COLO", lc(DB::Defaults::instance()->country()->{country_code}));

  #List of ClientVersion's:
  $ns->output("ClientVersion/01", "1.3.0-3.ssv1");
  $ns->output("ClientVersion/02", "1.3.0-4.ssv1");

  #List of Client's:
  $ns->output("Client/01", "PS");
  $ns->output("Client/02", "CP");

  $self->create_colos($ns);

  $ns->create(DB::DeviceChannel::Platform->blank(
    name => 'ADSCUserPropertyOS',
    type => 'OS',
    match_marker => 'ADSCUserPropertyOS',
    output_regexp => 'ADSCUserPropertyOS (.*)'));
}

1;
