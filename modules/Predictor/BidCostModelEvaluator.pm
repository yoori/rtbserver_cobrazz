package Predictor::BidCostModelEvaluator;

use strict;
use warnings;
use List::Util qw( min );

use Class::Struct;

use Predictor::CTRTrivialModel;
use Predictor::BidCostModel;

struct(Agg => [bids => '$', imps => '$', clicks => '$']);


use constant TOP_LEVEL_WIN_RATE_MIN_IMPS => 50;
use constant LEVEL_WIN_RATE_MIN_IMPS => 50;

sub new
{
  my $class = shift;

  my $self = {
    'agg_' => {}
  };

  bless $self, $class;

  return $self;
}

sub agg
{
  my ($self) = @_;
  return $self->{agg_};
}

sub load_agg_file
{
  my ($file_path, $prev_agg) = @_;

  my $res_agg = {};

  if(defined($prev_agg))
  {
    $res_agg = $prev_agg;
  }

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
    my $clicks = $fields[5];

    my $key = "$tag_id\t$domain\t$cost";
    if(!exists($res_agg->{$key}))
    {
      $res_agg->{$key} = Agg->new(bids => 0, imps => 0, clicks => 0);
    }

    my $agg = $res_agg->{$key};
    $agg->bids($agg->bids() + $bids);
    $agg->imps($agg->imps() + $imps);
    $agg->clicks($agg->clicks() + $clicks);
  }
  close($fh);

  return $res_agg;
}

sub add_file
{
  my ($self, $file_path) = @_;

  my $agg = $self->{agg_};

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
    my $clicks = $fields[5];

    $self->add_agg(
      $tag_id, $domain, $cost,
      Agg->new(bids => $bids, imps => $imps, clicks => $clicks));
  }
  close($fh);
}

sub add_agg
{
  my ($self, $tag_id, $domain, $cost, $agg) = @_;

  # <tag id>,<domain>,<cost>,<bids>,<imps>,<clicks>
  my $top_key = "$tag_id\t$domain";
  my $sub_key = $cost;

  if(!exists($self->{agg_}->{$top_key}))
  {
    $self->{agg_}->{$top_key} = {};
  }

  my $check_agg;

  if(!exists($self->{agg_}->{$top_key}->{$sub_key}))
  {
    $self->{agg_}->{$top_key}->{$sub_key} = $agg;
    $check_agg = $agg;
  }
  else
  {
    my $res_agg = $self->{agg_}->{$top_key}->{$sub_key};
    $res_agg->bids($res_agg->bids() + $agg->bids());
    $res_agg->imps($res_agg->imps() + $agg->imps());
    $res_agg->clicks($res_agg->clicks() + $agg->clicks());
    $check_agg = $res_agg;
  }

  if($check_agg->imps() == 0 && $check_agg->bids() == 0 && $check_agg->clicks() == 0)
  {
    delete $self->{agg_}->{$top_key}->{$sub_key};
  }
}

sub save_agg
{
  my ($self, $file_path) = @_;

  open(my $dump_fh, '>>', $file_path) or die "Could not open '$file_path' $!\n";

  while (my ($key, $sub_agg) = each (%{$self->{agg_}}))
  {
    while (my ($sub_key, $agg) = each (%$sub_agg))
    {
      print $dump_fh "$key\t$sub_key\t" . $agg->bids() . "\t" . $agg->imps() . "\t" . $agg->clicks() . "\n";
    }
  }

  close($dump_fh);
}

sub evaluate_ctr
{
  my ($self, $continue, $logger) = @_;
  my $res = new Predictor::CTRTrivialModel();

  my $save_model = 1;

  while(my ($top_key, $cost_dict) = each(%{$self->{agg_}}))
  {
    if($$continue == 0)
    {
      $save_model = 0;
      last;
    }

    my $all_imps = 0;
    my $all_clicks = 0;

    my @all_costs = sort { $a <=> $b } keys(%$cost_dict);

    foreach my $cost(reverse(@all_costs))
    {
      my $agg = $cost_dict->{$cost};
      $all_imps += $agg->imps();
      $all_clicks += $agg->clicks();
    }

    my @top_key_parts = split('\t', $top_key);
    my $tag_id = $top_key_parts[0];
    my $domain = $top_key_parts[1];
    $res->set_ctr($tag_id, $domain, $all_clicks, $all_imps);
  }

  if($save_model > 0)
  {
    return $res;
  }
  else
  {
    return undef;
  }
}

sub evaluate
{
  my ($self, $orig_points, $continue, $logger) = @_;
  my @points = reverse(sort { $a <=> $b } (@$orig_points));
  my $res = new Predictor::BidCostModel();

  my $save_model = 1;

  while(my ($top_key, $cost_dict) = each(%{$self->{agg_}}))
  {
    if($$continue == 0)
    {
      $save_model = 0;
      last;
    }

    # eval top win rate
    my $top_level_win_rate;

    my @all_costs = sort { $a <=> $b } keys(%$cost_dict);

    {
      my $bids = 0;
      my $imps = 0;
      foreach my $cost(reverse(@all_costs))
      {
        my $agg = $cost_dict->{$cost};
        $bids += $agg->bids();
        $imps += $agg->imps();
        if($imps >= TOP_LEVEL_WIN_RATE_MIN_IMPS)
        {
          last;
        }
      }

      if($bids > 0)
      {
        $top_level_win_rate = $imps * 1.0 / $bids;
        #print("Top win rate $top_level_win_rate(imps = $imps, bids = $bids)\n");
      }
    }

    if(defined($top_level_win_rate) && $top_level_win_rate > 0)
    {
      for my $point(@points)
      {
        my $check_win_rate = $top_level_win_rate * $point;
        my $target_cost;
        #print("find win rate $check_win_rate\n");
        my $max_cost = $all_costs[scalar(@all_costs) - 1];

        for(my $base_cost_i = 0; $base_cost_i < scalar(@all_costs); ++$base_cost_i)
        {
          my $bids = 0;
          my $imps = 0;
          my $cost_i;
          for($cost_i = $base_cost_i; $cost_i < scalar(@all_costs); ++$cost_i)
          {
            $bids += $cost_dict->{$all_costs[$cost_i]}->bids();
            $imps += $cost_dict->{$all_costs[$cost_i]}->imps();
            if($imps >= LEVEL_WIN_RATE_MIN_IMPS)
            {
              last;
            }
          }

          if($imps >= LEVEL_WIN_RATE_MIN_IMPS && $bids > 0)
          {
            my $local_win_rate = $imps * 1.0 / $bids;
            #print("> Local win rate: $local_win_rate\n");
            if($local_win_rate >= $check_win_rate)
            {
              $target_cost = $all_costs[min($cost_i, scalar(@all_costs) - 1)];
              last;
            }
          }
        }

        if(defined($target_cost))
        {
          #print("target cost for $check_win_rate : $target_cost\n");
          my @top_key_parts = split('\t', $top_key);
          my $tag_id = $top_key_parts[0];
          my $domain = $top_key_parts[1];
          $res->set_cost($tag_id, $domain, $point, $target_cost, $max_cost, $check_win_rate);
        }
      } # for points
    } # top_level_win_rate defined
  }

  if($save_model > 0)
  {
    return $res;
  }
  else
  {
    return undef;
  }
}

1;
