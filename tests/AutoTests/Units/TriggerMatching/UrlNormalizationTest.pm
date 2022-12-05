
package UrlNormalizationTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use encoding 'utf8';

sub init
{
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $basedomain = make_autotest_name($ns, "domain");
  my $domain1 = $basedomain . ".com";
  my $domain2 = $basedomain . "2.com";
  my $domain3 = $basedomain . "3.com";
  my $domain4 = $basedomain . "4.com";
  my $domain5 = $basedomain . "5.com";
  my $domain6 = $basedomain . "6.com";
  my $domain7 = $basedomain . ".fragment.com";
  my $domain8 = $basedomain . ".path.com";
  my $domain9 = $basedomain . ".query.com";
  my $domain10 = make_autotest_name($ns, "DOMain") . ".com";
  my $domain11 = make_autotest_name($ns, "dOMAin") . ".com";
  my $domain12 = make_autotest_name($ns, "domAIN") . ".com";
  my $domain13 = make_autotest_name($ns, "bad") . ".com";

  my $ipv4_address = "195.91.155.99";

  # Url with russian 'A' + 'B' letters
  my $russian_url1 = $domain3 . "/path1?" . qq[\xd0\x90] . "+" . qq[\xd0\x91] . "=";
  # Url with russian 'a' + 'b' letters
  my $russian_url2   =  $domain3 . "/path1?" . qq[\xd0\xb0] . "+" . qq[\xd0\xb1] . "=";
  # Url with russian 'A' + 'b' letters
  my $russian_url2_1 =  $domain3 . "/path1?" . qq[\xd0\x90] . "+" . qq[\xd0\xb1] . "=";
  # Url with russian 'anchor' word
  my $russian_url3 = $domain7 . "/path?p2=a#" . 
    qq[\xd1\x8f\xd0\xba\xd0\xbe\xd1\x80\xd1\x8c];
  # Url with russian 'main page' phrase
  my $russian_url4 = $domain8 . "/wiki/" .
    qq[\xd0\x97\xd0\xb0\xd0\xb3\xd0\xbb\xd0\xb0\xd0\xb2\xd0\xbd\xd0\xb0\xd1\x8f_\xd1\x81\xd1\x82\xd1\x80\xd0\xb0\xd0\xbd\xd0\xb8\xd1\x86\xd0\xb0];

  # Urls with korean 'image' word
  my $korean_url1 = $domain5 . "/path?" .
    qq[\xec\x98\x81\xec\x83\x81];
  my $korean_url2 = $domain9 . "/wiki?" .
    qq[\xec\x98\x81\xec\x83\x81];

  #requests
  $ns->output("REF1", "www." . $domain1 . "/path/path2?query=1#10");
  $ns->output("REF2", $domain2);
  $ns->output("REF3", "http://". $domain3 . "/PATH?2+2=");
  $ns->output("REF4", $russian_url1);
  $ns->output("REF5", $russian_url2_1);
  $ns->output("REF6", $domain2 .":8080/path");
  $ns->output("REF7", $domain2 . ":80/path");
  $ns->output("REF8", $domain2 . ":80/Path");
  $ns->output("REF9", $domain4 . "/path");
  $ns->output("REF10", $domain4 . ":80/path");
  $ns->output("REF11", $domain6 . "/?/");
  $ns->output("REF12", $domain6 . "?1/2");
  $ns->output("REF13", $russian_url4);
  $ns->output("REF14", $russian_url3);
  $ns->output("REF15", $domain7 . "/noticiaInt~id~23196~n~tendencias+maquiagem+outono+inverno+2012");
  $ns->output("REF16", "http://" . $domain9 . "/index.shtml?1=http://www.mapinter.net/eng");
  $ns->output("REF17", $domain9 . "/oiinternet/staticContent.do?path=/html/torpedo-cobrar");
  $ns->output("REF18", $ipv4_address);
  $ns->output("REF19", "http://-" . $domain13);

  my @urls = (
  # channel#1
  {
    $domain1 => 1,
    "http://www.$domain10/PATH" => 1,
    "http://www.$domain11/path/path2" => 1,
    "http://www.$domain12/path/path2?Query=1#2" => 1,
    "$domain2/path" => 2,
    "$domain3/path?2+2=" => 1,
    "$russian_url1" => 1,
    "$russian_url2" => 0,
    "$domain4:80" => 2,
    "$domain4:80/path" => 2,
    "http://$domain6?" => 2,
    "$korean_url1" => 0,
    "$korean_url2" => 0,
    "$domain7/path?p1=a#%D1%8F%D0%BA%D0%BE%D1%80%D1%8C" => 0,
    "$russian_url3" => 1,
    "$russian_url4" => 1,
    "$domain7/noticiaInt~id~23196~n~tendencias+maquiagem+outono+inverno+2012" => 1,
    "http://$domain9/index.shtml?1=http://www.mapinter.net/eng" => 1,
    "$domain9/oiinternet/staticContent.do?path=/html/torpedo-cobrar" => 1,
    "$ipv4_address" => 1,
    "http://-$domain13" => 0,
    "http://--$domain13" => 0 
  },
  # channel#2
  {
    "http://$domain4/path" => 2,
    "http://www.$domain10/PATH" => 1
  },
  # channel#3
  {
    "http://www.$domain11/path/path2" => 1,
    "$domain4:80/path" => 2
  },
  # channel#4
  {
    "http://www.$domain12/path/path2?Query=1#2" => 1
  });

  my @channels;
  $ns->output("ChannelList", 1000, "list of channels used in the test");
  for (my $i = 0; $i < scalar @urls; ++$i)
  {
    $channels[$i] = $ns->create(DB::BehavioralChannel->blank(
      name => $i,
      url_list => join("\n", keys %{$urls[$i]}),
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U')] ));

    $ns->output("Channel".($i + 1), $channels[$i]->url_key());
  }
  $ns->output("ChannelListEnd", 1000);

  $ns->output("ChannelTriggerList", 1000, "list of channel_triggers_id");
  for (my $i = 0; $i < scalar @channels; ++$i)
  {
    foreach my $trigger (@{$channels[$i]->url_channel_triggers()})
    {
      my $original_trigger = $trigger->original_trigger();
      my $encoded_trigger = $original_trigger;
      Encode::_utf8_on($encoded_trigger);
      $ns->output(
        $original_trigger, $trigger->channel_trigger_id(), 
           $urls[$i]->{$original_trigger}? $urls[$i]->{$original_trigger}: $urls[$i]->{$encoded_trigger});
    }
  }
  $ns->output("ChannelTriggerListEnd", 1000);

}

1;
