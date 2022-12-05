
package PrimaryChannelsMatchingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $keyword1_1 = make_autotest_name($ns, "exchange");
  my $keyword1_2 = make_autotest_name($ns, "rate");
  my $keyword1_3 = make_autotest_name($ns, "stocks");
  my $keyword1_4 = make_autotest_name($ns, "price");
  my $keyword1_5 = make_autotest_name($ns, "OpTion");
  my $keyword1_6 = make_autotest_name($ns, "cdml2search");

  my $keyword2_1 = make_autotest_name($ns, "longest1");
  my $keyword2_2 = make_autotest_name($ns, "matched1");
  my $keyword2_3 = make_autotest_name($ns, "trigger1");
  my $keyword2_4 = make_autotest_name($ns, "most1");
  my $keyword2_5 = make_autotest_name($ns, "expensive1");
  my $keyword2_6 = make_autotest_name($ns, "match1");

  my $keyword3_1 = make_autotest_name($ns, "home");
  my $keyword3_2 = make_autotest_name($ns, "pets");
  my $keyword3_3 = make_autotest_name($ns, "medicaments");
  my $keyword3_4 = make_autotest_name($ns, "dog");
  my $keyword3_5 = make_autotest_name($ns, "food");
  my $keyword3_6 = make_autotest_name($ns, "cat");

  my $keyword3_7 = "fulltextmatching";
  my $keyword3_8 = "fulltextmatching1";
  my $keyword3_9 = "fulltextmatching2";
  my $keyword3_10 = "fulltextmatching3";
  my $keyword3_11 = "fulltextmatching4";

  my $keyword4_1 = make_autotest_name($ns, "Kwd1");
  my $keyword4_2 = make_autotest_name($ns, "Kwd2");
  my $keyword4_3 = make_autotest_name($ns, "Kwd3");
  my $keyword4_4 = make_autotest_name($ns, "Kwd4");
  my $keyword4_5 = make_autotest_name($ns, "Kwd5");
  my $keyword4_6 = make_autotest_name($ns, "Kwd6");

  my $keyword5_1 = "url2";
  my $keyword5_2 = "keywords3";

  my $trigger_list1 = "\"" . $keyword1_1 . " " . $keyword1_2 . "\" " . $keyword1_3 . "\n" .
    $keyword1_3 . " " . $keyword1_4  . "\n" .
    $keyword1_3 . " " . $keyword1_5  . "\n" .
    $keyword1_6;

  my $trigger_list2 = "\"" . $keyword2_1 . " " . $keyword2_2 . "\"\n" .
    "\"" . $keyword2_1 . " " . $keyword2_2  . " " . $keyword2_3 . "\"\n" .
    "\"" . $keyword2_4 . " " . $keyword2_5  . " " . $keyword2_3  . " " . $keyword2_6 . "\"";

  my $trigger_list3 = "\"" . $keyword3_1 . " " . $keyword3_2 . "\" " . $keyword3_3 . "\n" .
    $keyword3_4 . " " . $keyword3_5 . "\n" .
    "\"" . $keyword3_1 . " " . $keyword3_6  . " " . $keyword3_5 . "\"\n" .
    $keyword3_7 . "\n" . $keyword3_8 . " " . $keyword3_9 . "\n" .
    "\"" . $keyword3_10 . " " . $keyword3_11 ."\"";

  my $trigger_list4_1 = "$keyword4_1\n$keyword4_2\n$keyword4_4 $keyword4_5";
  my $trigger_list4_2 = "$keyword4_1\n$keyword4_3\n$keyword4_5 $keyword4_4";
  my $trigger_list4_3 = "$keyword4_1\n$keyword4_6\n$keyword4_5 $keyword4_4";

  my $trigger_list5 = "$keyword5_1 $keyword5_2";

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $page = $ns->create(DB::BehavioralChannel->blank(
    name => "Page",
    account_id => $account,
    keyword_list => $trigger_list2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        time_to => 60 ) ]));

  my $search = $ns->create(DB::BehavioralChannel->blank(
    name => "Search",
    account_id => $account,
    search_list => $trigger_list1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S',
        time_to => 60 ) ]));

  my $ordinary = $ns->create(DB::BehavioralChannel->blank(
    name => "Ordinary",
    account_id => $account,
    keyword_list => $trigger_list3,
    url_kwd_list => $trigger_list3,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S',
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'R',
        time_to => 60 )]));

  my $splitpage = $ns->create(DB::BehavioralChannel->blank(
    name => "SplitPage",
    account_id => $account,
    keyword_list => $trigger_list4_1,
    search_list => $trigger_list4_2,
    url_kwd_list => $trigger_list4_3,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S',
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'R',
        time_to => 60 )]));

  my $url_kwd = $ns->create(DB::BehavioralChannel->blank(
    name => "UrlKwd",
    account_id => $account,
    url_kwd_list => $trigger_list5,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'R',
        time_to => 60 )]));

   
  $ns->output("SEARCH1", $keyword1_6);
  $ns->output("KW2",  $keyword2_4 . " " .  $keyword2_5 . " " .  $keyword2_3 . " " .
    $keyword2_6 . " word1,first1 " .  $keyword2_1 . " " .  
    $keyword2_2 . " " .  $keyword2_3);
    
  $ns->output("SEARCH3", $keyword3_6 . "+" . $keyword3_5);
  $ns->output("KW3", "generic " . $keyword3_3 . " home," .  $keyword3_2);

  $ns->output("SEARCH4",
    $keyword3_1 . "+" . $keyword3_6 . "+" . $keyword3_5);
  $ns->output("KW4", "generic " . $keyword3_3 . "," .  $keyword3_1 . " " . $keyword3_2);

  $ns->output("KW5", 
    $keyword3_1 . " "  . $keyword3_6 . ", " . $keyword3_5 . ", " .
    $keyword3_2 . " " . $keyword3_3);

  $ns->output("KW6", 
    $keyword3_1 . " "  . $keyword3_2 . ", " . 
    $keyword3_5  . " " . $keyword3_3);

  $ns->output("KW7", $keyword3_4 . " " . $keyword3_5);
  $ns->output("SEARCH7", $keyword3_4 . "+" . $keyword3_5);

  $ns->output("KW8", $keyword3_4);
  $ns->output("SEARCH8", " $keyword3_5");

  $ns->output("KW9", "$keyword4_1,$keyword4_2,$keyword4_3,$keyword4_4 $keyword4_5");
  $ns->output("SEARCH9", "$keyword4_1,$keyword4_2,$keyword4_3,$keyword4_6, $keyword4_4 $keyword4_5");

  # Do not call _xml_encode for FT parameters (already encoded)
  $ns->output_raw("FT10", "fulltext&#x0d;&#x0a;$keyword3_7");
  $ns->output("KW10", "$keyword3_7");

  $ns->output_raw("FT11", "$keyword3_8&#x0d;$keyword3_9");
  $ns->output_raw("FT12", "$keyword3_10&#x0d;&#x0a;$keyword3_11");
  $ns->output("FT13", "fulltext\nfulltextmatching3 ".
    "fulltextmatching4\nfulltextmatch");

  $ns->output("REF14", "http://url2.keywords3.ru/index.php");
  $ns->output("REF15", "http://a.com/" . $keyword3_1 . "?" . 
    $keyword3_6  . "," . $keyword3_5 . "&1");

  $ns->output("Channel1", $page->{channel_id});
  $ns->output("Channel2", $search->{channel_id});
  $ns->output("Channel3", $ordinary->{channel_id});

  $ns->output("BP1", $page->page_key());
  $ns->output("BP2", $search->search_key());
  $ns->output("BP3", $ordinary->page_key());
  $ns->output("BP4", $ordinary->search_key());
  $ns->output("BP5", $ordinary->url_kwd_key());
  $ns->output("BP6", $url_kwd->url_kwd_key());

  output_channel_triggers($ns, "SearchTrigger/5", $splitpage, 'S');
  output_channel_triggers($ns, "PageTrigger/5", $splitpage, 'P');
  output_channel_triggers($ns, "UrlKwdTrigger/5", $splitpage, 'R');
}

1;
