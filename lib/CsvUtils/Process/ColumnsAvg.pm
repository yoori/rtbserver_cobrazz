package CsvUtils::Process::ColumnsAvg;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::ColumnsAvg: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::ColumnsAvg: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = { fields_ => \@res_indexes };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $sum = 0.0;
  my $count = 0;
  foreach my $index(@{$self->{fields_}})
  {
    $sum += $row->[$index];
    ++$count;
  }

  my $avg = $count > 0 ? $sum / $count : 0.0;

  return [@$row, $avg];
}

sub flush
{}

1;
