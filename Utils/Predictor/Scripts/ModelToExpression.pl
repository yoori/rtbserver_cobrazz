#!/usr/bin/perl -w

use Class::Struct;

# cat model | XGBoostModelToExpr --gscore= --transform=logloss
# parse xgboost model dump and convert it to boolean expression with predefined output score
# logloss : 1 / (1 + exp(-x))
# Output example:
#  f1 &  f2 => 0.01
#  f1 & !f2 => 0.05
#

=comment
booster[0]:
    0:[f1<500.5] yes=1,no=2,missing=1,gain=2410.95,cover=2499.75
    1:[f0<500.5] yes=3,no=4,missing=3,gain=4908.89,cover=1242.5
    3:[f1<0.5] yes=5,no=6,missing=5,gain=25.193,cover=613.75
    5:leaf=-1.27273,cover=1.75
    6:[f0<0.5] yes=7,no=8,missing=7,gain=21.502,cover=612
    7:leaf=-1.2,cover=1.5
    8:leaf=1.99673,cover=610.5
    4:leaf=-1.99682,cover=628.75
    2:leaf=-1.99841,cover=1257.25
booster[1]:
    0:[f1<500.5] yes=1,no=2,missing=1,gain=325.258,cover=1052.65
    1:[f0<500.5] yes=3,no=4,missing=3,gain=660.805,cover=523.995
    3:[f1<0.5] yes=5,no=6,missing=5,gain=5.97545,cover=259.297
    5:leaf=-0.697278,cover=1.19645
    6:[f0<0.5] yes=7,no=8,missing=7,gain=5.41589,cover=258.101
    7:leaf=-0.671797,cover=1.06737
    8:leaf=1.13138,cover=257.034
    4:leaf=-1.13149,cover=264.698
    2:leaf=-1.13341,cover=528.652

>>>>>>>
f1(<X) & f0(<X)
=cut

struct(Booster => [leafs => '$', nodes => '$']);
struct(Node => [ feature => '$', yes_node => '$', no_node => '$' ]);
struct(Expression => [ expression => '$', score => '$' ]);

# return array of expressions
sub construct_expressions
{
  my ($root_id, $nodes, $leafs) = @_;

  #print "construct_expressions($root_id)\n";

  if(exists($leafs->{$root_id}))
  {
    #print "Score($root_id): " . $leafs->{$root_id} . "\n";
    return (new Expression(expression => undef, score => $leafs->{$root_id}));
  }

  my $node = $nodes->{$root_id};
  my $fid = $node->feature();
  my $yes_id = $node->yes_node();
  my $no_id = $node->no_node();
  my @yes_expressions = construct_expressions($yes_id, $nodes, $leafs);
  my @no_expressions = construct_expressions($no_id, $nodes, $leafs);
  my @result_expressions;

  foreach my $e(@yes_expressions)
  {
    my $expr = ' ' . $fid;
    if(defined($e->expression()))
    {
      #print "E <" . $e->expression() . ">\n";
      $expr = $expr . ' &' . $e->expression();
    }
    push(@result_expressions, new Expression(expression => $expr, score => $e->score));
  }

  foreach my $e(@no_expressions)
  {
    my $expr = '!' . $fid;
    if(defined($e->expression()))
    {
      #print "E <" . $e->expression() . ">\n";
      $expr = $expr . ' &' . $e->expression();
    }
    push(@result_expressions, new Expression(expression => $expr, score => $e->score));
  }

  return @result_expressions;
}

sub print_table
{
  my ($table) = @_;
  foreach my $key(keys %$table)
  {
    print "$key => " . $table->{$key} . "\n";
  }
}

my @boosters;

while(<STDIN>)
{
  my $line = $_;
  chomp $line;
  if($line =~ m/booster\[.*\]:/)
  {
    my $booster = new Booster(leafs => {}, nodes => {});
    push(@boosters, $booster);
  }
  elsif($line =~ m/^\s*(\d+):leaf=([-0-9.]+),.*$/) # leaf
  {
    my $booster = $boosters[-1];
    my $id = $1;
    my $val = $2;
    $booster->leafs()->{$id} = $val;
  }
  elsif($line =~ m/^\s*(\d+):\[([^<]+)<(.*)\] yes=([0-9]+),no=([0-9]+),missing=([0-9]+),.*$/) # node
  {
    my $booster = $boosters[-1];
    my $id = $1;
    my $fid = $2;
    my $val = $3;
    my $yes_id = $4;
    my $no_id = $5; # unused now
    my $miss_id = $6;
    if($val <= 0 || $val > 1)
    {
      #die "invalid model value: $val";
    }
    $booster->nodes()->{$id} = new Node(id => $id, feature => $fid, yes_node => $yes_id, no_node => $no_id);
  }
  else
  {
    die "Can't parse '$line'";
  }
}

# union expressions
#foreach my $booster(@boosters)
my $booster = $boosters[0];

{
  #{
    #print "LEAFS\n";
    #print_table($booster->leafs());
    #print "NODES\n";
    #print_table($booster->nodes());
  #}

  my @booster_expressions = construct_expressions(0, $booster->nodes(), $booster->leafs());
  foreach my $e(@booster_expressions)
  {
    print $e->expression() . " => " . $e->score() . "\n";
  }
}
