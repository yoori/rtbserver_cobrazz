#!/usr/bin/perl

use strict;
use warnings;

use Text::CSV_XS;
use open qw( :std :encoding(UTF-8) );

use Cwd;
use Class::Struct;
use List::Util qw( min minstr );
use File::Find;
use File::Basename;
use Path::Iterator::Rule;
use Log::Dispatch;
use XML::Twig;
use File::Spec::Functions;
use Pod::Usage;
use Proc::Daemon;
use Getopt::Long qw(:config gnu_getopt bundling);
use Date::Parse;
use DateTime;
use DateTime::Duration;
use POSIX qw(strftime);


use Predictor::BidCostModel;
use Predictor::BidCostModelEvaluator;

# reaggregate:
#   BidCostStatAgg.* => BidCostStatAgg.*
sub aggregate
{
  my ($input_folder) = @_;

  # search input files
  my $rule = Path::Iterator::Rule->new;
  $rule->name("BidCostAggStat.*");
  my @all_files = $rule->all(($input_folder));
  my $res_agg = {};

  foreach my $file(@all_files)
  {
    my ($file_name, $file_path) = fileparse($file);
    if($file_name =~ m/BidCostAggStat[.]([^.]+)[.].*/)
    {
      $res_agg = Predictor::BidCostModelEvaluator::load_agg_file($file, $res_agg);
    }
  }

  return $res_agg;
}

sub main
{
  my $agg_folder;
  my $max_ctr = 0.001;

  my %options = (
    "agg-dir|a=s" => \$agg_folder,
    "max-ctr|c=f" => \$max_ctr,
  );

  if(!GetOptions(%options))
  {
    pod2usage(1);
  }

  my $agg = aggregate($agg_folder);

  my %domain_agg;

  while(my ($top_key, $agg) = each(%$agg))
  {
    my $bids = $agg->bids();
    my $imps = $agg->imps();
    my $clicks = $agg->clicks();
    my @top_key_parts = split('\t', $top_key);
    my $domain = $top_key_parts[1];

    if(!exists($domain_agg{$domain}))
    {
      $domain_agg{$domain} = new Agg(bids => 0, imps => 0, clicks => 0);
    }

    my $agg = $domain_agg{$domain};
    $agg->bids($agg->bids() + $bids);
    $agg->imps($agg->imps() + $imps);
    $agg->clicks($agg->clicks() + $clicks);
  }

  while(my ($domain, $agg) = each(%domain_agg))
  {
    if($agg->imps() > 500 && $agg->clicks() * 1.0 / $agg->imps() < $max_ctr)
    {
      print($domain . "\t" . $agg->imps() . "\t" . $agg->clicks() . "\n");
    }
  }
}

my $exit_code = main();
exit $exit_code;
