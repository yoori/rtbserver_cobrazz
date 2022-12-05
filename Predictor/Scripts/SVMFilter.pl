#!/usr/bin/perl -w

use strict;
use ChiSquare;

my $file = $ARGV[0];
# max probability that result don't depends on feature,
# example 0.95 will filter all features for that probability of independence is great then 0.95
my $max_probability_of_independence = $ARGV[1];

my %feature_imps;
my %feature_clicks;
my $line_i = 0;
my $all_clicks = 0;

open FILE, $file or die $!;
open(my $res_file, '>', $file . '~') or die "Can't open '$file': $!";

#my $skipped_lines = 0;

while(<FILE>)
{
  my $line = $_;
  chomp $line;
  $line =~ s/\s+/ /g;

  my @arr = split(' ', $line);
  my $label = shift(@arr);
  if($label !~ m/^-?\d+(.\d+)?$/)
  {
    die "invalid label at line #" . $line_i . ": $line";   
  }

  my %features;
  foreach my $el(@arr)
  {
    if($el !~ m/^([0-9]+):([-.0-9]+)$/)
    {
      last;
      #die "can't parse '$el' from line #" . $line_i . ": $line";
    }

    my $key = $1;
    my $val = $2;
    
    if($key <= 2000000000) # if($key <= 4294967295)
    {
      $features{$key} = $val;

      if(!exists($feature_imps{$key}))
      {
        $feature_imps{$key} = 1;
      }
      else
      {
        ++$feature_imps{$key};
      }

      if($label == 1)
      {
        ++$all_clicks;

        if(!exists($feature_clicks{$key}))
        {
          $feature_clicks{$key} = 1;
        }
        else
        {
          ++$feature_clicks{$key};
        }
      }
    }
    else
    {
      print STDERR "skipped value: $key\n";
    }
  }

  my $res_line = $label;
  foreach my $key(sort {$a <=> $b} keys %features)
  {
    $res_line = $res_line . ' ' . $key . ':' . $features{$key};
  }

  print $res_file ($res_line . "\n");
  #print $res_line . "\n";
  ++$line_i;
}

close $res_file;
close FILE;

my %skip_features;
foreach my $key(keys %feature_imps)
{
  # eval chi square
  my $imps_with_feature = $feature_imps{$key};
  my $clicks_with_feature = exists($feature_clicks{$key}) ? $feature_clicks{$key} : 0;
  my $all_imps = $line_i;

  my ($min_prob, $max_prob) = chi_square(
    [$imps_with_feature - $clicks_with_feature, $clicks_with_feature],
    # expected
    [$imps_with_feature - $all_clicks * $imps_with_feature * 1.0 / $all_imps,
     $all_clicks * $imps_with_feature * 1.0 / $all_imps]);

  if($min_prob >= $max_probability_of_independence ||
    ($clicks_with_feature == 0 && (($imps_with_feature * 1.0 / ($all_imps - $all_clicks)) < 0.01)))
  {
    $skip_features{$key} = 1;
    print STDERR "skipped $key\n";
  }
  else
  {
    print "$max_prob $key\n";
  }

  #if($feature_counters{$key} > $line_i * $max / 100)
  #{
  #  $skip_features{$key} = 1;
  #}
}

print "to filter features\n";

open($res_file, '>', $file . '.f') or die "Can't open '$file.f': $!";
open FILE, $file . '~' or die $!;

while(<FILE>)
{
  my $line = $_;
  chomp $line;

  my @arr = split(' ', $line);
  my $label = shift(@arr);
  my @res_array;
  foreach my $el(@arr)
  {
    $el =~ m/^([0-9]+):([-.0-9]+)$/;
    my $key = $1;
    if(!exists($skip_features{$key}))
    {
      push(@res_array, $el);
    }
  }

  print $res_file ($label . ' ' . join(' ', @res_array) . "\n");
}

close FILE;
close $res_file;

unlink($file . '~');
unlink($file);
rename($file . '.f', $file);
