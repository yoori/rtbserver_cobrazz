
package BrokenChannelTriggers;

use strict;
use warnings;
use encoding 'utf8';
use DB::Defaults;
use DB::Util;

sub generate_string
{
  my ($ns, $length) = @_;

  my $buffer = make_autotest_name($ns, $length . "bytes");
  my $size = $length - length($buffer);

  for (my $i = 0; $i < $size; $i++)
  {
    $buffer .= chr(ord('0') + $i % 10);
  }

  return $buffer;
}

# Incorrect trigger definition test
sub incorrect_trigger
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("ITR");

  my $korean1  = qq[\xec\x95\xbc\xea\xb1\xb0];
  my $korean2  = qq[\xec\x98\xa4\xed\x9c\x98];

  my $ch1 = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    keyword_list => "nonexactmatch",
    url_list => 
      "www.gmail.ru\n" .
      "www:nt.ru\n" .
      "www.tuning1.ru\n" .
      "http:/www.ru",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P')]));

  my $ch2 = $ns->create(DB::BehavioralChannel->blank(
    name => 2,
    url_list => "ww.com",
    keyword_list =>
      "abazabar\n" .
      "test1\n" .
      "\"asd\n" .
      "bars\n" .
      "test2\n" .
      $korean1 . "\n" .
      $korean2 . "\n" .
      "[brokenexactmatch]",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S',
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U'),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P', time_to => 60)]));

  $ns->output("Channel1U", $ch1->url_key());
  $ns->output("Channel1P", $ch1->page_key());
  $ns->output("Channel2P", $ch2->page_key());
  $ns->output("Channel2U", $ch2->url_key());
  $ns->output("Channel2S", $ch2->search_key());

  $ns->output("REF1", "www.gmail.ru");
  $ns->output("KWD1", "\"asd");
  $ns->output("REF2", "www.gmail.ru");
  $ns->output("KWD2", "bars");
  $ns->output("REF3", "www.gmail.ru");
  $ns->output("KWD3", "abazabar");
  $ns->output("KWD4", $korean2 . " " . $korean1);
  $ns->output("SEARCH4", $korean2);
  $ns->output("REF5", "www:nt.ru");
  $ns->output("KWD5", "abazabar");
  $ns->output("REF6", "www.tuning1.ru");
  $ns->output("KWD6", "abazabar");
  $ns->output("KWD7", "brokenexactmatch,nonexactmatch"),
  $ns->output("SEARCH7", "brokenexactmatch"),
}

# Trigger length limit test
sub trigger_length
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("LEN");

  my $url1 = "http://www.google.ru/search?aq=f&complete=1&hl=ru&newwindow=1&q=2046-bytes&lr=qwerty" .
    generate_string($ns, 1962);
  my $url2 = "http://www.google.ru/search?aq=f&complete=1&hl=ru&newwindow=1&q=2048-bytes&lr=qwerty" .
    generate_string($ns, 1964);
  my $url3 = "http://www.google.ru/search?aq=f&complete=1&hl=ru&newwindow=1&q=2050-bytes&lr=qwerty" .
    generate_string($ns, 1966);

  my $keyword1 = generate_string($ns, 510);
  my $keyword2 = generate_string($ns, 512);
  my $keyword3 = generate_string($ns, 514);

  my $keyword_ex1 = "exact" . generate_string($ns, 505);
  my $keyword_ex2 = "exact" . generate_string($ns, 506);

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel',
    url_list =>
      $url1 . "\n" .
      $url2 . "\n" .
      $url3 ,
    keyword_list =>
      $keyword1 . "\n" .
      $keyword2 . "\n" .
      $keyword3 . "\n" .
      "[$keyword_ex1]\n" .
      "[$keyword_ex2]",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U'),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S', time_to => 60),]));

  $ns->output("ChannelP", $ch->page_key());
  $ns->output("ChannelU", $ch->url_key());
  $ns->output("ChannelS", $ch->search_key());

  $ns->output("KWD1", $keyword1);
  $ns->output("KWD2", $keyword2);
  $ns->output("KWD3", $keyword3);
  $ns->output("KWD4", "limit_test," . $keyword2);
  $ns->output("REF5", $url1);
  $ns->output("REF6", $url2);
  $ns->output("REF7", $url3);
  $ns->output("SEARCH8", $keyword_ex1);
  $ns->output("SEARCH9", $keyword_ex2);
}

