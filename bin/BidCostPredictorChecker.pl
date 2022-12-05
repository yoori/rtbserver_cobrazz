#!/usr/bin/perl

use strict;
use warnings;

use List::Util qw(min);
use Pod::Usage;
use Getopt::Long qw(:config gnu_getopt bundling);

use Predictor::BidCostModel;
use Predictor::BidCostModelEvaluator;

sub load_model
{
  my ($model_file_path) = @_;
  my $model = new Predictor::BidCostModel();
  $model->load($model_file_path);
  return $model;
}

sub agg_apply_model
{
  my ($agg_folder, $model) = @_;

  my $rule = Path::Iterator::Rule->new;
  $rule->name("BidCostAggStat.*");
  my @all_files = sort $rule->all(($agg_folder));

  my $orig_sum = 0;
  my $recommended_sum = 0;

  foreach my $file_path(reverse(@all_files))
  {
    # read aggregate
    open(my $fh, '<', $file_path) or die "Could not open '$file_path' $!\n";
    while(my $line = <$fh>)
    {
      chomp $line;
      my @fields = split('\t', $line);

      # <tag id>,<domain>,<cost>,<bids>,<imps>,<clicks>
      my $tag_id = $fields[0];
      my $domain = $fields[1];
      my $cost = $fields[2];
      my $bids = $fields[3];
      my $imps = $fields[4];

      my $res_cost = $model->get_cost($tag_id, $domain, 0.95, $cost);
      $orig_sum += $imps * $cost;
      $recommended_sum += $imps * $res_cost;
    }
    close($fh);
  }

  return ($orig_sum, $recommended_sum);
}

sub main
{
  my $model_file;
  my $agg_folder;

  my %options = (
    "model-path|m=s" => \$model_file,
    "agg-dir|a=s" => \$agg_folder,
  );

  if(!GetOptions(%options))
  {
    pod2usage(1);
  }

  my $model = load_model($model_file);
  my ($orig_sum, $recommended_sum) = agg_apply_model($agg_folder, $model);
  print("Original sum    : $orig_sum\n");
  print("Recommended sum : $recommended_sum\n");
}

main();
