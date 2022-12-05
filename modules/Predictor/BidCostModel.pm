package Predictor::BidCostModel;

use strict;
use warnings;

sub new
{
  my $class = shift;

  my $self = {
    'model_' => {},
    'win_rates_' => {}
  };

  bless $self, $class;

  return $self;
}

sub get_cost
{
  my ($self, $tag_id, $domain, $required_win_rate, $cur_cost) = @_;
  my $min_cost = 100000;
  my $max_cost = 0;
  while(my ($win_rate, $x) = each(%{$self->{'win_rates_'}}))
  {
    if($win_rate >= $required_win_rate)
    {
      if(exists($self->{'model_'}->{"$tag_id,$domain,$win_rate"}))
      {
        my $l = $self->{'model_'}->{"$tag_id,$domain,$win_rate"};
        my @a = split(',', $l);
        $min_cost = min($min_cost, $a[0]);
        $max_cost = min($max_cost, $a[1]);
      }
    }
  }

  if($cur_cost >= $max_cost)
  {
    return $cur_cost;
  }

  return min($cur_cost, $min_cost);
}

sub set_cost
{
  my ($self, $tag_id, $domain, $win_rate, $cost, $max_cost, @add_fields) = @_;
  $self->{'win_rates_'}->{$win_rate} = 1;
  $self->{'model_'}->{"$tag_id,$domain,$win_rate"} = "$cost,$max_cost," . join(',', @add_fields);
}

sub save
{
  my ($self, $file_path) = @_;

  open(my $fh, '>', $file_path) or die "Could not open '$file_path' $!\n";
  while (my ($key, $val) = each (%{$self->{'model_'}}))
  {
    print $fh "$key,$val\n";
  }
  close($fh);
}

sub load
{
  my ($self, $file_path) = @_;

  open(my $fh, '<', $file_path) or die "Could not open '$file_path' $!\n";

  while(my $line = <$fh>)
  {
    chomp $line;
    my @fields = split(',', $line);

    if(scalar(@fields) > 0)
    {
      # <tag_id>,<domain>,<win_rate>,<cost>,<max cost>
      my $tag_id = $fields[0];
      my $domain = $fields[1];
      my $win_rate = $fields[2];
      my $cost = $fields[3];
      my $max_cost = $fields[4];

      $self->{'win_rates_'}->{$win_rate} = 1;
      $self->{'model_'}->{"$tag_id,$domain,$win_rate"} = "$cost,$max_cost";
    }
  }

  close($fh);
}

1;
