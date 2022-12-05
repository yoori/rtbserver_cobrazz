
package LemmatisationTest;

use strict;
use warnings;
use encoding 'utf8';
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $i = 0;

  my $acc = $ns->create(Account => {
    name => "Advertiser",
    role_id => DB::Defaults::instance()->advertiser_role });

  {
    my @bp_triggers = (
      [ "en", "plane" ], #1
      [ "en", "Willem Wilsons\n" .
           "hotmail localhost\n" . 
              "\"iBooks iphone\"" ], #2
      [ "en", "ferries flurries" ], #3
      [ "en", "\"fly seven\"\n" .
          "\"plane\"\n" .
            "[\"exactlemmatisation cars\"]" ], #4
      [ "en", 
          qq[\xc3\xa6sir\x20gods\x20host]  ."\n" .
           "fly seven-miles"], #5
      [ "ja", qq[\xe9\xb3\xb4\xe3\x81\x84\xe3\x81\xa6\x20\xe9\xae\xae\xe6\x98\x8e] .
           "\n" . qq[\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\x95\xe3\x83\xb3\xe9\xbc\xbb\xe9\x9f\xb3\xe5\x8c\x96]], #6
      [ "en", "abonnement zurro\n" .
          "formula reserve\n" .
             "aa zurron zupa"  ], #7
      [ "pt", "abonnement zurro\n" .
          "formula reserve\n" .
             "aa zurron zupa\n" .
                qq[m\xc3\xbasica in\xc3\xa9dita] ] #8
    );
    

    for my $t (@bp_triggers)
    {
      $i++;
      my ($lang, $trigger) = @$t;
      my $channel = $ns->create(
        DB::BehavioralChannel->blank(
        name => 'BehavioralChannel-' . $i,
        account_id => $acc,
        keyword_list => $trigger,
        language => $lang,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            time_to => 5 * 60 ),
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'S',
            time_to => 5 * 60 )]));

      $ns->output("BPChannel/" . $i, $channel);
      $ns->output("BPPage/" . $i, $channel->page_key());
      $ns->output("BPSearch/" . $i, $channel->search_key());
    }
  }

  {
    my @bp_triggers = (
      [ 'en', "aa zymurgies\n" .
        "fly seven\n" .
        "[exactlemmatisation cars]\n" .
        "[soundbox]\n" .
        "\"soundbox\""  ] #1
    );

    for my $t (@bp_triggers)
    {
      $i++;
      my ($lang, $trigger) = @$t;
      my $channel = $ns->create(DB::BehavioralChannel->blank(
         name => 'BehavChannel-' . $i,
         account_id => $acc,
         keyword_list => $trigger,
         language => $lang,
         behavioral_parameters => [
           DB::BehavioralChannel::BehavioralParameter->blank(
             trigger_type => 'P',
             time_to => 5 * 60 ),
           DB::BehavioralChannel::BehavioralParameter->blank(
             trigger_type => 'S',
             time_to => 5 * 60 )]));

      $ns->output("BPChannel/" . $i, $channel);
      $ns->output("BPPage/" . $i, $channel->page_key());
      $ns->output("BPSearch/" . $i, $channel->search_key());
      output_channel_triggers($ns, "BPTrigger/" . $i, $channel, 'S');
    }
   
    my $channel = $ns->create(DB::BehavioralChannel->blank(
      name => 'UrlKwdChannel',
      account_id => $acc,
      url_kwd_list => "ferries flurries\n".
        "\"soundbox\"",
      language => 'en',
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'R',
          time_to => 5 * 60 )]));

    $ns->output("UrkKwdChannel", $channel);
    $ns->output("BPUrkKwd", $channel->url_kwd_key());
  }
   
  my @keywords = (
    "plane", #1
    "planes", #2
    "fly seven miles", #3
    "flew sevens miles", #4
    "flew seven miles", #5
    "aas zymurgy", #6
    "ferry flurrying", #7
    qq[\xc3\xa6sir\x20god\x20host], #8
    "asynjur god host", #9
    qq[\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\x95\xe3\x83\xb3\x20\xe9\xbc\xbb\xe9\x9f\xb3\xe5\x8c\x96], #10
    qq[\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\x95\xe3\x83\xb3\xe3\x81\x97\x20\xe9\xbc\xbb\xe9\x9f\xb3\xe5\x8c\x96], #11
    qq[\xe9\xae\xae\xe6\x98\x8e\x20\xe9\xb3\xb4\xe3\x81\x8d], #12
    "Willems Wilson", #13
    "abonnement zurro", #14
    "abonnements zurros", #15
    "formulae reserving", #16
    "formulamos reservais", #17
    "musicado ineditas", ##18
    qq[\xe9\xae\xae\xe6\x98\x8e\x20\xe3\x81\xaa\xe3\x81\x8f\xe3\x81\xaa\xe3\x81\x84\xe3\x81\xa6\xe3\x81\x99], #19
    "exactlemmatisation car", #20
    "soundboxes", #21
    "sportivi boxen ban channeling" #22
   );


  $i = 0;
  for my $kwd (@keywords)
  {
    $i++;
    Encode::_utf8_on($kwd);
    $ns->output("KWD/" . $i, $kwd);
  }
}
1;
