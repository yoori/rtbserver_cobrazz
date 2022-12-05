package ContextChannelsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  # search
  my $referer = "http://www.google.ru";
  my $page_kww = make_autotest_name($ns, "key_phrase_page");
  my $search_kww = make_autotest_name($ns, "key_phrase_search");
  my $context_kww = make_autotest_name($ns, "key_phrase_context");

  $ns->output("Referer/01", $referer, "referer url");
  $ns->output("PageKwd/01", $page_kww, "page keyword");
  $ns->output("SearchKwd/01", $search_kww, "search keyword");

  #context channels (min visits = 1 && time from = 0)
  create_behavioral_parameters(
    $ns, $acc, "Cntx%02d", { minimum_visits => 1, time_to => 259200 },
    U => $referer,
    P => $page_kww,
    S => $search_kww, # REVIEW !!!
    );

  #non-context channels (min visits > 1 || time from > 0)
  create_behavioral_parameters(
    $ns, $acc, "NCntx01", {
      minimum_visits => 1,
      time_from => 0,
      time_to => 0 },
    U => $referer,
    );

  create_behavioral_parameters(
    $ns, $acc, "NCntx02", {
      minimum_visits => 2,
      time_from => 30,
      time_to => 60 },
    P => $page_kww,
    );

  create_behavioral_parameters(
    $ns, $acc, "NCntx03", {
      minimum_visits => 2,
      time_from => 120,
      time_to => 180 },
    S => $search_kww,
    );
}

1;