# Triggers word count limit test
sub trigger_words
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("WORDS");

  my $words1 = qq[\xea\xb5\xac\x20\xeb\x93\xb1\x20\xec\xa0\x9c\x20] .
    qq[\xeb\x93\xb1\x20\xec\xa0\x9c\x20\xeb\x93\xb1\x20\xec\xa0\x9c\x20] .
    qq[\xeb\x93\xb1\x20\xec\xa0\x9c\x20\xeb\x93\xb1\x20\xec\xa0\x9c\x20] .
    qq[\xeb\x93\xb1\x20\xec\xa0\x9c\x20\xec\xb6\x95\x20\xed\x98\xb8\x20] .
    qq[\xed\x85\x94\x20\xea\xb5\xac\x20\xec\xb6\x95\x20\xed\x98\xb8\x20] .
    qq[\xed\x85\x94\x20\xec\xa0\x9c];

  my $words2 = "WW1 WW2 \"WW3 WW4 WW5\" \"WW6 WW7 WW8\" WW9 WW10 BB11 " .
    "WW12 WW13 WW14 WW15 WW16 WW17 WW18 WW19 WW20 WW21";

  my $words3 = "BB1 BB2 BB3 BB4 BB5 BB6 BB7 BB8 BB9 BB10 BB11 BB12 BB13 " .
    "BB14 BB15 BB16 BB17 BB18 BB19 BB20 BB21";

  my $words4 = "AA1 AA2 AA3 AA4 AA5 AA6 AA7 AA8 AA9 AA10 AA11 AA12 AA13 " .
    "AA14 AA15 AA16 AA17 AA18 AA19 AA20";

  my $words5 = "CC1 CC2 CC3 CC4 CC5 CC6 CC7 CC8 CC9 CC10 CC11 CC12 CC13 " .
    "CC14 CC15 CC16 CC17 CC18 CC19 CC20 CC21";

  my $words6 = "[Ex1 \"Ex2 Ex3 Ex4 Ex5 Ex6 Ex7 Ex8 Ex9 Ex10\" Ex11 Ex12 " .
    "Ex13 Ex14 Ex15 Ex16 Ex17 Ex18 Ex19 Ex20 Ex21]";

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel',
    keyword_list =>
      $words1 . "\n" .
      $words2 . "\n" .
      $words3 . "\n" .
      $words4 . "\n" .
      "-" . $words5,
    search_list =>
      $words1 . "\n" .
      $words2 . "\n" .
      $words3 . "\n" .
      $words4 . "\n" .
      $words6,
    url_kwd_list =>
      $words3  . "\n" .
      $words4,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'R', time_to => 60) ]));

  $ns->output("ChannelP", $ch->page_key());
  $ns->output("ChannelS", $ch->search_key());
  $ns->output("ChannelR", $ch->url_kwd_key());
  $ns->output("KWD1", $words4);
  $ns->output("SEARCH1", $words4);
  $ns->output("KWD2", $words3);
  $ns->output("SEARCH2", $words3);
  $ns->output("KWD3", 
    "CC1 CC2 CC3 CC4 CC5 CC6 CC7 CC8 CC9 CC10 CC11 " .
    "CC12 CC13 CC14 CC15 CC16 CC17 CC18 CC19 CC20 CC21, " .
    "AA1 AA2 AA3 AA4 AA5 AA6 AA7 AA8 AA9 AA10 AA11 AA12 " .
    "AA13 AA14 AA15 AA16 AA17 AA18 AA19 AA20");
  $ns->output("KWD4", 
    "WW1 WW2 WW3 WW4 WW5 WW6 WW7 WW8 WW9 WW10 BB11 WW12 " .
    "WW13 WW14 WW15 WW16 WW17 WW18 WW19 WW20 WW21");
  $ns->output("KWD5",
    qq[\xea\xb5\xac\x20\xeb\x93\xb1\x20\xec\xa0\x9c\x20] .
    qq[\xec\xb6\x95\x20\xed\x98\xb8\x20\xed\x85\x94\x20] .
    qq[\xea\xb5\xac\x20\xec\xb6\x95\x20\xed\x98\xb8\x20] .
    qq[\xed\x85\x94\x20\xec\xa0\x9c]);
  $ns->output("SEARCH6",
    "Ex1 Ex2 Ex3 Ex4 Ex5 Ex6 Ex7 Ex8 Ex9 Ex10 Ex11 Ex12 " .
    "Ex13 Ex14 Ex15 Ex16 Ex17 Ex18 Ex19 Ex20 Ex21");
}


# Hard keyword words count limit test
sub hard_words
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("HARD");

  my $words1 =
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89\x20] .
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89\x20] .
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89\x20] .
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89\x20] .
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89\x20] .
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89\x20] .
   qq[\xEC\xA0\x9C\xEC\xA3\xBC\xED\x98\xB8\xED\x85\x94\xEB\x93\xB1\xEA\xB8\x89];

  my $words2 =
   "BBB1 BBB2 BBB3 BBB4 BBB5 BBB6 BBB7 BBB8 BBB9 BBB10 BBB11 BBB12 BBB13 " .
   "BBB14 BBB15 BBB16 BBB17 BBB18 BBB19 BBB20 BBB21";

  my $words3 =
   "AAA1 AAA2 AAA3 AAA4 AAA5 AAA6 AAA7 AAA8 AAA9 AAA10 AAA11 AAA12 " .
   "AAA13 AAA14 AAA15 AAA16 AAA17 AAA18 AAA19 AAA20";

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel',
    keyword_list =>
      "\"" . $words1 . "\"\n" .
      "\"" . $words2 . "\"\n" .
      "\"" . $words3 . "\"",
    url_kwd_list =>
      "\"" . $words1 . "\"\n" .
      "\"" . $words2 . "\"\n" .
      "\"" . $words3 . "\"",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'R', time_to => 60) ]));

  $ns->output("ChannelP", $ch->page_key());
  $ns->output("ChannelS", $ch->search_key());
  $ns->output("ChannelR", $ch->url_kwd_key());

  $ns->output("KWD1", $words3);
  $ns->output("SEARCH1", $words3);
  $ns->output("KWD2", $words2);
  $ns->output("SEARCH2", $words2);
  $ns->output("KWD3", $words1);
  $ns->output("SEARCH3", $words1);
}

sub init
{
  my ($self, $ns) = @_;

  $self->incorrect_trigger($ns);
  $self->trigger_length($ns);
  $self->trigger_words($ns);
  $self->hard_words($ns);
}

1;
