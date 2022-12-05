package CsvUtils::Process::Tailor;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Tailor: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Tailor: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $exp = exists($params{'exp'}) ? $params{'exp'} : 2;

  my @res;
  for(my $i = 2; $i <= $exp; ++$i)
  {
    my @local_res = generate_combinations_(\@res_indexes, $i);

    #foreach my $s(@local_res)
    #{
    #  print "COMBINATION($i): " . join(',', @$s) . "\n";
    #}

    @res = (@res, @local_res);
  }


  my $fields = {
    fields_ => \@res_indexes,
    combs_ => \@res
  };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row = @$row;

  # check
  foreach my $index(@{$self->{fields_}})
  {
    if(!Scalar::Util::looks_like_number($row->[$index]))
    {
      die "CsvUtils::Process::Tailor: '" . $row->[$index] . "' isn't number";
    }
  }

  #
  foreach my $comb(@{$self->{combs_}})
  {
    my $res = 1.0;
    foreach my $index(@$comb)
    {
      $res *= $row->[$index];
    }

    push(@res_row, sprintf("%.10f", $res));
  }

  return \@res_row;
}

sub flush
{}

sub generate_combinations_
{
  my ($indexes, $size) = @_;
  my @res;

  if($size > 1)
  {
    for(my $i = 0; $i < scalar(@$indexes); ++$i)
    {
      my @sub_indexes = @{$indexes}[$i .. (scalar(@$indexes) - 1) ];
      my @local_res = generate_combinations_(\@sub_indexes, $size - 1);
      foreach my $arr(@local_res)
      {
        push(@res, [ $indexes->[$i], @$arr ]);
      }
    }
  }
  elsif($size == 1)
  {
    foreach my $index(@$indexes)
    {
      push(@res, [ $index ]);
    }
  }

  return @res;
}

1;
