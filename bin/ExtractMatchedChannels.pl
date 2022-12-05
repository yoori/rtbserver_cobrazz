#!/usr/bin/perl

# This script process the result of 'ProfileDump print-base' command.
# for extracting (UID, [array of matching Channel IDs]) list.

if ($#ARGV < 0)
{
  print "Usage: ExtractMatchedChannels.pl <ProfileDump print-base output file>\n";
  exit();
}

open(FILE, "<", $ARGV[0])
  or die "cannot open '$ARGV[0]': $!";

%profiles = ();
$user_id = "";

while ($line = <FILE>)
{
  if ($line =~ /^>>>>>>>>>> User '(.*)'/)
  {
    $user_id = $1;
    $profiles{$user_id} = ();
  }
  elsif ($line =~ /^\s*(page_history_matches|search_history_matches|url_history_matches) :/)
  {
    while ($line =~ m/channel_id = (\d*)/g)
    {
      $profiles{$user_id}{$1} = "";
    }
  }
  elsif ($line =~ /^\s*(page_ht_candidates|search_ht_candidates|url_ht_candidates) :/)
  {
    while ($line =~ m/channel_id = (\d*), req_visits = (\d*)/g)
    {
      if ($2 eq "0")
      {
        $profiles{$user_id}{$1} = "";
      }
    }
  }
}

for $id (sort keys %profiles)
{
  print "$id, [";
  print join ",", (sort {$a <=> $b} keys %{ $profiles{$id} });
  print "]\n";
}
