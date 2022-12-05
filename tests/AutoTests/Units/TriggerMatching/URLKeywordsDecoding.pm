# URL keywords decoding
# (https://confluence.ocslab.com/display/QA/CDML2+Test+Plan#CDML2TestPlan-Test115%C2%A0URLkeywordsdecoding)

package URLKeywordsDecoding;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use encoding 'utf8';

sub init {
  my ($self, $ns) = @_;

  my @triggers = (
    [qq[\xec\xa0\x9c\xec\xa3\xbc\xed\x98\xb8],
     qq[\xd1\x84\xd1\x8b\xd0\xb2\xd0\xb0\xd0\xbf]],
    ["domainurlkw"],
    ["pathurlkw"],
    ["nameurlkw"],
    ["valueurlkw"],
    ["fragmenturlkw"],
    ["abcurlkw"],
    [qq[\xd0\xb9\xd1\x86\xd1\x83\xd0\xba\xd0\xb5\xd0\xbd] . "12"],
    [qq[\xd0\xba\xd0\xbe\xd0\xbd\xd1\x82\xd0\xb0\xd0\xba\xd1\x82\xd1\x8b] . "12"],
    [qq[\xd0\xb0\xd0\xb1\xd0\xb2] . "12"],
    [qq[\xD0\xB3\xD0\xB4\xD0\xB5] . "34"],
    ["%25D1%2584%25D1%258B%25D0%25B2%25D0%25B0%25D0%25BF"]);

  my $i = 0;

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser'});

  $ns->create(SearchEngine => {
       name => "amfibi.com",
       host => "amfibi.com",
       regexp => "search/([^&]+).*",
       encoding => "utf8",
       decoding_depth => 2 });

  for my $ts (@triggers)
  {
    my $channel = $ns->create(
      DB::BehavioralChannel->blank(
        name => 'Channel' . ++$i,
        account_id => $advertiser,
        url_kwd_list => join ("\n", @$ts),
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => "R") ]));

    $ns->output("Channel/$i" , $channel->url_kwd_key());
    output_channel_triggers($ns, "Trigger/$i", $channel, 'R');
  }

  $ns->output("REF1" , "domainurlkw.com:80");
  $ns->output("REF2" , "http://www.domainurlkw.com/path/pathurlkw?name=v&nameurlkw=valueurlkw#fragmenturlkw;abcurlkw");
  $ns->output("REF3" , "http://www.domainurlkw.com/path/pathurlkw?%a1name=v&nameurlkw=valueurlkw#fragmenturlkw;abcurlkw");
  $ns->output("REF4" , "xn--12-mlcpft3bx.xn--p1ai/%D0%BA%D0%BE%D0%BD%D1%82%D0%B0%D0%BA%D1%82%D1%8B12");
  $ns->output("REF5" , "xn--9y5b2pu13a");
  $ns->output("REF6" , "http://www.domain.com/path?name=%D0%B0%D0%B1%D0%B212");
  $ns->output("REF7" , "http://www.domain.com/path?name=%D0%B0%D0%B1%D0%B212%2C%D0%B3%D0%B4%D0%B534");
  $ns->output("REF8" , "http://amfibi.com/search/%25D1%2584%25D1%258B%25D0%25B2%25D0%25B0%25D0%25BF");
}

1;
