#!/usr/bin/perl

# This script process the result of 'ProfileDump print-request' command.
# for extracting (Campaign ID, [array of UIDs who have clicked on this Campaign ID]) list

if ($#ARGV < 0)
{
  print "Usage: ExtractCampaignClick.pl <ProfileDump print-request output file>\n";
  exit();
}

open(FILE, "<", $ARGV[0])
  or die "cannot open '$ARGV[0]': $!";

%campaigns = ();
$user_id = "";
$campaign_id = "";
$impression_verified = "";

while ($line = <FILE>)
{
  if ($line =~ /^\s*user_id : (.*)$/)
  {
    $user_id = $1;
    $impression_verified = "";
  }
  elsif ($line =~ /^\s*campaign_id : (\d*)$/)
  {
    $campaign_id = $1;
  }
  elsif ($line =~ /^\s*impression_verified : (0|1)$/)
  {
    $impression_verified = $1;
  }
  elsif ($impression_verified eq "1" &&
         $line =~ /^\s*click_done : (0|1)$/)
  {
    $click_done = $1;

    if ($click_done eq "1")
    {
      $campaigns{$campaign_id}{$user_id} = "";
    }
  }
}

for $campaign_id (sort { $a <=> $b } keys %campaigns)
{
  print "$campaign_id, [";
  print join ",", (sort keys %{$campaigns{$campaign_id}});
  print "]\n";
}
