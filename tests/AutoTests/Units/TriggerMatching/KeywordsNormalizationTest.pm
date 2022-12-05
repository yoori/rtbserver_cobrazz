package KeywordsNormalizationTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub make_test_trigger
{
  my ($ns, $kwd)  = @_;
  (my $prefix =  $ns->namespace) =~ s/\W//g;
  return $prefix . $kwd;
}

sub init
{
    my ($self, $ns) = @_;

    # Keywords
    my $keyword1    = "\\\\cookie internet";
    my $keyword2    = "cookie: editor";
    my $keyword3_1  =  make_test_trigger($ns, "a1");
    my $keyword3_2  =  make_test_trigger($ns, "b1");
    my $keyword3_3  =  make_test_trigger($ns, "c1");
    my $keyword4_1  =  make_test_trigger($ns, "d1");
    my $keyword4_2  =  make_test_trigger($ns, "e1");
    my $keyword4_3  =  make_test_trigger($ns, "g1");
    my $keyword5_1  =  make_test_trigger($ns, "comma");
    my $keyword5_2  =  make_test_trigger($ns, "not");
    my $keyword5_3  =  make_test_trigger($ns, "delimiter1");
    my $keyword6_1  =  make_test_trigger($ns,"abc");
    my $keyword6_2  =  make_test_trigger($ns,"xyz");
    my $keyword6_3  =  make_test_trigger($ns,"portable");
    my $keyword6_4  =  make_test_trigger($ns,"audio");
    my $keyword6_5  =  make_test_trigger($ns, "def");
    my $keyword7_1  =  make_test_trigger($ns, "middle1");
    my $keyword7_2  =  make_test_trigger($ns, "quote1");
    my $keyword8_1  =  make_test_trigger($ns,"internet1");
    my $keyword8_2  =  make_test_trigger($ns,"connection");
    my $keyword9 =  make_test_trigger($ns, "slashkwd");
    my $keyword10_1 =  make_test_trigger($ns, "slash");
    my $keyword10_2 =  make_test_trigger($ns, "kwd");
    my $keyword11 = $keyword9;
    my $keyword12_1 =  make_test_trigger($ns, "SOFT1");
    my $keyword12_2 =  make_test_trigger($ns, "LowerCase");
    my $keyword13_1 =  make_test_trigger($ns, "hArD1");
    my $keyword13_2 =  make_test_trigger($ns, "lowerCASE");
    my $keyword14 =  make_test_trigger($ns, "minus1-");
    my $keyword15 =  make_test_trigger($ns, "minus2\\-");
    my $keyword16 =  make_test_trigger($ns, "minus\\-abc");
    my $keyword18 =  make_test_trigger($ns, "notnegative1");
    my $keyword19 =  make_test_trigger($ns, "enclosingquote");
    my $keyword20_1 =  make_test_trigger($ns, "notescaped1");
    my $keyword20_2 =  make_test_trigger($ns, "space1");
    my $keyword21   =  make_test_trigger($ns, "dupl1");
    my $keyword21_1 =  make_test_trigger($ns, "dupl2");
    my $keyword21_2 =  make_test_trigger($ns, "dupl3");
    my $keyword21_3 =  make_test_trigger($ns, "DUPL2");
    my $keyword21_4 =  make_test_trigger($ns, "DUPL3");
    my $keyword22_1 =  make_test_trigger($ns, "ADSC5964a1");
    my $keyword22_2 =  make_test_trigger($ns, "ADSC5964b1");
    my $keyword23_1 =  make_test_trigger($ns, "ADSC5964a2");
    my $keyword23_2 =  make_test_trigger($ns, "ADSC5964b2");
    my $keyword24_1 =  make_test_trigger($ns, "ADSC5964a3");
    my $keyword24_2 =  make_test_trigger($ns, "ADSC5964b3");
    my $keyword25_1 =  make_test_trigger($ns, "ADSC5964a4");
    my $keyword25_2 =  make_test_trigger($ns, "ADSC5964b4");
    my $keyword25_3 =  make_test_trigger($ns, "ADSC5964c4");
    my $keyword26_1 =  make_test_trigger($ns, "ADSC6723");
    my $keyword26_2 =  make_test_trigger($ns, "adsc6723");
    my $keyword26_3 =  make_test_trigger($ns, "AdsC6723");
    my $keyword26_4 =  make_test_trigger($ns, "aDSc6723");
    my $keyword27_1 = "Exactmatchnormal1";
    my $keyword27_2 = "exactmatchnormal2";
    my $keyword27_3 = "exactmatchnormal3";
    my $keyword27_4 = "exactmatchnormal4";
    my $keyword28_1 = "exactmatchnotnegative1";
    my $keyword28_2 = "exactmatchnotnegative2";

    my @channels = (
      # channel #1
      {
        "$keyword1" => { page_hits => 0, search_hits => 0 },
        "$keyword2" => { page_hits => 0, search_hits => 0 },
        "$keyword3_1  $keyword3_2    $keyword3_3" => { page_hits => 1, search_hits => 0 },
        "\" $keyword4_1   $keyword4_2 $keyword4_3 \"" => { page_hits => 1, search_hits => 0 },
        "$keyword5_1, $keyword5_2, $keyword5_3" => { page_hits => 1, search_hits => 0 },
        "$keyword6_1 $keyword6_2 \"$keyword6_3 $keyword6_4\" $keyword6_5" => { page_hits => 0, search_hits => 0 },
        "\"\"$keyword6_3 $keyword6_4\"\"" => { page_hits => 1, search_hits => 1 },
        "$keyword7_1 \\\" $keyword7_2" => { page_hits => 0, search_hits => 0 },
        "$keyword8_1 \\\\$keyword8_2\\\\" => { page_hits => 1, search_hits => 0 },
        "\\$keyword9" => { page_hits => 2, search_hits => 0 },
        "$keyword10_1\\$keyword10_2" => { page_hits => 1, search_hits => 0 },
        "$keyword11\\" => { page_hits => 2, search_hits => 0 },
        "$keyword12_1 $keyword12_2" => { page_hits => 1, search_hits => 0 },
        "\"$keyword13_1 $keyword13_2\"" => { page_hits => 0, search_hits => 0 },
        "$keyword14" => { page_hits => 1, search_hits => 1 },
        "$keyword15" => { page_hits => 1, search_hits => 0 },
        "$keyword16" => { page_hits => 1, search_hits => 0 },
        "\\-$keyword18" => { page_hits => 1, search_hits => 0 },
        "\"$keyword19\"" => { page_hits => 1, search_hits => 0 },
        "$keyword20_1 \\ $keyword20_2" => { page_hits => 1, search_hits => 0 },
        "$keyword21 $keyword21" => { page_hits => 1, search_hits => 0 },
        "\"$keyword21_1 $keyword21_2\"  \"$keyword21_3 $keyword21_2\" \"$keyword21_1 $keyword21_4\"" => { page_hits => 1, search_hits => 0 },
        "$keyword22_1 -$keyword22_2" => { page_hits => 1, search_hits => 0 },
        "$keyword23_1 \\-$keyword23_2" => { page_hits => 1, search_hits => 0 },
        "\\-$keyword24_2 \\-$keyword24_1" => { page_hits => 1, search_hits => 0 },
        "-$keyword25_1 -$keyword25_2" => { page_hits => 0, search_hits => 0 },
        "$keyword25_3" => { page_hits => 1, search_hits => 0 },
        "$keyword26_1" => { page_hits => 1, search_hits => 1 },
        "$keyword26_2" => { page_hits => 1, search_hits => 1 },
        "[ $keyword27_1 \" $keyword27_2 \", $keyword27_3+$keyword27_4 ]" => { page_hits => 0 , search_hits => 1 },
        "[$keyword28_1 -$keyword28_2]" => { page_hits => 0, search_hits => 1 }
      },
      # channel #3
      {
        $keyword26_4 => { page_hits => 1, search_hits => 1}
      },
      # channel #2
      {
        "\"longest1 matched1 trigger1\"" => { page_hits => 0, search_hits => 0 },
        "\"longest1 matched1\"" => { page_hits => 3, search_hits => 1 },
        "\"most1 expensive1 trigger1 match1\"" => { page_hits => 0, search_hits => 0 }
      } );

    my $account = $ns->create(Account => {
      name => 1,
      role_id => DB::Defaults::instance()->advertiser_role });

    my @channels_ids;

    for (my $i = 0; $i < scalar @channels; ++$i)
    {
      my $channel = $ns->create(DB::BehavioralChannel->blank(
        name => $i,
#        keyword_list => join("\n", keys %{$channels[$i]}),
        account_id => $account,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P', time_to => 60),
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S', time_to => 60)] ));

      $ns->output("Page".($i + 1), $channel->page_key());
      $ns->output("Search".($i + 1), $channel->search_key());
      push @channels_ids, $channel->channel_id();

      # Fill channel_trigger_ids
      foreach my $trigger (keys %{$channels[$i]})
      {
        $channels[$i]->{$trigger}->{page_trigger} =
          $channel->add_trigger($ns, $trigger, 'P')->channel_trigger_id();
        $channels[$i]->{$trigger}->{search_trigger} =
          $channel->add_trigger($ns, $trigger, 'S')->channel_trigger_id();
      }




#      foreach my $trigger ( @{$channel->keyword_channel_triggers()},
#                            @{$channel->search_channel_triggers()} )
#      {
#        my $original_trigger = $trigger->original_trigger();
#        $original_trigger = "-$original_trigger"
#          if $trigger->negative() eq "Y";
#        my $trigger_type = $trigger->trigger_type() eq 'P'
#          ? "page_trigger"
#          : "search_trigger";
#        die "Created original_trigger ($original_trigger) " .
#            "differs from initial. Perhaps it was changed in Commons"
#          unless exists $channels[$i]->{$original_trigger};
#        $channels[$i]->{$original_trigger}->{$trigger_type} =
#          $trigger->channel_trigger_id();
#      }
    }

    $ns->output("ChannelList", 1000, "list of channels used in the test");
    for ( my $i = 0; $i < scalar @channels_ids; ++$i)
    {
      $ns->output("Channel$i", $channels_ids[$i]);
    }
    $ns->output("ChannelListEnd", 1000);

    $ns->output("ChannelTriggerList", 1000, "list of channel_trigger_ids");
    foreach my $channel (@channels)
    {
      foreach my $trigger (keys %$channel)
      {
        $ns->output("P", $channel->{$trigger}->{page_trigger},
          $channel->{$trigger}->{page_hits});
        $ns->output("S", $channel->{$trigger}->{search_trigger},
        $channel->{$trigger}->{search_hits});
      }
    }
    $ns->output("ChannelTriggerListEnd", 1000);

    # Referer keywords
    $ns->output("KW1", " " . $keyword3_1 . " " . $keyword3_2 . " " . $keyword3_3);
    $ns->output("KW2", $keyword4_1 . " " . $keyword4_2 . "  " . $keyword4_3 . " ");    
    $ns->output("KW3", $keyword5_1 . "," . $keyword5_2 . "," . $keyword5_3);    
    $ns->output("KW4", "\"$keyword6_3 $keyword6_4\"" );
    $ns->output("SEARCH4", "\"$keyword6_3 $keyword6_4\"" );
    $ns->output("KWW5", $keyword7_1 . " \" " . $keyword7_2 );
    $ns->output("KW6", $keyword8_1 . " \\" . $keyword8_2  . "\\");
    $ns->output("KW7", "\\" . $keyword9);
    $ns->output("KW8", $keyword10_1 .  "\\" . $keyword10_2);
    $ns->output("KW9", $keyword11 .  "\\");
    $ns->output("FT10", uc($keyword12_1) . "\n\n" . uc($keyword12_2));
    $ns->output("FT11", make_test_trigger($ns, "Hard1") . "," . $keyword13_2);
    $ns->output("KW12", $keyword14);
    $ns->output("KW13", make_test_trigger($ns, "minus2\\-"));
    $ns->output("KW14", make_test_trigger($ns, "minus\\-abc"));
    $ns->output("KW15", "-" . $keyword18);
    $ns->output("KW16", $keyword19);
    $ns->output("FT17", $keyword21 . "\n" . $keyword21);
    $ns->output("KWW18", $keyword21_1 . " " . $keyword21_2);
    $ns->output("KW19", $keyword20_1 . " \\ " . $keyword20_2);
    $ns->output("KW20", $keyword22_1 . " -" . $keyword22_2);
    $ns->output("KW21", $keyword23_1 . " \\-" . $keyword23_2);
    $ns->output("KW22", $keyword25_3);
    $ns->output("KW23", $keyword25_1 . " -" . $keyword25_2 . "," . $keyword25_3);
    $ns->output("KW24", "\\-" . $keyword24_1 . " -" . $keyword24_2);
    $ns->output("SEARCH25", 
      " \"exactmatchnormal1\",exactMatchnormal2+exactmatchnormal3 exactmatchnormal4");
    $ns->output("SEARCH26", "exactmatchnotnegative1 exactmatchnotnegative2");

    # kn cases
    $ns->output("KW27", $keyword14 . ",longest1 matched1");
    $ns->output("KWW28", $keyword14 . ",longest1 matched1");
    $ns->output("SEARCH29", $keyword14 . " longest1 matched1");
    $ns->output("FT30", $keyword14 . ",longest1 matched1");

    # ADSC-6723
    $ns->output("KW31", $keyword26_3);
    $ns->output("SEARCH31",$keyword26_3);
}

1;
