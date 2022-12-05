package CsvUtils::Process::ColumnsDispersion;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::ColumnsDispersion: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Columns: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = { fields_ => \@res_indexes };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $count = 0;
  my $mx = 0.0;
  foreach my $index(@{$self->{fields_}})
  {
    $mx += $row->[$index];
    ++$count;
  }

  $mx = $mx / $count;

  my $disp = 0.0;
  if($count > 1)
  {
    # SUM(x^2 - (Mx)^2) / (n - 1)
    foreach my $index(@{$self->{fields_}})
    {
      $disp += $row->[$index] * $row->[$index] - $mx * $mx;
    }

    $disp = $disp / ($count - 1);
  }

  return [@$row, $disp];
}

sub flush
{}

1;
