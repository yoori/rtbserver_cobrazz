
package ExactUrlMatching;

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

  my $basedomain = make_autotest_name($ns, "domain") . ".com";
  my $domain = $basedomain . ".com";
  my $domain_1 = $basedomain . ".path.com";
  my $domain_2 = $basedomain . ".fragment.com";

  # Url with russian 'E' & 'B' letters
  my $russian_url1 = $domain . "/Path?" . qq[\xd0\x98] . "+" . qq[\xd0\x91] . "=";
  # Url with russian 'e' & 'b' letters
  my $russian_url1_1 =  $domain . "/Path?" . qq[\xd0\xb8] . "+" . qq[\xd0\xb1] . "=";
  # Url with russian 'EB' phrase
  my $russian_url2   =  $domain . "/Path?" . qq[\xd0\x98\xd0\x91];
  # Url with russian 'main page' phrase
  my $russian_url3 = $domain_2 . "/wiki/" .
      qq[\xd0\x97\xd0\xb0\xd0\xb3\xd0\xbb\xd0\xb0\xd0\xb2\xd0\xbd\xd0\xb0\xd1\x8f_\xd1\x81\xd1\x82\xd1\x80\xd0\xb0\xd0\xbd\xd0\xb8\xd1\x86\xd0\xb0];
  # Url with russian 'anchor' word
  my $russian_url4 = $domain_1 . "/path?p2=a#" . 
      qq[\xd1\x8f\xd0\xba\xd0\xbe\xd1\x80\xd1\x8c];

  # Urls with korean 'image' word
  my $korean_url =  $domain . "/Path?" .
      qq[\xec\x98\x81\xec\x83\x81];
  
  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      url_list =>  
        "\"" . $domain . "\"\n" . 
        "\"a." . uc($domain) . "\"\n" . 
        "\"" . uc($domain) . "/path\"\n" .
        "\"a." . uc($domain) . "/PATH\"\n" .
        "\"" . $domain . "/path/page\"\n" .
        "\"a." . $domain . "/path/page\"\n" .
        "\"" . $russian_url1 . "\"\n" .
        "\"" . $russian_url2 . "\"\n" .
        "\"a." . $domain . "/path?query=1#2\"\n" .
        "\"b." . $domain . ":80\"\n" .
        "\"b." . $domain . ":80/path\"\n" .
        "\"" . $korean_url . "\"\n" .
        "\"" . $domain_2 . "/path?p1=a#%D1%8F%D0%BA%D0%BE%D1%80%D1%8C\"\n" .
        "\"" . $russian_url3 . "\"\n" .
        "\"" . $russian_url4 . "\"\n" .
        "\"http://" . $domain . 
        "/oiinternet/staticContent.do?path=/html/torpedo-cobrar\"", 
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'U')] ));

  $ns->output("Channel", $channel->url_key());

  $ns->output("REF1", "a." . $domain);
  $ns->output("REF2",  lc($domain));
  $ns->output("REF3", uc($domain) . "/Path");
  $ns->output("REF4", "a." . $domain . "/path?query=1#10");
  $ns->output("REF5", $domain . "/path/page");
  $ns->output("REF6", "a." . $domain . "/path/page");
  $ns->output("REF7", $russian_url1);
  $ns->output("REF8", $russian_url1_1);
  $ns->output("REF9", "www." . $domain);
  $ns->output("REF10", "a." . $domain . ":80");
  $ns->output("REF11", "a." . $domain . ":8080");
  $ns->output("REF12", "b." . $domain . ":80/path");
  $ns->output("REF13", "b." . $domain . "/path");
  $ns->output("REF14", $russian_url3);
  $ns->output("REF15", $russian_url4);
  $ns->output("REF16", $domain . "/oiinternet/staticContent.do?path=/html/torpedo-cobrar");
}

1;
