use strict;
use warnings;
use Text::CSV_XS;
use open qw( :std :encoding(UTF-8) );

my $file = $ARGV[0];

use constant KEY_FIELDS => 2;
use constant BID_RATIO => 0.5;
use constant MIN_IMPS => 10;

sub eval_cost
{
  my ($costs) = @_;
  my $prev_ratio = 0;
  my $bid_ratio_max = 0;

  {
    my $start_count = 0;
    my $imps = 0;
    my $reqs = 0;

    foreach my $cost(reverse sort { $a <=> $b } keys %$costs)
    {
      my $arr = $costs->{$cost};
      $imps += $arr->[0];
      $reqs += $arr->[1];

      if($imps > MIN_IMPS)
      {
        last;
      }
    }

    print("reqs = " . $reqs . ", imps = " . $imps . "\n");
    $bid_ratio_max = $imps * 1.0 / $reqs;
  }

  my @costs_arr;
  foreach my $cost(sort { $a <=> $b } keys %$costs)
  {
    my $val = [ $cost, $costs->{$cost} ];
    push(@costs_arr, $val);
  }

  {
    for(my $i = 0; $i < scalar(@costs_arr); ++$i)
    {
      my $imps = 0;
      my $reqs = 0;
      my $cost = 0;
      for(my $j = 0; $j < scalar(@costs_arr); ++$j)
      {
        my $v = $costs_arr[$i + $j];
        $cost = $v->[0];
        my $a = $v->[1];
        $imps += $a->[0];
        $reqs += $a->[1];

        if($imps > MIN_IMPS && $reqs > 0)
        {
          my $cur_ratio = $imps * 1.0 / $reqs;
          if($cur_ratio >= $bid_ratio_max * BID_RATIO)
          {
            return $cost;
          }
        }
      }

      print("X reqs = " . $reqs . ", imps = " . $imps . "\n");
    }
  }

  return $costs_arr[scalar(@costs_arr) - 1]->[0];
}

# load file
my $csv = Text::CSV_XS->new({
  binary => 1,
  eol => undef,
  sep_char => "," });

open(my $fh, '<', $file) or die "Could not open '$file' $!\n";

binmode(STDOUT, "encoding(UTF-8)");

my $out_csv = Text::CSV_XS->new({
  binary => 1,
  eol => $/,
  sep_char => ",",
  quote_binary => 1,
  quote_space => 0,
  });

# process data
my $prev_key = undef;
my %cur_costs = ();

while (my $line = <$fh>)
{
  chomp $line;
  if ($csv->parse($line))
  {
    my @fields = $csv->fields();
    my $key = $fields[0];
    for(my $i = 0; $i < KEY_FIELDS; ++$i)
    {
      $key = $key . "," . $fields[$i];
    }
    my $cost = $fields[KEY_FIELDS];
    my $reqs = $fields[KEY_FIELDS + 1];
    my $imps = $fields[KEY_FIELDS + 2];

    if(defined($prev_key) && $key ne $prev_key)
    {
      my $bid_cost = eval_cost(\%cur_costs);
      my @k = split(',', $key);
      $out_csv->print(\*STDOUT, [ @k, $bid_cost ]);

      $prev_key = $key;
      %cur_costs = ();
    }
    else
    {
      if(exists($cur_costs{$cost}))
      {
	my $arr = $cur_costs{$cost};
        $arr->[0] += $imps;
        $arr->[1] += $reqs;
      }
      else
      {
        $cur_costs{$cost} = [$imps, $reqs];
      }

      $prev_key = $key;
    }
  }
}

if(defined($prev_key))
{
  my $bid_cost = eval_cost(\%cur_costs);
  my @k = split(',', $prev_key);
  $out_csv->print(\*STDOUT, [ @k, $bid_cost ]);
}
