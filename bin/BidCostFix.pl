#!/usr/bin/perl

use strict;
use warnings;

use open qw( :std :encoding(UTF-8) );

use Class::Struct;
use List::Util qw( min minstr );
use File::Basename;
use Pod::Usage;
use Getopt::Long qw(:config gnu_getopt bundling);
use Date::Parse;
use DateTime;
use POSIX qw(strftime);


use Predictor::BidCostModel;
use Predictor::BidCostModelEvaluator;

# fix:
#   BidCostStatAgg.* => BidCostStatAgg.*
sub fix
{
  my ($input_file, $output_folder, $req_coef, $imp_coef, $req_tag_coef_file, $imp_tag_coef_file) = @_;

  my %req_tag_coefs;

  if(defined($req_tag_coef_file))
  {
    open(my $fh, '<', $req_tag_coef_file) or die "Could not open '$req_tag_coef_file' $!\n";
    while(my $line = <$fh>)
    {
      chomp $line;
      my @fields = split(',', $line);
      my $tag_id = $fields[0];
      my $cost_coef = $fields[1];
      $req_tag_coefs{$tag_id} = $cost_coef;
    }
    close($fh);
  }

  my %imp_tag_coefs;

  if(defined($imp_tag_coef_file))
  {
    open(my $fh, '<', $imp_tag_coef_file) or die "Could not open '$imp_tag_coef_file' $!\n";
    while(my $line = <$fh>)
    {
      chomp $line;
      my @fields = split(',', $line);
      my $tag_id = $fields[0];
      my $cost_coef = $fields[1];
      $imp_tag_coefs{$tag_id} = $cost_coef;
    }
    close($fh);
  }

  eval
  {
    my $bid_cost_evaluator = new Predictor::BidCostModelEvaluator();
    $bid_cost_evaluator->add_file($input_file);

    my $new_bid_cost_evaluator = new Predictor::BidCostModelEvaluator();

    if($input_file =~ m/BidCostAggStat[.]([^.]+)[.].*/)
    {
      my $date = $1;
      my $base_dump_file_path = "BidCostAggStat." . $date . "." . sprintf( "%06d", rand(999999) );
      my $dump_tmp_file_path = $output_folder . "/~" . $base_dump_file_path;
      my $dump_file_path = $output_folder . "/" . $base_dump_file_path;

      while(my ($key, $sub_agg) = each(%{$bid_cost_evaluator->agg()}))
      {
        my @key_fields = split('\t', $key);
        my $tag_id = $key_fields[0];
        my $domain = $key_fields[1];

        while(my ($cost, $agg) = each(%$sub_agg))
        {
          #print("X $key\t$cost\n");
          # key : $tag_id\t$domain\t$cost
          # value : bids, imps, clicks

          #print("tag_id = $tag_id\n");

          # req coefs
          my $local_req_coef;
          if(exists($req_tag_coefs{$tag_id}))
          {
            $local_req_coef = $req_tag_coefs{$tag_id};
          }
          else
          {
            $local_req_coef = $req_coef;
          }
          #print("local_req_coef = $local_req_coef\n");
          my $req_cost = sprintf("%.6f", $cost * $local_req_coef);

          # imp coefs
          my $local_imp_coef;
          if(exists($imp_tag_coefs{$tag_id}))
          {
            $local_imp_coef = $imp_tag_coefs{$tag_id};
          }
          else
          {
            $local_imp_coef = $imp_coef;
          }
          my $imp_cost = sprintf("%.6f", $cost * $local_imp_coef);

          #print("COSTS: $req_cost,$imp_cost,$req_coef,$imp_coef,$cost,". ($cost * $req_coef) ."\n");
          my $req_agg = new Agg(bids => $agg->bids(), imps => 0, clicks => 0);
          $new_bid_cost_evaluator->add_agg($tag_id, $domain, $req_cost, $req_agg);

          my $imp_agg = new Agg(bids => 0, imps => $agg->imps(), clicks => $agg->clicks());
          $new_bid_cost_evaluator->add_agg($tag_id, $domain, $imp_cost, $imp_agg);
        }
      }

      $new_bid_cost_evaluator->save_agg($dump_tmp_file_path);
      print("Result saved into $dump_tmp_file_path\n");

      rename($dump_tmp_file_path, $dump_file_path) ||
        die("can't rename $dump_tmp_file_path to $dump_file_path");

      unlink($input_file);
    }
    else
    {
      die("invalid file : $input_file");
    }
  };

  if($@)
  {
    print("ERROR: $@");
  }
}

sub main
{
  my $agg_file;
  my $req_coef = 1;
  my $imp_coef = 1;
  my $req_tag_coef_file;
  my $imp_tag_coef_file;

  my %options = (
    "file|f=s" => \$agg_file,
    "req=f" => \$req_coef,
    "imp=f" => \$imp_coef,
    "req-tag-coef-file=s" => \$req_tag_coef_file,
    "imp-tag-coef-file=s" => \$imp_tag_coef_file,
  );

  if (!GetOptions(%options))
  {
    pod2usage(1);
  }

  fix($agg_file, dirname($agg_file), $req_coef, $imp_coef, $req_tag_coef_file, $imp_tag_coef_file);

  return 0;
}

my $exit_code = main();
exit $exit_code;
